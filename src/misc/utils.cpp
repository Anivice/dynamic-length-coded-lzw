#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "utils.h"
#include "error.h"
#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
#include "cppcrc.h"
#include <ctime>

std::string lzw::utils::getenv(const std::string& name) noexcept
{
    const auto var = ::getenv(name.c_str());
    if (var == nullptr) {
        return "";
    }

    return var;
}

std::string lzw::utils::replace_all(
    std::string & original,
    const std::string & target,
    const std::string & replacement) noexcept
{
    if (target.empty()) return original; // Avoid infinite loop if target is empty

    if (target.size() == 1 && replacement.empty()) {
        std::erase_if(original, [&target](const char c) { return c == target[0]; });
        return original;
    }

    size_t pos = 0;
    while ((pos = original.find(target, pos)) != std::string::npos) {
        original.replace(pos, target.length(), replacement);
        pos += replacement.length(); // Move past the replacement to avoid infinite loop
    }

    return original;
}

uint64_t lzw::utils::arithmetic::count_cell_with_cell_size(const uint64_t cell_size, const uint64_t particles)
{
    if (cell_size == 0) {
        throw std::runtime_error("DIV/0");;
    }
#ifdef __x86_64__
    // 50% performance boost bc we used div only once
    uint64_t q, r;
    asm ("divq %[d]"
         : "=a"(q), "=d"(r)
         : "0"(particles), "1"(0), [d]"r"(cell_size)
         : "cc");

    if (r != 0) return q + 1;
    else return q;
#else
    const uint64_t cells = particles / cell_size + (particles % cell_size == 0 ? 0 : 1);
    return cells;
#endif
}

uint64_t lzw::utils::arithmetic::hash64(const uint8_t *data, const size_t length) noexcept
{
    return CRC64::ECMA::calc(data, length);
}
