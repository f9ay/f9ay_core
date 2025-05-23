#pragma once

#include <algorithm>
#include <any>
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

namespace f9ay::internal {

//[[deprecated]]
inline std::pair<uint8_t, size_t> billy_to_value(const std::vector<std::byte> &billy) {
    uint8_t result = 0;
    for (int i = 0; i < billy.size(); i++) {
        result |= static_cast<int>(billy[i]) << i;
    }
    return {result, billy.size()};
}
}  // namespace f9ay::internal

namespace f9ay {
class Jpeg {
private:
#pragma pack(push, 1)
    struct JFIF_APP0 {
        uint16_t marker;
        uint16_t length;  // Length of segment excluding APP0 marker
        char identifier[5];
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
            // 先不管 小畫家生出來的也只到1.1
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
        std::println("{}", dcs[0]);
        for (auto &[k, v] : y_dc.getCodeMap()) {
            std::println("({}, {})", k, static_cast<int>(internal::billy_to_value(v).first));
        }

        std::println("{}", acs);
        size_t size = 65354133;  // TODO
        std::unique_ptr<std::byte[]> result(new std::byte[size]{});
        auto it = result.get();

        write_byte<uint16_t>(it, 0xD8FF);  // little endian
        it = write_app0(it);
        it = write_dqt(it, y_quantization_matrix, uv_quantization_matrix);  // TODO cbcr qt
        it = write_huffman_all(it, y_dc.getCodeMap(), y_ac.getCodeMap(), uv_dc.getCodeMap(), uv_ac.getCodeMap());
        it = write_sof0_segment(it, src.row(), src.col());
        it = write_binary_stream(it, y_dc, y_ac, uv_dc, uv_ac, dcs, acs);
        return {std::move(result), size};
    }

private:
    template <typename T, std::endian endian = std::endian::native>
    static void write_byte(std::byte *&it, const T &val) {
        if constexpr (endian == std::endian::little) {
            *reinterpret_cast<T *>(it) = val;
            it += sizeof(T);
        } else {
            *reinterpret_cast<T *>(it) = std::byteswap(val);
            it += sizeof(T);
        }
    }
    static std::byte *write_app0(std::byte *it) {
        JFIF_APP0 header{.identifier = "JFIF"};
        header.marker = 0xE0FF;  // little endian
        // header.length = sizeof(JFIF_APP0); big endian
        header.length = std::byteswap<uint16_t>(sizeof(JFIF_APP0) - sizeof(header.marker));
        header.version = 0x0101;
        header.Density_units = 0;
        header.Xdensity = 0;
        header.Ydensity = 0;
        header.Xthumbnail = 0;
        header.Ythumbnail = 0;
        std::println(" len {}", header.length);
        return std::copy(reinterpret_cast<std::byte *>(&header), reinterpret_cast<std::byte *>(&header + 1), it);
    }
    // DQT (Define Quantization Table)
    static std::byte *write_dqt(std::byte *it, const std::array<std::array<uint8_t, 8>, 8> &qt_y,
                                const std::array<std::array<uint8_t, 8>, 8> &qt_uv) {
        write_byte<uint16_t>(it, 0xDBFF);
        std::byte *size_ptr = it;
        it += 2;
        // 精度 << 8 | ID
        write_byte<uint8_t>(it, 0);  // 0 for Y
        for (const auto &[i, j] : zigzag<8>()) {
            write_byte<uint8_t>(it, qt_y[i][j]);
        }
        write_byte<uint8_t>(it, 1);  // 1 for cbcr
        for (const auto &[i, j] : zigzag<8>()) {
            write_byte<uint8_t>(it, qt_uv[i][j]);
        }
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);  // 填入 size
        return it;
    }
    static std::byte *write_huffman_all(
        std::byte *it, const std::unordered_map<int8_t, std::vector<std::byte>> &y_dc,
        const std::unordered_map<int8_t, std::vector<std::byte>> &y_ac,
        const std::unordered_map<int8_t, std::vector<std::byte>> &uv_dc,
        const std::unordered_map<int8_t, std::vector<std::byte>> &uv_ac) {
        write_byte<uint16_t>(it, 0xC4FF);

        auto size_ptr = it;
        it += 2;
        /* Y DC segment */
        write_byte<uint8_t>(it, 0);  // [DC : 0  ac : 1 ; ID]
        it = write_huffman_data(it, y_dc);
        /* Y AC segment */
        write_byte<uint8_t>(it, 1 << 4 | 0);
        it = write_huffman_data(it, y_ac);
        /* CB CR  DC segment */
        write_byte<uint8_t>(it, 1);
        it = write_huffman_data(it, uv_dc);
        /* CB CR  AC segment */
        write_byte<uint8_t>(it, 1 << 4 | 1);
        it = write_huffman_data(it, uv_ac);

        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        return it;
    }

    static std::byte *write_huffman_data(std::byte *it,
                                         const std::unordered_map<int8_t, std::vector<std::byte>> &table) {
        std::print("no unreached");
        std::array<uint8_t, 16> bits_array{};
        for (const auto &v : table | std::views::values) {
            if (v.size() == 0) [[unlikely]] {
                throw std::logic_error("Huffman table is empty");
            }
            bits_array[v.size() - 1]++;  // size 不可能是 0
        }
        write_byte(it, bits_array);
        std::vector<std::pair<uint8_t, uint8_t>> ht;
        for (const auto &[k, v] : table) {
            ht.emplace_back(v.size(), k);
        }
        std::ranges::sort(ht);
        for (auto &v : ht | std::views::values) {
            write_byte(it, v);
        }
        return it;
    }
    static std::byte *write_sof0_segment(std::byte *it, int width, int height) {
        write_byte<uint16_t>(it, 0xC0FF);  // SOF0 marker
        write_byte<uint16_t, std::endian::big>(
            it, 17);                 // segment length (precision(1) + height(2) + width(2) + channels(1) + 3*channels)
        write_byte<uint8_t>(it, 8);  // precision
        write_byte<uint16_t, std::endian::big>(it, height);  // height
        write_byte<uint16_t, std::endian::big>(it, width);   // width
        write_byte<uint8_t>(it, 3);                          // 3 channels

        // Channel: Y
        write_byte<uint8_t>(it, 1);     // component ID: Y
        write_byte<uint8_t>(it, 0x22);  // sampling factors: H=2, V=2 (4:2:0)
        write_byte<uint8_t>(it, 0);     // quant table ID: 0

        // Channel: Cb
        write_byte<uint8_t>(it, 2);     // component ID: Cb
        write_byte<uint8_t>(it, 0x11);  // sampling factors: H=1, V=1
        write_byte<uint8_t>(it, 1);     // quant table ID: 1

        // Channel: Cr
        write_byte<uint8_t>(it, 3);     // component ID: Cr
        write_byte<uint8_t>(it, 0x11);  // sampling factors: H=1, V=1
        write_byte<uint8_t>(it, 1);     // quant table ID: 1

        return it;
    }
    static std::byte *write_binary_stream(
        std::byte *it, const HuffmanCoding<int8_t> &y_dc_huffman, const HuffmanCoding<int8_t> &y_ac_huffman,
        HuffmanCoding<int8_t> &uv_dc_huffman, HuffmanCoding<int8_t> &uv_ac_huffman,
        std::vector<std::vector<signed char>> &dcs,
        std::vector<Matrix<std::vector<std::pair<signed char, unsigned char>>>> &acs) {
        write_byte<uint16_t>(it, 0xDAFF);
        auto size_ptr = it;
        it += 2;
        write_byte<uint8_t>(it, 3);     // 3 channels for Y Cb Cr
        write_byte<uint8_t>(it, 1);     // Y
        write_byte<uint8_t>(it, 0x00);  // Y huffman id
        write_byte<uint8_t>(it, 2);     // Cb
        write_byte<uint8_t>(it, 0x11);  // Cb huffman id
        write_byte<uint8_t>(it, 3);     // Cr
        write_byte<uint8_t>(it, 0x11);  // Cr huffman id
        write_byte<uint8_t>(it, 0x01);  // Ss = 1
        write_byte<uint8_t>(it, 0x3F);  // Se = 63
        write_byte<uint8_t>(it, 0x00);  // Successive Approximation Bit Setting, Ah/Al

        const size_t mcu_cnt = dcs[0].size() / 4;
        if (dcs[0].size() / 4 != dcs[1].size() && dcs[0].size() / 4 != dcs[2].size()) {
            throw std::logic_error("data size mismatch");
        }
        BitWriter bit_writer;
        for (int i = 0; i < mcu_cnt; i++) {
            // Y channel
            for (int j = 0; j < 4; j++) {
                // write dc
                {
                    const auto &y = dcs[0][i * 4 + j];
                    auto [category_huff, huff_cnt] = internal::billy_to_value(y_dc_huffman.getMapping(category(y)));

                    bit_writer.writeBitsFromMSB(category_huff, huff_cnt);

                    bit_writer.writeBit(dcs[0][i * 4 + j]);

                    bit_writer.writeBitsFromMSB(dcs[0][i * 4 + j], category(dcs[0][i * 4 + j]));
                }

                auto ac_span = acs[0].flattenToSpan();

                auto current_ac_span = std::span(ac_span[i * 4 + j]);
                // write ac
                for (int k = 0; k < current_ac_span.size(); k++) {
                    // TODO 寫入 AC
                    auto [huff, huff_cnt] = internal::billy_to_value(y_ac_huffman.getMapping(current_ac_span[k].first));
                    bit_writer.writeBitsFromMSB(huff, huff_cnt);

                    int amp_size = current_ac_span[k].first & 0xF;
                    if (amp_size != 0) {
                        bit_writer.writeBitsFromMSB(current_ac_span[k].second, amp_size);
                    }
                }
            }

            for (int j = 1; j <= 2; j++) {  // 1 : cb 2 : cr
                {
                    const auto &y = dcs[j][i];
                    auto [category_huff, huff_cnt] = internal::billy_to_value(uv_dc_huffman.getMapping(category(y)));

                    bit_writer.writeBitsFromMSB(category_huff, huff_cnt);

                    bit_writer.writeBit(dcs[j][i]);

                    bit_writer.writeBitsFromMSB(dcs[j][i], category(dcs[j][i]));
                }

                auto ac_span = acs[j].flattenToSpan();

                auto current_ac_span = std::span(ac_span[i]);

                // write ac
                for (int k = 0; k < current_ac_span.size(); k++) {
                    // TODO 寫入 AC
                    auto [huff, huff_cnt] =
                        internal::billy_to_value(uv_ac_huffman.getMapping(current_ac_span[k].first));
                    bit_writer.writeBitsFromMSB(huff, huff_cnt);

                    int amp_size = current_ac_span[k].first & 0xF;
                    if (amp_size != 0) {
                        bit_writer.writeBitsFromMSB(current_ac_span[k].second, amp_size);
                    }
                }
            }
        }

        auto buffer = bit_writer.getBuffer();

        for (auto &b : buffer) {
            write_byte(it, b);
            if (b == std::byte{0xFFu}) {
                write_byte(it, static_cast<std::byte>(0));
            }
        }
        write_byte<uint8_t>(it, uint8_t(0xD9FFu));
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        std::print(" {} ", it - size_ptr);
        return it;
    }
    static auto encode(const Matrix<colors::RGB> &src) {
        auto yuv = src.trans_convert([](const colors::RGB &color) -> colors::YCbCr {
            const auto &[r, g, b] = color;
            return colors::YCbCr{
                static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b),
                static_cast<uint8_t>(-0.168736f * r - 0.331364f * g + 0.5f * b + 128),
                static_cast<uint8_t>(0.5f * r - 0.418688f * g - 0.081312f * b + 128)};
        });
        // 分離有助於 cache

        Matrix<uint8_t> Y(src.row(), src.col());
#pragma loop(hint_parallel(0))
        for (int i = 0; i < src.row(); i++) {
            for (int j = 0; j < src.col(); j++) {
                Y[i, j] = yuv[i, j].y;
                // Cb[i, j] = yuv[i, j].cb;
                // Cr[i, j] = yuv[i, j].cr;
            }
        }
        // Downsampling
        // TODO 原圖不是二的倍數
        Matrix<uint8_t> Cb(src.row() / 2, src.col() / 2);
        Matrix<uint8_t> Cr(src.row() / 2, src.col() / 2);
#pragma loop(hint_parallel(0))
        for (int i = 0; i < Cb.row(); i++) {
            for (int j = 0; j < Cr.col(); j++) {
                Cb[i, j] = yuv[i * 2, j * 2].cb;
                Cr[i, j] = yuv[i * 2, j * 2].cr;
            }
        }

        auto split_y = split<8>(Y);
        auto split_cb = split<8>(Cb);
        auto split_cr = split<8>(Cr);

        HuffmanCoding<int8_t> y_dc, y_ac, uv_dc, uv_ac;
        std::vector<std::vector<int8_t>> dcs;
        dcs.reserve(3);
        std::vector<Matrix<std::vector<std::pair<signed char, unsigned char>>>> acs;
        acs.reserve(3);
        for (const auto m_ptr : {&split_y, &split_cb, &split_cr}) {
            auto &m = *m_ptr;

            // TODO 用表達式模板優化
            /* clang-format off */
            auto zigzaged =
                m.transform([](Matrix<uint8_t> &block) {
                     block.transform([](uint8_t &x) {
                         x -= 128;
                     });
                })
                .trans_convert(Dct<8>::dct<uint8_t, int16_t>)
                .transform([m_ptr, y_ptr = &split_y](Matrix<int16_t> &block) {
                    if (m_ptr == y_ptr) {
                        block.round_div(y_quantization_matrix);
                    }
                    else {
                        block.round_div(uv_quantization_matrix);
                    }
                })
                .trans_convert([](const Matrix<int16_t> &block) {
                    // zig zag 排列
                    // 忽略 uninitialize error 因為每個 index 都會填東西
                    std::array<int16_t, 8 * 8> block_uint8;  // NOLINT(*-pro-type-member-init)
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

            auto dc_category = dc | std::views::all | std::views::transform([](const int16_t x) {
                                   return category(x);
                               });

            auto ac = zigzaged.trans_convert([](const std::array<int16_t, 8 * 8> &arr) {
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
                                static_cast<int8_t>(cnt << 4 | size_for_bit), to_amplitude(arr[i], size_for_bit));
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
                y_dc.add(dc_category | std::ranges::to<std::vector>()).build();
                HuffmanCoding<int8_t> ac_huffman;
                y_ac.add(ac_merged | std::ranges::to<std::vector>()).build();
            } else {
                uv_dc.add(dc_category | std::ranges::to<std::vector>());
                uv_ac.add(ac_merged | std::ranges::to<std::vector>());
            }
            dcs.emplace_back(std::move(dc));
            acs.emplace_back(std::move(ac));
        }
        uv_dc.build();
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

    template <typename int_type>
        requires requires { sizeof(int_type) <= 2; }
    static int8_t calculate_binary_size(int_type x) {
        [[assume(x > 0)]];
#ifdef _MSC_VER
        // lzcnt return leading zero => 0000 0000 1000 0000  return 8
        return static_cast<int8_t>(16 - __lzcnt16(x));
#else
        return 32 - __builtin_clz(x);
#endif
    }

public:
    static int8_t category(int16_t x) {
        if (x == 0) [[unlikely]] {
            return 0;
        }
        if (x < 0) x = -x;
        return calculate_binary_size(x);
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

    constexpr static std::array<std::array<uint8_t, 8>, 8> y_quantization_matrix = {
        std::array<uint8_t, 8>{16, 11, 10, 16, 24, 40, 51, 61},
        {12, 12, 14, 19, 26, 58, 60, 55},
        {14, 13, 16, 24, 40, 57, 69, 56},
        {14, 17, 22, 29, 51, 87, 80, 62},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {24, 35, 55, 64, 81, 104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}};

    constexpr static std::array<std::array<uint8_t, 8>, 8> uv_quantization_matrix = {
        std::array<uint8_t, 8>{17, 18, 24, 47, 99, 99, 99, 99},
        {18, 21, 26, 66, 99, 99, 99, 99},
        {24, 26, 56, 99, 99, 99, 99, 99},
        {47, 66, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99}};
};
}  // namespace f9ay

// namespace f9ay::internal
