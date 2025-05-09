#pragma once

namespace f9ay {
template <typename MATRIX_TYPE>
concept MATRIX_CONCEPT = requires(MATRIX_TYPE matrix) {
    { matrix.begin() };
    { matrix.end() };
    { matrix[0] };
    { matrix[0, 0] };
    { matrix.row() } -> std::convertible_to<int>;
    { matrix.col() } -> std::convertible_to<int>;
};
}  // namespace f9ay
