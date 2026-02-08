#include <iostream>
#include <memory>
#include <sstream>
#include "error.h"
#include "args.h"
#define USE_TSL_HOPSCOTCH_MAP
#include "lzw6.h"
#include "mmap.h"
#include <fstream>
#include <thread>
#include <cstring>
#include "cppcrc.h"

namespace utils = lzw::utils;

utils::PreDefinedArgumentType::PreDefinedArgument MainArgument = {
    { .short_name = 'h', .long_name = "help",       .argument_required = false, .description = "Show help" },
    { .short_name = 'v', .long_name = "version",    .argument_required = false, .description = "Show version" },
    { .short_name = 'i', .long_name = "input",      .argument_required = true,  .description = "Input file" },
    { .short_name = 'o', .long_name = "output",     .argument_required = true,  .description = "Output file" },
    { .short_name = 'd', .long_name = "decompress", .argument_required = false, .description = "Decompress instead of compress" },
    { .short_name = 'T', .long_name = "threads",    .argument_required = true,  .description = "Specify the number of worker threads" },
};


struct section_head_16bit_t {
    uint16_t section_size;
};

int main(int argc, char** argv)
{
    try
    {
        const utils::PreDefinedArgumentType PreDefinedArguments(MainArgument);
        utils::ArgumentParser ArgumentParser(argc, argv, PreDefinedArguments);
        const auto parsed = ArgumentParser.parse();
        if (parsed.contains("help")) {
            std::cout << *argv << " [Arguments [OPTIONS...]...]" << std::endl;
            std::cout << PreDefinedArguments.print_help();
            return EXIT_SUCCESS;
        }

        if (parsed.contains("version")) {
            std::cout << *argv << VERSION << ", DEBUG=" << DEBUG << std::endl;
            return EXIT_SUCCESS;
        }

        if (!parsed.contains("input") || !parsed.contains("output")) {
            std::cout << *argv << " [Arguments [OPTIONS...]...]" << std::endl;
            std::cout << PreDefinedArguments.print_help();
            return EXIT_FAILURE;
        }

        const auto input_file = parsed.at("input");
        const auto output_file = parsed.at("output");

        lzw::basic_io::mmap input_mmap(input_file, true);
        std::ofstream output_stream(output_file);
        if (!output_stream) {
            throw std::runtime_error("Could not open file " + output_file + ": " + std::strerror(errno));
        }

        constexpr uint64_t bit_size = 12;
        constexpr uint64_t block_size = lzw::const_two_power(bit_size) - 1;
        bool compress = !parsed.contains("decompress");
        unsigned int workers = std::thread::hardware_concurrency();
        if (parsed.contains("threads")) {
            try {
                workers = std::strtoul(parsed.at("threads").c_str(), nullptr, 10);
            } catch (...) {
                std::cerr << "Malformed thread number, ignored" << std::endl;
            }
        }

        if (compress)
        {
            std::atomic < uint64_t > lzw_used(0);
            std::atomic < uint64_t > huffman_used(0);
            const auto blocks = input_mmap.size() / block_size + (input_mmap.size() % block_size != 0);
            struct pool_frame_t {
                std::vector<uint8_t> output;
                section_head_16bit_t section_head { };
                uint64_t index { };
            };

            std::vector < std::pair < std::thread, std::unique_ptr < pool_frame_t > > > thread_pool;
            int active_threads = 0;

            auto sync_thread = [&] {
                std::ranges::for_each(thread_pool, [&](std::pair < std::thread, std::unique_ptr < pool_frame_t > > & T) {
                    if (T.first.joinable()) T.first.join();
                    const auto & frame = *T.second;
                    output_stream.write(reinterpret_cast<const char *>(&frame.section_head), sizeof(frame.section_head));
                    output_stream.write(reinterpret_cast<const char *>(frame.output.data()), static_cast<std::streamsize>(frame.output.size()));
                });

                thread_pool.clear();
            };

            for (uint64_t i = 0; i < blocks; i++)
            {
                auto frame_ = std::make_unique<pool_frame_t>();
                frame_->index = i;

                thread_pool.emplace_back(std::thread([&](pool_frame_t * frame)
                {
                    std::vector<uint8_t> input(input_mmap.data() + block_size * frame->index,
                        input_mmap.data() + std::min(static_cast<uint64_t>(input_mmap.size()), block_size * (frame->index + 1)));
                    {
                        std::vector<uint8_t> output_huffman;
                        std::vector<uint8_t> output_lzw;
                        lzw::lzw<bit_size> Compressor(input, output_lzw);
                        lzw::Huffman huffman(input, output_huffman);
                        Compressor.compress();
                        huffman.compress();

                        auto write_buffer = [&](const std::vector<uint8_t> & buffer, const char signature)
                        {
                            frame->output.clear();
                            frame->output.reserve(output_lzw.size() + 1);
                            frame->output.push_back(signature);
                            frame->output.insert(frame->output.end(), buffer.begin(), buffer.end());
                        };

                        if (output_huffman.size() > output_lzw.size()) {
                            ++lzw_used;
                            write_buffer(output_lzw, 'L');
                        } else {
                            ++huffman_used;
                            write_buffer(output_huffman, 'H');
                        }
                    }

                    if (frame->output.size() > 0xFFFF) {
                        throw std::runtime_error("Compression failed for this data set");
                    }
                    frame->section_head.section_size = static_cast<uint16_t>(frame->output.size());
                }, frame_.get()), std::move(frame_));

                active_threads++;

                if (active_threads >= workers) {
                    sync_thread();
                    active_threads = 0;
                }
            }

            sync_thread();
            const double lzw_perc = lzw_used / static_cast<double>(lzw_used + huffman_used);
            fprintf(stderr, "LZW: %lu (%0.2f%%), Huffman: %lu (%0.2f%%), overall %lu * %lu\n",
                lzw_used.load(), lzw_perc * 100, huffman_used.load(), (1 - lzw_perc)*100, lzw_used + huffman_used, block_size);
        }
        else
        {
            struct pool_frame_t {
                std::vector<uint8_t> output;
                char * begin = nullptr;
                char * end = nullptr;
            };

            std::vector < std::pair < std::thread, std::unique_ptr < pool_frame_t > > > thread_pool;
            int active_threads = 0;

            auto sync_thread = [&] {
                std::ranges::for_each(thread_pool, [&](std::pair < std::thread, std::unique_ptr < pool_frame_t > > & T) {
                    if (T.first.joinable()) T.first.join();
                    const auto & output = T.second->output;
                    output_stream.write(reinterpret_cast<const char *>(output.data()), static_cast<std::streamsize>(output.size()));
                });

                thread_pool.clear();
            };

            uint64_t offset = 0;
            auto read_head = [&]()->section_head_16bit_t
            {
                section_head_16bit_t ret { };
                std::memcpy(&ret, input_mmap.data() + offset, sizeof(ret));
                offset += sizeof(ret);
                return ret;
            };

            for (uint64_t i = 0; offset < input_mmap.size(); i++)
            {
                auto frame_ = std::make_unique<pool_frame_t>();
                const auto [section_size] = read_head();
                frame_->begin = input_mmap.data() + offset;
                frame_->end = input_mmap.data() + offset + section_size;
                offset += section_size;

                thread_pool.emplace_back(std::thread([](pool_frame_t * frame)
                {
                    std::vector<uint8_t> input(frame->begin, frame->end);
                    if (!input.empty() && input.front() == 'H') { // Huffman
                        input.erase(input.begin());
                        lzw::Huffman huffman(input, frame->output);
                        huffman.decompress();
                    } else {
                        input.erase(input.begin());
                        lzw::lzw<bit_size> Compressor(input, frame->output);
                        Compressor.decompress();
                    }
                }, frame_.get()), std::move(frame_));

                active_threads++;

                if (active_threads >= workers) {
                    sync_thread();
                    active_threads = 0;
                }
            }

            sync_thread();
        }

        output_stream.close();
        input_mmap.close();

        return EXIT_SUCCESS;
    }
    catch (std::exception & e) {
        std::cout << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
