#pragma once
#include <cmath>

#include "matrix_concept.hpp"

enum class FilterType { None, Sub, Up, Average, Paeth };

namespace f9ay::deflate {
template <FilterType filterType, MATRIX_CONCEPT Matrix>
inline constexpr Matrix filter(const Matrix& matrix) {
    auto filteredMatrix = matrix;

    switch (filterType) {
        case FilterType::None:
            break;
        case FilterType::Sub:
            for (int i = 0; i < matrix.row(); i++) {
                for (int j = 1; j < matrix.col(); j++) {
                    filteredMatrix[i, j] -= matrix[i, j - 1];
                }
            }
            break;
        case FilterType::Up:
            for (int i = 1; i < matrix.row(); i++) {
                for (int j = 0; j < matrix.col(); j++) {
                    filteredMatrix[i, j] -= matrix[i - 1, j];
                }
            }
            break;
        case FilterType::Average:
            for (int i = 1; i < matrix.row(); i++) {
                for (int j = 1; j < matrix.col(); j++) {
                    filteredMatrix[i, j] -=
                        (matrix[i - 1, j] + matrix[i, j - 1]) / 2;
                }
            }
            break;
        case FilterType::Paeth:
            for (int i = 1; i < matrix.row(); i++) {
                for (int j = 1; j < matrix.col(); j++) {
                    auto a = matrix[i, j - 1];
                    auto b = matrix[i - 1, j];
                    auto c = matrix[i - 1, j - 1];
                    auto p = a + b - c;
                    auto pa = abs(a - p);
                    auto pb = abs(b - p);
                    auto pc = abs(c - p);
                    if (pa <= pb && pa <= pc) {
                        filteredMatrix[i, j] -= a;
                    } else if (pb <= pc) {
                        filteredMatrix[i, j] -= b;
                    } else {
                        filteredMatrix[i, j] -= c;
                    }
                }
            }
            break;
    }

    return filteredMatrix;
}
}  // namespace f9ay::deflate
