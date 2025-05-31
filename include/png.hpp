#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "deflate.hpp"
#include "filter.hpp"
#include "matrix_concept.hpp"
#include "util.hpp"
namespace f9ay {
class PNG {
#pragma pack(push, 1)
    struct PNGSignature {
        std::byte signature[8];

        bool isPNG() const {
            const static std::byte pngSignature[8] = {
                std::byte{0x89}, std::byte{0x50}, std::byte{0x4e}, std::byte{0x47},
                std::byte{0x0d}, std::byte{0x0a}, std::byte{0x1a}, std::byte{0x0a}};
            return std::memcmp(signature, pngSignature, sizeof(signature)) == 0;
        }
    };

    struct IDHRChunk {
        char chunkType[4] = {'I', 'H', 'D', 'R'};
        uint32_t width;
        uint32_t height;
        uint8_t bitDepth;
        uint8_t colorType;
        uint8_t compressionMethod;
        uint8_t filterMethod;
        uint8_t interlaceMethod;
    };
    struct IDATChunk {
        char chunkType[4] = {'I', 'D', 'A', 'T'};
    };
    struct IENDChunk {
        char chunkType[4] = {'I', 'E', 'N', 'D'};
    };
#pragma pack(pop)
    static_assert(sizeof(PNGSignature) == 8);
    static_assert(sizeof(IDHRChunk) == 17);
    static_assert(sizeof(IDATChunk) == 4);
    static_assert(sizeof(IENDChunk) == 4);

public:
    template <typename T>
    static std::pair<std::unique_ptr<std::byte[]>, size_t> exportToByte(const Matrix<T>& matrix) {
        using ElementType = std::decay_t<decltype(matrix[0][0])>;
        // write signature to buffer
        // compress first to get size and data

        // force the color type to be RGB

        auto rgbMatrix = matrix.trans_convert([](const auto& origColor) {
            if constexpr (sizeof(ElementType) == 4) {
                return colors::color_cast<colors::RGBA>(origColor);
            } else {
                return colors::color_cast<colors::RGB>(origColor);
            }
        });

        // then apply filter

        constexpr auto filterType = FilterType::Sub;

        auto filteredMatrix = deflate::filter(rgbMatrix, filterType);

        // padding the first column with filter type

        auto rawDataByte = reinterpret_cast<const std::byte*>(filteredMatrix.raw());

        auto expandedMatrix =
            Matrix<std::byte>(rawDataByte, filteredMatrix.row(), filteredMatrix.col() * sizeof(ElementType));

        auto paddedMatrix = Matrix<std::byte>(expandedMatrix.row(), expandedMatrix.col() + 1);

        for (size_t i = 0; i < paddedMatrix.row(); i++) {
            paddedMatrix[i][0] = static_cast<std::byte>(filterType);
            for (size_t j = 1; j < paddedMatrix.col(); j++) {
                paddedMatrix[i][j] = expandedMatrix[i][j - 1];
            }
        }

        auto [compressedData, compressedSize] = deflate::Deflate<deflate::BlockType::Dynamic>::compress(paddedMatrix);

        size_t size =
            sizeof(PNGSignature) + /* PNG簽名  */
            4 +                    /* IHDR長度欄位  */
            sizeof(IDHRChunk) +    /* IHDR區塊        */
            4 +                    /* IHDR的CRC */
            4 +                    /* IDAT長度欄位 */
            sizeof(IDATChunk) +    /* IDAT區塊頭     */
            compressedSize +       /* 壓縮後的數據 */
            4 +                    /* IDAT的CRC      */
            4 +                    /* IEND長度欄位 */
            sizeof(IENDChunk) +    /* IEND區塊 */
            4;                     /* IEND的CRC   */

        std::unique_ptr<std::byte[]> data(new std::byte[size]);
        auto pngSignature = PNGSignature{{std::byte{0x89}, std::byte{0x50}, std::byte{0x4e}, std::byte{0x47},
                                          std::byte{0x0d}, std::byte{0x0a}, std::byte{0x1a}, std::byte{0x0a}}};
        auto offset = std::copy(
            reinterpret_cast<std::byte*>(&pngSignature), reinterpret_cast<std::byte*>(&pngSignature + 1), data.get());

        uint32_t ihdrDataLength = sizeof(IDHRChunk) - sizeof(decltype(IDHRChunk::chunkType));
        offset = _writeToBuffer(offset, ihdrDataLength, ihdrDataLength);

        // write IHDR chunk
        auto ihdrChunk = IDHRChunk{};
        ihdrChunk.width = checkAndSwapToBigEndian(matrix.col());
        ihdrChunk.height = checkAndSwapToBigEndian(matrix.row());
        ihdrChunk.bitDepth = 8;
        if constexpr (std::is_same_v<ElementType, colors::BGR> || std::is_same_v<ElementType, colors::RGB>) {
            ihdrChunk.colorType = 2;  // RGB
        } else if constexpr (std::is_same_v<ElementType, colors::BGRA> || std::is_same_v<ElementType, colors::RGBA>) {
            ihdrChunk.colorType = 6;  // RGBA
        } else {
            static_assert("Unsupported color type");
        }
        ihdrChunk.compressionMethod = 0;
        ihdrChunk.filterMethod = 0;
        ihdrChunk.interlaceMethod = 0;
        offset =
            std::copy(reinterpret_cast<std::byte*>(&ihdrChunk), reinterpret_cast<std::byte*>(&ihdrChunk + 1), offset);
        // calculate CRC
        auto ihdrChunkCRC = _calculateCRC(reinterpret_cast<std::byte*>(&ihdrChunk), sizeof(IDHRChunk));

        // write crc to buffer
        offset = _writeToBuffer(offset, sizeof(uint32_t), ihdrChunkCRC);

        // write IDAT chunk
        auto idatChunk = IDATChunk{};
        auto idatChunkLength = static_cast<uint32_t>(compressedSize);
        offset = _writeToBuffer(offset, sizeof(uint32_t), idatChunkLength);
        // create idat chunks with data
        offset =
            std::copy(reinterpret_cast<std::byte*>(&idatChunk), reinterpret_cast<std::byte*>(&idatChunk + 1), offset);

        // create temp buffer for idat chunk
        // inorder to calculate crc
        std::unique_ptr<std::byte[]> idatData(new std::byte[compressedSize + sizeof(IDATChunk)]);
        auto tempOffset = std::copy(
            reinterpret_cast<std::byte*>(&idatChunk), reinterpret_cast<std::byte*>(&idatChunk + 1), idatData.get());

        std::copy(compressedData.get(), compressedData.get() + compressedSize, tempOffset);

        // write idat chunk to buffer
        offset = std::copy(compressedData.get(), compressedData.get() + compressedSize, offset);

        // calculate crc
        auto idatChunkCRC = _calculateCRC(idatData.get(), compressedSize + sizeof(IDATChunk));

        // write crc to buffer
        offset = _writeToBuffer(offset, sizeof(uint32_t), idatChunkCRC);

        // write IEND chunk length
        uint32_t iendDataLength = 0;
        offset = _writeToBuffer(offset, sizeof(uint32_t), iendDataLength);

        // write IEND chunk
        auto iendChunk = IENDChunk{};
        offset =
            std::copy(reinterpret_cast<std::byte*>(&iendChunk), reinterpret_cast<std::byte*>(&iendChunk + 1), offset);
        // calculate CRC
        auto iendChunkCRC = _calculateCRC(reinterpret_cast<std::byte*>(&iendChunk), sizeof(IENDChunk));
        // write crc to buffer
        offset = _writeToBuffer(offset, sizeof(uint32_t), iendChunkCRC);

        return {std::move(data), size};
    }

private:
    static uint32_t _calculateCRC(const std::byte* data, size_t length) {
        uint32_t crc = 0xffffffff;
        for (size_t i = 0; i < length; i++) {
            crc ^= static_cast<uint32_t>(data[i]);
            for (int j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xedb88320;
                } else {
                    crc >>= 1;
                }
            }
        }
        return ~crc;
    }

    template <typename T>
    static std::byte* _writeToBuffer(std::byte* buffer, size_t length, T value) {
        if (std::endian::native == std::endian::little) {
            value = std::byteswap(value);
        }
        return std::copy(reinterpret_cast<std::byte*>(&value), reinterpret_cast<std::byte*>(&value + 1), buffer);
    }
    template <typename T>
    static std::byte* _writeToBuffer(std::byte* buffer, size_t length, const T* value) {
        if (std::endian::native == std::endian::little) {
            for (size_t i = 0; i < length; i++) {
                value[i] = std::byteswap(value[i]);
            }
        }
        return std::copy(
            reinterpret_cast<const std::byte*>(value), reinterpret_cast<const std::byte*>(value + length), buffer);
    }
};
}  // namespace f9ay