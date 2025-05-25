#pragma once

#include <algorithm>
#include <any>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

#include "dct.hpp"
#include "huffman_tree.hpp"
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

        if (*reinterpret_cast<const uint16_t *>(it) != 0xFFDAu) {
            throw std::logic_error("JFIF SOS NOT match");
        }
        it += 2;
        // 	compressed image data

        return {};
    }
    inline static std::byte *start;
    static std::pair<std::unique_ptr<std::byte[]>, size_t> write(const Matrix<colors::RGB> &src) {
        auto [dcs, acs] = encode(src);
        auto [y_dc, y_ac, uv_dc, uv_ac] = build_huffman_tree(dcs, acs);
        y_dc.validate();
        y_ac.validate();
        uv_dc.validate();
        uv_ac.validate();
        std::println("acs size : {}", acs[1][0, 0].size());
        std::println("acs : {}", acs[1]);
        size_t size = 65354133;  // TODO
        std::unique_ptr<std::byte[]> result(new std::byte[size]{});
        start = result.get();
        auto it = result.get();
        write_byte<uint16_t, std::endian::big>(it, 0xFFD8u);
        it = write_app0(it);
        it = write_dqt(it, y_quantization_matrix, uv_quantization_matrix);
        it = write_huffman_all(it, y_dc, y_ac, uv_dc, uv_ac);
        it = write_sof0_segment(it, src.row(), src.col());
        it = write_binary_stream(it, y_dc, y_ac, uv_dc, uv_ac, dcs, acs);
        return {std::move(result), it - result.get()};
    }

private:
    template <typename T, std::endian endian = std::endian::native>
    static void write_byte(std::byte *&it, const T &val) {
        if constexpr (endian == std::endian::little) {
            //*reinterpret_cast<T *>(it) = val;
            it =
                std::copy(reinterpret_cast<const std::byte *>(&val), reinterpret_cast<const std::byte *>(&val + 1), it);
        } else {
            auto t = std::byteswap(val);
            it = std::copy(reinterpret_cast<const std::byte *>(&t), reinterpret_cast<const std::byte *>(&t + 1), it);
        }
    }
    static std::byte *write_app0(std::byte *it) {
        JFIF_APP0 header{.identifier = "JFIF"};
        header.marker = 0xE0FFu;
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
        write_byte<uint16_t, std::endian::big>(it, 0xFFDBu);
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

    static std::byte *write_huffman_all(std::byte *it, Huffman_tree &y_dc, Huffman_tree &y_ac, Huffman_tree &uv_dc,
                                        Huffman_tree &uv_ac) {
        /* Y DC segment */
        write_byte<uint16_t, std::endian::big>(it, 0xFFC4u);
        auto size_ptr = it;
        it += 2;
        write_byte<uint8_t>(it, 0);  // [DC : 0  ac : 1 ; ID]
        it = write_huffman_data(it, y_dc);
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        std::println("size => {}", it - size_ptr);
        /* Y AC segment */
        write_byte<uint16_t, std::endian::big>(it, 0xFFC4u);
        size_ptr = it;
        it += 2;
        write_byte<uint8_t>(it, (1u << 4) | 0);
        it = write_huffman_data(it, y_ac);
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        /* CB CR  DC segment */
        write_byte<uint16_t, std::endian::big>(it, 0xFFC4u);
        size_ptr = it;
        it += 2;
        write_byte<uint8_t>(it, 1);
        it = write_huffman_data(it, uv_dc);
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        /* CB CR  AC segment */
        write_byte<uint16_t, std::endian::big>(it, 0xFFC4u);
        size_ptr = it;
        it += 2;
        write_byte<uint8_t>(it, (1u << 4) | 1);
        it = write_huffman_data(it, uv_ac);
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);

        return it;
    }

    static std::byte *write_huffman_all0(std::byte *it, Huffman_tree &y_dc, Huffman_tree &y_ac, Huffman_tree &uv_dc,
                                         Huffman_tree &uv_ac) {
        write_byte<uint16_t, std::endian::big>(it, 0xFFC4u);

        auto size_ptr = it;
        it += 2;
        /* Y DC segment */
        write_byte<uint8_t>(it, 0);  // [DC : 0  ac : 1 ; ID]
        it = write_huffman_data(it, y_dc);
        /* Y AC segment */
        write_byte<uint8_t>(it, (1u << 4) | 0);
        it = write_huffman_data(it, y_ac);
        /* CB CR  DC segment */
        write_byte<uint8_t>(it, 1);
        it = write_huffman_data(it, uv_dc);
        /* CB CR  AC segment */
        write_byte<uint8_t>(it, (1u << 4) | 1);
        it = write_huffman_data(it, uv_ac);

        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        return it;
    }

    static std::byte *write_huffman_data(std::byte *it, Huffman_tree &tree) {
        std::array<uint8_t, 16> bits_array{};
        auto &standard_huffman_table = tree.get_standard_huffman_table();
        std::println("standard_huffman_table : {}", standard_huffman_table);
        for (const auto &[val, len] : standard_huffman_table) {
            if (len == 0) {
                throw std::runtime_error("error");
            }
            bits_array[len - 1]++;  // size 不可能是 0
        }
        for (int i = 0; i < 16; i++) {
            write_byte<uint8_t>(it, bits_array[i]);
        }
        for (const auto &val : tree.get_standard_huffman_table() | std::views::keys) {
            write_byte<uint8_t>(it, val);
        }
        return it;
    }
    static std::byte *write_sof0_segment(std::byte *it, int height, int width) {
        write_byte<uint16_t, std::endian::big>(it, 0xFFC0u);  // SOF0 marker
        auto size_ptr = it;
        it += 2;                     // segment length (precision(1) + height(2) + width(2) + channels(1) +3*channels)
        write_byte<uint8_t>(it, 8);  // precision
        write_byte<uint16_t, std::endian::big>(it, height);
        write_byte<uint16_t, std::endian::big>(it, width);  // width
        write_byte<uint8_t>(it, 3);                         // 3 channels

        // Channel: Y
        write_byte<uint8_t>(it, 1);     // component ID: Y
        write_byte<uint8_t>(it, 0x11);  // sampling factors: H=2, V=2 (4:2:0)
        write_byte<uint8_t>(it, 0);     // quant table ID: 0

        // Channel: Cb
        write_byte<uint8_t>(it, 2);     // component ID: Cb
        write_byte<uint8_t>(it, 0x11);  // sampling factors: H=1, V=1
        write_byte<uint8_t>(it, 1);     // quant table ID: 1

        // Channel: Cr
        write_byte<uint8_t>(it, 3);     // component ID: Cr
        write_byte<uint8_t>(it, 0x11);  // sampling factors: H=1, V=1
        write_byte<uint8_t>(it, 1);     // quant table ID: 1

        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        return it;
    }
    static std::byte *write_binary_stream(
        std::byte *it, Huffman_tree &y_dc_huffman, Huffman_tree &y_ac_huffman, Huffman_tree &uv_dc_huffman,
        Huffman_tree &uv_ac_huffman, std::vector<std::vector<int32_t>> &dcs,
        std::vector<Matrix<std::vector<std::pair<unsigned char, int>>>> &acs) {
        write_byte<uint16_t, std::endian::big>(it, 0xFFDAu);
        auto size_ptr = it;
        it += 2;
        write_byte<uint8_t>(it, 3);     // 3 channels for Y Cb Cr
        write_byte<uint8_t>(it, 1);     // Y
        write_byte<uint8_t>(it, 0x00);  // Y huffman id
        write_byte<uint8_t>(it, 2);     // Cb
        write_byte<uint8_t>(it, 0x11);  // Cb huffman id
        write_byte<uint8_t>(it, 3);     // Cr
        write_byte<uint8_t>(it, 0x11);  // Cr huffman id
        write_byte<uint8_t>(it, 0x00);  // Ss = 0
        write_byte<uint8_t>(it, 0x3F);  // Se = 63
        write_byte<uint8_t>(it, 0x00);  // Successive Approximation Bit Setting, Ah/Al
        write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);

        BitWriter bit_writer;
        bit_writer.changeWriteSequence(WriteSequence::MSB);
        auto y_dc_encoded = encode_huffman_dc(dcs[0], y_dc_huffman);
        auto cb_dc_encoded = encode_huffman_dc(dcs[1], uv_dc_huffman);
        auto cr_dc_encoded = encode_huffman_dc(dcs[2], uv_dc_huffman);
        const size_t mcu_cnt = dcs[1].size();
        // std::println("mcu_cnt : {}", mcu_cnt);
        const size_t mcu_ratio = 1;
        // std::println("{}\n{}\n{}", y_dc_encoded, cb_dc_encoded, cr_dc_encoded);
        for (int i = 0; i < mcu_cnt; i++) {
            for (int y_index = i * mcu_ratio; y_index < i * mcu_ratio + mcu_ratio && y_index < dcs[0].size();
                 y_index++) {
                auto acspan = acs[0].flattenToSpan();
                write_block(bit_writer, y_dc_encoded[y_index], acspan[y_index], y_ac_huffman);
            }
            {  // cb
                auto acspan = acs[1].flattenToSpan();
                write_block(bit_writer, cb_dc_encoded[i], acspan[i], uv_ac_huffman);
            }
            {  // cr
                auto acspan = acs[2].flattenToSpan();
                write_block(bit_writer, cr_dc_encoded[i], acspan[i], uv_ac_huffman);
            }
        }

        auto buffer = bit_writer.getBuffer();

        // std::println("cb dc statdard mapping {}", uv_dc_huffman.get_standard_huffman_mapping());
        // std::println("cr ac statdard mapping {}", uv_ac_huffman.get_standard_huffman_mapping());
        auto current = it;
        for (auto &b : buffer) {
            // std::println("{:X} : {} ", (size_t)(current++ - start), std::bitset<8>(unsigned(b)).to_string());
            if (b == std::byte{0xFFu}) {
                // std::println("{:X} : {} ", (size_t)(current++ - start), std::bitset<8>(unsigned(0)).to_string());
            }
        }

        for (auto &b : buffer) {
            write_byte(it, b);
            if (b == std::byte{0xFFu}) {
                write_byte<uint8_t>(it, 0u);
            }
        }
        write_byte<uint16_t, std::endian::big>(it, static_cast<uint16_t>(0xFFD9u));
        // write_byte<uint16_t, std::endian::big>(size_ptr, it - size_ptr);
        // std::print(" {} ", it - size_ptr);
        return it;
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
        // Downsampling
        // TODO 原圖不是二的倍數
        //         Matrix<uint8_t> Cb(src.row() / 2, src.col() / 2);
        //         Matrix<uint8_t> Cr(src.row() / 2, src.col() / 2);
        // #pragma loop(hint_parallel(0))
        //         for (int i = 0; i < Cb.row(); i++) {
        //             for (int j = 0; j < Cr.col(); j++) {
        //                 Cb[i, j] = yuv[i * 2, j * 2].cb;
        //                 Cr[i, j] = yuv[i * 2, j * 2].cr;
        //             }
        //         }

        auto split_y = split<8>(Y);
        auto split_cb = split<8>(Cb);
        auto split_cr = split<8>(Cr);

        std::vector<std::vector<int32_t>> dcs;
        dcs.reserve(3);
        std::vector<Matrix<std::vector<std::pair<uint8_t, int>>>> acs;
        acs.reserve(3);
        for (const auto m_ptr : {&split_y, &split_cb, &split_cr}) {
            auto &m = *m_ptr;

            // TODO 用表達式模板優化
            /* clang-format off */
            auto zigzaged =
                m.transform([](Matrix<int> &block) {
                     block.transform([](int &x) {
                         x -= 128;
                     });
                })
                .trans_convert(Dct<8>::dct<int, int>)
                .transform([m_ptr, y_ptr = &split_y](Matrix<int> &block) {
                    if (m_ptr == y_ptr) {
                        block.round_div(y_quantization_matrix);
                    }
                    else {
                        block.round_div(uv_quantization_matrix);
                    }
                })
                .trans_convert([](const Matrix<int> &block) {
                    // zig zag 排列
                    // 忽略 uninitialize error 因為每個 index 都會填東西
                    std::array<int, 8 * 8> block_zig;  // NOLINT(*-pro-type-member-init)
                    int index = 0;
                    for (auto &[i, j] : zigzag<8>()) {
                        block_zig[index++] = block[i, j];
                    }
                    return block_zig;
                });
            /* clang-format on */
            std::vector<int32_t> dc(zigzaged.flattenToSpan().size());
            for (int i = 0; i < zigzaged.flattenToSpan().size(); i++) {
                // dc 使用 差分
                if (i == 0) {
                    dc[i] = zigzaged.flattenToSpan()[i][0];
                } else {
                    dc[i] = zigzaged.flattenToSpan()[i][0] - zigzaged.flattenToSpan()[i - 1][0];
                }
            }

            auto dc_category = dc | std::views::all | std::views::transform([](const int16_t x) {
                                   return category(x);
                               });

            auto ac = zigzaged.trans_convert(calculate_rle);

            // TODO 這裡之後可以優化到 計算RLE的地方
            auto ac_merged = ac.flattenToSpan() | std::views::join | std::views::keys;

            dcs.emplace_back(std::move(dc));
            acs.emplace_back(std::move(ac));
        }

        return std::tuple{std::move(dcs), std::move(acs)};
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
               }) |
               std::ranges::to<std::vector>();
    }

    static auto build_huffman_tree(std::vector<std::vector<int32_t>> &dcs,
                                   std::vector<Matrix<std::vector<std::pair<unsigned char, int>>>> &acs) {
        Huffman_tree y_dc;
        for (auto &[cat, value] : convert_dc_to_size_value(dcs[0])) {
            y_dc.add_one(cat);
        }
        y_dc.build<Huffman_tree::cls::DC>();
        Huffman_tree y_ac;
        for (auto &ac : acs[0].flattenToSpan()) {
            for (auto &[first, value] : ac) {
                y_ac.add_one(first);
            }
        }
        y_ac.build<Huffman_tree::cls::AC>();
        ///////////////////////////////
        /// CB CR
        //////////////////////////////
        Huffman_tree uv_dc;
        for (auto &[cat, value] : convert_dc_to_size_value(dcs[1])) {
            uv_dc.add_one(cat);
        }
        for (auto &[cat, value] : convert_dc_to_size_value(dcs[2])) {
            uv_dc.add_one(cat);
        }
        uv_dc.build<Huffman_tree::cls::DC>();
        ////////////////////
        Huffman_tree uv_ac;
        for (auto &ac : acs[1].flattenToSpan()) {
            for (auto &[first, value] : ac) {
                // if (first == 0) throw std::runtime_error("invalid huffman value");
                uv_ac.add_one(first);
            }
        }
        for (auto &ac : acs[2].flattenToSpan()) {
            for (auto &[first, value] : ac) {
                // if (first == 0) throw std::runtime_error("invalid huffman value");
                uv_ac.add_one(first);
            }
        }
        uv_ac.build<Huffman_tree::cls::AC>();

        return std::make_tuple(std::move(y_dc), std::move(y_ac), std::move(uv_dc), std::move(uv_ac));
    }

    static auto encode_huffman_dc(auto &dc, auto &huffman) {
        // std::vector<int8_t>
        std::vector<std::pair<bit_content, std::optional<bit_content>>> result;
        for (auto &[cat, value] : convert_dc_to_size_value(dc)) {
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
    template <int N, int M = N, typename T>
    static Matrix<Matrix<T>> split(Matrix<T> &mtx) {
        const int row_sz = mtx.row() / N + int(mtx.row() % N != 0);
        const int col_sz = mtx.col() / M + int(mtx.col() % M != 0);
        Matrix<Matrix<T>> result(row_sz, col_sz);
        for (int i = 0; i < row_sz; i++) {
            for (int j = 0; j < col_sz; j++) {
                Matrix<T> block(N, M);
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

    template <typename int_type>
        requires requires { sizeof(int_type) <= 2; }
    static int8_t calculate_binary_size(int_type x) {
        if (x <= 0) throw std::runtime_error("binary size must be greater than zero");
        [[assume(x > 0)]];
#ifdef _MSC_VER
        // lzcnt return leading zero => 0000 0000 1000 0000  return 8
        return static_cast<int8_t>(16 - __lzcnt16(x));
#else
        return 32 - __builtin_clz(x);
#endif
    }

public:
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
        // 從 1 開始 因為 dc 不編碼
        for (int i = 1; i <= last_non_zero; i++) {
            if (arr[i] != 0 || cnt == 15) {
                // [run_len, size_for_bit], amp
                if (arr[i] == 0) {
                    rle.emplace_back(0xF0, 0);
                } else {
                    const int8_t size_for_bit = calculate_binary_size(std::abs(arr[i]));
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
            throw std::runtime_error("mjaoiwdhbAO");
        }
        // std::println("rle check {}", arr);
        return rle;
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
    friend int main(int argc, char **argv);
    friend int main();
    friend void test_jpeg();
    friend void test_rle(std::array<int16_t, 64> &arr);
};
}  // namespace f9ay

template <typename Char_T>
struct std::formatter<f9ay::Jpeg::bit_content, Char_T> : std::formatter<std::string, Char_T> {
    auto format(const f9ay::Jpeg::bit_content &coe, auto &ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{}", f9ay::to_str(coe.value, coe.size));
        return out;
    }
};

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
