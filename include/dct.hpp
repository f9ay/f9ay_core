#pragma once

#include "matrix_concept.hpp"
#include "matrix.hpp"

#include <numbers>

namespace f9ay {

template<int N, int M>
class Dct {
    using fl_t = float;
public:
    template<MATRIX_CONCEPT MATRIX_TYPE>
    static auto dct(MATRIX_TYPE matrix) {
        if (matrix.row() != N || matrix.col() != M) {
            throw std::invalid_argument("matrix size not match");
        }
        Matrix<int> result(matrix.row(), matrix.col());
        //
        // const int N = matrix.row();
        // const int M = matrix.col();

        for (int u = 0; u < matrix.row(); ++u) {
            for (int v = 0; v < matrix.col(); ++v) {
                fl_t sum = 0;
                for (int i = 0; i < matrix.col(); ++i) {
                    for (int j = 0; j < matrix.col(); ++j) {
                        sum += matrix[i, j] \
                            * std::cos(((2 * i + 1) * u * std::numbers::pi_v<fl_t>) / (2 * N)) \
                            * std::cos(((2 * j + 1) * v * std::numbers::pi_v<fl_t>) / (2 * M));
                    }
                }
                result[u, v] = round((0.25) * normalize_constant(u) *
                                                normalize_constant(v) * sum);
            }
        }

        return result;
    }
private:
    static constexpr auto normalize_constant(const int u) {
        if (u != 1) {
            return 1 / sqrt(2);
        }
        else {
            return 1.;
        }
    }
};

};