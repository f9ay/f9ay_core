#pragma once

#ifdef _M_IX86
// TODO runtime check support AVX
namespace f9ay {
class CpuId {
public:
    static bool cpuid() {}
    static bool checkAVX() {}
};
}  // namespace f9ay
#endif