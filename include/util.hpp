#pragma once
#include <algorithm>
#include <bit>
#include <bitset>
#include <cstddef>
#include <vector>

namespace f9ay {

inline std::string to_str(uint32_t val, uint32_t len) {
    std::string s = std::bitset<32>(val).to_string();
    std::ranges::reverse(s);
    s.resize(len);
    std::ranges::reverse(s);
    return s;
}

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

inline std::byte revertByteBits(std::byte byte) {
    std::byte result{0};

    for (int i = 0; i < 8; i++) {
        result |= (byte >> (7 - i) & (std::byte{1})) << i;
    }
    return result;
}

enum class WriteSequence {
    MSB,
    LSB,
};

class BitWriter {
public:
    BitWriter() {
        _buffer.push_back(std::byte{0});
    }

    BitWriter(const std::vector<std::byte>& buffer) : _buffer(buffer), _bitPos(0) {
        _buffer.push_back(std::byte{0});
    }

    BitWriter(std::byte* buffer, size_t size) : _buffer(buffer, buffer + size), _bitPos(0) {
        _buffer.push_back(std::byte{0});
    }

    void writeBit(bool bit) {
        if (_bitPos == 8) {
            _buffer.push_back(std::byte{0});
            _bitPos = 0;
        }

        if (bit) {
            switch (_writeSequence) {
                case WriteSequence::MSB:
                    _buffer.back() |= (std::byte{1} << (7 - _bitPos));
                    break;
                case WriteSequence::LSB:
                    _buffer.back() |= (std::byte{1} << _bitPos);
                    break;
            }
        }

        _bitPos++;
    }

    template <typename T>
    void writeBitsFromMSB(T data, int count) {
        // std::println("MSB {}", to_str(data, count));
        for (int i = count - 1; i >= 0; i--) {
            writeBit(static_cast<bool>((data >> i) & T{1}));
        }
    }

    void changeWriteSequence(WriteSequence sequence, bool needRevert = true) {
        if (sequence == _writeSequence) {
            return;
        } else {
            if (_bitPos % 8 != 0 && needRevert) {
                // revert the last byte
                _buffer.back() = revertByteBits(_buffer.back());
            } else if (_bitPos % 8 != 0 && !needRevert) {
            }
        }
        _writeSequence = sequence;
    }

    template <typename T>
    void writeBitsFromLSB(T data, int count) {
        for (int i = 0; i < count; i++) {
            writeBit(static_cast<bool>((data >> i) & T{1}));
        }
    }

    [[nodiscard]] size_t getBitPos() const {
        return _bitPos;
    }

    [[nodiscard]] std::vector<std::byte> getBuffer() const {
        return _buffer;
    }

private:
    std::vector<std::byte> _buffer;
    size_t _bitPos = 0;

    // decide write from MSB or LSB
    WriteSequence _writeSequence = WriteSequence::MSB;
};

}  // namespace f9ay
