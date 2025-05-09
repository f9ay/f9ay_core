#pragma once
#include <cstddef>
#include <tuple>

namespace f9ay::colors {

struct BGR {
    std::byte b, g, r;
};

struct RGB {
    std::byte r, g, b;
};

struct BGRA {
    std::byte b, g, r, a;
};

struct RGBA {
    std::byte r, g, b, a;
};

}  // namespace f9ay::colors
