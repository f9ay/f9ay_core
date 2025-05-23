#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <print>
#include <queue>
#include <ranges>
#include <source_location>

#include "bmp.hpp"
#include "dct.hpp"
#include "huffman_coding.hpp"
#include "lz77_compress.hpp"
#include "matrix.hpp"
#include "matrix_view.hpp"
#include "png.hpp"

#ifdef WIN32
#include "platform/windows_show.hpp"
#endif

using namespace f9ay;

void test_Matrix() {
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

    std::println("====================");
    /*
    [  [16,  11,  10,  16,  24,  40,  51,  61],
    [12,  12,  14,  19,  26,  58,  60,  55],
    [14,  13,  16,  24,  40,  57,  69,  56],
    [14,  17,  22,  29,  51,  87,  80,  62],
    [18,  22,  37,  56,  68, 109, 103,  77],
    [24,  35,  55,  64,  81, 104, 113,  92],
    [49,  64,  78,  87, 103, 121, 120, 101],
    [72,  92,  95,  98, 112, 100, 103,  99] ]
    */

    Matrix<float> mtx3 = {{16, 11, 10, 16, 24, 40, 51, 61},
                          {12, 12, 14, 19, 26, 58, 60, 55},
                          {14, 13, 16, 24, 40, 57, 69, 56},
                          {14, 17, 22, 29, 51, 87, 80, 62},
                          {18, 22, 37, 56, 68, 109, 103, 77},
                          {24, 35, 55, 64, 81, 104, 113, 92},
                          {49, 64, 78, 87, 103, 121, 120, 101},
                          {72, 92, 95, 98, 112, 100, 103, 99}};
    const auto view3 = Matrix_view(mtx3);

    auto result = Dct<8, 8>::dct(view3);

    std::println("{}", result);

    std::println("======================================");

    Matrix<float> mtx4(8, 8);

    for (auto x : mtx4) {
        for (auto& y : x) {
            y = 128;
        }
    }

    const auto view4 = Matrix_view(mtx4);

    auto res2 = Dct<8, 8>::dct(view4);

    std::print("{}", res2);
    std::println("======================================");
}

int main(int argc, char** argv) {
    std::filesystem::path path = std::source_location::current().file_name();
    path = path.parent_path().parent_path() / "test_data" / "fire.bmp";
    std::cout << path << std::endl;
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }
    const auto file = readFile(fs);
    auto result = Bmp::import(file.get());
    std::unique_ptr<std::byte[]> buffer;
    size_t sz;
    std::visit(
        [&buffer, &sz, &path](auto&& arg) {
            // std::tie(buffer, sz) = Bmp::write(arg);

            std::ofstream out(path.parent_path() / "test.png",
                              std::ios::binary);

            auto [buffer, size] = PNG::exportToByte(arg, FilterType::Sub);
            out.write(reinterpret_cast<const char*>(buffer.get()), size);
        },
        result);

    Matrix<colors::BGR> mtx(3, 3);

    std::ofstream out(path.parent_path() / "fire_converted.bmp",
                      std::ios::binary);

    std::string test = "AAAAAAAAAAAAAAAAAAAA";

    auto vec = LZ77::lz77EncodeSlow(test);
    for (auto [offset, length, value] : vec) {
        std::cout << "Offset: " << offset << ", Length: " << length
                  << ", Value: " << (value.has_value() ? *value : ' ') << "\n";
    }

    auto decoded = LZ77::lz77decode<std::string>(vec);
    std::cout << "Decoded: " << decoded << "\n";

    std::cout << "done" << std::endl;

#ifdef WIN32
    f9ay::test::windows::Windows windows{};
    std::visit([&windows](auto&& arg) { windows.show(arg); }, result);
#endif
}