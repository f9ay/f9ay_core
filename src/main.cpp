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
#include "jpeg.hpp"
#include "lz77_compress.hpp"
#include "matrix.hpp"
#include "matrix_view.hpp"
#include "png.hpp"

#ifdef WIN32
#include "platform/windows_show.hpp"
#endif

using namespace f9ay;

// void test_Matrix() {
//     Matrix<std::tuple<int, int, int>> mtx(3, 3);
//     for (int i = 0; i < 3; i++) {
//         for (int j = 0; j < 3; j++) {
//             mtx[i, j] = std::make_tuple(1, 2, 3);
//         }
//     }
//     std::print("{}", mtx);
//     std::println("====================");
//     auto view = Matrix_view_fixed<1>(mtx);
//     for (int i = 0; i < 3; i++) {
//         for (int j = 0; j < 3; j++) {
//             std::cout << view[i, j] << " ";
//         }
//         std::cout << std::endl;
//     }
//     std::println("====================");
//     for (int i = 0; i < 3; i++) {
//         for (int j = 0; j < 3; j++) {
//             std::cout << view[i][j] << " ";
//         }
//         std::cout << std::endl;
//     }
//     std::println("====================");
//     for (const auto& row : view) {
//         for (const auto& col : row) {
//             std::cout << col << " ";
//         }
//         std::cout << std::endl;
//     }
//     /////////////////////////////
//     /// rumtime///////////////////
//     /////////////////////////////
//     std::println("====================");
//     auto mtx2 = Matrix<std::array<int, 3>>(3, 3);
//
//     for (int i = 0; i < 3; i++) {
//         for (int j = 0; j < 3; j++) {
//             mtx2[i, j][2] = -3;
//         }
//     }
//
//     auto view2 = Matrix_view(mtx2, 2);
//     for (int i = 0; i < 3; i++) {
//         for (int j = 0; j < 3; j++) {
//             std::cout << view2[i, j] << " ";
//         }
//         std::cout << std::endl;
//     }
//
//     std::println("====================");
//     /*
//     [  [16,  11,  10,  16,  24,  40,  51,  61],
//     [12,  12,  14,  19,  26,  58,  60,  55],
//     [14,  13,  16,  24,  40,  57,  69,  56],
//     [14,  17,  22,  29,  51,  87,  80,  62],
//     [18,  22,  37,  56,  68, 109, 103,  77],
//     [24,  35,  55,  64,  81, 104, 113,  92],
//     [49,  64,  78,  87, 103, 121, 120, 101],
//     [72,  92,  95,  98, 112, 100, 103,  99] ]
//     */
//
//     Matrix<float> mtx3 = {{16, 11, 10, 16, 24, 40, 51, 61},
//                           {12, 12, 14, 19, 26, 58, 60, 55},
//                           {14, 13, 16, 24, 40, 57, 69, 56},
//                           {14, 17, 22, 29, 51, 87, 80, 62},
//                           {18, 22, 37, 56, 68, 109, 103, 77},
//                           {24, 35, 55, 64, 81, 104, 113, 92},
//                           {49, 64, 78, 87, 103, 121, 120, 101},
//                           {72, 92, 95, 98, 112, 100, 103, 99}};
//     const auto view3 = Matrix_view(mtx3);
//
//     auto result = Dct<8, 8>::dct(view3);
//
//     std::println("{}", result);
//
//     std::println("======================================");
//
//     Matrix<float> mtx4(8, 8);
//
//     for (auto x : mtx4) {
//         for (auto& y : x) {
//             y = 128;
//         }
//     }
//
//     const auto view4 = Matrix_view(mtx4);
//
//     auto res2 = Dct<8, 8>::dct(view4);
//
//     std::print("{}", res2);
//     std::println("======================================");
// }

// int test_img() {
//     std::filesystem::path path = std::source_location::current().file_name();
//     path = path.parent_path().parent_path() / "test_data" / "fire.bmp";
//     std::cout << path << std::endl;
//     std::ifstream fs(path, std::ios::binary);
//     if (!fs.is_open()) {
//         std::cerr << "Failed to open file" << std::endl;
//         return 1;
//     }
//     const auto file = readFile(fs);
//     auto result = Bmp::import(file.get());
//     std::unique_ptr<std::byte[]> buffer;
//     size_t sz;
//     std::visit(
//         [&buffer, &sz, &path](auto&& arg) {
//             // std::tie(buffer, sz) = Bmp::write(arg);
//
//             Matrix<colors::RGB> mtx(1, 1);
//             mtx[0, 0] = {0xFF, 0x00, 0x00};
//
//             std::ofstream out(path.parent_path() / "test.png", std::ios::binary);
//
//             auto [buffer, size] = PNG::exportToByte(arg, FilterType::Up);
//             out.write(reinterpret_cast<const char*>(buffer.get()), size);
//         },
//         result);
//
//     Matrix<colors::BGR> mtx(3, 3);
//
//     std::ofstream out(path.parent_path() / "fire_converted.bmp", std::ios::binary);
//
//     std::string test = "aacaacabcabaaac";
//
//     auto vec = LZ77::lz77Encode(test);
//     for (auto [offset, length, value] : vec) {
//         std::cout << "Offset: " << offset << ", Length: " << length << ", Value: " << (value.has_value() ? *value : '
//         ')
//                   << "\n";
//     }
//
//     std::cout << "done" << std::endl;
//
// #ifdef WIN32
//     f9ay::test::windows::Windows windows{};
//     std::visit(
//         [&windows](auto&& arg) {
//             windows.show(arg);
//         },
//         result);
// #endif
//     return 0;
// }

std::string convert_amp(uint32_t amp, uint32_t size) {
    std::string amp_str = std::bitset<16>(amp).to_string();
    std::ranges::reverse(amp_str);
    amp_str.resize(size);
    std::ranges::reverse(amp_str);
    return amp_str;
}

void test_rle(std::array<int16_t, 64>& arr) {
    for (auto&& [r, amp] : Jpeg::calculate_rle(arr)) {
        int len = r >> 4;
        int size_for_bit = r & 0xFu;
        std::string amp_str = convert_amp(amp, size_for_bit);
        std::print("{{ {} {} }}", len, amp_str);
    }
    std::println("");
}

void test_dc_pair(std::vector<int8_t>& dc_test) {
    auto converted = Jpeg::convert_dc_to_size_value(dc_test);
    for (auto& [size, value] : converted) {
        std::string amp_str = convert_amp(value, size);
        std::print("{{ {} {} }}", size, amp_str);
    }
    std::println("=======================");
}

void test_jpeg() {
    std::vector<int8_t> dc_test_1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 77};
    test_dc_pair(dc_test_1);

    std::vector<int8_t> dc_test_2 = {0, -1, -2, -3, -4, -5, -6, -7, -8, -77};
    test_dc_pair(dc_test_2);
}

#include <stacktrace>

void msvc_terminate_handler() {
    std::cout << std::stacktrace::current() << '\n';
    auto exception_ptr = std::current_exception();
    try {
        if (exception_ptr) {
            std::rethrow_exception(exception_ptr);
        }
    } catch (const std::exception& e) {
        std::println(stderr, "Unhandled exception:\ntype : {}\nwhat: {}", typeid(e).name(), e.what());
    } catch (...) {
        std::println(stderr, "Unknown exception occurred");
    }
    std::abort();
}

int main(int argc, char** argv) {
#ifdef _MSC_VER
    std::set_terminate(msvc_terminate_handler);
#endif
    auto category_test = {0, 1, 2, 3, 4, 5, 6, 7, 8, -5};
    for (auto&& c : category_test) {
        std::println(" {} {}", Jpeg::category(c), Jpeg::to_amplitude(c, Jpeg::category(c)));
    }
    std::println("");
    std::array<int16_t, 64> rle_test_1 = {0, 0};
    test_rle(rle_test_1);
    std::array<int16_t, 64> rle_test_2 = {0, 0, 1, 0, 0, 2, 3, 4};
    test_rle(rle_test_2);
    std::array<int16_t, 64> rle_test_3 = {};
    rle_test_3[16] = 1;
    test_rle(rle_test_3);
    std::array<int16_t, 64> rle_test_4 = {};
    rle_test_4[17] = 1;
    test_rle(rle_test_4);
    std::array<int16_t, 64> rle_test_5 = {};
    for (int i = 0; i < 64; i++) {
        rle_test_5[i] = i;
    }
    test_rle(rle_test_5);
    std::println("=================================");
    test_jpeg();
    // bit writer test
    BitWriter writer;
    writer.changeWriteSequence(WriteSequence::MSB);
    writer.writeBitsFromMSB(0b10, 2);
    writer.writeBitsFromMSB(0b1101100, 7);
    auto buffer1 = writer.getBuffer();
    for (auto& b : buffer1) {
        std::print("{} ", std::bitset<8>(uint8_t(b)).to_string());
    }

    std::println("=================================");

    std::println("{}", Jpeg::category(1024));
    std::println("{}\n{}", std::bitset<16>(60).to_string(), std::bitset<16>(6).to_string());
    std::filesystem::path path = std::source_location::current().file_name();
    path = path.parent_path().parent_path() / "test_data" / "1.bmp";
    std::cout << path << std::endl;
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }
    const auto file = readFile(fs);
    auto result = Bmp::import(file.get());
    auto res = std::get<Matrix<colors::BGR>>(result);
    Matrix<colors::RGB> mtx(res.row(), res.col());
    for (auto i = 0; i < res.row(); i++) {
        for (auto j = 0; j < res.col(); j++) {
            mtx[i, j].r = res[i, j].r;
            mtx[i, j].g = res[i, j].g;
            mtx[i, j].b = res[i, j].b;
        }
    }
    auto [buffer, size] = Jpeg::write(mtx);

    std::ofstream out(path.parent_path() / "current.jpeg", std::ios::binary);
    out.write(reinterpret_cast<const char*>(buffer.get()), size);
    std::cout << "done" << std::endl;
}