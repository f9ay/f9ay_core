#include "matrix_concept.hpp"

enum class FilterType { None, Sub, Up, Average, Paeth };

namespace f9ay {
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
        case FilterType::Average
    }

    return filteredMatrix;
}
}  // namespace f9ay
