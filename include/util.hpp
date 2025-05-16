#pragma once
#include <bit>
#include <cstddef>
#include <vector>

namespace f9ay {

template <int N>
concept PowerOf2 = (N & (N - 1)) == 0;

template <int N>
    requires PowerOf2<N>
constexpr auto align(auto x) {
    return (x + (N - 1)) & ~(N - 1);
}
template <typename T>
constexpr T checkAndSwapToBigEndian(T value) {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
    if (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
    return value;
}

class BitWriter {
public:
    BitWriter() {
        _buffer.push_back(std::byte{0});
    }

    void writeBit(bool bit) {
        if (_bitPos == 8) {
            _buffer.push_back(std::byte{0});
            _bitPos = 0;
        }

        if (bit) {
            _buffer.back() |= std::byte{0b00000001} << (7 - _bitPos);
        }

        _bitPos++;
    }

    void writeBits(std::byte bits, size_t count) {
        for (size_t i = 0; i < count; i++) {
            writeBit(static_cast<bool>(bits >> i & std::byte{0b00000001}));
        }
    }

    size_t getBitPos() const {
        return _bitPos;
    }

private:
    std::vector<std::byte> _buffer;
    size_t _bitPos = 0;
};
}  // namespace f9ay
