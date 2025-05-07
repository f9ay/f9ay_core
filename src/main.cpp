#include <matrix.h>

#include <functional>
#include <iostream>
#include <map>
#include <print>

int main(int argc, char **argv) {
    Matrix<int> matrix(3, 3);

    for (auto it = matrix.begin(); it != matrix.end(); ++it) {
        for (auto it2 = (*it).begin(); it2 != (*it).end(); ++it2) {
            std::cout << *it2 << " ";
        }
    }

    std::cout << "\n";

    for (auto x : matrix) {
        for (auto &y : x) {
            y = 1;
        }
    }

    matrix[1, 2] = 3;

    matrix[2][2] = 4;

    for (auto x : matrix) {
        for (auto y : x) {
            std::cout << y << " ";
        }
        std::cout << "\n";
    }

    std::print("{}", matrix);

    std::cout << "======================\n";

    std::cout << matrix << "\n";
}
