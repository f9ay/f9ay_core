#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

#include "dct.hpp"
#include "huffman_coding.hpp"
#include "importer.hpp"
#include "matrix.hpp"
#include "matrix_concept.hpp"
#include "matrix_view.hpp"
#include "util.hpp"

namespace f9ay {
class Jpeg {
private:
#pragma pack(push, 1)
    struct JFIF_APP0 {
        uint16_t marker;
        uint16_t length;  // Length of segment excluding APP0 marker
        char Identifier[5];
        uint16_t version;
        uint8_t Density_units;
        uint16_t Xdensity, Ydensity;
        uint8_t Xthumbnail, Ythumbnail;
        //  c data	=> 3 × n => n = Xthumbnail × Ythumbnail
    };
    struct JFIF_extension_APP0 {
        uint16_t marker;
        uint16_t length;
        char Identifier[5];
        char Thumbnail_format;
    };
#pragma pack(pop)
    static_assert(sizeof(JFIF_APP0) == 18);
    static_assert(sizeof(JFIF_extension_APP0) == 10);

public:
    static Midway import(const std::byte *source) {
        const auto *jfif_app0 = safeMemberAssign<JFIF_APP0>(source);
        std::pair<const std::byte *, size_t> thumbnail_data = {
            source + sizeof(JFIF_APP0), 3 * jfif_app0->Xthumbnail * jfif_app0->Ythumbnail};
        const std::byte *it = source + sizeof(JFIF_APP0) + 3 * jfif_app0->Xthumbnail * jfif_app0->Ythumbnail;
        if (jfif_app0->version >= 0x0102) {
            // jfif extension app0 available
            throw std::logic_error("JFIF version does not match");
        }

        if (*reinterpret_cast<const uint16_t *>(it) != 0xFFDA) {
            throw std::logic_error("JFIF SOS NOT match");
        }
        it += 2;
        // 	compressed image data

        return {};
    }

    static std::pair<std::unique_ptr<std::byte[]>, size_t> write(const Matrix<colors::RGB> &src) {
        auto [y_dc, y_ac, uv_dc, uv_ac, dcs, acs] = encode(src);

        return {nullptr, 0};
    }

private:
    static auto encode(const Matrix<colors::RGB> &src) {
        auto yuv = src.trans_convert([](const colors::RGB &color) -> colors::YCbCr {
            const auto &[r, g, b] = color;
            return colors::YCbCr{
                static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b),
                static_cast<uint8_t>(-0.168736f * r - 0.331364f * g + 0.5f * b + 128),
                static_cast<uint8_t>(0.5f * r - 0.418688f * g - 0.081312f * b + 128)};
        });
        // 分離有助於 cache
        // TODO Downsampling
        Matrix<uint8_t> Y(src.row(), src.col());
        Matrix<uint8_t> Cb(src.row(), src.col());
        Matrix<uint8_t> Cr(src.row(), src.col());
#pragma loop(hint_parallel(0))
        for (int i = 0; i < src.row(); i++) {
            for (int j = 0; j < src.col(); j++) {
                Y[i, j] = yuv[i, j].y;
                Cb[i, j] = yuv[i, j].cb;
                Cr[i, j] = yuv[i, j].cr;
            }
        }

        auto split_y = split<8>(Y);
        auto split_cb = split<8>(Cb);
        auto split_cr = split<8>(Cr);

        // std::vector<HuffmanCoding<int8_t>> result_huffman(4);  // y_dc y_ac cb cr
        HuffmanCoding<int8_t> y_dc, y_ac, uv_dc, uv_ac;
        std::vector<std::vector<int8_t>> dcs;
        dcs.reserve(3);
        std::vector<Matrix<std::vector<std::pair<signed char, unsigned char>>>> acs;
        acs.reserve(3);
        for (const auto m_ptr : {&split_y, &split_cb, &split_cr}) {
            auto &m = *m_ptr;

            // TODO 用表達式模板優化
            auto zigzaged =
                /* clang-format off */
                m.transform([](Matrix<uint8_t> &block) {
                    block.transform([](uint8_t &x) {
                        x -= 128;
                    });
                })
                .trans_convert(Dct<8>::dct<uint8_t, int16_t>)
                .trans_convert([](const Matrix<int16_t> &block) {
                    // zig zag 排列
                    // 忽略 uninitialize error 因為每個 index 都會填東西
                    std::array<int8_t, 8 * 8> block_uint8; // NOLINT(*-pro-type-member-init)
                    int index = 0;
                    for (auto &[i, j] : zigzag<8>()) {
                        block_uint8[index++] = block[i, j];
                    }
                    return block_uint8;
                });
            /* clang-format on */
            std::vector<int8_t> dc(zigzaged.flattenToSpan().size());
            int index = 0;
            for (auto &x : zigzaged.flattenToSpan()) {
                // dc 使用 差分
                if (index != 0) {
                    dc[index] = x[0] - dc[index - 1];  // de[index - 1] + dx[index] = x[0]
                    index++;
                } else {
                    dc[index++] = x[0];
                }
            }

            auto ac = zigzaged.trans_convert([](const std::array<int8_t, 8 * 8> &arr) {
                // run length encoding
                std::vector<std::pair<int8_t, uint8_t>> rle;
                int last_non_zero = 0;
                for (int i = arr.size() - 1; i >= 0; i--) {
                    if (arr[i] != 0) {
                        last_non_zero = i;
                        break;
                    }
                }
                int cnt = 0;
                // 從 1 開始 因為 dc 不編碼
                for (int i = 1; i <= last_non_zero; i++) {
                    if (arr[i] != 0 || cnt == 15) {
                        // [run_len, size_for_bit], amp
                        if (arr[i] == 0) {
                            rle.emplace_back(static_cast<signed char>(0xF0), 0);
                        } else {
                            const int8_t size_for_bit = calculate_binary_size(abs_int8(arr[i]));
                            rle.emplace_back(
                                static_cast<int8_t>(cnt << 8 | size_for_bit), to_amplitude(arr[i], size_for_bit));
                        }
                        cnt = 0;
                    } else {  // cnt < 15 and arr[i] == 0
                        cnt++;
                    }
                }
                rle.emplace_back(static_cast<int8_t>(0x00), 0);  // EOB -- end of block
                return rle;
            });

            // TODO 這裡之後可以優化到 計算RLE的地方
            /* clang-format off */
            auto ac_merged = ac.flattenToSpan()
                | std::views::join
                | std::views::transform([](const std::pair<int8_t, int8_t> &rle_pair) {
                        return rle_pair.first;
                    });
            /* clang-format on */

            if (m_ptr == &split_y) {
                y_dc.add(dc).build();
                HuffmanCoding<int8_t> ac_huffman;
                y_ac.add(ac_merged | std::ranges::to<std::vector>()).build();
            } else {
                // dc merge ac
                uv_dc.add(dc).build();

                uv_ac.add(ac_merged | std::ranges::to<std::vector>());
            }
            dcs.emplace_back(std::move(dc));
            acs.emplace_back(std::move(ac));
        }

        uv_ac.build();

        return std::tuple{
            std::move(y_dc), std::move(y_ac), std::move(uv_dc), std::move(uv_ac), std::move(dcs), std::move(acs)};
    }

private:
    template <int N, int M = N>
    static Matrix<Matrix<uint8_t>> split(Matrix<uint8_t> &mtx) {
        const int row_sz = mtx.row() / N + (mtx.row() % N != 0);
        const int col_sz = mtx.col() / M + (mtx.col() % M != 0);
        Matrix<Matrix<uint8_t>> result(row_sz, col_sz);
        for (int i = 0; i < row_sz; i++) {
            for (int j = 0; j < col_sz; j++) {
                Matrix<uint8_t> block(N, M);
                for (int k = 0; k < N; k++) {
                    for (int l = 0; l < M; l++) {
                        // 希望編譯器會優化
                        if (i * N + k >= mtx.row() || j * M + l >= mtx.col()) [[unlikely]] {
                            block[k, l] = 128;  // TODO 先填 128 後面再改
                        } else {
                            block[k, l] = mtx[i * N + k, j * M + l];
                        }
                    }
                }
                result[i, j] = std::move(block);
            }
        }
        return result;
    }

    constexpr static int8_t abs_int8(int8_t x) {
        if (x < 0) return -x;
        return x;
    }

    static int8_t calculate_binary_size(int8_t x) {
        [[assume(x > 0)]];
#ifdef _MSC_VER
        // lzcnt return leading zero => 0000 0000 1000 0000  return 8
        return static_cast<int8_t>(16 - __lzcnt16(x));
#else
        return 32 - __builtin_clz(x);
#endif
    }

    constexpr static int8_t to_amplitude(int8_t val, int8_t size) {
        if (val > 0) {
            return val;
        }
        return (1 << size) - 1 - abs_int8(val);
    }

    template <int n>
    static consteval auto zigzag() {
        std::array<std::pair<int, int>, n * n> res;
        int index = 0;
        int row = 0, col = 0;
        for (int len = 1; len <= n; len++) {
            for (int i = 0; i < len - 1; i++) {
                res[index++] = {row, col};
                if (len % 2 == 0) {
                    row++;
                    col--;
                } else {
                    col++;
                    row--;
                }
            }
            res[index++] = {row, col};
            if (len % 2 == 0) {
                row++;
            } else {
                col++;
            }
        }
        row = n - 1;
        col = 1;
        for (int len = n - 1; len >= 1; --len) {
            for (int i = 0; i < len - 1; i++) {
                res[index++] = {row, col};
                if (len % 2 == 0) {
                    row++;
                    col--;
                } else {
                    col++;
                    row--;
                }
            }
            res[index++] = {row, col};
            if (len % 2 != 0) {
                row++;
            } else {
                col++;
            }
        }
        return res;
    }

    constexpr static std::array<std::array<uint8_t, 8>, 8> Quantization_Matrix = {
        std::array<uint8_t, 8>{16, 11, 10, 16, 24, 40, 51, 61},
        {12, 12, 14, 19, 26, 58, 60, 55},
        {14, 13, 16, 24, 40, 57, 69, 56},
        {14, 17, 22, 29, 51, 87, 80, 62},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {24, 35, 55, 64, 81, 104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}};
};
}  // namespace f9ay
