#include "lzw6.h"
#include <random>

int main()
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(0, UINT64_MAX);
    std::uniform_int_distribution<std::mt19937::result_type> dist7(1, 0x3F);

    std::vector<uint8_t> bit_stream_data;
    std::vector<uint64_t> target, result, bitwidth;

    {
        lzw::BitWriterLSB bit_stream(bit_stream_data);
        for (uint64_t i = 0; i < 8192; i++) {
            const uint64_t bits = dist6(rng);
            const uint64_t bitSize = dist7(rng);
            const uint64_t bitMask = lzw::mask(bitSize);
            bit_stream.write(bits, bitSize);
            bitwidth.push_back(bitSize);
            target.push_back(bits & bitMask);
        }
    }

    std::vector<std::pair< uint64_t, uint64_t> > vec;
    lzw::BitReaderLSB bit_stream(bit_stream_data);
    uint64_t i = 0;
    for (const auto width : bitwidth) {
        result.push_back(bit_stream.read(width));
        const auto a = result.back(), b = target[i];
        if (a != b) {
            return 1;
        }
        vec.clear();
        i++;
    }

    return 0;
}
