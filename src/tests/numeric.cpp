#include <algorithm>
#include "lzw6.h"
#include <random>
#include <thread>

int main(int argc, char ** argv)
{
    auto mask = [](const uint64_t bits)->uint64_t
    {
        uint64_t mask_result = 0;
        for (uint64_t bit = 0; bit < bits; ++bit) {
            mask_result |= (1ULL << bit);
        }
        return mask_result;
    };

    for (uint64_t j = 0; j < std::strtol(argv[1], nullptr, 10) / std::thread::hardware_concurrency(); j++)
    {
        std::vector<std::thread> threads;

        for (uint64_t k = 0; k < std::thread::hardware_concurrency(); ++k)
        threads.emplace_back([&]
        {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist6(0, UINT64_MAX);
            std::uniform_int_distribution<std::mt19937::result_type> dist7(1, 0x3F);

            std::vector<uint8_t> bit_stream_data;
            std::vector<uint64_t> target, result, bitwidth;

            lzw::BitWriterLSB bit_stream(bit_stream_data);
            for (uint64_t i = 0; i < 8192; i++) {
                const uint64_t bits = dist6(rng);
                const uint64_t bitSize = dist7(rng);
                const uint64_t bitMask = mask(bitSize);
                bit_stream.write(bits, bitSize);
                bitwidth.push_back(bitSize);
                target.push_back(bits & bitMask);
            }

            lzw::BitReaderLSB bit_stream_r(bit_stream_data);
            uint64_t i = 0;
            for (const auto width : bitwidth)
            {
                result.push_back(bit_stream_r.read(width));
                const auto a = result.back(), b = target[i];
                if (a != b) {
                    exit(1);
                }
                i++;
            }
        });

        std::ranges::for_each(threads, [](std::thread & T) { if (T.joinable()) { T.join(); } });
    }

    return 0;
}
