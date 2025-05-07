#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <print>

#include "matrix.hpp"
#include "matrix_view.hpp"

using namespace f9ay;

int main(int argc, char **argv) {
    Matrix<std::tuple<int, int, int>> mtx(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx[i, j] = std::make_tuple(1, 2, 3);
        }
    }
    std::print("{}", mtx);
    std::println("====================");
    auto view = Matrix_view_fixed<1>(mtx);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << view[i, j] << " ";
        }
        std::cout << std::endl;
    }
    std::println("====================");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << view[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::println("====================");
    for (const auto& row : view) {
        for (const auto& col : row) {
            std::cout << col << " ";
        }
        std::cout << std::endl;
    }
    /////////////////////////////
    /// rumtime///////////////////
    /////////////////////////////
    std::println("====================");
    auto mtx2 = Matrix<std::array<int, 3>>(3, 3);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx2[i, j][2] = -3;
        }
    }

    auto view2 = Matrix_view(mtx2, 2);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << view2[i, j] << " ";
        }
        std::cout << std::endl;
    }
}
