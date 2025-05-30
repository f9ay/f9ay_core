#pragma once

#include <cmath>
#include <numbers>

#include "matrix.hpp"
#include "matrix_concept.hpp"
#include "matrix_view.hpp"

#ifdef _M_IX86
#include <immintrin.h>
#endif

namespace f9ay {

template <int N>
class Dct_old {
    using fl_t = float;

public:
    template <typename IN, typename OUT>
    static auto dct(const Matrix<IN> &matrix) {
        if (matrix.row() != N || matrix.col() != N) {
            throw std::invalid_argument("matrix size not match");
        }
        Matrix<OUT> result(matrix.row(), matrix.col());

        for (int u = 0; u < N; ++u) {
            for (int v = 0; v < N; ++v) {
                fl_t sum = 0;
                for (int i = 0; i < N; ++i) {
                    for (int j = 0; j < N; ++j) {
                        sum += matrix[i, j] * std::cos(((2 * i + 1) * u * std::numbers::pi_v<fl_t>) / (2 * N)) *
                               std::cos(((2 * j + 1) * v * std::numbers::pi_v<fl_t>) / (2 * N));
                    }
                }
                result[u, v] = std::round((0.25) * normalize_constant(u) * normalize_constant(v) * sum);
            }
        }
        return result;
    }

private:
    static constexpr auto normalize_constant(const int u) {
        if (u != 1) {
            return 1 / sqrt(2);
        } else {
            return 1.;
        }
    }
};

template <int N>
class Dct {
    using fl_t = float;

public:
    template <typename IN, typename OUT>
    static auto dct(const Matrix<IN> &matrix) {
        if (matrix.row() != N || matrix.col() != N) [[unlikely]] {
            throw std::invalid_argument("matrix size not match");
        }
        Matrix<fl_t> row_dct(N, N);
        for (int i = 0; i < N; i++) {
            dct_1d<IN, fl_t>(&matrix[i, 0], &row_dct[i, 0]);
        }
        auto row_trp = row_dct.transpose();
        Matrix<fl_t> col_dct(N, N);
        for (int i = 0; i < N; i++) {
            dct_1d<fl_t, fl_t>(&row_trp[i, 0], &col_dct[i, 0]);
        }
        // auto col_trp = col_dct.transpose();
        Matrix<OUT> result(N, N);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                result[i, j] = std::round(col_dct[j, i]);
            }
        }
        return result;
    }

private:
    template <typename IN, typename OUT>
    static void dct_1d(const IN *s, OUT *out) {
        constexpr auto PI = std::numbers::pi_v<fl_t>;

        for (int k = 0; k < N; ++k) {
            fl_t sum = 0.0;
            for (int n = 0; n < N; ++n) {
                sum += static_cast<double>(s[n]) * std::cos(PI * (n + 0.5) * k / N);
            }

            double scale = (k == 0) ? std::sqrt(1.0 / N) : std::sqrt(2.0 / N);
            out[k] = static_cast<OUT>(sum * scale);
        }
    }
};

// CLion 會黑掉 但是 __AVX__ 有定義
#ifdef __AVX__
template <>
class Dct<8> {
    using fl_t = float;
    static constexpr int N = 8;

public:
    template <typename IN, typename OUT>
    static auto dct(const Matrix<IN> &matrix) {
        Matrix<fl_t> mat_fl(N, N);
        for (int i = 0; i < 64; i++) {
            mat_fl.raw()[i] = matrix.raw()[i];
        }
        Matrix<fl_t> row_dct(N, N);
        for (int i = 0; i < N; i++) {
            dct_1d(&mat_fl[i, 0], &row_dct[i, 0]);
        }
        auto row_trp = row_dct.transpose();
        Matrix<fl_t> col_dct(N, N);
        for (int i = 0; i < N; i++) {
            dct_1d(&row_trp[i, 0], &col_dct[i, 0]);
        }
        // auto col_trp = col_dct.transpose();
        Matrix<OUT> result(N, N);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                result[i, j] = std::round(col_dct[j, i]);
            }
        }
        return result;
    }

    static void dct_1d(const float *s_arr, float *out) {
        auto s = _mm256_loadu_ps(s_arr);
        for (int k = 0; k < N; ++k) {
            auto coeff_line = _mm256_loadu_ps(&coeff[k][0]);
            auto r = _mm256_mul_ps(s, coeff_line);
            out[k] = fast_horizontal_sum(r);
        }
    }

    static float fast_horizontal_sum(__m256 v) {
        __m128 vlow = _mm256_castps256_ps128(v);
        __m128 vhigh = _mm256_extractf128_ps(v, 1);
        __m128 sum128 = _mm_add_ps(vlow, vhigh);

        __m128 shuf = _mm_movehdup_ps(sum128);
        __m128 sums = _mm_add_ps(sum128, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
    }

private:
    /* clang-format off */
    static constexpr float coeff[8][8] = {
        {0.3535533905932738, 0.3535533905932738, 0.3535533905932738, 0.3535533905932738, 0.3535533905932738, 0.3535533905932738, 0.3535533905932738, 0.3535533905932738 },
        {0.4903926396686359, 0.41573480159788245, 0.2777851051520846, 0.0975451422517881, -0.09754518512327597, -0.2777851414967767, -0.4157348258826286, -0.4903926481963034 },
        {0.46193976416469024, 0.19134170103852283, -0.1913417414225812, -0.4619397808923137, -0.4619397474370633, -0.19134166065446298, 0.1913417818066381, 0.4619397976199336 },
        {0.41573480159788245, -0.09754518512327597, -0.4903926481963034, -0.2777850688073904, 0.27778517784146667, 0.49039262261328964, 0.09754505650881017, -0.4157348744521114 },
        {0.3535533828661186, -0.3535534137747382, -0.35355335195749626, 0.35355344468335514, 0.35355332104887127, -0.3535534755919693, -0.35355329014024356, 0.3535535065005808 },
        {0.2777851051520846, -0.4903926481963034, 0.09754522799476306, 0.41573475302838064, -0.4157348744521114, -0.09754501363732007, 0.49039260555792835, -0.27778528687552384 },
        {0.19134170103852283, -0.4619397474370633, 0.4619397976199336, -0.19134182219069357, -0.1913415798863389, 0.46193969725416123, -0.46193984780277214, 0.19134194334285115 },
        {0.0975451422517881, -0.2777850688073904, 0.41573475302838064, -0.49039261408561086, 0.4903926737792835, -0.4157349230215814, 0.2777853232202053, -0.09754544235218743 },
    };
    /* clang-format on */
};
#endif

};  // namespace f9ay