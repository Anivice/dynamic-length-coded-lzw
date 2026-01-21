/* numeric.inl
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

#ifndef NUMERIC_INL
#define NUMERIC_INL

#include <iostream>
#include <algorithm>
#include <cstring>
#include <array>

namespace lzw {
    template < unsigned BitSize, unsigned RequiredBytes = BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1) >
    [[nodiscard]]
    std::array < uint8_t, RequiredBytes >
    bitcopy(
        uint64_t,
        uint8_t,
        uint8_t,
        std::vector<uint8_t> &,
        const std::vector < uint8_t >&);

    template < unsigned BitSize >
    [[nodiscard]]
    std::vector < uint8_t >
    bitcopy(
        const uint64_t bit_start,
        const uint8_t bit_copy,
        const std::vector < uint8_t > & data)
    {
        std::vector < uint8_t > ret{};
        const uint64_t byte_starting = bit_start / 8;
        const uint8_t bit_starting = bit_start % 8;
        const auto array_from_bitcopy = bitcopy<BitSize>(byte_starting, bit_starting, bit_copy, data);
        ret.resize(bit_copy / 8 + (bit_copy % 8 == 0 ? 0 : 1));
        std::memcpy(ret.data(), array_from_bitcopy.data(), ret.size() * sizeof(uint8_t));
        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bool
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    overflow(
        const uint16_t data,
        const uint8_t compute_bit_sz)
    {
        // first, compute compliment
        const uint16_t compliment = COMPUTE_8_BIT_COMPLIMENT(compute_bit_sz);
        // second, see if data overflow
        return ((data & (~compliment)) > 0) || (data > (1 << BitSize) - 1);
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    bitwise_numeric(bitwise_numeric &&self) noexcept
    {
        if (lm_checksum_ != self.lm_checksum_) {
            return;
        }

        this->data = self.data;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    bitwise_numeric(const bitwise_numeric &self)
    {
        if (lm_checksum_ != self.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        memcpy(this->data.data(), self.data.data(), this->data.size() * sizeof(byte));
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator+(const bitwise_numeric & other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        bitwise_numeric <BitSize> ret;
        ret.data = this->data;

        int carry = 0;
        for (unsigned i = 0; i < ret.data.size(); i++)
        {
            uint16_t byte_result =
                static_cast<uint16_t>(other.data[i].num) +
                static_cast<uint16_t>(ret.data[i].num) + carry;
            carry = overflow(byte_result, ret.data[i].bit) ? 1 : 0;
            ret.data[i].num = byte_result & 0xFF;
        }

        ret.overflow_ = carry;
        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator-(const bitwise_numeric & other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        bitwise_numeric <BitSize> ret;
        ret.data = this->data;

        int carry = 0;
        for (unsigned i = 0; i < ret.data.size(); i++)
        {
            uint16_t byte_result =
                static_cast<uint16_t>(ret.data[i].num) -
                static_cast<uint16_t>(other.data[i].num) - carry;
            carry = overflow(byte_result, ret.data[i].bit) ? 1 : 0;
            ret.data[i].num = byte_result & 0xFF;
        }

        ret.overflow_ = carry;
        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bool bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator==(const bitwise_numeric & other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }


        for (unsigned i = 0; i < RequiredByteBlocks; i++)
        {
            if ((this->data[i].num & COMPUTE_8_BIT_COMPLIMENT(this->data[i].bit)) !=
                (other.data[i].num & COMPUTE_8_BIT_COMPLIMENT(other.data[i].bit)))
            {
                return false;
            }
        }

        return true;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bool
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator!=(const bitwise_numeric & other) const
    {
        return !this->operator==(other);
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator^(const bitwise_numeric & other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        bitwise_numeric <BitSize> ret;
        ret.data = this->data;

        for (unsigned i = 0; i < this->data.size(); i++)
        {
            const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
            ret.data[i].num ^= (other.data[i].num & complement);
            ret.data[i].num &= complement;
        }

        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator&(const bitwise_numeric & other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        bitwise_numeric <BitSize> ret;
        ret.data = this->data;

        for (unsigned i = 0; i < this->data.size(); i++)
        {
            const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
            ret.data[i].num &= (other.data[i].num & complement);
        }

        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::operator~() const
    {
        bitwise_numeric <BitSize> ret;
        ret.data = this->data;

        for (unsigned i = 0; i < this->data.size(); i++)
        {
            const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
            ret.data[i].num = ~ret.data[i].num;
            ret.data[i].num &= complement;
        }

        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits> &
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator=(bitwise_numeric && other) noexcept
    {
        if (lm_checksum_ != other.lm_checksum_) {
            return *this;
        }

        this->data = other.data;
        return *this;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>&
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator =(const bitwise_numeric& other)
    {
        if (lm_checksum_ != other.lm_checksum_) {
            return *this;
        }

        this->data = other.data;
        return *this;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bool
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator <(const bitwise_numeric& other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        if (this->operator==(other))
        {
            return false;
        }

        for (int64_t i = RequiredByteBlocks - 1; i >= 0; --i)
        {
            const auto other_8_bit = other.data[i].num & COMPUTE_8_BIT_COMPLIMENT(other.data[i].bit);
            const auto my_8_bit = this->data[i].num & COMPUTE_8_BIT_COMPLIMENT(this->data[i].bit);
            if (my_8_bit > other_8_bit) {
                return false;
            } else if (my_8_bit < other_8_bit) {
                return true;
            } else {
                continue;
            }
        }

        return true;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bool
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator >(const bitwise_numeric& other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        if (this->operator==(other))
        {
            return false;
        }

        for (int64_t i = RequiredByteBlocks - 1; i >= 0; --i)
        {
            const auto other_8_bit = other.data[i].num & COMPUTE_8_BIT_COMPLIMENT(other.data[i].bit);
            const auto my_8_bit = this->data[i].num & COMPUTE_8_BIT_COMPLIMENT(this->data[i].bit);
            if (my_8_bit > other_8_bit) {
                return true;
            } else if (my_8_bit < other_8_bit) {
                return false;
            } else {
                continue;
            }
        }

        return false;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator|(const bitwise_numeric & other) const
    {
        if (lm_checksum_ != other.lm_checksum_) {
            throw std::runtime_error("Operation between two bitwise-unequal numbers");
        }

        bitwise_numeric <BitSize> ret;
        ret.data = this->data;

        for (unsigned i = 0; i < this->data.size(); i++)
        {
            const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
            ret.data[i].num |= (other.data[i].num & complement);
            ret.data[i].num &= complement;
        }

        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    template < unsigned_integral Numeric >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator <<(const Numeric shifting_bits) const
    {
        bitwise_numeric reference;
        reference.data = this->data;
        auto shift_left_1_bit = [&]()
        {
            bool last_carry = false;
            bool least_significant_byte = true;
            for (unsigned i = 0; i < RequiredByteBlocks; i++)
            {
                const bool carry = (reference.data[i].num >> (reference.data[i].bit - 1)) != 0;
                reference.data[i].num <<= 1; // shift
                reference.data[i].num &= COMPUTE_8_BIT_COMPLIMENT(reference.data[i].bit); // clear unwanted bits

                // carry over
                if (!least_significant_byte)
                {
                    // MSB carry over
                    reference.data[i].num |= last_carry ? 0x01 : 0x00;
                } else {
                    least_significant_byte = false;
                }

                // update carry flag
                last_carry = carry;
            }
        };

        for (Numeric i = 0; i < shifting_bits; ++i)
        {
            shift_left_1_bit();
        }

        return reference;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    template < unsigned_integral Numeric >
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
    bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    operator >>(const Numeric shifting_bits) const
    {
        bitwise_numeric reference;
        reference.data = this->data;
        auto shift_right_1_bit = [&]()
        {
            bool last_carry = false;
            bool most_significant_byte = true;
            for (int64_t i = RequiredByteBlocks - 1; i >= 0; --i)
            {
                const bool carry = reference.data[i].num & 0x01;
                reference.data[i].num >>= 1; // shift

                // carry over
                if (!most_significant_byte)
                {
                    // LSB carry over
                    reference.data[i].num |= last_carry << (reference.data[i].bit - 1); // shift to MSB then add in
                } else {
                    most_significant_byte = false;
                }

                // update carry flag
                last_carry = carry;
            }
        };

        for (Numeric i = 0; i < shifting_bits; ++i)
        {
            shift_right_1_bit();
        }

        return reference;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits > &
    bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >::
    operator ++()
    {
        *this = this->operator+(make_bitwise_numeric_loosely(1));
        return *this;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >&
    bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >::
    operator --()
    {
        *this = this->operator-(make_bitwise_numeric_loosely(1));
        return *this;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    template < unsigned_integral Numeric >

    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >

    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    make_bitwise_numeric(const Numeric number)
    {
        const unsigned tailing_compliment = COMPUTE_8_BIT_COMPLIMENT(AdditionalTailingBits);
        if (RequiredByteBlocks < 1) {
            throw std::runtime_error("Required byte blocks must be greater than 0");
        }
        bitwise_numeric < BitSize > ret;

        // init
        for (unsigned i = 0; i < RequiredByteBlocks; i++)
        {
            ret.data[i].bit = 8;
            ret.data[i].num = 0;
        }

        // copy over
        for (unsigned i = 0; i < std::min(static_cast<uint64_t>(sizeof(Numeric)),
            static_cast<uint64_t>(RequiredByteBlocks)); i++)
        {
            ret.data[i].num = ((uint8_t*)&number)[i];
            ret.data[i].bit = 8;
        }

        // tailing
        if (AdditionalTailingBits != 0) {
            ret.data[RequiredByteBlocks - 1].bit = AdditionalTailingBits;
            ret.data[RequiredByteBlocks - 1].num &= tailing_compliment;
        }
        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >

    const

    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >

    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::max_num()
    {
        bitwise_numeric ret = make_bitwise_numeric_loosely(1);

        for (unsigned i = 0; i < BitSize; i++)
        {
            ret = ret << 1u;
            auto bit1 = make_bitwise_numeric_loosely(1);
            ret = ret | bit1;
        }

        return ret;
    }

#define HEX_MIN(a, b) ((a) < (b) ? (a) : (b))

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    constexpr bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >
    bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
    static_hash()
    {
        return make_bitwise_numeric_loosely(
            ((CurrentBitSize << (CurrentBitSize / 2)) | (CurrentBitSize ^ RequiredByteBlocks))
            ^ (AdditionalTailingBits << HEX_MIN(CurrentBitSize - AdditionalTailingBits, 3))
        );
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    [[nodiscard]] bool
    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    operator >=(const bitwise_numeric& other) const
    {
        return operator>(other) || operator==(other);
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    [[nodiscard]] bool
    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    operator <=(const bitwise_numeric& other) const
    {
        return operator<(other) || operator==(other);
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    [[nodiscard]]
    const
    typename bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::data_type &
    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
        dump() const
    {
        return this->data;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    [[nodiscard]] std::vector < uint8_t >
    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    dump_to_vector() const
    {
        std::vector < uint8_t > ret;
        ret.reserve(RequiredByteBlocks);
        for (unsigned i = 0; i < RequiredByteBlocks; i++)
        {
            ret.push_back(data[i].num);
        }

        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    template < typename NumericType >
    requires (std::is_integral_v < NumericType >)
    [[nodiscard]] NumericType
    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    export_numeric_force() const
    {
        NumericType ret { };
        for (unsigned i = 0; i < std::min(static_cast<uint64_t>(RequiredByteBlocks),
            static_cast<uint64_t>(sizeof(NumericType))); i++)
        {
            ((uint8_t*)(&ret))[i] = data[i].num & COMPUTE_8_BIT_COMPLIMENT(data[i].bit);
        }
        return ret;
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    template < typename NumericType >
    requires (std::is_integral_v < NumericType > && (sizeof(NumericType) * 8) >= BitSize)
    [[nodiscard]] NumericType
    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    export_numeric() const
    {
        return export_numeric_force<NumericType>();
    }

    template <
        const unsigned BitSize,
        const unsigned CurrentBitSize,
        const unsigned RequiredByteBlocks,
        const unsigned AdditionalTailingBits >
    template < typename Numeric >
    requires std::is_integral_v < Numeric >
    [[nodiscard]] bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >

    bitwise_numeric <
        BitSize,
        CurrentBitSize,
        RequiredByteBlocks,
        AdditionalTailingBits >::
    make_bitwise_numeric_loosely(Numeric numeric)
    {
        return make_bitwise_numeric(static_cast<std::make_unsigned_t<Numeric>>(numeric));
    }

    template<unsigned BitSize>
    std::vector<uint8_t> bitwise_numeric_stack<BitSize>::export_array() const
    {
        struct conjunct_result
        {
            uint8_t conjuncted;
            uint8_t conj_effective_bits;
            uint8_t left_over;
            uint8_t left_over_effective_bits;
        };

        auto conjunct_two_uint8_t = [&](
            const uint8_t previous,
            const uint8_t previous_effective_bits,
            const uint8_t current,
            const uint8_t current_effective_bits)
        ->conjunct_result
        {
            // compute tail size
            const uint8_t tail_size = std::min(
                static_cast<uint8_t>(8 - previous_effective_bits), current_effective_bits);
            // this is the bits in current that is going to be added to the previous, forming a bitwise conjunction
            const uint8_t tailing_current = current
                & COMPUTE_8_BIT_COMPLIMENT(tail_size)
                & COMPUTE_8_BIT_COMPLIMENT(current_effective_bits);
            // now, we add the tailing current to the previous, forming the conjunction we wanted
            const auto conjuncted = static_cast<uint8_t>((tailing_current << previous_effective_bits) | previous);
            // then, check for effective bits in conjunction
            const uint8_t conj_effective_bits = std::min(
                static_cast<uint8_t>(previous_effective_bits + current_effective_bits),
                static_cast<uint8_t>(8));
            // then, compute left_over byte
            // first, remove current's tailing bits
            uint8_t left_over = current
                & COMPUTE_8_BIT_COMPLIMENT(current_effective_bits)
                & (~COMPUTE_8_BIT_COMPLIMENT(tail_size));
            // second, shift left_over to the right, removing the tail
            left_over >>= tail_size;
            // third, compute left_over effective bits
            const uint8_t left_over_effective_bits = current_effective_bits - tail_size;

            return conjunct_result
                {   .conjuncted = conjuncted,
                    .conj_effective_bits = conj_effective_bits,
                    .left_over = left_over,
                    .left_over_effective_bits = left_over_effective_bits
                };
        };

        std::vector<uint8_t> ret{};
        conjunct_result result_holder {};
        bool processed_for_result_holder = false;
        for (const auto & numeric : stack_)
        {
            // result is empty?
            if (!processed_for_result_holder)
            {
                // push the first numeric element
                std::vector<uint8_t> first_element;
                for (const auto& element : numeric.dump())
                {
                    first_element.push_back(element.num);
                }

                result_holder.conj_effective_bits = numeric.dump().back().bit;
                result_holder.conjuncted = numeric.dump().back().num;

                ret.insert(ret.end(), first_element.begin(), first_element.end());
                ret.pop_back(); // remove last item since it's recorded in result_holder
                processed_for_result_holder = true;
            }
            else
            {
                // iterate through all bytes
                for (const auto& element : numeric.dump())
                {
                    // stitch the bytes
                    result_holder = conjunct_two_uint8_t(
                        result_holder.conjuncted,
                        result_holder.conj_effective_bits,
                        element.num,
                        element.bit);

                    // update info
                    if (result_holder.conj_effective_bits == 8)
                    {
                        ret.push_back(result_holder.conjuncted);
                        result_holder.conj_effective_bits = result_holder.left_over_effective_bits;
                        result_holder.conjuncted = result_holder.left_over;
                        result_holder.left_over = 0;
                        result_holder.left_over_effective_bits = 0;
                    }
                }
            }
        }

        // if there are something left in the result_holder, add it to the result
        if (result_holder.conj_effective_bits != 0)
        {
            ret.push_back(result_holder.conjuncted & COMPUTE_8_BIT_COMPLIMENT(result_holder.conj_effective_bits));
        }

        return ret;
    }


    template < unsigned BitSize, unsigned RequiredBytes >
    [[nodiscard]]
    std::array < uint8_t, RequiredBytes > bitcopy(
        const uint64_t byte_starting,
        const uint8_t bit_starting,
        const uint8_t bit_size,
        std::vector < uint8_t > & result,
        const std::vector < uint8_t > & data)
    {
        if (bit_starting > 8)
        {
            throw std::runtime_error("Bit starting must be less than 8");
        }

        const uint8_t first_copy_bits = std::min(static_cast<uint8_t>(8 - bit_starting), bit_size);
        const uint8_t continuous_bytes = (bit_size - first_copy_bits) / 8;
        const uint8_t last_copy_bits = (bit_size - first_copy_bits) % 8;
        std::vector < uint8_t > complete_bytes {};
        uint8_t fist_byte = 0, last_byte = 0;

        // copy fist byte
        fist_byte = data[byte_starting];
        fist_byte &= ~COMPUTE_8_BIT_COMPLIMENT(bit_starting);
        fist_byte >>= bit_starting;
        //unpacked[0] = fist_byte;

        // copy continuous bytes
        complete_bytes.reserve(continuous_bytes);
        for (uint8_t i = 0; i < continuous_bytes; i++)
        {
            complete_bytes.push_back(data[byte_starting + i + 1]);
        }

        // copy last byte
        if (last_copy_bits != 0)
        {
            last_byte = data[byte_starting + continuous_bytes + 1];
            last_byte &= COMPUTE_8_BIT_COMPLIMENT(last_copy_bits);
            //unpacked[unpacked.size() - 1] = last_byte;
        }

        // return the result
        result.reserve(utils::arithmetic::count_cell_with_cell_size(8, bit_size));

        // we need to stitch the unpacked bytes, logic is exactly like dump()
        struct conjunct_result
        {
            uint8_t conjuncted;
            uint8_t conj_effective_bits;
            uint8_t left_over;
            uint8_t left_over_effective_bits;
        };

        auto conjunct_two_uint8_t = [&](
            const uint8_t previous,
            const uint8_t previous_effective_bits,
            const uint8_t current,
            const uint8_t current_effective_bits)
            ->conjunct_result
        {
            // compute tail size
            const uint8_t tail_size = std::min(
                static_cast<uint8_t>(8 - previous_effective_bits), current_effective_bits);
            // this is the bits in current that is going to be added to the previous, forming a bitwise conjunction
            const uint8_t tailing_current = current
                & COMPUTE_8_BIT_COMPLIMENT(tail_size)
                & COMPUTE_8_BIT_COMPLIMENT(current_effective_bits);
            // now, we add the tailing current to the previous, forming the conjunction we wanted
            const auto conjuncted = static_cast<uint8_t>((tailing_current << previous_effective_bits) | previous);
            // then, check for effective bits in conjuncted
            const uint8_t conj_effective_bits = std::min(
                static_cast<uint8_t>(previous_effective_bits + current_effective_bits),
                static_cast<uint8_t>(8));
            // then, compute left_over byte
            // first, remove current's tailing bits
            uint8_t left_over = current
                & COMPUTE_8_BIT_COMPLIMENT(current_effective_bits)
                & (~COMPUTE_8_BIT_COMPLIMENT(tail_size));
            // second, shift left_over to the right, removing the tail
            left_over >>= tail_size;
            // third, compute left_over effective bits
            const uint8_t left_over_effective_bits = current_effective_bits - tail_size;

            return conjunct_result
            { .conjuncted = conjuncted,
                .conj_effective_bits = conj_effective_bits,
                .left_over = left_over,
                .left_over_effective_bits = left_over_effective_bits
            };
        };

        conjunct_result result_holder{};
        result_holder.conjuncted = fist_byte;
        result_holder.conj_effective_bits = first_copy_bits;
        auto iterate = [&](const uint8_t numeric, const uint8_t effective_bits)->void
        {
            // stitch the bytes
            result_holder = conjunct_two_uint8_t(
                result_holder.conjuncted,
                result_holder.conj_effective_bits,
                numeric,
                effective_bits);

            // update info
            if (result_holder.conj_effective_bits == 8)
            {
                result.push_back(result_holder.conjuncted);
                result_holder.conj_effective_bits = result_holder.left_over_effective_bits;
                result_holder.conjuncted = result_holder.left_over;
                result_holder.left_over = 0;
                result_holder.left_over_effective_bits = 0;
            }
        };

        // piece complete bytes
        for (const auto& numeric : complete_bytes /* unpacked only contains bits of complete bytes */)
        {
            iterate(numeric, 8);
        }

        // piece last tailing byte (if there is one)
        if (last_copy_bits != 0)
        {
            iterate(last_byte, last_copy_bits);
        }

        // if there are something left in the result_holder, add it to the result
        if (result_holder.conj_effective_bits != 0)
        {
            result.push_back(result_holder.conjuncted & COMPUTE_8_BIT_COMPLIMENT(result_holder.conj_effective_bits));
        }

        // verify
        std::array<uint8_t, BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1)> ret;
        if (result.size() != ret.size())
        {
            throw std::runtime_error("Error when copying bits, possibly a bug");
        }

        // copy result
        std::memcpy(ret.data(), result.data(), ret.size() * sizeof(uint8_t));

        return ret;
    }

    template<unsigned BitSize>
    void bitwise_numeric_stack<BitSize>::import_array(const std::vector<uint8_t>& data, const uint64_t expected_len)
    {
        // clear stack
        stack_.clear();
        stack_.reserve(expected_len);

        uint64_t byte_offset = 0;
        uint8_t bit_offset = 0;
        std::vector<uint8_t> bitcopy_buffer;

        auto add_offset = [&](const uint8_t off)->void {
            // add offset
            bit_offset += off;
            while (bit_offset >= 8)
            {
                byte_offset++;
                bit_offset -= 8;
            }
        };

        auto import_numeric = [&](const std::array<uint8_t, BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1)> & src)
        {
            // create a new numeric
            bitwise_numeric<BitSize> numeric;
            // copy over the data
            for (uint8_t i = 0; i < numeric.dump().size(); i++)
            {
                numeric.data[i].num = src[i];
                numeric.data[i].bit = 8;
            }
            numeric.data.back().bit = additional_tailing_bits;

            // push to stack
            stack_.push_back(numeric);
        };

        // import bits by bits
        for (uint64_t i = 0; i < expected_len; i++)
        {
            // copy bits over
            bitcopy_buffer.clear();
            auto result =
                bitcopy<BitSize>(
                    byte_offset,
                    bit_offset,
                    static_cast<uint8_t>(BitSize),
                    bitcopy_buffer, // used to decrease allocator cost
                    data);
            // increase offset
            add_offset(BitSize);

            // import result
            import_numeric(result);
        }
    }

    template<unsigned BitSize>
    void bitwise_numeric_stack<BitSize>::lazy_import_array(const std::vector<uint8_t> & data)
    {
        // packed data to original length
        const auto import_length = data.size() * 8 /* bits */ / BitSize;
        import_array(data, import_length);
    }
}

#endif // NUMERIC_INL
