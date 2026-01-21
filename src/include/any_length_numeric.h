#ifndef LZW_ANY_LENGTH_NUMERIC_H
#define LZW_ANY_LENGTH_NUMERIC_H

#include <cmath>
#include <cstdint>
#include <concepts>
#include <cmath>
#include <stdexcept>
#include <vector>
#include "error.h"

namespace lzw {
    constexpr uint8_t reverse_bits(const uint8_t x)
    {
        constexpr uint8_t lookup[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
        return (lookup[x & 0x0F] << 4) | lookup[x >> 4];
    }

    constexpr uint16_t reverse_bits(const uint16_t x)
    {
        return static_cast<uint16_t>(reverse_bits(static_cast<uint8_t>(x))) << 8 | static_cast<uint16_t>(reverse_bits(static_cast<uint8_t>(x >> 8)));
    }

    constexpr uint32_t reverse_bits(const uint32_t x)
    {
        return static_cast<uint32_t>(reverse_bits(static_cast<uint16_t>(x))) << 16 | static_cast<uint32_t>(reverse_bits(static_cast<uint16_t>(x >> 16)));
    }

    constexpr uint64_t reverse_bits(const uint64_t x)
    {
        return static_cast<uint64_t>(reverse_bits(static_cast<uint32_t>(x))) << 32 | static_cast<uint64_t>(reverse_bits(static_cast<uint32_t>(x >> 32)));
    }

    constexpr uint64_t max_representable_bit_len = 63;

    template < unsigned MaxBitLength > requires (MaxBitLength <= max_representable_bit_len)
    constexpr uint64_t mask()
    {
        uint64_t result = 0;
        for (uint64_t i = 0; i < MaxBitLength; ++i) {
            result <<= 1;
            result |= 0x01;
        }

        return result;
    }

    constexpr uint64_t celdiv(const uint64_t a, const uint64_t b) {
        return (a / b) + (a % b == 0 ? 0 : 1);
    }

    template < unsigned MaxBitLength, const unsigned BitMask = mask<MaxBitLength>() > requires (MaxBitLength <= max_representable_bit_len)
    class any_length_numeric {
        uint64_t data_ { };

    public:
        template < typename Type > requires (std::is_integral_v<Type> && (sizeof (Type) >= celdiv(MaxBitLength, 8)))
        explicit operator Type() const {
            return static_cast<Type>(data_);
        }

        any_length_numeric() = default;

        template < typename Type > requires std::is_integral_v<Type>
        any_length_numeric(const Type data) : data_(data) { data_ &= BitMask; }

        // arithmatic
        any_length_numeric operator +(const any_length_numeric & other) {
            return any_length_numeric(data_ + other.data_);
        }

        any_length_numeric operator -(const any_length_numeric & other) {
            return any_length_numeric(data_ - other.data_);
        }

        any_length_numeric operator *(const any_length_numeric & other) {
            return any_length_numeric(data_ * other.data_);
        }

        any_length_numeric operator /(const any_length_numeric & other) {
            return any_length_numeric(data_ / other.data_);
        }

        any_length_numeric operator %(const any_length_numeric & other) {
            return any_length_numeric(data_ % other.data_);
        }

        any_length_numeric & operator +=(const any_length_numeric & other) {
            data_ += other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator -=(const any_length_numeric & other) {
            data_ -= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator *=(const any_length_numeric & other) {
            data_ *= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator /=(const any_length_numeric & other) {
            data_ /= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator %=(const any_length_numeric & other) {
            data_ %= other.data_;
            data_ &= BitMask;
            return *this;
        }

        // bitwise
        any_length_numeric operator &(const any_length_numeric & other) {
            return any_length_numeric(data_ & other.data_ & BitMask);
        }

        any_length_numeric operator |(const any_length_numeric & other) {
            return any_length_numeric((data_ | other.data_) & BitMask);
        }

        any_length_numeric operator ^(const any_length_numeric & other) {
            return any_length_numeric((data_ ^ other.data_) & BitMask);
        }

        any_length_numeric operator <<(const any_length_numeric & other) {
            return any_length_numeric((data_ << other.data_) & BitMask);
        }

        any_length_numeric operator >>(const any_length_numeric & other) {
            return any_length_numeric((data_ >> other.data_) & BitMask);
        }

        any_length_numeric & operator &=(const any_length_numeric & other) {
            data_ &= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator |=(const any_length_numeric & other) {
            data_ |= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator ^=(const any_length_numeric & other) {
            data_ ^= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator >>=(const any_length_numeric & other) {
            data_ >>= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator <<=(const any_length_numeric & other) {
            data_ <<= other.data_;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator=(const any_length_numeric & other) {
            data_ = other.data_;
            data_ &= BitMask;
            return *this;
        }

        bool operator > (const any_length_numeric & other) const {
            return data_ > other.data_;
        }

        bool operator < (const any_length_numeric & other) const {
            return data_ < other.data_;
        }

        bool operator == (const any_length_numeric & other) const {
            return data_ == other.data_;
        }

        bool operator != (const any_length_numeric & other) const {
            return data_ != other.data_;
        }

        bool operator >= (const any_length_numeric & other) const {
            return data_ >= other.data_;
        }

        bool operator <= (const any_length_numeric & other) const {
            return data_ <= other.data_;
        }

        any_length_numeric & operator++() {
            data_++;
            data_ &= BitMask;
            return *this;
        }

        any_length_numeric & operator--() {
            data_++;
            data_ &= BitMask;
            return *this;
        }

        template < typename Type > requires std::is_integral_v<Type>
        any_length_numeric & operator=(Type other) {
            data_ = other;
            data_ &= BitMask;
            return *this;
        }

        template < typename Type > requires std::is_integral_v<Type>
        static any_length_numeric < MaxBitLength > make_any(const Type n) {
            return any_length_numeric < MaxBitLength > (n);
        }
    };

    template < unsigned MaxBitLength > requires (MaxBitLength <= max_representable_bit_len)
    std::pair < std::vector < uint8_t >, uint64_t > pack_numeric(const std::vector<any_length_numeric<MaxBitLength>> & list)
    {
        enum log2types : int { CELL_01_is_1, CELL_0_is_0_and_1_is_1, CELL_01_is_0 };
        auto log2_cell = [](const uint64_t data, const int log2_type)->uint64_t
        {
            if (data <= 1) {
                switch (log2_type) {
                    case CELL_01_is_1:
                        return 1;
                    case CELL_0_is_0_and_1_is_1:
                        return data;
                    case CELL_01_is_0:
                        return 0;
                    default:
                        lzw_assert_simple(false);
                }
            }

            const double r = log2(static_cast<double>(data) + 1);
            const auto int_ = static_cast<uint64_t>(r);
            const auto tail_ = r - static_cast<double>(int_);
            return int_ + (tail_ == 0 ? 0 : 1);
        };

        struct push_bit_frame_t {
            uint64_t last_write_pos_in_last_byte = 0;
        };

        auto push_bit = [](std::vector < uint8_t > & bit_list, const bool bit, push_bit_frame_t & frame)
        {
            if (bit_list.empty()) {
                bit_list.push_back(bit);
                frame.last_write_pos_in_last_byte = 0;
                return;
            }

            if (frame.last_write_pos_in_last_byte < 7) {
                bit_list.back() <<= 1;
                bit_list.back() |= bit;
                ++frame.last_write_pos_in_last_byte;
            } else {
                bit_list.back() = reverse_bits(bit_list.back());
                bit_list.push_back(bit);
                frame.last_write_pos_in_last_byte = 0;
            }
        };

        auto finish_push = [](std::vector < uint8_t > & bit_list, push_bit_frame_t & frame)
        {
            if (frame.last_write_pos_in_last_byte < 7) {
                bit_list.back() <<= (7 - frame.last_write_pos_in_last_byte);
                bit_list.back() = reverse_bits(bit_list.back());
            } else {
                bit_list.back() = reverse_bits(bit_list.back());
            }
        };

        constexpr bool plain_bit = false;
        constexpr bool compressed_bit = true;

        std::vector < uint8_t > result;
        push_bit_frame_t push_frame { };

        auto push_data = [&](const int64_t data_len, const uint64_t data) {
            for (int64_t i = data_len - 1; i >= 0; --i) {
                push_bit(result, (data >> i) & 0x01, push_frame);
            }
        };

        // calculate average
        long double mean_val = 0;
        for (const auto & numeric_ : list) {
            const auto numeric = static_cast<uint64_t>(numeric_);
            const uint64_t NBits = log2_cell(numeric, CELL_01_is_1);
            const uint64_t size_of_N = log2_cell(NBits, CELL_01_is_0);
            mean_val += size_of_N;
        }

        mean_val /= list.size();

        long double std_val = 0;
        for (const auto & numeric_ : list) {
            const auto numeric = static_cast<uint64_t>(numeric_);
            const uint64_t NBits = log2_cell(numeric, CELL_01_is_1);
            const uint64_t size_of_N = log2_cell(NBits, CELL_01_is_0);
            std_val += (size_of_N - mean_val) * (size_of_N - mean_val);
        }

        std_val /= list.size();
        std_val = sqrt(static_cast<double>(std_val));
        const auto threshold_upper = mean_val + 3 * std_val;
        const auto threshold_lower = std::max(mean_val - 3 * std_val, static_cast<long double>(0.00));

        long double normalized_position_average_len_d = 0;
        long long int count = 0;
        for (const auto & numeric_ : list)
        {
            const auto numeric = static_cast<uint64_t>(numeric_);
            const uint64_t NBits = log2_cell(numeric, CELL_01_is_1);
            const uint64_t size_of_N = log2_cell(NBits, CELL_01_is_0);
            if (size_of_N >= threshold_lower && size_of_N <= threshold_upper) {
                normalized_position_average_len_d += size_of_N;
                count++;
            }
        }

        normalized_position_average_len_d /= count;
        auto normalized_position_average_len = static_cast<uint64_t>(normalized_position_average_len_d);
        normalized_position_average_len += normalized_position_average_len_d - normalized_position_average_len == 0 ? 0 : 1;

        for (const auto & numeric_ : list)
        {
            const auto numeric = static_cast<uint64_t>(numeric_);
            /// [FLIP BIT] [N] [N Bits]
            const uint64_t NBits = log2_cell(numeric, CELL_01_is_1);
            const uint64_t size_of_N = log2_cell(NBits, CELL_01_is_0);
            const uint64_t compression_encoded_length = (normalized_position_average_len + NBits + 1 /* Flip bit */);
            if (compression_encoded_length > (MaxBitLength + 1) || size_of_N > normalized_position_average_len) {
                push_bit(result, plain_bit, push_frame);
                push_data(MaxBitLength, numeric);
            } else {
                push_bit(result, compressed_bit, push_frame); // flip bit
                push_data(normalized_position_average_len, NBits);
                push_data(NBits, numeric); // numeric
            }
        }

        finish_push(result, push_frame);
        return { result, normalized_position_average_len };
    }

    template < unsigned MaxBitLength > requires (MaxBitLength <= max_representable_bit_len)
    std::vector < any_length_numeric<MaxBitLength> > unpack_numeric(const std::vector<uint8_t> & list, const uint64_t bit_position_normalized_len)
    {
        std::vector < any_length_numeric<MaxBitLength> > result;

        struct pop_bit_frame_t {
            uint64_t r_pos = 0;
        };

        auto pop_bit = [](const std::vector<uint8_t> & bit_list, pop_bit_frame_t & frame)->bool
        {
            if (frame.r_pos >= bit_list.size() * 8) {
                throw std::out_of_range("pop_bit_frame_t::r_pos >= bit_list.size() * 8");
            }

            const uint64_t byte_pos = frame.r_pos / 8;
            const uint64_t bit_pos = frame.r_pos % 8;
            const uint8_t byte = bit_list[byte_pos];
            ++frame.r_pos;
            const auto pop_result = (byte >> bit_pos) & 0x01;
            return pop_result;
        };

        auto pop_bit_val = [&pop_bit](const std::vector<uint8_t> & bit_list, pop_bit_frame_t & frame, const uint64_t len)->uint64_t
        {
            uint64_t val = 0;
            for (uint64_t i = 0; i < len; ++i)
            {
                const auto bit = pop_bit(bit_list, frame);
                val |= bit;
                val <<= 1;
            }

            val >>= 1;
            return val;
        };

        pop_bit_frame_t pop_frame = { };
        while (true)
        {
            try {
                if (const auto bit = pop_bit(list, pop_frame); bit) { // control bit
                    // true, compressed bit
                    // format: [FLIP BIT (Already popped)] [N (len=bit_position_normalized_len)] [N Bits]
                    const uint64_t N = pop_bit_val(list, pop_frame, bit_position_normalized_len);
                    const uint64_t val = pop_bit_val(list, pop_frame, N);
                    result.push_back(val);
                }
                else {
                    // control bit off, no compression
                    result.push_back(pop_bit_val(list, pop_frame, MaxBitLength));
                }
            } catch (const std::out_of_range &) {
                break;
            }
        }

        return result;
    }
}

#endif //LZW_ANY_LENGTH_NUMERIC_H
