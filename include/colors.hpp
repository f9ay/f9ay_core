#pragma once
#include <cstddef>
#include <iostream>
#include <tuple>

namespace f9ay::colors {

struct BGR {
    uint8_t b, g, r;
};

struct RGB {
    uint8_t r, g, b;
};

struct BGRA {
    uint8_t b, g, r, a;
};

struct RGBA {
    uint8_t r, g, b, a;
};

struct YCbCr {
    uint8_t y, cb, cr;
};

template <typename T>
concept color_type =
    std::same_as<T, colors::RGB> || std::same_as<T, colors::BGR> ||
    std::same_as<T, colors::RGBA> || std::same_as<T, colors::BGRA> ||
    std::same_as<T, colors::YCbCr>;

template <color_type ColorType>
int distance(const ColorType& l, const ColorType& r) {
    if constexpr (sizeof(ColorType) == 4) {
        auto [a, b, c, d] = l;
        auto [e, f, g, h] = r;
        // need to cast to int to avoid overflow
        return std::abs(static_cast<int>(a) - static_cast<int>(e)) +
               std::abs(static_cast<int>(b) - static_cast<int>(f)) +
               std::abs(static_cast<int>(c) - static_cast<int>(g)) +
               std::abs(static_cast<int>(d) - static_cast<int>(h));
    } else {
        auto [a, b, c] = l;
        auto [e, f, g] = r;
        // need to cast to int to avoid overflow
        return std::abs(static_cast<int>(a) - static_cast<int>(e)) +
               std::abs(static_cast<int>(b) - static_cast<int>(f)) +
               std::abs(static_cast<int>(c) - static_cast<int>(g));
    }
}

template <color_type ColorType>
ColorType operator+(const ColorType& l, const ColorType& r) {
    if constexpr (sizeof(ColorType) == 4) {
        auto [a, b, c, d] = l;
        auto [e, f, g, h] = r;
        return {static_cast<uint8_t>(a + e), static_cast<uint8_t>(b + f),
                static_cast<uint8_t>(c + g), static_cast<uint8_t>(d + h)};
    } else {
        auto [a, b, c] = l;
        auto [e, f, g] = r;
        return {static_cast<uint8_t>(a + e), static_cast<uint8_t>(b + f),
                static_cast<uint8_t>(c + g)};
    }
}

template <color_type ColorType>
ColorType operator-(const ColorType& l, const ColorType& r) {
    if constexpr (sizeof(ColorType) == 4) {
        auto [a, b, c, d] = l;
        auto [e, f, g, h] = r;
        return {static_cast<uint8_t>(a - e), static_cast<uint8_t>(b - f),
                static_cast<uint8_t>(c - g), static_cast<uint8_t>(d - h)};
    } else {
        auto [a, b, c] = l;
        auto [e, f, g] = r;
        return {static_cast<uint8_t>(a - e), static_cast<uint8_t>(b - f),
                static_cast<uint8_t>(c - g)};
    }
}

template <color_type ColorType>
ColorType operator*(const ColorType& l, const ColorType& r) {
    if constexpr (sizeof(ColorType) == 4) {
        auto [a, b, c, d] = l;
        auto [e, f, g, h] = r;
        return {a * e, b * f, c * g, h * d};
    } else {
        auto [a, b, c] = l;
        auto [e, f, g] = r;
        return {a * e, b * f, c * g};
    }
}

template <color_type ColorType>
ColorType operator/(const ColorType& l, const ColorType& r) {
    if constexpr (sizeof(ColorType) == 4) {
        auto [a, b, c, d] = l;
        auto [e, f, g, h] = r;
        return {a / e, b / f, c / g, h / d};
    } else {
        auto [a, b, c] = l;
        auto [e, f, g] = r;
        return {a / e, b / f, c / g};
    }
}

template <color_type ColorType>
ColorType operator/(const ColorType& l, int r) {
    if constexpr (sizeof(ColorType) == 4) {
        auto [a, b, c, d] = l;
        return {static_cast<uint8_t>(a / r), static_cast<uint8_t>(b / r),
                static_cast<uint8_t>(c / r), static_cast<uint8_t>(d / r)};
    } else {
        auto [a, b, c] = l;
        return {static_cast<uint8_t>(a / r), static_cast<uint8_t>(b / r),
                static_cast<uint8_t>(c / r)};
    }
}
}  // namespace f9ay::colors

template <f9ay::colors::color_type ColorType, typename Char_T>
struct std::formatter<ColorType, Char_T> : std::formatter<std::string, Char_T> {
    auto format(const ColorType& color, auto& ctx) const {
        if constexpr (sizeof(ColorType) == 4) {
            auto [a, b, c, d] = color;
            return std::format_to(ctx.out(), "({}, {}, {}, {})", a, b, c, d);

        } else {
            auto [a, b, c] = color;
            return std::format_to(ctx.out(), "({}, {}, {})", a, b, c);
        }
    }
};

template <f9ay::colors::color_type ColorType>
struct std::hash<ColorType> {
    std::size_t operator()(const ColorType& color) const {
        if constexpr (sizeof(ColorType) == 4) {
            return std::hash<uint32_t>()(std::bit_cast<uint32_t>(color));
        } else {
            auto [a, b, c] = color;
            uint32_t val = a << 16 | b << 8 | c;
            return std::hash<uint32_t>()(val);
        }
    }
};
