/* entropy.cpp
 *
 * Copyright 2025 Anivice Ives
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <fstream>
#include <iostream>
#include <ranges>
#include <thread>
#include <cmath>
#include <iomanip>
#include <memory>
#include "error.h"
#include "args.h"
#include "lzw6.h"
#include <unordered_map>

namespace utils = lzw::utils;

utils::PreDefinedArgumentType::PreDefinedArgument MainArgument = {
    { .short_name = 'h', .long_name = "help",       .argument_required = false, .description = "Show help" },
    { .short_name = 'v', .long_name = "version",    .argument_required = false, .description = "Show version" },
    { .short_name = 'i', .long_name = "input",      .argument_required = true,  .description = "Input file" },
    { .short_name = 'T', .long_name = "threads",    .argument_required = true,  .description = "Specify the number of worker threads" },
};

long double entropy_of(const std::vector<uint8_t>& data, std::unordered_map <uint8_t, uint64_t> & frequency_map)
{
    for (const auto & code : data) {
        frequency_map[code]++;
    }

    uint64_t total_tokens = 0;
    for (const auto & freq: frequency_map | std::views::values) {
        total_tokens += freq;
    }

    std::vector < long double > EntropyList;
    EntropyList.reserve(256);
    for (const auto & freq: frequency_map | std::views::values) {
        const auto prob = static_cast<long double>(freq) / static_cast<long double>(total_tokens);
        EntropyList.push_back(prob * log2l(prob));
    }

    long double entropy = 0;
    for (const auto & entropy_sig: EntropyList) {
        entropy += entropy_sig;
    }
    entropy = -entropy;
    return entropy;
}

int main(const int argc, char **argv)
{
    constexpr int BLOCK_SIZE = 1024 * 256; // 256 KB
    try {
        uint64_t thread_count = 1;
        const utils::PreDefinedArgumentType PreDefinedArguments(MainArgument);
        utils::ArgumentParser ArgumentParser(argc, argv, PreDefinedArguments);
        const auto parsed = ArgumentParser.parse();
        if (parsed.contains("help")) {
            std::cout << *argv << " [Arguments [OPTIONS...]...]" << std::endl;
            std::cout << PreDefinedArguments.print_help();
            return EXIT_SUCCESS;
        }

        if (parsed.contains("version")) {
            std::cout << *argv << VERSION << std::endl;
            return EXIT_SUCCESS;
        }

        if (!parsed.contains("input")) {
            std::cout << *argv << " [Arguments [OPTIONS...]...]" << std::endl;
            std::cout << PreDefinedArguments.print_help();
            return EXIT_FAILURE;
        }

        if (parsed.contains("threads")) {
            thread_count = std::strtoul(parsed.at("threads").c_str(), nullptr, 10);
        }

        auto build_frequency_map = [](const std::vector<uint8_t> * data, std::unordered_map <uint8_t, uint64_t> * frequency_map)
        {
            for (const auto & code : *data) {
                (*frequency_map)[code]++;
            }
        };

        if (parsed.contains("input"))
        {
            const auto input_file = parsed.at("input");
            std::ifstream input_file_stream(input_file, std::ios::binary);
            if (!input_file_stream.is_open()) {
                throw std::runtime_error("Failed to open input file " + input_file);
            }

            std::unordered_map <uint8_t, uint64_t> all_frequency_map;

            while (!input_file_stream.eof())
            {
                std::vector < std::vector <uint8_t> > data;
                std::vector < std::thread > threads;
                std::vector < std::unordered_map <uint8_t, uint64_t> > frequency_map;
                for (int i = 0; i < thread_count; i++)
                {
                    std::vector <uint8_t> buffer;
                    buffer.resize(BLOCK_SIZE);
                    input_file_stream.read(reinterpret_cast<char *>(buffer.data()), BLOCK_SIZE);
                    const auto actual = input_file_stream.gcount();
                    if (actual == 0) {
                        break;
                    }
                    buffer.resize(actual);
                    data.emplace_back(std::move(buffer));
                }

                frequency_map.resize(data.size());
                for (int i = 0; i < data.size(); i++) {
                    threads.emplace_back(build_frequency_map, &data[i], &frequency_map[i]);
                }

                for (auto & thread: threads) {
                    if (thread.joinable()) {
                        thread.join();
                    }
                }

                for (const auto & map: frequency_map)
                {
                    for (const auto & [sym, freq]: map) {
                        all_frequency_map[sym] += freq;
                    }
                }
            }

            std::cout << input_file << ": " << std::fixed << std::setprecision(4)
                      << entropy_of({}, all_frequency_map) << std::endl;

            return EXIT_SUCCESS;
        }

         return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
