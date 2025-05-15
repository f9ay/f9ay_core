#include <cstddef>
#include <fstream>

namespace f9ay {
inline void writeFile(std::ofstream& fs, std::byte* buffer, size_t size) {
    fs.write(reinterpret_cast<char*>(buffer), size);
}
}  // namespace f9ay