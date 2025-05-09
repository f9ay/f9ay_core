#pragma once

template <typename MATRIX_TYPE>
concept MATRIX_TYPE = requires(MATRIX_TYPE matrix) {
    { matrix.begin };
    { matrix.end };
    { matrix[0] };
    { matrix[0, 0] };
    { matrix.row() };
    { matrix.col() };
};
