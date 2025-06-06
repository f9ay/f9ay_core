#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <format>
#include <map>
#include <memory>
#include <thread>

#include "colors.hpp"
#include "filter.hpp"
#include "huffman_tree.hpp"
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
    static std::pair<std::unique_ptr<std::byte[]>, size_t> compress(Matrix<T>& img) {
        // Compress the input data

        BitWriter bitWriter;
        // calculate the adler32 checksum

        auto adler32 = _calculateAdler32(reinterpret_cast<std::byte*>(img.raw()), img.row() * img.col());

        auto imgSpan = img.flattenToSpan();

        bitWriter.changeWriteSequence(WriteSequence::MSB);

        // write zlib header
        bitWriter.writeBitsFromMSB(std::byte{0x78}, 8);  // CMF
        bitWriter.writeBitsFromMSB(std::byte{0x01}, 8);  // FLG

        switch (blockType) {
            case BlockType::Uncompressed:

                throw std::runtime_error("Uncompressed block type is not supported in this implementation");
            case BlockType::Fixed:
                _compressFixed(imgSpan, bitWriter);
                break;
            case BlockType::Dynamic:
                constexpr size_t blockSize = 65536;  // 64KB
                size_t imgSize = imgSpan.size();
                if (imgSize > blockSize) {
                    // split the image into blocks of size blockSize
                    auto numBlocks = (imgSize + blockSize - 1) / blockSize;  // round up division

                    for (size_t i = 0; i < numBlocks; ++i) {
                        size_t start = i * blockSize;
                        size_t end = std::min(start + blockSize, imgSize);
                        auto blockSpan = imgSpan.subspan(start, end - start);
                        _compressDynamic(blockSpan, bitWriter, i == numBlocks - 1 ? 1 : 0);
                    }
                } else {
                    _compressDynamic(imgSpan, bitWriter, 1);
                }
                break;
        }

        bitWriter.changeWriteSequence(WriteSequence::MSB);
        // write adler32 checksum
        bitWriter.writeBitsFromMSB(adler32, 32);  // ADLER32
        // write the compressed data
        auto buffer = bitWriter.getBuffer();
        auto compressedData = std::make_unique<std::byte[]>(buffer.size());
        std::copy(buffer.begin(), buffer.end(), compressedData.get());
        return {std::move(compressedData), buffer.size()};
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
    static void _compressFixed(std::span<T> imgSpan, BitWriter& bitWriter) {
        static_assert(_fixedDistanceTable.size() == 32769, "Fixed distance table size mismatch");
        static_assert(_fixedHuffmanCodesTable.size() == 288, "Fixed Huffman table size mismatch");
        static_assert(_fixedLengthTable.size() == 259, "Fixed length table size mismatch");
        using value_type = std::decay_t<decltype(imgSpan[0])>;

        // start change write from MSB to LSB
        // according to deflate spec

        bitWriter.changeWriteSequence(WriteSequence::LSB);
        // write the block header
        bitWriter.writeBit(1);  // BFINAL 1 == last block
        bitWriter.writeBitsFromLSB(std::byte{0b00000001},
                                   2);  // BTYPE (fixed)

        // Apply LZ77 compression
        auto vec = LZ77::lz77EncodeSlow(imgSpan);

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
    }
    template <typename T>
    static void _compressDynamic(const std::span<T>& flattened, BitWriter& bitWriter, uint8_t BFINAL = 1) {
        static_assert(_fixedDistanceTable.size() == 32769, "Fixed distance table size mismatch");
        static_assert(_fixedHuffmanCodesTable.size() == 288, "Fixed Huffman table size mismatch");
        static_assert(_fixedLengthTable.size() == 259, "Fixed length table size mismatch");
        using value_type = std::decay_t<decltype(flattened[0])>;

        auto lz77Compressed = LZ77::lz77EncodeSlow(flattened);

        // Build Huffman tree for dynamic compression
        Huffman_tree litLengthTree;
        Huffman_tree distanceTree;

        for (auto& [distance, length, literal] : lz77Compressed) {
            if (literal.has_value()) {
                litLengthTree.add_one(static_cast<uint16_t>(literal.value()));
            }
            if (length > 0) {
                litLengthTree.add_one(_fixedLengthTable[length].code);
                distanceTree.add_one(_fixedDistanceTable[distance].code);
            }
        }

        litLengthTree.add_one(256);  // End of block code

        litLengthTree.build<15, true>();
        distanceTree.build<15, true>();

        auto litLengthCodes = litLengthTree.get_standard_huffman_table();
        auto distanceCodes = distanceTree.get_standard_huffman_table();

        std::ranges::sort(litLengthCodes, [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        std::ranges::sort(distanceCodes, [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        // padding with zero if the symbol doesn't exist

        int maxLitLengthSymbol = litLengthCodes.back().first;

        int maxDistSymbol = distanceCodes.size() > 0 ? distanceCodes.back().first : 0;

        std::vector<std::pair<int, int>> paddedLitLengthCodes;
        for (int i = 0; i <= maxLitLengthSymbol; i++) {
            auto it = std::find_if(litLengthCodes.begin(), litLengthCodes.end(), [i](const auto& pair) {
                return pair.first == i;
            });
            if (it != litLengthCodes.end()) {
                paddedLitLengthCodes.push_back(*it);
            } else {
                paddedLitLengthCodes.emplace_back(i, 0);  // padding with zero
            }
        }
        std::vector<std::pair<int, int>> paddedDistanceCodes;
        for (int i = 0; i <= maxDistSymbol; i++) {
            auto it = std::find_if(distanceCodes.begin(), distanceCodes.end(), [i](const auto& pair) {
                return pair.first == i;
            });
            if (it != distanceCodes.end()) {
                paddedDistanceCodes.push_back(*it);
            } else {
                paddedDistanceCodes.emplace_back(i, 0);  // padding with zero
            }
        }

        if (paddedDistanceCodes.size() < 2) {
            paddedDistanceCodes.clear();
            paddedDistanceCodes.emplace_back(0, 1);
            paddedDistanceCodes.emplace_back(1, 1);
        }
        // print the padded litLengthCodes and distanceCodes for debugging

        // start change write from MSB to LSB
        // according to deflate spec

        bitWriter.changeWriteSequence(WriteSequence::LSB, false);
        // write the block header
        bitWriter.writeBit(BFINAL);  // BFINAL 1 == last block
        bitWriter.writeBitsFromLSB(std::byte{0b00000010},
                                   2);  // BTYPE (dynamic)

        _writeCodeLengths(bitWriter, paddedLitLengthCodes, paddedDistanceCodes);

        // write actual data
        for (auto& [distance, length, literal] : lz77Compressed) {
            if (length == 0 && literal.has_value()) {
                auto [litCode, litLength] = litLengthTree.getMapping(static_cast<uint16_t>(literal.value()));

                bitWriter.writeBitsFromMSB(litCode, litLength);
            } else {
                auto [lengthCode, lengthExtraBit, lengthExtraBitLength] = _fixedLengthTable[length];

                auto [huffLengthCode, huffLength] = litLengthTree.getMapping(lengthCode);

                bitWriter.writeBitsFromMSB(huffLengthCode, huffLength);

                if (lengthExtraBitLength > 0) {
                    bitWriter.writeBitsFromLSB(lengthExtraBit, lengthExtraBitLength);
                }

                auto [distCode, distExtraBit, distExtraBitLength] = _fixedDistanceTable[distance];
                auto [huffDistCode, huffDistLength] = distanceTree.getMapping(distCode);

                bitWriter.writeBitsFromMSB(huffDistCode, huffDistLength);
                if (distExtraBitLength > 0) {
                    bitWriter.writeBitsFromLSB(distExtraBit, distExtraBitLength);
                }

                if (literal.has_value()) {
                    auto [litCode, litLength] = litLengthTree.getMapping(static_cast<uint16_t>(literal.value()));

                    bitWriter.writeBitsFromMSB(litCode, litLength);
                }
            }
        }

        // write end of block
        auto [eobCode, eobLength] = litLengthTree.getMapping(256);

        bitWriter.writeBitsFromMSB(eobCode, eobLength);
        // write the last byte
        if (BFINAL == 1) {
            // check if the last byte is not aligned
            while (bitWriter.getBitPos() % 8 != 0) {
                bitWriter.writeBit(0);
            }
        }
    }

    static void _writeCodeLengths(BitWriter& bitWriter, const std::vector<std::pair<int, int>>& litLengthCodes,
                                  const std::vector<std::pair<int, int>>& distanceCodes) {
        auto litLengthRLE = _getDeflateRLE(litLengthCodes);
        auto distRLE = _getDeflateRLE(distanceCodes);

        decltype(litLengthRLE) combinedRLE;
        combinedRLE.insert(combinedRLE.end(), litLengthRLE.begin(), litLengthRLE.end());
        combinedRLE.insert(combinedRLE.end(), distRLE.begin(), distRLE.end());

        // build Code Lengths huffman tree
        Huffman_tree codeLengthTree;

        for (const auto& [value, extraFreq] : combinedRLE) {
            codeLengthTree.add_one(value);
        }

        codeLengthTree.build<7, true>();

        auto codeLengthHuffmanTable = codeLengthTree.get_standard_huffman_table();

        std::array<int, 19> codeLengthArray = {0};

        for (const auto& [symbol, length] : codeLengthHuffmanTable) {
            auto it = std::find(_rleOrder.begin(), _rleOrder.end(), symbol);

            if (it != _rleOrder.end()) {
                auto index = std::distance(_rleOrder.begin(), it);

                codeLengthArray[index] = length;
            }
        }
        int maxIndex = 18;
        while (maxIndex > 3 && codeLengthArray[maxIndex] == 0) {
            maxIndex--;
        }

        uint8_t hlit = static_cast<uint8_t>(litLengthCodes.size() - 257);
        uint8_t hdist = static_cast<uint8_t>(distanceCodes.size() - 1);
        uint8_t hclen = static_cast<uint8_t>(maxIndex + 1 - 4);

        // write the HLIT header
        bitWriter.writeBitsFromLSB(hlit, 5);
        bitWriter.writeBitsFromLSB(hdist, 5);
        bitWriter.writeBitsFromLSB(hclen, 4);

        for (int i = 0; i <= maxIndex; i++) {
            bitWriter.writeBitsFromLSB(codeLengthArray[i], 3);
        }

        for (auto& [code, extraFreq] : combinedRLE) {
            auto [huffmanCode, huffmanLength] = codeLengthTree.getMapping(code);

            bitWriter.writeBitsFromMSB(huffmanCode, huffmanLength);
            if (code == 16) {
                bitWriter.writeBitsFromLSB(extraFreq, 2);
            } else if (code == 17) {
                bitWriter.writeBitsFromLSB(extraFreq, 3);
            } else if (code == 18) {
                bitWriter.writeBitsFromLSB(extraFreq, 7);
            }
        }
    }

    static auto _getDeflateRLE(const std::vector<std::pair<int, int>>& huffmanCodeTable) {
        std::vector<std::pair<int, int>> result;

        if (huffmanCodeTable.empty()) {
            return result;
        }

        for (int i = 0; i < huffmanCodeTable.size(); i++) {
            auto [symbol, length] = huffmanCodeTable[i];

            int repeatCount = 1;

            for (int j = i + 1; j < huffmanCodeTable.size(); j++) {
                auto [nextSymbol, nextLength] = huffmanCodeTable[j];

                if (nextLength == length) {
                    repeatCount++;
                } else {
                    break;
                }
            }

            if (length == 0 && repeatCount >= 3) {  // code 17
                if (repeatCount < 11) {
                    result.emplace_back(17, static_cast<uint8_t>(repeatCount - 3));
                } else {
                    if (repeatCount > 138) {
                        repeatCount = 138;
                    }
                    result.emplace_back(18, static_cast<uint8_t>(repeatCount - 11));
                }
                i += repeatCount - 1;
            } else {
                if (repeatCount > 3) {
                    // emplace back the first one
                    result.emplace_back(static_cast<uint8_t>(length), 0);
                    repeatCount -= 1;
                    if (repeatCount > 6) {
                        repeatCount = 6;
                    }
                    result.emplace_back(16, static_cast<uint8_t>(repeatCount - 3));  // need to delete the first one
                    i += repeatCount;
                } else {
                    result.emplace_back(static_cast<uint8_t>(length), 0);  // code 0-15
                }
            }
        }

        return result;
    }

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
    static constexpr std::array<int, 19> _rleOrder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
};
}  // namespace f9ay::deflate