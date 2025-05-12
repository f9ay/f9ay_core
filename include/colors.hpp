#pragma once
#include <cstddef>
#include <iostream>
#include <tuple>

namespace f9ay::colors {

struct BGR {
    std::byte b, g, r;

    BGR operator+(const BGR& other) {
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        return *this;
    }
    BGR operator-(const BGR& other) {
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        return *this;
    }
    BGR& operator+=(const BGR& other) {
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        return *this;
    }
    BGR& operator-=(const BGR& other) {
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        return *this;
    }
};

struct RGB {
    std::byte r, g, b;
    RGB operator+(const RGB& other) {
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        return *this;
    }
    RGB operator-(const RGB& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        return *this;
    }
    RGB& operator+=(const RGB& other) {
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        return *this;
    }
    RGB& operator-=(const RGB& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        return *this;
    }
};

struct BGRA {
    std::byte b, g, r, a;
    BGRA operator+(const BGRA& other) {
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        a = std::byte(static_cast<char>(a) + static_cast<char>(other.a));
        return *this;
    }
    BGRA operator-(const BGRA& other) {
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        a = std::byte(static_cast<char>(a) - static_cast<char>(other.a));
        return *this;
    }
    BGRA& operator+=(const BGRA& other) {
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        a = std::byte(static_cast<char>(a) + static_cast<char>(other.a));
        return *this;
    }
    BGRA& operator-=(const BGRA& other) {
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        a = std::byte(static_cast<char>(a) - static_cast<char>(other.a));
        return *this;
    }
};

struct RGBA {
    std::byte r, g, b, a;
    RGBA operator+(const RGBA& other) {
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        a = std::byte(static_cast<char>(a) + static_cast<char>(other.a));
        return *this;
    }
    RGBA operator-(const RGBA& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        a = std::byte(static_cast<char>(a) - static_cast<char>(other.a));
        return *this;
    }
    RGBA& operator+=(const RGBA& other) {
        r = std::byte(static_cast<char>(r) + static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) + static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) + static_cast<char>(other.b));
        a = std::byte(static_cast<char>(a) + static_cast<char>(other.a));
        return *this;
    }
    RGBA& operator-=(const RGBA& other) {
        r = std::byte(static_cast<char>(r) - static_cast<char>(other.r));
        g = std::byte(static_cast<char>(g) - static_cast<char>(other.g));
        b = std::byte(static_cast<char>(b) - static_cast<char>(other.b));
        a = std::byte(static_cast<char>(a) - static_cast<char>(other.a));
        return *this;
    }
};

}  // namespace f9ay::colors

template <typename Char_T>
struct std::formatter<f9ay::colors::BGR, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::BGR& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "BGR({}, {}, {})", std::to_integer<int>(color.b),
            std::to_integer<int>(color.g), std::to_integer<int>(color.r));
    }
};
template <typename Char_T>
struct std::formatter<f9ay::colors::RGB, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::RGB& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "RGB({}, {}, {})", std::to_integer<int>(color.r),
            std::to_integer<int>(color.g), std::to_integer<int>(color.b));
    }
};
template <typename Char_T>
struct std::formatter<f9ay::colors::BGRA, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::BGRA& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "BGRA({}, {}, {}, {})", std::to_integer<int>(color.b),
            std::to_integer<int>(color.g), std::to_integer<int>(color.r),
            std::to_integer<int>(color.a));
    }
};
template <typename Char_T>
struct std::formatter<f9ay::colors::RGBA, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::colors::RGBA& color, auto& ctx) const {
        return std::format_to(
            ctx.out(), "RGBA({}, {}, {}, {})", std::to_integer<int>(color.r),
            std::to_integer<int>(color.g), std::to_integer<int>(color.b),
            std::to_integer<int>(color.a));
    }
};