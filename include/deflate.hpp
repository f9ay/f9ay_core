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
    static std::pair<std::unique_ptr<std::byte[]>, size_t> compress(
        Matrix<T>& matrix, FilterType filterType) {
        // Compress the input data

        switch (blockType) {
            case BlockType::Uncompressed:
                break;
            case BlockType::Fixed:
                return _compressFixed(matrix, filterType);
                break;
            case BlockType::Dynamic:
                break;
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

    static constexpr auto _buildFixedHuffmanTable() {
        std::array<FixedHuffmanCode, 288> fixedHuffmanCodesTable{};

        for (int i = 0; i < 144; i++) {
            constexpr uint16_t baseCode = 0b00110000;
            uint16_t code = baseCode + i;

            static_assert(baseCode + 143 == 0b10111111);

            fixedHuffmanCodesTable[i] = {code, 8};
        }

        for (int i = 144; i < 256; i++) {
            constexpr uint16_t baseCode = 0b110010000;
            uint16_t code = baseCode + (i - 144);

            static_assert(baseCode + (255 - 144) == 0b111111111);

            fixedHuffmanCodesTable[i] = {code, 9};
        }

        fixedHuffmanCodesTable[256] = {0, 7};  // end of block

        for (int i = 257; i < 280; i++) {
            constexpr uint16_t baseCode = 0b0000000;
            uint16_t code = baseCode + (i - 256);
            static_assert(baseCode + (279 - 256) == 0b0010111);

            fixedHuffmanCodesTable[i] = {code, 7};
        }

        for (int i = 280; i < 288; i++) {
            constexpr uint16_t baseCode = 0b11000000;
            uint16_t code = baseCode + (i - 280);
            static_assert(baseCode + (287 - 280) == 0b11000111);

            fixedHuffmanCodesTable[i] = {code, 8};
        }

        return fixedHuffmanCodesTable;
    }

    static constexpr auto _buildFixedLengthTable() {
        std::array<FixedLengthCode, 259> fixedLengthTable{};

        for (int i = 3; i < 11; i++) {
            constexpr uint16_t baseCode = 257;
            const uint16_t code = baseCode + (i - 3);

            static_assert(baseCode + (10 - 3) == 264);

            fixedLengthTable[i] = {code, 0, 0};
        }

        for (int i = 11; i < 13; i++) {
            constexpr uint16_t baseCode = 265;
            // no need to add on baseCode
            uint8_t extraBit = (i - 11);

            fixedLengthTable[i] = {baseCode, extraBit, 1};
        }

        for (int i = 13; i < 15; i++) {
            constexpr uint16_t baseCode = 266;
            // no need to add on baseCode
            uint8_t extraBit = (i - 13);

            fixedLengthTable[i] = {baseCode, extraBit, 1};
        }

        for (int i = 15; i < 17; i++) {
            constexpr uint16_t baseCode = 267;
            // no need to add on baseCode
            uint8_t extraBit = (i - 15);

            fixedLengthTable[i] = {baseCode, extraBit, 1};
        }

        for (int i = 17; i < 19; i++) {
            constexpr uint16_t baseCode = 268;
            // no need to add on baseCode
            uint8_t extraBit = (i - 17);

            fixedLengthTable[i] = {baseCode, extraBit, 1};
        }

        for (int i = 19; i < 23; i++) {
            constexpr uint16_t baseCode = 269;
            // no need to add on baseCode
            uint8_t extraBit = (i - 19);

            fixedLengthTable[i] = {baseCode, extraBit, 2};
        }

        for (int i = 23; i < 27; i++) {
            constexpr uint16_t baseCode = 270;
            // no need to add on baseCode
            uint8_t extraBit = (i - 23);

            fixedLengthTable[i] = {baseCode, extraBit, 2};
        }

        for (int i = 27; i < 31; i++) {
            constexpr uint16_t baseCode = 271;
            // no need to add on baseCode
            uint8_t extraBit = (i - 27);

            fixedLengthTable[i] = {baseCode, extraBit, 2};
        }

        for (int i = 31; i < 35; i++) {
            constexpr uint16_t baseCode = 272;
            // no need to add on baseCode
            uint8_t extraBit = (i - 31);

            fixedLengthTable[i] = {baseCode, extraBit, 2};
        }

        for (int i = 35; i < 43; i++) {
            constexpr uint16_t baseCode = 273;
            // no need to add on baseCode
            uint8_t extraBit = (i - 35);

            fixedLengthTable[i] = {baseCode, extraBit, 3};
        }

        for (int i = 43; i < 51; i++) {
            constexpr uint16_t baseCode = 274;
            // no need to add on baseCode
            uint8_t extraBit = (i - 43);

            fixedLengthTable[i] = {baseCode, extraBit, 3};
        }

        for (int i = 51; i < 59; i++) {
            constexpr uint16_t baseCode = 275;
            // no need to add on baseCode
            uint8_t extraBit = (i - 51);

            fixedLengthTable[i] = {baseCode, extraBit, 3};
        }

        for (int i = 59; i < 67; i++) {
            constexpr uint16_t baseCode = 276;
            // no need to add on baseCode
            uint8_t extraBit = (i - 59);

            fixedLengthTable[i] = {baseCode, extraBit, 3};
        }

        for (int i = 67; i < 83; i++) {
            constexpr uint16_t baseCode = 277;
            // no need to add on baseCode
            uint8_t extraBit = (i - 67);

            fixedLengthTable[i] = {baseCode, extraBit, 4};
        }

        for (int i = 83; i < 99; i++) {
            constexpr uint16_t baseCode = 278;
            // no need to add on baseCode
            uint8_t extraBit = (i - 83);

            fixedLengthTable[i] = {baseCode, extraBit, 4};
        }

        for (int i = 99; i < 115; i++) {
            constexpr uint16_t baseCode = 279;
            // no need to add on baseCode
            uint8_t extraBit = (i - 99);

            fixedLengthTable[i] = {baseCode, extraBit, 4};
        }

        for (int i = 115; i < 131; i++) {
            constexpr uint16_t baseCode = 280;
            // no need to add on baseCode
            uint8_t extraBit = (i - 115);

            fixedLengthTable[i] = {baseCode, extraBit, 4};
        }

        for (int i = 131; i < 163; i++) {
            constexpr uint16_t baseCode = 281;
            // no need to add on baseCode
            uint8_t extraBit = (i - 131);

            fixedLengthTable[i] = {baseCode, extraBit, 5};
        }

        for (int i = 163; i < 195; i++) {
            constexpr uint16_t baseCode = 282;
            // no need to add on baseCode
            uint8_t extraBit = (i - 163);

            fixedLengthTable[i] = {baseCode, extraBit, 5};
        }

        for (int i = 195; i < 227; i++) {
            constexpr uint16_t baseCode = 283;
            // no need to add on baseCode
            uint8_t extraBit = (i - 195);

            fixedLengthTable[i] = {baseCode, extraBit, 5};
        }

        for (int i = 227; i < 258; i++) {
            constexpr uint16_t baseCode = 284;
            // no need to add on baseCode
            uint8_t extraBit = (i - 227);

            fixedLengthTable[i] = {baseCode, extraBit, 5};
        }

        fixedLengthTable[258] = {285, 0, 0};  // end of block

        return fixedLengthTable;
    }

    static constexpr auto _buildFixedDistanceTable() {
        std::array<FixedDistanceCode, 32769> fixedDistanceTable{};

        // 距離1-4直接對應編碼0-3，無額外位元
        for (int i = 1; i <= 4; i++) {
            fixedDistanceTable[i] = {static_cast<uint8_t>(i - 1), 0, 0};
        }

        // 編碼4: 距離5-6 (需要1個額外位元)
        fixedDistanceTable[5] = {4, 0, 1};
        fixedDistanceTable[6] = {4, 1, 1};

        // 編碼5: 距離7-8 (需要1個額外位元)
        fixedDistanceTable[7] = {5, 0, 1};
        fixedDistanceTable[8] = {5, 1, 1};

        // 編碼6: 距離9-12 (需要2個額外位元)
        for (int i = 9; i <= 12; i++) {
            fixedDistanceTable[i] = {6, static_cast<uint16_t>(i - 9), 2};
        }

        // 編碼7: 距離13-16 (需要2個額外位元)
        for (int i = 13; i <= 16; i++) {
            fixedDistanceTable[i] = {7, static_cast<uint16_t>(i - 13), 2};
        }

        // 編碼8: 距離17-24 (需要3個額外位元)
        for (int i = 17; i <= 24; i++) {
            fixedDistanceTable[i] = {8, static_cast<uint16_t>(i - 17), 3};
        }

        // 編碼9: 距離25-32 (需要3個額外位元)
        for (int i = 25; i <= 32; i++) {
            fixedDistanceTable[i] = {9, static_cast<uint16_t>(i - 25), 3};
        }

        // 編碼10: 距離33-48 (需要4個額外位元)
        for (int i = 33; i <= 48; i++) {
            fixedDistanceTable[i] = {10, static_cast<uint16_t>(i - 33), 4};
        }

        // 編碼11: 距離49-64 (需要4個額外位元)
        for (int i = 49; i <= 64; i++) {
            fixedDistanceTable[i] = {11, static_cast<uint16_t>(i - 49), 4};
        }

        // 編碼12: 距離65-96 (需要5個額外位元)
        for (int i = 65; i <= 96; i++) {
            fixedDistanceTable[i] = {12, static_cast<uint16_t>(i - 65), 5};
        }

        // 編碼13: 距離97-128 (需要5個額外位元)
        for (int i = 97; i <= 128; i++) {
            fixedDistanceTable[i] = {13, static_cast<uint16_t>(i - 97), 5};
        }

        // 編碼14-29: 依此類推...
        // 編碼14: 距離129-192
        for (int i = 129; i <= 192; i++) {
            fixedDistanceTable[i] = {14, static_cast<uint16_t>(i - 129), 6};
        }

        // 編碼15: 距離193-256
        for (int i = 193; i <= 256; i++) {
            fixedDistanceTable[i] = {15, static_cast<uint16_t>(i - 193), 6};
        }

        // 編碼16: 距離257-384
        for (int i = 257; i <= 384; i++) {
            fixedDistanceTable[i] = {16, static_cast<uint16_t>(i - 257), 7};
        }

        // 編碼17: 距離385-512
        for (int i = 385; i <= 512; i++) {
            fixedDistanceTable[i] = {17, static_cast<uint16_t>(i - 385), 7};
        }

        // 編碼18: 距離513-768
        for (int i = 513; i <= 768; i++) {
            fixedDistanceTable[i] = {18, static_cast<uint16_t>(i - 513), 8};
        }

        // 編碼19: 距離769-1024
        for (int i = 769; i <= 1024; i++) {
            fixedDistanceTable[i] = {19, static_cast<uint16_t>(i - 769), 8};
        }

        // 編碼20: 距離1025-1536
        for (int i = 1025; i <= 1536; i++) {
            fixedDistanceTable[i] = {20, static_cast<uint16_t>((i - 1025)), 9};
        }

        // 編碼21: 距離1537-2048
        for (int i = 1537; i <= 2048; i++) {
            fixedDistanceTable[i] = {21, static_cast<uint16_t>((i - 1537)), 9};
        }

        // 編碼22: 距離2049-3072
        for (int i = 2049; i <= 3072; i++) {
            fixedDistanceTable[i] = {22, static_cast<uint16_t>((i - 2049)), 10};
        }

        // 編碼23: 距離3073-4096
        for (int i = 3073; i <= 4096; i++) {
            fixedDistanceTable[i] = {23, static_cast<uint16_t>((i - 3073)), 10};
        }

        // 編碼24: 距離4097-6144
        for (int i = 4097; i <= 6144; i++) {
            fixedDistanceTable[i] = {24, static_cast<uint16_t>((i - 4097)), 11};
        }

        // 編碼25: 距離6145-8192
        for (int i = 6145; i <= 8192; i++) {
            fixedDistanceTable[i] = {25, static_cast<uint16_t>((i - 6145)), 11};
        }

        // 編碼26: 距離8193-12288
        for (int i = 8193; i <= 12288; i++) {
            fixedDistanceTable[i] = {26, static_cast<uint16_t>((i - 8193)), 12};
        }

        // 編碼27: 距離12289-16384
        for (int i = 12289; i <= 16384; i++) {
            fixedDistanceTable[i] = {27, static_cast<uint16_t>((i - 12289)),
                                     12};
        }

        // 編碼28: 距離16385-24576
        for (int i = 16385; i <= 24576; i++) {
            fixedDistanceTable[i] = {28, static_cast<uint16_t>((i - 16385)),
                                     13};
        }

        // 編碼29: 距離24577-32768
        for (int i = 24577; i <= 32768; i++) {
            fixedDistanceTable[i] = {29, static_cast<uint16_t>((i - 24577)),
                                     13};
        }

        return fixedDistanceTable;
    }

    template <typename T>
    static std::pair<std::unique_ptr<std::byte[]>, size_t> _compressFixed(
        Matrix<T>& origMatrix, FilterType filterType) {
        static_assert(_fixedDistanceTable.size() == 32769,
                      "Fixed distance table size mismatch");
        static_assert(_fixedHuffmanCodesTable.size() == 288,
                      "Fixed Huffman table size mismatch");
        static_assert(_fixedLengthTable.size() == 259,
                      "Fixed length table size mismatch");
        using value_type = std::decay_t<decltype(origMatrix[0, 0])>;

        auto filteredMatrix = filter(origMatrix, filterType);

        // expand the scanline to include the filter type

        auto filterTypeByte =
            static_cast<std::byte>(static_cast<uint8_t>(filterType));

        auto expandedRaw = reinterpret_cast<std::byte*>(filteredMatrix.raw());

        auto expandedMatrix =
            Matrix{expandedRaw, filteredMatrix.row(),
                   filteredMatrix.col() * static_cast<int>(sizeof(value_type))};

        Matrix<std::byte> expandedMatrixWithFilter(expandedMatrix.row(),
                                                   expandedMatrix.col() + 1);

        for (int i = 0; i < expandedMatrix.row(); i++) {
            expandedMatrixWithFilter[i][0] = filterTypeByte;
            for (int j = 1; j < expandedMatrix.col(); j++) {
                expandedMatrixWithFilter[i][j] =
                    expandedMatrix[i][j - 1];  // copy the data
            }
        }

        // calculate the adler32 checksum

        auto adler32 = _calculateAdler32(
            reinterpret_cast<std::byte*>(expandedMatrixWithFilter.raw()),
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
        auto vec = LZ77::lz77Encode(flattened);

        for (auto& [offset, length, value] : vec) {
            if (length == 0 && value.has_value()) {
                auto [litCode, litLength] =
                    _fixedHuffmanCodesTable[static_cast<uint8_t>(
                        value.value())];

                bitWriter.writeBitsFromMSB(litCode, litLength);
            } else {
                auto [lengthCode, extraBit, extraBitLength] =
                    _fixedLengthTable[length];
                auto [huffCode, huffLength] =
                    _fixedHuffmanCodesTable[lengthCode];

                bitWriter.writeBitsFromMSB(huffCode, huffLength);

                if (extraBitLength > 0) {
                    bitWriter.writeBitsFromLSB(extraBit, extraBitLength);
                }

                // write distance
                auto [distCode, distExtraBit, distExtraBitLength] =
                    _fixedDistanceTable[offset];

                // write first 5 fixed distance bits
                bitWriter.writeBitsFromMSB(distCode, 5);
                if (distExtraBitLength > 0) {
                    // check endian
                    bitWriter.writeBitsFromLSB(distExtraBit,
                                               distExtraBitLength);
                }

                // if the value is not empty, write the value
                // to be compatible with the lz77 algorithm
                if (value.has_value()) {
                    auto [litCode, litLength] =
                        _fixedHuffmanCodesTable[static_cast<uint8_t>(
                            value.value())];

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

    static uint32_t _calculateAdler32(const std::byte* data, size_t length) {
        uint32_t a = 1, b = 0;

        for (size_t i = 0; i < length; i++) {
            a = (a + static_cast<uint32_t>(data[i])) % 65521;
            b = (b + a) % 65521;
        }

        return (b << 16) | a;
    }

    static constexpr auto _fixedHuffmanCodesTable = _buildFixedHuffmanTable();
    static constexpr auto _fixedLengthTable = _buildFixedLengthTable();
    static constexpr auto _fixedDistanceTable = _buildFixedDistanceTable();
};
}  // namespace f9ay::deflate