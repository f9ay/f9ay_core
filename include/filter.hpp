#pragma once
#include <cmath>

#include "matrix.hpp"
#include "matrix_concept.hpp"

enum class FilterType { None, Sub, Up, Average, Paeth };

namespace f9ay::deflate {
template <typename T>
T paethPredictor(T left, T above, T upperLeft) {
    // in theory the T will only one byte

    int p = static_cast<int>(left) + static_cast<int>(above) -
            static_cast<int>(upperLeft);

    int pa = std::abs(static_cast<int>(left) - p);
    int pb = std::abs(static_cast<int>(above) - p);
    int pc = std::abs(static_cast<int>(upperLeft) - p);
    if (pa <= pb && pa <= pc) {
        return left;
    } else if (pb <= pc) {
        return above;
    } else {
        return upperLeft;
    }
}

template <MATRIX_CONCEPT InputMatrix>
inline Matrix<std::decay_t<decltype(std::declval<InputMatrix>()[0, 0])>> filter(
    const InputMatrix& matrix, FilterType filterType) {
    using T_val = std::decay_t<decltype(matrix[0, 0])>;
    f9ay::Matrix<T_val> filteredMatrix(matrix.row(), matrix.col());

    for (int i = 0; i < matrix.row(); ++i) {
        // Add filter type to the first byte of the scanline
        for (int j = 0; j < matrix.col(); ++j) {
            T_val currentByte = matrix[i, j];

            // Raw(x-bpp): byte at same component in previous pixel
            T_val left = (j == 0) ? static_cast<T_val>(0)
                                  : matrix[i, j - 1];  // byte to the left
            T_val above = (i == 0) ? static_cast<T_val>(0)
                                   : matrix[i - 1, j];  // byte above
            T_val upperLeft = (i == 0 || j == 0)
                                  ? static_cast<T_val>(0)
                                  : matrix[i - 1, j - 1];  // byte upper-left

            T_val filteredByteVal;

            switch (filterType) {
                case FilterType::None:
                    filteredByteVal = currentByte;
                    break;
                case FilterType::Sub:
                    filteredByteVal = currentByte - left;
                    break;
                case FilterType::Up:
                    filteredByteVal = currentByte - above;
                    break;
                case FilterType::Average: {
                    if constexpr (sizeof(T_val) == 4) {
                        auto [a, b, c, d] = currentByte;
                        auto [e, f, g, h] = left;
                        auto [i, j, k, l] = above;

                        a -= (e + i) / 2;
                        b -= (f + j) / 2;
                        c -= (g + k) / 2;
                        d -= (h + l) / 2;
                        filteredByteVal = {
                            static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                            static_cast<uint8_t>(c), static_cast<uint8_t>(d)};
                    } else if (sizeof(T_val) == 3) {
                        auto [a, b, c] = currentByte;
                        auto [e, f, g] = left;
                        auto [i, j, k] = above;

                        a -= (e + i) / 2;
                        b -= (f + j) / 2;
                        c -= (g + k) / 2;
                        filteredByteVal = {static_cast<uint8_t>(a),
                                           static_cast<uint8_t>(b),
                                           static_cast<uint8_t>(c)};
                    } else {
                        throw std::runtime_error("Unsupported color type");
                    }
                    break;
                }
                case FilterType::Paeth: {
                    if constexpr (sizeof(T_val) == 4) {
                        auto [a, b, c, d] = currentByte;
                        auto [e, f, g, h] = left;
                        auto [i, j, k, l] = above;
                        auto [m, n, o, p] = upperLeft;

                        a -= paethPredictor(e, i, m);
                        b -= paethPredictor(f, j, n);
                        c -= paethPredictor(g, k, o);
                        d -= paethPredictor(h, l, p);
                        filteredByteVal = {
                            static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                            static_cast<uint8_t>(c), static_cast<uint8_t>(d)};
                    } else if constexpr (sizeof(T_val) == 3) {
                        auto [a, b, c] = currentByte;
                        auto [e, f, g] = left;
                        auto [i, j, k] = above;
                        auto [m, n, o] = upperLeft;

                        a -= paethPredictor(e, i, m);
                        b -= paethPredictor(f, j, n);
                        c -= paethPredictor(g, k, o);
                        filteredByteVal = {static_cast<uint8_t>(a),
                                           static_cast<uint8_t>(b),
                                           static_cast<uint8_t>(c)};
                    } else {
                        throw std::runtime_error("Unsupported color type");
                    }
                    break;
                }
            }
            filteredMatrix[i, j] = filteredByteVal;
        }
    }

    return filteredMatrix;
}
}  // namespace f9ay::deflate
