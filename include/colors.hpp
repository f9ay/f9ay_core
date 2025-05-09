#pragma once
#include <cstddef>
#include <tuple>

namespace f9ay::colors {

// struct RGB : std::tuple<std::byte, std::byte, std::byte> {
//     using std::tuple<std::byte, std::byte, std::byte>::tuple;
// };
// struct BGR : std::tuple<std::byte, std::byte, std::byte> {
//     using std::tuple<std::byte, std::byte, std::byte>::tuple;
// };
// struct RGBA : std::tuple<std::byte, std::byte, std::byte, std::byte> {
//     using std::tuple<std::byte, std::byte, std::byte, std::byte>::tuple;
// };
// struct BGRA : std::tuple<std::byte, std::byte, std::byte, std::byte> {
//     using std::tuple<std::byte, std::byte, std::byte, std::byte>::tuple;
// };

using RGB = std::tuple<std::byte, std::byte, std::byte>;
using BGR = std::tuple<std::byte, std::byte, std::byte>;
using RGBA = std::tuple<std::byte, std::byte, std::byte, std::byte>;
using BGRA = std::tuple<std::byte, std::byte, std::byte, std::byte>;

}  // namespace f9ay::colors
