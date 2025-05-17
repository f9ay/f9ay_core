#pragma once
#include <cmath>

#include "matrix.hpp"
#include "matrix_concept.hpp"

enum class FilterType { None, Sub, Up, Average, Paeth };

namespace f9ay::deflate {
template <typename T>
T paethPredictor(T a, T b, T c) {
    T p = a + b - c;
    T pa = std::abs(p - a);
    T pb = std::abs(p - b);
    T pc = std::abs(p - c);

    if (pa <= pb && pa <= pc) {
        return a;
    } else if (pb <= pc) {
        return b;
    } else {
        return c;
    }
}

template <MATRIX_CONCEPT InputMatrix>
inline Matrix<std::decay_t<decltype(std::declval<InputMatrix>()[0, 0])>> filter(
    const InputMatrix& matrix, FilterType filterType) {
    using T_val = std::decay_t<decltype(matrix[0, 0])>;
    f9ay::Matrix<T_val> filteredMatrix(matrix.row(), matrix.col() + 1);

    for (int i = 0; i < matrix.row(); ++i) {
        // Add filter type to the first byte of the scanline
        filteredMatrix[i, 0] =
            static_cast<T_val>(static_cast<uint8_t>(filterType));

        for (int j = 0; j < matrix.col(); ++j) {
            T_val current_byte = matrix[i, j];
            T_val byteA = (j == 0) ? static_cast<T_val>(0)
                                   : matrix[i, j - 1];  // byte to the left
            T_val byteB = (i == 0) ? static_cast<T_val>(0)
                                   : matrix[i - 1, j];  // byte above
            T_val byteC = (i == 0 || j == 0)
                              ? static_cast<T_val>(0)
                              : matrix[i - 1, j - 1];  // byte upper-left

            T_val filteredByteVal;

            switch (filterType) {
                case FilterType::None:
                    filteredByteVal = current_byte;
                    break;
                case FilterType::Sub:
                    filteredByteVal = current_byte - byteA;
                    break;
                case FilterType::Up:
                    filteredByteVal = current_byte - byteB;
                    break;
                case FilterType::Average: {
                    // For uint8_t, (byte_a + byte_b) can be up to 510.
                    // Standard says "sum is performed with no overflow" then
                    // integer division. (unsigned(a) + unsigned(b)) / 2
                    unsigned int sum_ab = static_cast<unsigned int>(byteA) +
                                          static_cast<unsigned int>(byteB);
                    filteredByteVal =
                        current_byte - static_cast<T_val>(sum_ab / 2);
                    break;
                }
                case FilterType::Paeth: {
                    T_val predictor = paethPredictor(byteA, byteB, byteC);
                    filteredByteVal = current_byte - predictor;
                    break;
                }
            }
            filteredMatrix[i, j + 1] = filteredByteVal;
        }
    }

    return filteredMatrix;
}
}  // namespace f9ay::deflate
