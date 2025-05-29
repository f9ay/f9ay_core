
#pragma once
#include <print>
#include <tuple>
#include <variant>

#include "colors.hpp"
#include "importer.hpp"
#include "matrix.hpp"
#include "matrix_concept.hpp"
#include "util.hpp"

namespace f9ay {
class Bmp {
#pragma pack(push, 1)
    struct FileHeader {
        unsigned short type;                 /* type : Magic identifier,一般為BM(0x42,0x4d) */
        unsigned int size;                   /* File size in bytes,全部的檔案大小 */
        unsigned short reserved1, reserved2; /* 保留欄位 */
        unsigned int offset;                 /* Offset to image data, bytes */
    };
    struct InfoHeader {
        unsigned int size;             /* Info Header size in bytes */
        int width, height;             /* Width and height of image */
        unsigned short planes;         /* Number of colour planes */
        unsigned short bits;           /* Bits per pixel */
        unsigned int compression;      /* Compression type */
        unsigned int imagesize;        /* Image size in bytes */
        int xResolution, yResolution;  /* Pixels per meter */
        unsigned int nColours;         /* Number of colours */
        unsigned int importantColours; /* Important colours */
    };
    struct ColorPalette {
        std::byte blue;
        std::byte green;
        std::byte red;
        std::byte alpha;
    };
#pragma pack(pop)
    static_assert(sizeof(FileHeader) == 14);
    static_assert(sizeof(InfoHeader) == 40);
    static_assert(sizeof(ColorPalette) == 4);

public:
    static Midway importFromByte(const std::byte *source) {
        const FileHeader *fileHeader = safeMemberAssign<FileHeader>(source);
        const InfoHeader *infoHeader = safeMemberAssign<InfoHeader>(source + sizeof(FileHeader));
        if (infoHeader->compression == true) {
            throw std::runtime_error("Unsupported BMP compression");
        }

        if (infoHeader->bits <= 8 && infoHeader->compression == false) {
            // read ColorPalette 先不管
        }
        if (infoHeader->bits == 32) {
            // BGRA
            Matrix<colors::BGRA> result(infoHeader->height, infoHeader->width);
            read_data(source, result, fileHeader, infoHeader);
            return result;
        }
        if (infoHeader->bits == 24) {
            // BGR
            Matrix<colors::BGR> result(infoHeader->height, infoHeader->width);
            read_data(source, result, fileHeader, infoHeader);
            return result;
        }
        throw std::runtime_error("Unsupported BMP format");
    }

    template <typename T>
        requires std::same_as<T, colors::BGR> || std::same_as<T, colors::BGRA>
    static std::pair<std::unique_ptr<std::byte[]>, size_t> write(const Matrix<T> &mtx) {
        using value_type = std::decay_t<decltype(mtx[0, 0])>;
        constexpr auto element_size = sizeof(value_type);
        const auto rawRowSize = align<4>(mtx.col() * element_size);
        const auto size = sizeof(FileHeader) + sizeof(InfoHeader) + rawRowSize * mtx.row();
        std::unique_ptr<std::byte[]> result(new std::byte[size]);
        FileHeader fileHeader{};
        fileHeader.type = 0x4D42;
        fileHeader.size = size;
        fileHeader.offset = sizeof(FileHeader) + sizeof(InfoHeader);
        InfoHeader infoHeader{};
        infoHeader.size = sizeof(InfoHeader);
        infoHeader.width = mtx.col();
        infoHeader.height = mtx.row();
        infoHeader.planes = 1;
        infoHeader.bits = element_size * 8;
        infoHeader.compression = false;
        infoHeader.imagesize = mtx.col() * mtx.row() * sizeof(value_type);

        //////////////////////////////////////////
        auto it = std::copy(
            reinterpret_cast<std::byte *>(&fileHeader), reinterpret_cast<std::byte *>(&fileHeader + 1), result.get());
        it = std::copy(reinterpret_cast<std::byte *>(&infoHeader), reinterpret_cast<std::byte *>(&infoHeader + 1), it);
        auto data = it;

        for (int i = 0; i < mtx.row(); i++) {
            std::byte *row = data + (std::abs(mtx.row()) - 1 - i) * rawRowSize;
            for (int j = 0; j < mtx.col(); j++) {
                reinterpret_cast<value_type *>(row)[j] = mtx[i, j];
            }
        }

        return {std::move(result), size};
    }

    template <typename T>
    static std::pair<std::unique_ptr<std::byte[]>, size_t> exportToByte(const Matrix<T> &src) {
        if constexpr (std::same_as<T, colors::BGR> || std::same_as<T, colors::BGRA>) {
            return write(src);
        } else if constexpr (requires { T().a; }) {
            auto mtx = src.trans_convert([](auto &&ele) {
                return colors::color_cast<colors::BGRA>(ele);
            });
            return write(mtx);
        } else {
            auto mtx = src.trans_convert([](auto &&ele) {
                return colors::color_cast<colors::BGR>(ele);
            });
            return write(mtx);
        }
    }

private:
    template <typename T>
    static void read_data(const std::byte *source, Matrix<T> &dst, const FileHeader *fileHeader,
                          const InfoHeader *infoHeader) {
        const bool seqRead = infoHeader->height < 0;  // 當 height 為負時 順序讀取
        const auto data = source + fileHeader->offset;
        const auto pixelSize = infoHeader->bits / 8;  // byte
        const auto rowSize = infoHeader->width * pixelSize;
        const auto rawRowSize = align<4>(rowSize);
        for (int i = 0; i < std::abs(infoHeader->height); i++) {
            const std::byte *row = nullptr;
            if (seqRead) {  // suppose compiler 會 優化
                row = data + i * rawRowSize;
            } else {
                row = data + (infoHeader->height - 1 - i) * rawRowSize;
            }
            for (int j = 0; j < infoHeader->width; j++) {
                dst[i, j] = *reinterpret_cast<const T *>(&row[j * pixelSize]);
            }
        }
    }
};
};  // namespace f9ay
