#pragma once

#include <bit>
#include <cstddef>
#include <map>
#include <memory>

#include "colors.hpp"
#include "filter.hpp"
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
    static std::pair<std::unique_ptr<std::byte[]>, size_t> compress(Matrix<T>& matrix, FilterType filterType) {
        // Compress the input data

        switch (blockType) {
            case BlockType::Uncompressed:
                break;
            case BlockType::Fixed:
                return _compressFixed(matrix, filterType);
            case BlockType::Dynamic:
                return _compressDynamic(matrix, filterType);
        }
        throw std::runtime_error("Unsupported block type");
    }

private:
    struct FixedHuffmanCode {
        uint16_t bitCode;
        uint16_t length;
    };

    struct FixedLengthCode {
        uint16_t code;
        uint8_t extraBit;
        uint16_t extraBitLength;
    };

    struct FixedDistanceCode {
        uint8_t code;
        uint16_t extraBit;
        uint8_t extraBitLength;
    };

    template <typename T>
    static std::pair<std::unique_ptr<std::byte[]>, size_t> _compressFixed(
        Matrix<T>& origMatrix, FilterType filterType) {
        static_assert(_fixedDistanceTable.size() == 32769, "Fixed distance table size mismatch");
        static_assert(_fixedHuffmanCodesTable.size() == 288, "Fixed Huffman table size mismatch");
        static_assert(_fixedLengthTable.size() == 259, "Fixed length table size mismatch");
        using value_type = std::decay_t<decltype(origMatrix[0, 0])>;

        auto filteredMatrix = filter(origMatrix, filterType);

        // expand the scanline to include the filter type

        auto filterTypeByte = static_cast<std::byte>(static_cast<uint8_t>(filterType));

        auto expandedRaw = reinterpret_cast<std::byte*>(filteredMatrix.raw());

        auto expandedMatrix =
            Matrix{expandedRaw, filteredMatrix.row(), filteredMatrix.col() * static_cast<int>(sizeof(value_type))};

        Matrix<std::byte> expandedMatrixWithFilter(expandedMatrix.row(), expandedMatrix.col() + 1);

        for (int i = 0; i < expandedMatrix.row(); i++) {
            expandedMatrixWithFilter[i][0] = filterTypeByte;
            for (int j = 1; j < expandedMatrix.col(); j++) {
                expandedMatrixWithFilter[i][j] = expandedMatrix[i][j - 1];  // copy the data
            }
        }

        // calculate the adler32 checksum

        auto adler32 = _calculateAdler32(reinterpret_cast<std::byte*>(expandedMatrixWithFilter.raw()),
                                         expandedMatrixWithFilter.row() * expandedMatrixWithFilter.col());

        BitWriter bitWriter;

        // write zlib header
        bitWriter.writeBitsFromMSB(std::byte{0x78}, 8);  // CMF
        bitWriter.writeBitsFromMSB(std::byte{0x01}, 8);  // FLG

        // start change write from MSB to LSB
        // according to deflate spec

        bitWriter.changeWriteSequence(WriteSequence::LSB);
        // write the block header
        bitWriter.writeBit(1);  // BFINAL 1 == last block
        bitWriter.writeBitsFromLSB(std::byte{0b00000001},
                                   2);  // BTYPE (fixed)

        auto flattened = expandedMatrixWithFilter.flattenToSpan();
        // Apply LZ77 compression
        auto vec = LZ77::lz77EncodeSlow(flattened);

        for (auto& [offset, length, value] : vec) {
            if (length == 0 && value.has_value()) {
                auto [litCode, litLength] = _fixedHuffmanCodesTable[static_cast<uint8_t>(value.value())];

                bitWriter.writeBitsFromMSB(litCode, litLength);
            } else {
                auto [lengthCode, extraBit, extraBitLength] = _fixedLengthTable[length];
                auto [huffCode, huffLength] = _fixedHuffmanCodesTable[lengthCode];

                bitWriter.writeBitsFromMSB(huffCode, huffLength);

                if (extraBitLength > 0) {
                    bitWriter.writeBitsFromLSB(extraBit, extraBitLength);
                }

                // write distance
                auto [distCode, distExtraBit, distExtraBitLength] = _fixedDistanceTable[offset];

                // write first 5 fixed distance bits
                bitWriter.writeBitsFromMSB(distCode, 5);
                if (distExtraBitLength > 0) {
                    // check endian
                    bitWriter.writeBitsFromLSB(distExtraBit, distExtraBitLength);
                }

                // if the value is not empty, write the value
                // to be compatible with the lz77 algorithm
                if (value.has_value()) {
                    auto [litCode, litLength] = _fixedHuffmanCodesTable[static_cast<uint8_t>(value.value())];

                    bitWriter.writeBitsFromMSB(litCode, litLength);
                }
            }
        }

        // write end of block
        auto [eobCode, eobLength] = _fixedHuffmanCodesTable[256];
        bitWriter.writeBitsFromMSB(eobCode, eobLength);

        // write the last byte
        // check if the last byte is not aligned
        while (bitWriter.getBitPos() % 8 != 0) {
            bitWriter.writeBit(0);
        }

        bitWriter.changeWriteSequence(WriteSequence::MSB);

        // adler32 = checkAndSwapToBigEndian(adler32);

        // write adler32
        bitWriter.writeBitsFromMSB(adler32, 32);

        while (bitWriter.getBitPos() % 8 != 0) {
            bitWriter.writeBit(0);
        }
        auto buffer = bitWriter.getBuffer();

        auto size = buffer.size();

        auto compressedData = std::make_unique<std::byte[]>(size);
        std::copy(buffer.begin(), buffer.end(), compressedData.get());
        return {std::move(compressedData), size};
    }

    static std::pair<std::unique_ptr<std::byte[]>, size_t> _compressDynamic(
        Matrix<std::byte>& matrix, FilterType filterType) {}
    static uint32_t _calculateAdler32(const std::byte* data, size_t length) {
        uint32_t a = 1, b = 0;

        for (size_t i = 0; i < length; i++) {
            a = (a + static_cast<uint32_t>(data[i])) % 65521;
            b = (b + a) % 65521;
        }

        return (b << 16) | a;
    }
    static consteval auto _buildFixedHuffmanTable() {
        std::array<FixedHuffmanCode, 288> fixedHuffmanCodesTable{};

        auto generateCode = [&fixedHuffmanCodesTable](
                                uint16_t startLitValue, uint16_t endLitValue, uint16_t baseCode, uint16_t length) {
            for (uint16_t i = startLitValue; i <= endLitValue; i++) {
                uint16_t code = baseCode + (i - startLitValue);
                fixedHuffmanCodesTable[i] = {
                    code,   // bitCode
                    length  // length
                };
            }
        };

        generateCode(0, 143, 0b00110000, 8);
        generateCode(144, 255, 0b110010000, 9);
        generateCode(256, 279, 0b0000000, 7);
        generateCode(280, 287, 0b11000000, 8);

        return fixedHuffmanCodesTable;
    }

    static consteval auto _buildFixedLengthTable() {
        std::array<FixedLengthCode, 259> fixedLengthTable{};

        auto generateLengthCode =
            [&fixedLengthTable](uint16_t start, uint16_t end, uint16_t code, uint8_t extraBitsLength) {
                uint8_t extraBitCodeBase = 0;
                for (uint16_t i = start; i <= end; i++) {
                    uint8_t extraBit = extraBitCodeBase++;
                    fixedLengthTable[i] = {
                        code,            // code
                        extraBit,        // extraBit
                        extraBitsLength  // extraBitLength
                    };
                }
            };

        generateLengthCode(3, 3, 257, 0);
        generateLengthCode(4, 4, 258, 0);
        generateLengthCode(5, 5, 259, 0);
        generateLengthCode(6, 6, 260, 0);
        generateLengthCode(7, 7, 261, 0);
        generateLengthCode(8, 8, 262, 0);
        generateLengthCode(9, 9, 263, 0);
        generateLengthCode(10, 10, 264, 0);
        generateLengthCode(11, 12, 265, 1);
        generateLengthCode(13, 14, 266, 1);
        generateLengthCode(15, 16, 267, 1);
        generateLengthCode(17, 18, 268, 1);
        generateLengthCode(19, 22, 269, 2);
        generateLengthCode(23, 26, 270, 2);
        generateLengthCode(27, 30, 271, 2);
        generateLengthCode(31, 34, 272, 2);
        generateLengthCode(35, 42, 273, 3);
        generateLengthCode(43, 50, 274, 3);
        generateLengthCode(51, 58, 275, 3);
        generateLengthCode(59, 66, 276, 3);
        generateLengthCode(67, 82, 277, 4);
        generateLengthCode(83, 98, 278, 4);
        generateLengthCode(99, 114, 279, 4);
        generateLengthCode(115, 130, 280, 4);
        generateLengthCode(131, 162, 281, 5);
        generateLengthCode(163, 194, 282, 5);
        generateLengthCode(195, 226, 283, 5);
        generateLengthCode(227, 257, 284, 5);
        generateLengthCode(258, 258, 285, 0);  // End of block code
        return fixedLengthTable;
    }

    static consteval auto _buildFixedDistanceTable() {
        std::array<FixedDistanceCode, 32769> fixedDistanceTable{};

        auto generateDistanceCode =
            [&fixedDistanceTable](uint16_t start, uint16_t end, uint8_t code, uint8_t extraBitLength) {
                uint16_t extraBitBase = 0;
                for (uint16_t i = start; i <= end; i++) {
                    uint16_t extraBit = extraBitBase++;
                    fixedDistanceTable[i] = {
                        code,           // code
                        extraBit,       // extraBit
                        extraBitLength  // extraBitLength
                    };
                }
            };

        generateDistanceCode(1, 1, 0, 0);
        generateDistanceCode(2, 2, 1, 0);
        generateDistanceCode(3, 3, 2, 0);
        generateDistanceCode(4, 4, 3, 0);
        generateDistanceCode(5, 6, 4, 1);
        generateDistanceCode(7, 8, 5, 1);
        generateDistanceCode(9, 12, 6, 2);
        generateDistanceCode(13, 16, 7, 2);
        generateDistanceCode(17, 24, 8, 3);
        generateDistanceCode(25, 32, 9, 3);
        generateDistanceCode(33, 48, 10, 4);
        generateDistanceCode(49, 64, 11, 4);
        generateDistanceCode(65, 96, 12, 5);
        generateDistanceCode(97, 128, 13, 5);
        generateDistanceCode(129, 192, 14, 6);
        generateDistanceCode(193, 256, 15, 6);
        generateDistanceCode(257, 384, 16, 7);
        generateDistanceCode(385, 512, 17, 7);
        generateDistanceCode(513, 768, 18, 8);
        generateDistanceCode(769, 1024, 19, 8);
        generateDistanceCode(1025, 1536, 20, 9);
        generateDistanceCode(1537, 2048, 21, 9);
        generateDistanceCode(2049, 3072, 22, 10);
        generateDistanceCode(3073, 4096, 23, 10);
        generateDistanceCode(4097, 6144, 24, 11);
        generateDistanceCode(6145, 8192, 25, 11);
        generateDistanceCode(8193, 12288, 26, 12);
        generateDistanceCode(12289, 16384, 27, 12);
        generateDistanceCode(16385, 24576, 28, 13);
        generateDistanceCode(24577, 32768, 29, 13);

        return fixedDistanceTable;
    }

    static constexpr auto _fixedHuffmanCodesTable = _buildFixedHuffmanTable();
    static constexpr auto _fixedLengthTable = _buildFixedLengthTable();
    static constexpr auto _fixedDistanceTable = _buildFixedDistanceTable();
};
}  // namespace f9ay::deflate