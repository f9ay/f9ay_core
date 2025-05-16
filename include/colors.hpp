#pragma once
#include <cstddef>
#include <iostream>
#include <tuple>

namespace f9ay::colors {

struct BGR {
    std::byte b, g, r;

    BGR operator+(const BGR& other) const {
        return BGR{
            std::byte(static_cast<char>(b) + static_cast<char>(other.b)),
            std::byte(static_cast<char>(g) + static_cast<char>(other.g)),
            std::byte(static_cast<char>(r) + static_cast<char>(other.r))};
    }
    BGR operator+(int val) const {
        return {std::byte(static_cast<char>(b) + val),
                std::byte(static_cast<char>(g) + val),
                std::byte(static_cast<char>(r) + val)};
    }
    BGR operator-(const BGR& other) const {
        return BGR{
            std::byte(static_cast<char>(b) - static_cast<char>(other.b)),
            std::byte(static_cast<char>(g) - static_cast<char>(other.g)),
            std::byte(static_cast<char>(r) - static_cast<char>(other.r))};
    }
    BGR operator-(int val) const {
        return BGR{std::byte(static_cast<char>(b) - val),
                   std::byte(static_cast<char>(g) - val),
                   std::byte(static_cast<char>(r) - val)};
    }
    BGR operator/(int val) const {
        return BGR{std::byte(static_cast<char>(b) / val),
                   std::byte(static_cast<char>(g) / val),
                   std::byte(static_cast<char>(r) / val)};
    }
    BGR operator*(int val) const {
        return BGR{std::byte(static_cast<char>(b) * val),
                   std::byte(static_cast<char>(g) * val),
                   std::byte(static_cast<char>(r) * val)};
    }
    BGR& operator+=(const BGR& other) {
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        return *this;
    }
    BGR& operator+=(int val) {
        b = std::byte(static_cast<char>(b) + val);
        g = std::byte(static_cast<char>(g) + val);
        r = std::byte(static_cast<char>(r) + val);
        return *this;
    }
    BGR& operator-=(const BGR& other) {
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        return *this;
    }
    BGR& operator-=(int val) {
        b = std::byte(static_cast<char>(b) - val);
        g = std::byte(static_cast<char>(g) - val);
        r = std::byte(static_cast<char>(r) - val);
        return *this;
    }
    BGR& operator/=(int val) {
        b = std::byte(static_cast<char>(b) / val);
        g = std::byte(static_cast<char>(g) / val);
        r = std::byte(static_cast<char>(r) / val);
        return *this;
    }
    BGR& operator*=(int val) {
        b = std::byte(static_cast<char>(b) * val);
        g = std::byte(static_cast<char>(g) * val);
        r = std::byte(static_cast<char>(r) * val);
        return *this;
    }
    bool operator==(const BGR& other) const {
        return b == other.b && g == other.g && r == other.r;
    }
    bool operator!=(const BGR& other) const {
        return !(*this == other);
    }

    bool operator<(const BGR& other) const {
        return std::tie(b, g, r) < std::tie(other.b, other.g, other.r);
    }
};

struct RGB {
    std::byte r, g, b;
    RGB operator+(const RGB& other) const {
        return RGB{
            std::byte(static_cast<char>(r) + static_cast<char>(other.r)),
            std::byte(static_cast<char>(g) + static_cast<char>(other.g)),
            std::byte(static_cast<char>(b) + static_cast<char>(other.b))};
    }
    RGB operator-(const RGB& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        return *this;
    }
    RGB operator-(int val) const {
        return RGB{std::byte(static_cast<char>(r) - val),
                   std::byte(static_cast<char>(g) - val),
                   std::byte(static_cast<char>(b) - val)};
    }
    RGB operator/(int val) const {
        return RGB{std::byte(static_cast<char>(r) / val),
                   std::byte(static_cast<char>(g) / val),
                   std::byte(static_cast<char>(b) / val)};
    }
    RGB operator*(int val) const {
        return RGB{std::byte(static_cast<char>(r) * val),
                   std::byte(static_cast<char>(g) * val),
                   std::byte(static_cast<char>(b) * val)};
    }
    RGB& operator+=(const RGB& other) {
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        return *this;
    }
    RGB& operator+=(int val) {
        r = std::byte(static_cast<char>(r) + val);
        g = std::byte(static_cast<char>(g) + val);
        b = std::byte(static_cast<char>(b) + val);
        return *this;
    }
    RGB& operator-=(const RGB& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        return *this;
    }
    RGB& operator-=(int val) {
        r = std::byte(static_cast<char>(r) - val);
        g = std::byte(static_cast<char>(g) - val);
        b = std::byte(static_cast<char>(b) - val);
        return *this;
    }
    RGB& operator/=(int val) {
        r = std::byte(static_cast<char>(r) / val);
        g = std::byte(static_cast<char>(g) / val);
        b = std::byte(static_cast<char>(b) / val);
        return *this;
    }
    RGB& operator*=(int val) {
        r = std::byte(static_cast<char>(r) * val);
        g = std::byte(static_cast<char>(g) * val);
        b = std::byte(static_cast<char>(b) * val);
        return *this;
    }
    bool operator==(const RGB& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const RGB& other) const {
        return !(*this == other);
    }
    bool operator<(const RGB& other) const {
        return std::tie(r, g, b) < std::tie(other.r, other.g, other.b);
    }
};

struct BGRA {
    std::byte b, g, r, a;
    BGRA operator+(const BGRA& other) const {
        return BGRA{
            std::byte(static_cast<char>(b) + static_cast<char>(other.b)),
            std::byte(static_cast<char>(g) + static_cast<char>(other.g)),
            std::byte(static_cast<char>(r) + static_cast<char>(other.r)),
            std::byte(static_cast<char>(a) + static_cast<char>(other.a))};
    }
    BGRA operator+(int val) const {
        return BGRA{std::byte(static_cast<char>(b) + val),
                    std::byte(static_cast<char>(g) + val),
                    std::byte(static_cast<char>(r) + val),
                    std::byte(static_cast<char>(a) + val)};
    }
    BGRA operator-(const BGRA& other) const {
        return BGRA{
            std::byte(static_cast<char>(b) - static_cast<char>(other.b)),
            std::byte(static_cast<char>(g) - static_cast<char>(other.g)),
            std::byte(static_cast<char>(r) - static_cast<char>(other.r)),
            std::byte(static_cast<char>(a) - static_cast<char>(other.a))};
    }
    BGRA operator-(int val) const {
        return BGRA{std::byte(static_cast<char>(b) - val),
                    std::byte(static_cast<char>(g) - val),
                    std::byte(static_cast<char>(r) - val),
                    std::byte(static_cast<char>(a) - val)};
    }
    BGRA operator/(int val) const {
        return BGRA{std::byte(static_cast<char>(b) / val),
                    std::byte(static_cast<char>(g) / val),
                    std::byte(static_cast<char>(r) / val),
                    std::byte(static_cast<char>(a) / val)};
    }
    BGRA operator*(int val) const {
        return BGRA{std::byte(static_cast<char>(b) * val),
                    std::byte(static_cast<char>(g) * val),
                    std::byte(static_cast<char>(r) * val),
                    std::byte(static_cast<char>(a) * val)};
    }
    BGRA& operator+=(const BGRA& other) {
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        a = std::byte(static_cast<char>(a) + static_cast<char>(other.a));
        return *this;
    }
    BGRA& operator+=(int val) {
        b = std::byte(static_cast<char>(b) + val);
        g = std::byte(static_cast<char>(g) + val);
        r = std::byte(static_cast<char>(r) + val);
        a = std::byte(static_cast<char>(a) + val);
        return *this;
    }
    BGRA& operator-=(const BGRA& other) {
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        a = std::byte(static_cast<char>(a) - static_cast<char>(other.a));
        return *this;
    }
    BGRA& operator-=(int val) {
        b = std::byte(static_cast<char>(b) - val);
        g = std::byte(static_cast<char>(g) - val);
        r = std::byte(static_cast<char>(r) - val);
        a = std::byte(static_cast<char>(a) - val);
        return *this;
    }
    BGRA& operator/=(int val) {
        b = std::byte(static_cast<char>(b) / val);
        g = std::byte(static_cast<char>(g) / val);
        r = std::byte(static_cast<char>(r) / val);
        a = std::byte(static_cast<char>(a) / val);
        return *this;
    }
    BGRA& operator*=(int val) {
        b = std::byte(static_cast<char>(b) * val);
        g = std::byte(static_cast<char>(g) * val);
        r = std::byte(static_cast<char>(r) * val);
        a = std::byte(static_cast<char>(a) * val);
        return *this;
    }
    bool operator==(const BGRA& other) const {
        return b == other.b && g == other.g && r == other.r && a == other.a;
    }
    bool operator!=(const BGRA& other) const {
        return !(*this == other);
    }
    bool operator<(const BGRA& other) const {
        return std::tie(b, g, r, a) < std::tie(other.b, other.g, other.r, other.a);
    }
};

struct RGBA {
    std::byte r, g, b, a;
    RGBA operator+(const RGBA& other) const {
        return RGBA{
            std::byte(static_cast<char>(r) + static_cast<char>(other.r)),
            std::byte(static_cast<char>(g) + static_cast<char>(other.g)),
            std::byte(static_cast<char>(b) + static_cast<char>(other.b)),
            std::byte(static_cast<char>(a) + static_cast<char>(other.a))};
    }
    RGBA operator+(int val) const {
        return RGBA{std::byte(static_cast<char>(r) + val),
                    std::byte(static_cast<char>(g) + val),
                    std::byte(static_cast<char>(b) + val),
                    std::byte(static_cast<char>(a) + val)};
    }
    RGBA operator-(const RGBA& other) const {
        return RGBA{
            std::byte(static_cast<char>(r) - static_cast<char>(other.r)),
            std::byte(static_cast<char>(g) - static_cast<char>(other.g)),
            std::byte(static_cast<char>(b) - static_cast<char>(other.b)),
            std::byte(static_cast<char>(a) - static_cast<char>(other.a))};
    }
    RGBA operator-(int val) {
        return RGBA{std::byte(static_cast<char>(r) - val),
                    std::byte(static_cast<char>(g) - val),
                    std::byte(static_cast<char>(b) - val),
                    std::byte(static_cast<char>(a) - val)};
    }
    RGBA operator/(int val) const {
        return RGBA{std::byte(static_cast<char>(r) / val),
                    std::byte(static_cast<char>(g) / val),
                    std::byte(static_cast<char>(b) / val),
                    std::byte(static_cast<char>(a) / val)};
    }
    RGBA operator*(int val) const {
        return RGBA{std::byte(static_cast<char>(r) * val),
                    std::byte(static_cast<char>(g) * val),
                    std::byte(static_cast<char>(b) * val),
                    std::byte(static_cast<char>(a) * val)};
    }
    RGBA& operator+=(const RGBA& other) {
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        a = std::byte(static_cast<char>(a) + static_cast<char>(other.a));
        return *this;
    }
    RGBA& operator+=(int val) {
        r = std::byte(static_cast<char>(r) + val);
        g = std::byte(static_cast<char>(g) + val);
        b = std::byte(static_cast<char>(b) + val);
        a = std::byte(static_cast<char>(a) + val);
        return *this;
    }
    RGBA& operator-=(const RGBA& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        a = std::byte(static_cast<char>(a) - static_cast<char>(other.a));
        return *this;
    }
    RGBA& operator-=(int val) {
        r = std::byte(static_cast<char>(r) - val);
        g = std::byte(static_cast<char>(g) - val);
        b = std::byte(static_cast<char>(b) - val);
        a = std::byte(static_cast<char>(a) - val);
        return *this;
    }
    RGBA& operator/=(int val) {
        r = std::byte(static_cast<char>(r) / val);
        g = std::byte(static_cast<char>(g) / val);
        b = std::byte(static_cast<char>(b) / val);
        a = std::byte(static_cast<char>(a) / val);
        return *this;
    }
    RGBA& operator*=(int val) {
        r = std::byte(static_cast<char>(r) * val);
        g = std::byte(static_cast<char>(g) * val);
        b = std::byte(static_cast<char>(b) * val);
        a = std::byte(static_cast<char>(a) * val);
        return *this;
    }
    bool operator==(const RGBA& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const RGBA& other) const {
        return !(*this == other);
    }
    bool operator<(const RGBA& other) const {
        return std::tie(r, g, b, a) < std::tie(other.r, other.g, other.b, other.a);
    }
};
inline int abs(const BGR& color) {
    return std::abs(std::to_integer<int>(color.b)) +
           std::abs(std::to_integer<int>(color.g)) +
           std::abs(std::to_integer<int>(color.r));
}
inline int abs(const RGB& color) {
    return std::abs(std::to_integer<int>(color.r)) +
           std::abs(std::to_integer<int>(color.g)) +
           std::abs(std::to_integer<int>(color.b));
}
inline int abs(const BGRA& color) {
    return std::abs(std::to_integer<int>(color.b)) +
           std::abs(std::to_integer<int>(color.g)) +
           std::abs(std::to_integer<int>(color.r)) +
           std::abs(std::to_integer<int>(color.a));
}
inline int abs(const RGBA& color) {
    return std::abs(std::to_integer<int>(color.r)) +
           std::abs(std::to_integer<int>(color.g)) +
           std::abs(std::to_integer<int>(color.b)) +
           std::abs(std::to_integer<int>(color.a));
}
}  // namespace f9ay::colors

template <typename Char_T>
struct std::formatter<f9ay::colors::BGR, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::BGR& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "({}, {}, {})", std::to_integer<int>(color.b),
            std::to_integer<int>(color.g), std::to_integer<int>(color.r));
    }
};
template <typename Char_T>
struct std::formatter<f9ay::colors::RGB, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::RGB& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "({}, {}, {})", std::to_integer<int>(color.r),
            std::to_integer<int>(color.g), std::to_integer<int>(color.b));
    }
};
template <typename Char_T>
struct std::formatter<f9ay::colors::BGRA, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::BGRA& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "({}, {}, {}, {})", std::to_integer<int>(color.b),
            std::to_integer<int>(color.g), std::to_integer<int>(color.r),
            std::to_integer<int>(color.a));
    }
};
template <typename Char_T>
struct std::formatter<f9ay::colors::RGBA, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::RGBA& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "({}, {}, {}, {})", std::to_integer<int>(color.r),
            std::to_integer<int>(color.g), std::to_integer<int>(color.b),
            std::to_integer<int>(color.a));
    }
};

namespace std {
template <>
struct hash<f9ay::colors::BGR> {
    std::size_t operator()(const f9ay::colors::BGR& color) const {
        return std::hash<std::byte>()(color.b) ^
               std::hash<std::byte>()(color.g) ^
               std::hash<std::byte>()(color.r);
    }
};

template <>
struct hash<f9ay::colors::RGB> {
    std::size_t operator()(const f9ay::colors::RGB& color) const {
        return std::hash<std::byte>()(color.r) ^
               std::hash<std::byte>()(color.g) ^
               std::hash<std::byte>()(color.b);
    }
};

template <>
struct hash<f9ay::colors::BGRA> {
    std::size_t operator()(const f9ay::colors::BGRA& color) const {
        return std::hash<std::byte>()(color.b) ^
               std::hash<std::byte>()(color.g) ^
               std::hash<std::byte>()(color.r) ^
               std::hash<std::byte>()(color.a);
    }
};

template <>
struct hash<f9ay::colors::RGBA> {
    std::size_t operator()(const f9ay::colors::RGBA& color) const {
        return std::hash<std::byte>()(color.r) ^
               std::hash<std::byte>()(color.g) ^
               std::hash<std::byte>()(color.b) ^
               std::hash<std::byte>()(color.a);
    }
};
}  // namespace std