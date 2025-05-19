#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "importer.hpp"

namespace f9ay {
class Jpeg {
private:
#pragma pack(push, 1)
    struct JFIF_APP0 {
        uint16_t marker;
        uint16_t length;  // Length of segment excluding APP0 marker
        char Identifier[5];
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
    static Midway import(const std::byte *source) {
        const auto *jfif_app0 = safeMemberAssign<JFIF_APP0>(source);
        std::pair<const std::byte *, size_t> thumbnail_data = {
            source + sizeof(JFIF_APP0),
            3 * jfif_app0->Xthumbnail * jfif_app0->Ythumbnail};
        const std::byte *it = source + sizeof(JFIF_APP0) +
                              3 * jfif_app0->Xthumbnail * jfif_app0->Ythumbnail;
        if (jfif_app0->version >= 0x0102) {
            // jfif extension app0 available
            throw std::logic_error("JFIF version does not match");
        }

        if (*reinterpret_cast<const uint16_t *>(it) != 0xFFDA) {
            throw std::logic_error("JFIF SOS NOT match");
        }
        it += 2;
        // 	compressed image data

        return {};
    }

    static std::pair<std::unique_ptr<std::byte[]>, size_t> write(
        const Matrix<colors::RGB> &src) {
        Matrix<colors::YCbCr> mtx = toYCbCr(src);
        return {nullptr, 0};
    }

private:
    static Matrix<colors::YCbCr> toYCbCr(const Matrix<colors::RGB> &src) {
        Matrix<colors::YCbCr> result(src.row(), src.col());
#pragma loop(hint_parallel(0))
        for (int i = 0; i < src.row(); i++) {
            for (int j = 0; j < src.col(); j++) {
                result[i, j] = {0.299f * src[i, j].r + 0.587f * src[i, j].g +
                                    0.114f * src[i, j].b,
                                -0.168736f * src[i, j].r -
                                    0.331364f * src[i, j].g +
                                    0.5f * src[i, j].b + 128,
                                0.5f * src[i, j].r - 0.418688f * src[i, j].g -
                                    0.081312f * src[i, j].b + 128};
            }
        }
        return result;
    }
}
};
}  // namespace f9ay
