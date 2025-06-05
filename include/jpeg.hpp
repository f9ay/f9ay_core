#pragma once

#include <algorithm>
#include <any>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

#include "dct.hpp"
#include "huffman_tree.hpp"
#include "importer.hpp"
#include "matrix.hpp"
#include "matrix_concept.hpp"
#include "matrix_view.hpp"
#include "steal_vector.hpp"
#include "util.hpp"

namespace f9ay {

enum class Jpeg_sampling { ds_4_4_4, ds_4_2_0 };

template <Jpeg_sampling sampling_type = Jpeg_sampling::ds_4_2_0>
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
    struct bit_content {
        uint32_t value;
        uint8_t size;
        bit_content() = default;
        bit_content(uint32_t v, uint8_t s) : value(v), size(s) {}

        template <class T, class U>
        bit_content(const std::pair<T, U> &p) : value(p.first), size(p.second) {}
    };
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

public:
    static auto convert_dc_to_size_value(auto &dc) {
        return dc | std::views::all | std::views::transform([](auto &x) {
                   uint32_t value = 0;
                   if (x >= 0) {
                       value = x;
                   } else {
                       value = (1 << category(x)) - 1 + x;  // 正確的負數編碼
                   }
                   return std::pair<uint8_t, uint32_t>{category(x), value};  // size value
               });
    }

private:
    static auto build_huffman_tree(std::vector<std::vector<int32_t>> &dcs,
                                   std::vector<std::vector<std::vector<std::pair<unsigned char, int>>>> &acs) {
        Huffman_tree y_dc;
        for (const auto &[cat, value] : convert_dc_to_size_value(dcs[0])) {
            y_dc.add_one(cat);
        }
        y_dc.build<16>();
        Huffman_tree y_ac;
        for (auto &ac : acs[0]) {
            for (auto &[first, value] : ac) {
                y_ac.add_one(first);
            }
        }
        y_ac.build<16>();
        ///////////////////////////////
        /// CB CR
        //////////////////////////////
        Huffman_tree uv_dc;
        for (const auto &[cat, value] : convert_dc_to_size_value(dcs[1])) {
            uv_dc.add_one(cat);
        }
        for (const auto &[cat, value] : convert_dc_to_size_value(dcs[2])) {
            uv_dc.add_one(cat);
        }
        uv_dc.build<16>();
        ////////////////////
        Huffman_tree uv_ac;
        for (auto &ac : acs[1]) {
            for (auto &[first, value] : ac) {
                uv_ac.add_one(first);
            }
        }
        for (auto &ac : acs[2]) {
            for (auto &[first, value] : ac) {
                uv_ac.add_one(first);
            }
        }
        uv_ac.build<16>();

        return std::make_tuple(std::move(y_dc), std::move(y_ac), std::move(uv_dc), std::move(uv_ac));
    }

    static auto encode_huffman_dc(auto &dc, auto &huffman) {
        // std::vector<int8_t>
        std::vector<std::pair<bit_content, std::optional<bit_content>>> result;
        for (const auto &[cat, value] : convert_dc_to_size_value(dc)) {
            auto enc = huffman.getMapping(cat);
            auto val_len_pair = bit_content(enc.value, enc.length);
            if (cat == 0) {
                result.push_back({val_len_pair, std::nullopt});
            } else {
                result.push_back({val_len_pair, std::pair{value, cat}});
            }
        }
        return result;
    }

    static auto encode_huffman_ac(auto &ac, auto &huffman) {
        std::vector<std::pair<bit_content, std::optional<bit_content>>> result;
        for (auto &[first, value] : ac) {
            auto enc = huffman.getMapping(first);
            auto val_len_pair = bit_content(enc.value, enc.length);
            auto size = first & 0xFu;
            if (size == 0) {
                result.push_back({val_len_pair, std::nullopt});
            } else {
                result.push_back({val_len_pair, std::pair{value, size}});
            }
        }
        return result;
    }

private:
    template <typename T, std::endian endian = std::endian::native>
    static void write_byte(std::byte *it, const T &val) {
        if constexpr (endian == std::endian::little) {
            std::copy(reinterpret_cast<const std::byte *>(&val), reinterpret_cast<const std::byte *>(&val + 1), it);
        } else {
            auto t = std::byteswap(val);
            std::copy(reinterpret_cast<const std::byte *>(&t), reinterpret_cast<const std::byte *>(&t + 1), it);
        }
    }

    inline static int cnt = 0;
    template <typename T, std::endian endian = std::endian::native>
    static void write_data(std::vector<std::byte> &buffer, const T &val) {
        buffer.resize(buffer.size() + sizeof(T));
        auto dest = &buffer.back() - sizeof(T) + 1;
        if constexpr (endian == std::endian::little) {
            std::copy(reinterpret_cast<const std::byte *>(&val), reinterpret_cast<const std::byte *>(&val + 1), dest);
        } else {
            auto t = std::byteswap(val);
            std::copy(reinterpret_cast<const std::byte *>(&t), reinterpret_cast<const std::byte *>(&t + 1), dest);
        }
    }

    static void write_app0(std::vector<std::byte> &buffer) {
        JFIF_APP0 header{.identifier = "JFIF"};
        header.marker = 0xE0FFu;
        header.length = std::byteswap<uint16_t>(sizeof(JFIF_APP0) - sizeof(header.marker));
        header.version = 0x0101;
        header.Density_units = 0;
        header.Xdensity = 0;
        header.Ydensity = 0;
        header.Xthumbnail = 0;
        header.Ythumbnail = 0;
        write_data(buffer, header);
    }
    // DQT (Define Quantization Table)
    static void write_dqt(std::vector<std::byte> &buffer, const std::array<std::array<uint8_t, 8>, 8> &qt_y,
                          const std::array<std::array<uint8_t, 8>, 8> &qt_uv) {
        write_data<uint16_t, std::endian::big>(buffer, 0xFFDBu);
        int size_index = buffer.size();
        buffer.resize(buffer.size() + 2);
        // 精度 << 8 | ID
        write_data<uint8_t>(buffer, 0);  // 0 for Y
        for (const auto &[i, j] : zigzag<8>()) {
            write_data<uint8_t>(buffer, qt_y[i][j]);
        }
        write_data<uint8_t>(buffer, 1);  // 1 for cbcr
        for (const auto &[i, j] : zigzag<8>()) {
            write_data<uint8_t>(buffer, qt_uv[i][j]);
        }
        write_byte<uint16_t, std::endian::big>(&buffer[size_index], buffer.size() - size_index);  // 填入 size
    }

    static void write_huffman_all(std::vector<std::byte> &buffer, Huffman_tree &y_dc, Huffman_tree &y_ac,
                                  Huffman_tree &uv_dc, Huffman_tree &uv_ac) {
        write_data<uint16_t, std::endian::big>(buffer, 0xFFC4u);
        int size_index = buffer.size();
        buffer.resize(buffer.size() + 2);
        /* Y DC segment */
        write_data<uint8_t>(buffer, 0);  // [DC : 0  ac : 1 ; ID]
        write_huffman_data(buffer, y_dc);
        /* Y AC segment */
        write_data<uint8_t>(buffer, (1u << 4) | 0);
        write_huffman_data(buffer, y_ac);
        /* CB CR  DC segment */
        write_data<uint8_t>(buffer, 1);
        write_huffman_data(buffer, uv_dc);
        /* CB CR  AC segment */
        write_data<uint8_t>(buffer, (1u << 4) | 1);
        write_huffman_data(buffer, uv_ac);

        write_byte<uint16_t, std::endian::big>(&buffer[size_index], buffer.size() - size_index);  // 填入 size
    }

    static void write_huffman_data(std::vector<std::byte> &buffer, Huffman_tree &tree) {
        std::array<uint8_t, 16> bits_array{};
        for (const auto &[len, cnt] : tree.get_numOfLength()) {
            if (cnt == 0) {
                continue;
            }
            if (len == 0 || len > 16) [[unlikely]] {
                throw std::runtime_error("error : length out of range");
            }
            bits_array[len - 1] = cnt;
        }
        write_data(buffer, bits_array);
        for (const auto &val : tree.get_standard_huffman_table() | std::views::keys) {
            write_data<uint8_t>(buffer, val);
        }
    }
    static void write_sof0_segment(std::vector<std::byte> &buffer, int height, int width) {
        write_data<uint16_t, std::endian::big>(buffer, 0xFFC0u);
        int size_index = buffer.size();
        buffer.resize(buffer.size() + 2);
        write_data<uint8_t>(buffer, 8);  // precision
        write_data<uint16_t, std::endian::big>(buffer, height);
        write_data<uint16_t, std::endian::big>(buffer, width);
        write_data<uint8_t>(buffer, 3);  // 3 channels

        // Channel: Y
        write_data<uint8_t>(buffer, 1);  // component ID: Y
        if constexpr (sampling_type == Jpeg_sampling::ds_4_2_0) {
            write_data<uint8_t>(buffer, 0x22);  // sampling factors: H=1, V=1 (4:2:0)
        } else {
            write_data<uint8_t>(buffer, 0x11);  // sampling factors: H=1, V=1 (4:4:4)
        }
        write_data<uint8_t>(buffer, 0);  // quant table ID: 0

        // Channel: Cb
        write_data<uint8_t>(buffer, 2);     // component ID: Cb
        write_data<uint8_t>(buffer, 0x11);  // sampling factors: H=1, V=1
        write_data<uint8_t>(buffer, 1);     // quant table ID: 1

        // Channel: Cr
        write_data<uint8_t>(buffer, 3);     // component ID: Cr
        write_data<uint8_t>(buffer, 0x11);  // sampling factors: H=1, V=1
        write_data<uint8_t>(buffer, 1);     // quant table ID: 1

        write_byte<uint16_t, std::endian::big>(&buffer[size_index], buffer.size() - size_index);  // 填入 size
    }
    static void write_binary_stream(
        std::vector<std::byte> &buffer, Huffman_tree &y_dc_huffman, Huffman_tree &y_ac_huffman,
        Huffman_tree &uv_dc_huffman, Huffman_tree &uv_ac_huffman, std::vector<std::vector<int32_t>> &dcs,
        std::vector<std::vector<std::vector<std::pair<unsigned char, int>>>> &acs) {
        write_data<uint16_t, std::endian::big>(buffer, 0xFFDAu);
        int size_index = buffer.size();
        buffer.resize(buffer.size() + 2);
        write_data<uint8_t>(buffer, 3);     // 3 channels for Y Cb Cr
        write_data<uint8_t>(buffer, 1);     // Y
        write_data<uint8_t>(buffer, 0x00);  // Y huffman id
        write_data<uint8_t>(buffer, 2);     // Cb
        write_data<uint8_t>(buffer, 0x11);  // Cb huffman id
        write_data<uint8_t>(buffer, 3);     // Cr
        write_data<uint8_t>(buffer, 0x11);  // Cr huffman id
        write_data<uint8_t>(buffer, 0x00);  // Ss = 0
        write_data<uint8_t>(buffer, 0x3F);  // Se = 63
        write_data<uint8_t>(buffer, 0x00);  // Successive Approximation Bit Setting, Ah/Al
        write_byte<uint16_t, std::endian::big>(&buffer[size_index], buffer.size() - size_index);  // 填入 size

        BitWriter bit_writer;
        bit_writer.changeWriteSequence(WriteSequence::MSB);
        auto y_dc_encoded = encode_huffman_dc(dcs[0], y_dc_huffman);
        auto cb_dc_encoded = encode_huffman_dc(dcs[1], uv_dc_huffman);
        auto cr_dc_encoded = encode_huffman_dc(dcs[2], uv_dc_huffman);
        const size_t mcu_cnt = dcs[1].size();

        size_t mcu_ratio = 1;
        if constexpr (sampling_type == Jpeg_sampling::ds_4_2_0) {
            mcu_ratio = 4;
        }

        for (int i = 0; i < mcu_cnt; i++) {
            for (int y_index = i * mcu_ratio; y_index < i * mcu_ratio + mcu_ratio && y_index < dcs[0].size();
                 y_index++) {
                std::span acspan = acs[0];
                write_block(bit_writer, y_dc_encoded[y_index], acspan[y_index], y_ac_huffman);
            }
            {  // cb
                std::span acspan = acs[1];
                write_block(bit_writer, cb_dc_encoded[i], acspan[i], uv_ac_huffman);
            }
            {  // cr
                std::span acspan = acs[2];
                write_block(bit_writer, cr_dc_encoded[i], acspan[i], uv_ac_huffman);
            }
        }

        auto bit_buffer = bit_writer.getBuffer();

        for (auto &b : bit_buffer) {
            write_data(buffer, b);
            if (b == std::byte{0xFFu}) {
                write_data<uint8_t>(buffer, 0u);
            }
        }
        write_data<uint16_t, std::endian::big>(buffer, static_cast<uint16_t>(0xFFD9u));
    }
    static void write_block(BitWriter &bit_writer, const std::pair<bit_content, std::optional<bit_content>> &dc,
                            const std::vector<std::pair<unsigned char, int>> &ac, auto &huffman) {
        auto &[f, s] = dc;
        bit_writer.writeBitsFromMSB(f.value, f.size);
        if (s.has_value()) {
            bit_writer.writeBitsFromMSB(s.value().value, s.value().size);
        }
        auto ac_encoded = encode_huffman_ac(ac, huffman);
        for (auto &[f, s] : ac_encoded) {
            bit_writer.writeBitsFromMSB(f.value, f.size);
            if (s.has_value()) {
                bit_writer.writeBitsFromMSB(s.value().value, s.value().size);
            }
        }
    }

    static auto encode(const Matrix<colors::RGB> &src) {
        auto yuv = src.trans_convert([](const colors::RGB &color) -> colors::YCbCr {
            const auto &[r, g, b] = color;
            return colors::YCbCr{
                int(std::clamp(roundf(0.299f * r + 0.587f * g + 0.114f * b), 0.0f, 255.0f)),
                int(std::clamp(roundf(-0.168736f * r - 0.331364f * g + 0.5f * b + 128), 0.0f, 255.0f)),
                int(std::clamp(roundf(0.5f * r - 0.418688f * g - 0.081312f * b + 128), 0.0f, 255.0f))};
        });
        // 分離有助於 cache

        Matrix<int> Y(src.row(), src.col());
        Matrix<int> Cb(src.row(), src.col());
        Matrix<int> Cr(src.row(), src.col());
#pragma loop(hint_parallel(0))
        for (int i = 0; i < src.row(); i++) {
            for (int j = 0; j < src.col(); j++) {
                Y[i, j] = yuv[i, j].y;
                Cb[i, j] = yuv[i, j].cb;
                Cr[i, j] = yuv[i, j].cr;
            }
        }

        std::vector<Matrix<int>> y_seq;
        std::vector<Matrix<int>> cb_seq;
        std::vector<Matrix<int>> cr_seq;

        if constexpr (sampling_type == Jpeg_sampling::ds_4_2_0) {
            split<16>(Y)
                .trans_convert([](const auto &mtx) {
                    return split<8>(mtx);
                })
                .for_each([&y_seq](const auto &mtx) {
                    for (int i = 0; i < mtx.row(); i++) {
                        for (int j = 0; j < mtx.col(); j++) {
                            y_seq.push_back(mtx[i, j]);
                        }
                    }
                });
            split<16>(Cb).for_each([&cb_seq](auto &&mtx) {
                cb_seq.push_back(down_sample(mtx));
            });
            split<16>(Cr).for_each([&cr_seq](auto &&mtx) {
                cr_seq.push_back(down_sample(mtx));
            });
        } else {
            split<8>(Y).for_each([&y_seq](const auto &mtx) {
                y_seq.push_back(mtx);
            });
            split<8>(Cb).for_each([&cb_seq](auto &&mtx) {
                cb_seq.push_back(mtx);
            });
            split<8>(Cr).for_each([&cr_seq](auto &&mtx) {
                cr_seq.push_back(mtx);
            });
        }

        std::vector<std::vector<int32_t>> dcs;
        dcs.reserve(3);
        std::vector<std::vector<std::vector<std::pair<uint8_t, int>>>> acs;
        acs.reserve(3);
        for (const auto seq_ptr : {&y_seq, &cb_seq, &cr_seq}) {
            auto &seq = *seq_ptr;

            /* clang-format off */
            auto zigzaged = seq
                | std::views::transform([](Matrix<int> &block) {
                    block.transform([](int &x) {
                        x -= 128;
                    });
                    return block;
                })
                | std::views::transform(Dct<8>::dct<int, int>)
                | std::views::transform([seq_ptr, y_ptr = &y_seq](const Matrix<int> &block) -> Matrix<int> {
                    if (seq_ptr == y_ptr) {
                        return block.round_div_convert(y_quantization_matrix);
                    }
                    return block.round_div_convert(uv_quantization_matrix);
                })
                | std::views::transform([](const Matrix<int> &block) {
                    // zig zag 排列
                    // 忽略 uninitialize error 因為每個 index 都會填東西
                    std::array<int, 8 * 8> block_zig;  // NOLINT(*-pro-type-member-init)
                    int index = 0;
                    for (const auto &[i, j]: zigzag<8>()) {
                        block_zig[index++] = block[i, j];
                    }
                    return block_zig;
                })
                | std::ranges::to<std::vector>();
            /* clang-format on */
            std::vector<int32_t> dc(zigzaged.size());
            for (int i = 0; i < zigzaged.size(); i++) {
                // dc 使用 差分
                if (i == 0) {
                    dc[i] = zigzaged[i][0];
                } else {
                    dc[i] = zigzaged[i][0] - zigzaged[i - 1][0];
                }
            }

            auto ac = zigzaged | std::views::transform(calculate_rle) | std::ranges::to<std::vector>();

            dcs.emplace_back(std::move(dc));
            acs.emplace_back(std::move(ac));
        }

        return std::tuple{std::move(dcs), std::move(acs)};
    }

public:
    static std::pair<std::unique_ptr<std::byte[]>, size_t> write(const Matrix<colors::RGB> &src) {
        auto [dcs, acs] = encode(src);
        auto [y_dc, y_ac, uv_dc, uv_ac] = build_huffman_tree(dcs, acs);
        std::vector<std::byte> buffer;
        write_data<uint16_t, std::endian::big>(buffer, 0xFFD8u);
        write_app0(buffer);
        write_dqt(buffer, y_quantization_matrix, uv_quantization_matrix);
        write_huffman_all(buffer, y_dc, y_ac, uv_dc, uv_ac);
        write_sof0_segment(buffer, src.row(), src.col());
        write_binary_stream(buffer, y_dc, y_ac, uv_dc, uv_ac, dcs, acs);

        std::unique_ptr<std::byte[]> result(new std::byte[buffer.size()]);
        std::copy(buffer.begin(), buffer.end(), result.get());
        return {std::move(result), buffer.size()};
    }

    template <typename T>
    static std::pair<std::unique_ptr<std::byte[]>, size_t> exportToByte(const Matrix<T> &src) {
        if constexpr (std::same_as<T, colors::RGB>) {
            return write(src);
        } else {
            auto mtx = src.trans_convert([](auto &&ele) {
                return colors::color_cast<colors::RGB>(ele);
            });
            return write(mtx);
        }
    }

private:
    template <int N, int M = N, typename T>
    static Matrix<Matrix<T>> split(const Matrix<T> &mtx) {
        const int row_sz = mtx.row() / N + int(mtx.row() % N != 0);
        const int col_sz = mtx.col() / M + int(mtx.col() % M != 0);
        Matrix<Matrix<T>> result(row_sz, col_sz);
        for (int i = 0; i < row_sz; i++) {
            for (int j = 0; j < col_sz; j++) {
                Matrix<T> block(N, M);
                for (int k = 0; k < N; k++) {
                    for (int l = 0; l < M; l++) {
                        if (i * N + k >= mtx.row() || j * M + l >= mtx.col()) [[unlikely]] {
                            int new_h = std::min(i * N + k, mtx.row() - 1);
                            int new_w = std::min(j * M + l, mtx.col() - 1);
                            block[k, l] = mtx[new_h, new_w];
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

    template <typename T>
    static Matrix<T> down_sample(const Matrix<T> &mtx) {
        Matrix<T> result(8, 8);
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                auto sum =
                    mtx[i * 2, j * 2] + mtx[i * 2 + 1, j * 2] + mtx[i * 2, j * 2 + 1] + mtx[i * 2 + 1, j * 2 + 1];
                result[i, j] = sum / 4;
            }
        }
        return result;
    }

    template <typename int_type>
    static uint8_t calculate_binary_size(int_type x) {
        [[assume(x > 0)]];
#ifdef _MSC_VER
        return 32 - __lzcnt(x);
#else
        return 32 - __builtin_clz(x);
#endif
    }
    static uint8_t category(int16_t x) {
        if (x == 0) [[unlikely]] {
            return 0;
        }
        if (x < 0) x = -x;
        return calculate_binary_size(x);
    }

    constexpr static uint32_t to_amplitude(int val, int size) {
        if (val > 0) {
            return val;
        }
        return (1u << size) - 1 - std::abs(val);
    }

    static std::vector<std::pair<uint8_t, int>> calculate_rle(const std::array<int, 8 * 8> &arr) {
        // run length encoding
        std::vector<std::pair<uint8_t, int>> rle;
        int last_non_zero = 0;
        for (int i = arr.size() - 1; i >= 0; i--) {
            if (arr[i] != 0) {
                last_non_zero = i;
                break;
            }
        }
        int cnt = 0;
        for (int i = 1; i <= last_non_zero; i++) {
            if (arr[i] != 0 || cnt == 15) {
                // [run_len, size_for_bit], amp
                if (arr[i] == 0) {
                    rle.emplace_back(0xF0, 0);
                } else {
                    const uint8_t size_for_bit = calculate_binary_size(std::abs(arr[i]));
                    rle.emplace_back(unsigned(cnt) << 4u | size_for_bit, to_amplitude(arr[i], size_for_bit));
                }
                cnt = 0;
            } else {  // cnt < 15 and arr[i] == 0
                cnt++;
            }
        }
        if (arr.back() == 0) {
            rle.emplace_back(0x00, 0);  // EOB -- end of block
        }
        if (rle.size() > 63) {
            throw std::runtime_error("...");
        }
        return rle;
    }

    // constexpr static std::array<std::array<uint8_t, 8>, 8> y_quantization_matrix = {
    //     std::array<uint8_t, 8>{16, 11, 10, 16, 24, 40, 51, 61},
    //     {12, 12, 14, 19, 26, 58, 60, 55},
    //     {14, 13, 16, 24, 40, 57, 69, 56},
    //     {14, 17, 22, 29, 51, 87, 80, 62},
    //     {18, 22, 37, 56, 68, 109, 103, 77},
    //     {24, 35, 55, 64, 81, 104, 113, 92},
    //     {49, 64, 78, 87, 103, 121, 120, 101},
    //     {72, 92, 95, 98, 112, 100, 103, 99}};

    /*DQT, Row #0:   2   1   1   2   3   5   6   7
    DQT, Row #1:   1   1   2   2   3   7   7   7
    DQT, Row #2:   2   2   2   3   5   7   8   7
    DQT, Row #3:   2   2   3   3   6  10  10   7
    DQT, Row #4:   2   3   4   7   8  13  12   9
    DQT, Row #5:   3   4   7   8  10  12  14  11
    DQT, Row #6:   6   8   9  10  12  15  14  12
    DQT, Row #7:   9  11  11  12  13  12  12  12
*/
    constexpr static std::array<std::array<uint8_t, 8>, 8> y_quantization_matrix = {
        std::array<uint8_t, 8>{2, 1, 1, 2, 3, 5, 6, 7},
        {1, 1, 2, 2, 3, 7, 7, 7},
        {2, 2, 2, 3, 5, 7, 8, 7},
        {2, 2, 3, 3, 6, 10, 10, 7},
        {2, 3, 4, 7, 8, 13, 12, 9},
        {3, 4, 7, 8, 10, 12, 14, 11},
        {6, 8, 9, 10, 12, 15, 14, 12},
        {9, 11, 11, 12, 13, 12, 12, 12}};

    // constexpr static std::array<std::array<uint8_t, 8>, 8> uv_quantization_matrix = {
    //     std::array<uint8_t, 8>{17, 18, 24, 47, 99, 99, 99, 99},
    //     {18, 21, 26, 66, 99, 99, 99, 99},
    //     {24, 26, 56, 99, 99, 99, 99, 99},
    //     {47, 66, 99, 99, 99, 99, 99, 99},
    //     {99, 99, 99, 99, 99, 99, 99, 99},
    //     {99, 99, 99, 99, 99, 99, 99, 99},
    //     {99, 99, 99, 99, 99, 99, 99, 99},
    //     {99, 99, 99, 99, 99, 99, 99, 99}};

    /*  Precision=8 bits
  Destination ID=1 (Chrominance)
    DQT, Row #0:   2   2   3   6  12  12  12  12
    DQT, Row #1:   2   3   3   8  12  12  12  12
    DQT, Row #2:   3   3   7  12  12  12  12  12
    DQT, Row #3:   6   8  12  12  12  12  12  12
    DQT, Row #4:  12  12  12  12  12  12  12  12
    DQT, Row #5:  12  12  12  12  12  12  12  12
    DQT, Row #6:  12  12  12  12  12  12  12  12
    DQT, Row #7:  12  12  12  12  12  12  12  12
    Approx quality factor = 93.93 (scaling=12.14 variance*/

    constexpr static std::array<std::array<uint8_t, 8>, 8> uv_quantization_matrix = {
        std::array<uint8_t, 8>{2, 2, 3, 6, 12, 12, 12, 12},
        {2, 3, 3, 8, 12, 12, 12, 12},
        {3, 3, 7, 12, 12, 12, 12, 12},
        {6, 8, 12, 12, 12, 12, 12, 12},
        {12, 12, 12, 12, 12, 12, 12, 12},
        {12, 12, 12, 12, 12, 12, 12, 12},
        {12, 12, 12, 12, 12, 12, 12, 12},
        {12, 12, 12, 12, 12, 12, 12, 12}};
};
}  // namespace f9ay

// template <typename Char_T, auto type>
// struct std::formatter<typename f9ay::Jpeg<type>::bit_content, Char_T> : std::formatter<std::string, Char_T> {
//     auto format(const typename f9ay::Jpeg<type>::bit_content &coe, auto &ctx) const {
//         auto out = ctx.out();
//         out = std::format_to(out, "{}", f9ay::to_str(coe.value, coe.size));
//         return out;
//     }
// };

template <typename T, typename Char_T>
struct std::formatter<std::optional<T>, Char_T> : std::formatter<std::string, Char_T> {
    auto format(const std::optional<T> &op, auto &ctx) const {
        auto out = ctx.out();
        if (op.has_value()) {
            out = std::format_to(out, "{}", op.value());
        } else {
            out = std::format_to(out, "{{}}");
        }

        return out;
    }
};