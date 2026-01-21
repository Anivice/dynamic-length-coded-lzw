#include "any_length_numeric.h"
#include <random>
#include <iostream>

int main()
{
    std::vector < lzw::any_length_numeric <12> > result;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(0, 0xFFF);

    for (uint64_t i = 0; i < 8192; i++) {
        result.emplace_back(dist6(rng));
    }

    const auto [cmp, len] = lzw::pack_numeric<12>(result);
    const auto ori = lzw::unpack_numeric<12>(cmp, len);
}
