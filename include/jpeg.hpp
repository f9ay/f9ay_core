#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

#include "dct.hpp"
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
            source + sizeof(JFIF_APP0),
            3 * jfif_app0->Xthumbnail * jfif_app0->Ythumbnail};
        const std::byte *it = source + sizeof(JFIF_APP0) +
                              3 * jfif_app0->Xthumbnail * jfif_app0->Ythumbnail;
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

    static std::pair<std::unique_ptr<std::byte[]>, size_t> write(
        const Matrix<colors::RGB> &src) {
        Matrix<colors::YCbCr> ycbcr = toYCbCr(src);
        // 分離有助於 cache
        // TODO Downsampling
        Matrix<uint8_t> Y(src.row(), src.col());
        Matrix<uint8_t> Cb(src.row(), src.col());
        Matrix<uint8_t> Cr(src.row(), src.col());
#pragma loop(hint_parallel(0))
        for (int i = 0; i < src.row(); i++) {
            for (int j = 0; j < src.col(); j++) {
                Y[i, j] = ycbcr[i, j].y;
                Cb[i, j] = ycbcr[i, j].cb;
                Cr[i, j] = ycbcr[i, j].cr;
            }
        }

        auto split_y = split<8>(Y);
        auto split_cb = split<8>(Cb);
        auto split_cr = split<8>(Cr);

        for (const auto m_ptr : {&split_y, &split_cb, &split_cr}) {
            auto &m = *m_ptr;

            // TODO 用表達式模板優化
            auto before_zigzag =
                m.transform([](Matrix<uint8_t> &block) {
                     block.transform([](uint8_t &x) { x -= 128; });
                 })
                    .transform(Dct<8>::dct<uint8_t>)
                    .transform([](Matrix<uint8_t> &block) {
                        block.round_div(Quantization_Matrix);
                    })
                    .trans_convert([](const Matrix<uint8_t> &block) {
                        // zig zag 排列
                        std::array<uint8_t, 8 * 8> block_uint8;
                        int index = 0;
                        for (auto [i, j] : zigzag<8>()) {
                            block_uint8[index++] = block[i, j];
                        }
                        return block_uint8;
                    });
            before_zigzag.dump();
        }

        // auto &dct_y =
        // split_y.transform(sub_128).transform(Dct<8>::dct<int8_t>); auto
        // &dct_cb =
        //     split_cb.transform(sub_128).transform(Dct<8>::dct<int8_t>);
        // auto &dct_cr =
        //     split_cr.transform(sub_128).transform(Dct<8>::dct<int8_t>);

        //         // Downsampling to  4:2:0
        //         Matrix<uint8_t> Cb(src.col() / 2, src.row() / 2);
        //         Matrix<uint8_t> Cr(src.col() / 2, src.row() / 2);
        // #pragma loop(hint_parallel(0))
        //         for (int i = 0; i < Cb.row(); i++) {
        //             for (int j = 0; j < Cb.col(); j++) {
        //                 Cb[i, j] = ycbcr[i * 2, j * 2].cb;
        //                 Cr[i, j] = ycbcr[i * 2 + 1, j * 2].cr;
        //             }
        //         }

        return {nullptr, 0};
    }

private:
    static Matrix<colors::YCbCr> toYCbCr(const Matrix<colors::RGB> &src) {
        Matrix<colors::YCbCr> result(src.row(), src.col());
// 自動向量化
#pragma loop(hint_parallel(0))
        for (int i = 0; i < src.row(); i++) {
            for (int j = 0; j < src.col(); j++) {
                result[i, j] = {
                    static_cast<uint8_t>(0.299f * src[i, j].r +
                                         0.587f * src[i, j].g +
                                         0.114f * src[i, j].b),
                    static_cast<uint8_t>(-0.168736f * src[i, j].r -
                                         0.331364f * src[i, j].g +
                                         0.5f * src[i, j].b + 128),
                    static_cast<uint8_t>(0.5f * src[i, j].r -
                                         0.418688f * src[i, j].g -
                                         0.081312f * src[i, j].b + 128)};
            }
        }
        return result;
    }

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
                        if (i * N + k >= mtx.row() || j * M + l >= mtx.col())
                            [[unlikely]] {
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

    constexpr static std::array<std::array<uint8_t, 8>, 8> Quantization_Matrix =
        {std::array<uint8_t, 8>{16, 11, 10, 16, 24, 40, 51, 61},
         {12, 12, 14, 19, 26, 58, 60, 55},
         {14, 13, 16, 24, 40, 57, 69, 56},
         {14, 17, 22, 29, 51, 87, 80, 62},
         {18, 22, 37, 56, 68, 109, 103, 77},
         {24, 35, 55, 64, 81, 104, 113, 92},
         {49, 64, 78, 87, 103, 121, 120, 101},
         {72, 92, 95, 98, 112, 100, 103, 99}};
};
}  // namespace f9ay
