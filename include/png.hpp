#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "filter.hpp"
#include "matrix_concept.hpp"

namespace f9ay {
class PNG {
#pragma pack(push, 1)
    struct PNGSignature {
        std::byte signature[8];

        bool isPNG() const {
            const static std::byte pngSignature[8] = {
                std::byte{0x89}, std::byte{0x50}, std::byte{0x4e},
                std::byte{0x47}, std::byte{0x0d}, std::byte{0x0a},
                std::byte{0x1a}, std::byte{0x0a}};
            return std::memcmp(signature, pngSignature, sizeof(signature)) == 0;
        }
    };

    struct IDHRChunk {
        char chunkType[4] = {'I', 'H', 'D', 'R'};
        u_int32_t width;
        u_int32_t height;
        u_int8_t bitDepth;
        u_int8_t colorType;
        u_int8_t compressionMethod;
        u_int8_t filterMethod;
        u_int8_t interlaceMethod;
    };
    struct ChunkHeader {
        uint32_t length;
        char type[4];
    };
#pragma pack(pop)
    static_assert(sizeof(PNGSignature) == 8);
    static_assert(sizeof(IDHRChunk) == 17);
    static_assert(sizeof(ChunkHeader) == 8);

public:
    using ExportType = std::pair<std::byte*, size_t>;
    template <MATRIX_CONCEPT Matrix_Type>
    ExportType exportToByte(Matrix_Type& matrix, FilterType filterType) {
        // write signature to buffer
        auto pngSignature =
            PNGSignature{{std::byte{0x89}, std::byte{0x50}, std::byte{0x4e},
                          std::byte{0x47}, std::byte{0x0d}, std::byte{0x0a},
                          std::byte{0x1a}, std::byte{0x0a}}};

        _data.resize(sizeof(PNGSignature));
        std::memcpy(_data.data(), reinterpret_cast<std::byte*>(&pngSignature),
                    sizeof(PNGSignature));

        // write IHDR chunk
        auto ihdrChunk = IDHRChunk{};
        ihdrChunk.width = matrix.col();
        ihdrChunk.height = matrix.row();
        ihdrChunk.bitDepth = 8;
        using ElementType = std::decay_t<decltype(matrix[0][0])>;
        if constexpr (std::is_same_v<ElementType, colors::BGR> ||
                      std::is_same_v<ElementType, colors::RGB>) {
            ihdrChunk.colorType = 2;  // RGB
        } else if constexpr (std::is_same_v<ElementType, colors::BGRA> ||
                             std::is_same_v<ElementType, colors::RGBA>) {
            ihdrChunk.colorType = 6;  // RGBA
        } else {
            static_assert("Unsupported color type");
        }
        ihdrChunk.compressionMethod = 0;
        ihdrChunk.filterMethod = static_cast<u_int8_t>(filterType);
        ihdrChunk.interlaceMethod = 0;
        _data.resize(_data.size() + sizeof(IDHRChunk));
        std::memcpy(_data.data() + sizeof(PNGSignature),
                    reinterpret_cast<std::byte*>(&ihdrChunk),
                    sizeof(IDHRChunk));
        // write data

        return std::make_pair(reinterpret_cast<std::byte*>(_data.data()),
                              _data.size());
    }

private:
    PNGSignature* _pngSignature = nullptr;
    IDHRChunk* _ihdrChunk = nullptr;

    std::vector<std::byte> _data;

    u_int32_t _calculateCRC(const std::byte* data, size_t length) {}
};
}  // namespace f9ay