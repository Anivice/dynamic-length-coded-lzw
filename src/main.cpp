#include <iostream>
#include <memory>
#include <sstream>
#include "colors.h"
#include "error.h"
#include "utils.h"
#include "args.h"
#include "lzw.h"
#include "mmap.h"
#include <fstream>
#include <thread>

namespace utils = lzw::utils;

utils::PreDefinedArgumentType::PreDefinedArgument MainArgument = {
    { .short_name = 'h', .long_name = "help",       .argument_required = false, .description = "Show help" },
    { .short_name = 'v', .long_name = "version",    .argument_required = false, .description = "Show version" },
    { .short_name = 'i', .long_name = "input",      .argument_required = true,  .description = "Input file" },
    { .short_name = 'o', .long_name = "output",     .argument_required = true,  .description = "Output file" },
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

        lzw::basic_io::mmap input_mmap(input_file);
        std::ofstream output_stream(output_file);
        if (!output_stream) {
            throw std::runtime_error("Could not open file " + output_file + ": " + std::strerror(errno));
        }

        constexpr uint64_t bit_size = 12;
        constexpr uint64_t block_size = lzw::two_power(bit_size) - 1;

        const auto blocks = lzw::utils::arithmetic::count_cell_with_cell_size(block_size, input_mmap.size());
        std::vector < std::pair < std::thread, std::unique_ptr < std::vector<uint8_t> > > > thread_pool;
        int active_threads = 0;
        for (uint64_t i = 0; i < blocks; i++)
        {
            auto output = std::make_unique<std::vector<uint8_t>>();
            thread_pool.emplace_back(std::thread([&input_mmap](std::vector<uint8_t> * output_, const uint64_t index)
            {
                std::vector<uint8_t> input(input_mmap.data() + block_size * index,
                    input_mmap.data() + std::min(static_cast<uint64_t>(input_mmap.size()), block_size * (index + 1)));

                lzw::lzw<bit_size> Compressor(input, *output_);
                Compressor.compress();
            }, output.get(), i), std::move(output));

            active_threads++;

            if (active_threads == std::thread::hardware_concurrency()) {
                std::ranges::for_each(thread_pool, [&](std::pair < std::thread, std::unique_ptr<std::vector<uint8_t>> > & T) {
                    if (T.first.joinable()) T.first.join();
                    output_stream.write(reinterpret_cast<const char *>(T.second->data()), static_cast<std::streamsize>(T.second->size()));
                });

                thread_pool.clear();
                active_threads = 0;
            }
        }

        std::ranges::for_each(thread_pool, [&](std::pair < std::thread, std::unique_ptr<std::vector<uint8_t>> > & T) {
            if (T.first.joinable()) T.first.join();
            output_stream.write(reinterpret_cast<const char *>(T.second->data()), static_cast<std::streamsize>(T.second->size()));
        });

        thread_pool.clear();

        output_stream.close();
        input_mmap.close();

        return EXIT_SUCCESS;
    }
    catch (std::exception & e) {
        elog(e.what(), "\n");
        return EXIT_FAILURE;
    }
}
