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
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
    struct FileHeader __attribute__((packed)) {
        unsigned short type; /* type : Magic identifier,一般為BM(0x42,0x4d) */
        unsigned int size;   /* File size in bytes,全部的檔案大小 */
        unsigned short reserved1, reserved2; /* 保留欄位 */
        unsigned int offset;                 /* Offset to image data, bytes */
    };
    struct InfoHeader __attribute__((packed)) {
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
    struct ColorPalette __attribute__((packed)) {
        std::byte blue;
        std::byte green;
        std::byte red;
        std::byte alpha;
    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif
private:
    const FileHeader *fileHeader = nullptr;
    const InfoHeader *infoHeader = nullptr;
    bool seqRead = false;

public:
    using result_type = std::variant<Matrix<colors::BGR>, Matrix<colors::BGRA>>;
    result_type import(const std::byte *source) {
        fileHeader = safeMemberAssign<FileHeader>(source);
        infoHeader = safeMemberAssign<InfoHeader>(source + sizeof(FileHeader));

        seqRead = infoHeader->height < 0;  // 當 height 為正時 順序讀取

        if (infoHeader->bits <= 8 && infoHeader->compression == false) {
            // read ColorPalette 先不管
        }
        if (infoHeader->bits == 32) {
            // RGBA
            Matrix<colors::BGRA> result(infoHeader->height, infoHeader->width);
            read_data(source, result);
            return result;
        } else if (infoHeader->bits == 24) {
            // RGB
            Matrix<colors::BGR> result(infoHeader->height, infoHeader->width);
            read_data(source, result);
            return result;
        }
        throw std::runtime_error("Unsupported BMP format");
    }

private:
    template <typename T>
    void read_data(const std::byte *source, Matrix<T> &dst) {
        const auto data = source + fileHeader->offset;
        const auto pixelSize = infoHeader->bits / 8;  // byte
        const auto rowSize = infoHeader->width * pixelSize;
        const auto rawRowSize = rowSize + (4 - rowSize % 4);  // 填充過後
        for (int i = 0; i < infoHeader->height; i++) {
            const std::byte *row = nullptr;
            if (seqRead) {
                row = data + i * rawRowSize;
            } else {
                row = data + (infoHeader->height - 1 - i) * rawRowSize;
            }
            for (int j = 0; j < infoHeader->width; j++) {
                if constexpr (std::is_same_v<T, colors::BGRA>) {
                    dst[i, j] = {row[j * pixelSize], row[j * pixelSize + 1],
                                 row[j * pixelSize + 2],
                                 row[j * pixelSize + 3]};
                } else if constexpr (std::is_same_v<T, colors::BGR>) {
                    dst[i, j] = {row[j * pixelSize], row[j * pixelSize + 1],
                                 row[j * pixelSize + 2]};
                }
            }
        }
    }
};
};  // namespace f9ay
