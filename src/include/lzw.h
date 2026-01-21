/* lzw.h
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

#ifndef LZW_H
#define LZW_H

#include "any_length_numeric.h"
#include <vector>
#include "tsl/hopscotch_map.h"

namespace lzw {
	struct section_head_16bit_t {
		uint16_t section_size;
	};

	struct section_head_32bit_t {
		uint32_t section_size;
	};

	struct section_head_64bit_t {
		uint64_t section_size;
	};

	template <typename Type>
	constexpr Type two_power(const Type n)
	{
		static_assert(std::is_integral_v<Type>, "Type must be an integral type");
		return static_cast<Type>(0x01) << n;
	}

	template <
		unsigned LzwCompressionBitSize,
		unsigned DictionarySize = two_power(LzwCompressionBitSize) - 1 >
	requires (LzwCompressionBitSize > 8)
	class lzw
	{
		std::vector < uint8_t > & input_stream_;
		std::vector < uint8_t > & output_stream_;
		tsl::hopscotch_map < std::string, any_length_numeric < LzwCompressionBitSize > > dictionary_;
		bool discarding_this_instance = false;

	public:
		explicit lzw(
			std::vector < uint8_t >& input_stream,
			std::vector < uint8_t >& output_stream);

		// forbid any copy/move constructor or assignment
		lzw(const lzw&) = delete;
		lzw(lzw&&) = delete;
		lzw& operator =(const lzw&) = delete;
		lzw& operator =(lzw&&) = delete;

		// destructor
		~lzw() = default;

		// basic operations
		void compress();
		void decompress();
	};

}
#endif //LZW_H

// inline definition for lzw utilities
#include "lzw.inl"
