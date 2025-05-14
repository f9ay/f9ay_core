#pragma once

namespace f9ay {

template <int N>
concept PowerOf2 = (N & (N - 1)) == 0;

template <int N>
    requires PowerOf2<N>
constexpr auto align(auto x) {
    return (x + (N - 1)) & ~(N - 1);
}
}  // namespace f9ay
