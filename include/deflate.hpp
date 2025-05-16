#pragma once
#include <bit>
#include <cstddef>
#include <map>
#include <memory>

#include "lz77_compress.hpp"
#include "matrix.hpp"
#include "matrix_concept.hpp"
#include "util.hpp"

namespace f9ay::deflate {
enum class BlockType { Uncompressed = 0, Fixed = 1, Dynamic = 2 };

template <BlockType blockType>
class Deflate {
public:
    template <typename T>
    static std::pair<std::unique_ptr<std::byte>, size_t> compress(
        Matrix<T>& matrix) {
        // Compress the input data

        switch (blockType) {
            case BlockType::Uncompressed:
                break;
            case BlockType::Fixed:
                _compressFixed(matrix);
                break;
            case BlockType::Dynamic:
                break;
        }
    }

private:
    struct FixedHuffmanCode {
        u_int16_t code;
        u_int16_t length;
    };

    constexpr auto _buildFixedHuffmanCode() {
        std::array<FixedHuffmanCode, 288> fixedHuffmanCodesTable;

        for (size_t i = 0; i < 144; i++) {
            constexpr u_int16_t baseCode = 0b00110000;
            u_int16_t code = baseCode + i;

            static_assert(baseCode + 143 == 0b10111111);

            if (std::endian::native == std::endian::little) {
                code = std::byteswap(code);
            }

            fixedHuffmanCodesTable[i] = {code, 8};
        }

        for (size_t i = 144; i < 256; i++) {
            constexpr u_int16_t baseCode = 0b110010000;
            u_int16_t code = baseCode + (i - 144);

            static_assert(baseCode + (255 - 144) == 0b111111111);
            if (std::endian::native == std::endian::little) {
                code = std::byteswap(code);
            }

            fixedHuffmanCodesTable[i] = {code, 9};
        }

        fixedHuffmanCodesTable[256] = {0, 7};  // end of block

        for (size_t i = 257; i < 280; i++) {
            constexpr u_int16_t baseCode = 0b0000000;
            u_int16_t code = baseCode + (i - 256);
            static_assert(baseCode + (279 - 256) == 0b0010111);
            if (std::endian::native == std::endian::little) {
                code = std::byteswap(code);
            }
            fixedHuffmanCodesTable[i] = {code, 7};
        }

        for (size_t i = 280; i < 288; i++) {
            constexpr u_int16_t baseCode = 0b11000000;
            u_int16_t code = baseCode + (i - 280);
            static_assert(baseCode + (287 - 280) == 0b11111111);
            if (std::endian::native == std::endian::little) {
                code = std::byteswap(code);
            }
            fixedHuffmanCodesTable[i] = {code, 8};
        }

        return fixedHuffmanCodesTable;
    }

    template <typename T>
    static std::pair<std::unique_ptr<std::byte>, size_t> _compressFixed(
        Matrix<T>& matrix) {
        using value_type = std::decay_t<decltype(matrix[0, 0])>;

        auto flattened = matrix.flattenToSpan();

        for (auto& element : flattened) {
            element = checkAndSwapToBigEndian(element);
        }
        // Apply LZ77 compression
        auto vec = LZ77::lz77Encode(flattened);

        for (auto& [offset, length, value] : vec) {
        }
    }
};
}  // namespace f9ay::deflate