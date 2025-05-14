#pragma once

#include <print>
#include <tuple>
#include <variant>

#include "colors.hpp"
#include "importer.hpp"
#include "matrix.hpp"
#include "matrix_concept.hpp"

namespace f9ay {
class Bmp {
#pragma pack(push, 1)
    struct FileHeader {
        unsigned short type; /* type : Magic identifier,一般為BM(0x42,0x4d) */
        unsigned int size;   /* File size in bytes,全部的檔案大小 */
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
    static Midway import(const std::byte *source) {
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

    template<MATRIX_CONCEPT Matrix_Type>
    static std::unique_ptr<std::byte[]> write(Matrix_Type mtx) {
        FileHeader fileHeader;
        fileHeader.type = 0x42 << 1 | 0x4d;
        InfoHeader infoHeader;
        infoHeader.size = sizeof(InfoHeader);
        return nullptr;
    }

private:
    template <typename T>
    static void read_data(const std::byte *source, Matrix<T> &dst, const FileHeader *fileHeader, const InfoHeader *infoHeader) {
        const bool seqRead = infoHeader->height < 0;  // 當 height 為正時 順序讀取
        const auto data = source + fileHeader->offset;
        const auto pixelSize = infoHeader->bits / 8;  // byte
        const auto rowSize = infoHeader->width * pixelSize;
        const auto rawRowSize = 4 * (rowSize / 4 + (rowSize % 4 != 0));  // 填充過後
        for (int i = 0; i < infoHeader->height; i++) {
            const std::byte *row = nullptr;
            if (seqRead) {
                row = data + i * rawRowSize;
            } else {
                row = data + (std::abs(infoHeader->height) - 1 - i) * rawRowSize;
            }
            for (int j = 0; j < infoHeader->width; j++) {
                dst[i, j] = *reinterpret_cast<const T *>(&row[j * pixelSize]);
            }
        }
    }
};
};  // namespace f9ay
