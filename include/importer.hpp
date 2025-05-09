#pragma once

#ifdef _MSC_VER
#define __attribute__(x)
#endif

#include <fstream>

#include "colors.hpp"
#include "matrix.hpp"

namespace f9ay {

template <typename STRUCT>
const STRUCT *safeMemberAssign(const std::byte *source) {
    // TODO it is unsafe now
    // use reflect to safe assign
    auto *dst = reinterpret_cast<const STRUCT *>(source);
    return dst;
}

inline std::unique_ptr<std::byte[]> readFile(std::ifstream &fs) {
    fs.seekg(0, std::ios::end);
    const auto file_size = fs.tellg();
    fs.seekg(0, std::ios::beg);
    std::unique_ptr<std::byte[]> buffer(new std::byte[file_size]);
    fs.read(reinterpret_cast<char *>(buffer.get()), file_size);
    return buffer;
}

using Midway = std::variant<f9ay::Matrix<colors::BGR>, Matrix<colors::BGRA>>;

};  // namespace f9ay
