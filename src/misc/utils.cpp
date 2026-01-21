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

std::vector<std::string> lzw::utils::splitString(const std::string& s, const char delim)
{
    std::vector<std::string> parts;
    std::string token;
    std::stringstream ss(s);

    while (std::getline(ss, token, delim)) {
        parts.push_back(token);
    }

    return parts;
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

std::pair < const int, const int > lzw::utils::get_screen_row_col() noexcept
{
    constexpr int term_col_size = 80;
    constexpr int term_row_size = 25;
    const auto col_size_from_env = lzw::utils::getenv("COLUMNS");
    const auto row_size_from_env = lzw::utils::getenv("LINES");
    long col_env = -1;
    long row_env = -1;

    try
    {
        if (!col_size_from_env.empty() && !row_size_from_env.empty()) {
            col_env = std::strtol(col_size_from_env.c_str(), nullptr, 10);
            row_env = std::strtol(row_size_from_env.c_str(), nullptr, 10);
        }
    } catch (...) {
        col_env = -1;
        row_env = -1;
    }

    auto get_pair = [&]->std::pair < const int, const int >
    {
        if (col_env != -1 && row_env != -1) {
            return {row_env, col_env};
        }

        return {term_row_size, term_col_size};
    };

    bool is_terminal = false;
    struct stat st{};
    if (fstat(STDOUT_FILENO, &st) == -1) {
        return get_pair();
    }

    if (isatty(STDOUT_FILENO)) {
        is_terminal = true;
    }

    if (is_terminal)
    {
        winsize w{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != 0 || (w.ws_col | w.ws_row) == 0) {
            return get_pair();
        }

        return {w.ws_row, w.ws_col};
    }

    return get_pair();
}

uint64_t lzw::utils::get_timestamp() noexcept
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

timespec lzw::utils::get_timespec() noexcept
{
    timespec ts{};
    timespec_get(&ts, TIME_UTC);
    return ts;
}

void lzw::utils::print_table(const std::vector<std::pair<std::string, int>> &titles,
    const std::vector<std::vector<std::string>> &vales, const std::string &label)
{

    const auto [row, col] = lzw::utils::get_screen_row_col();
    const auto max_available_per_col = (col - (titles.size() + 1)) / titles.size();
    std::map < uint64_t, uint64_t > spaces;
    bool use_max_avail = true;
    auto find_max = [&](const auto & list, std::map < uint64_t, uint64_t > & spaces_)
    {
        lzw_assert_simple(list.size() == titles.size());
        for (auto i = 0ul; i < list.size(); i++) {
            if (list[i].length() > max_available_per_col) use_max_avail = false;
            if (spaces_[i] < list[i].length()) spaces_[i] = list[i].length();
        }
    };

    find_max(titles | std::views::keys, spaces);
    std::ranges::for_each(vales, [&](const std::vector<std::string> & value){ find_max(value, spaces); });
    if (use_max_avail) { std::ranges::for_each(spaces, [&](auto & value){ value.second = max_available_per_col; }); }

    const std::string separator(col, '=');
    {
        const std::string left((col - (label.length() + 2)) / 2, '=');
        const std::string right(col - left.length() - label.length() - 2, '=');
        std::cout << left << " " << label << " " << right << std::endl;
    }

    std::vector<std::string> on_screen_content;
    auto print = [&on_screen_content, &spaces](const auto & values, const auto & justification)
    {
        uint64_t index = 0;
        for (const auto & value : values)
        {
            std::ostringstream oss;
            const auto max_len = spaces[index];
            if (justification[index] == Center) // center
            {
                const auto left_len = std::max((max_len - value.length()) / 2, 0ul);
                const auto right_len = std::max(max_len - left_len - value.length(), 0ul);
                const std::string left(left_len, ' ');
                const std::string right(right_len, ' ');
                oss << left << value << right;
            }
            else if (justification[index] == Left) { // left
                constexpr auto left_len = 1;
                const auto right_len = std::max(static_cast<int>(max_len) - static_cast<int>(left_len) - static_cast<int>(value.length()), 0);
                const std::string left(left_len, ' ');
                const std::string right(right_len, ' ');
                oss << left << value << right;
            }
            else if (justification[index] == Right) { // right
                constexpr auto right_len = 1;
                const auto left_len = std::max(static_cast<int>(max_len) - static_cast<int>(right_len) - static_cast<int>(value.length()), 0);
                const std::string left(left_len, ' ');
                const std::string right(right_len, ' ');
                oss << left << value << right;
            }
            on_screen_content.push_back(oss.str());
            index++;
        }
    };

    auto show = [&]
    {
        int index = 0;
        std::ostringstream oss;
        std::ranges::for_each(on_screen_content, [&](const std::string & str) {
            oss << "|" << str;
            index++;
        });
        const auto before = oss.str().length();
        oss << std::string(std::max(static_cast<int>(col) - static_cast<int>(before) - 1, 0), ' ') << "|";
        std::cout << oss.str();
    };

    print(titles | std::views::keys, std::vector<int>(titles.size(), 1));
    show(); on_screen_content.clear();
    std::cout << std::endl;
    if (col - 2 > 0) std::cout << "+" << std::string(col - 2, '-') << "+" << std::endl;

    std::ranges::for_each(vales, [&](const std::vector<std::string> & value)
    {
        print(value, titles | std::views::values);
        show(); on_screen_content.clear();
        std::cout << std::endl;
    });

    std::cout << separator << std::endl;
}

std::string lzw::utils::value_to_human(
    const unsigned long long value,
    const std::string &lv1, const std::string &lv2,
    const std::string &lv3, const std::string &lv4)
{
    std::stringstream ss;
    if (value < 1024ull || value >= 1024ull * 1024ull * 1024ull * 1024ull * 1024ull) {
        ss << value << " " << lv1;
    } else if (value < 1024ull * 1024ull) {
        ss << std::fixed << std::setprecision(2) << (static_cast<double>(value) / 1024ull) << " " << lv2;
    } else if (value < 1024ull * 1024ull * 1024ull) {
        ss << std::fixed << std::setprecision(2) << (static_cast<double>(value) / (1024ull * 1024ull)) << " " << lv3;
    } else if (value < 1024ull * 1024ull * 1024ull * 1024ull) {
        ss << std::fixed << std::setprecision(2) << (static_cast<double>(value) / (1024ull * 1024ull * 1024ull)) << " " << lv4;
    }

    return ss.str();
}
uint64_t lzw::utils::arithmetic::count_cell_with_cell_size(const uint64_t cell_size, const uint64_t particles)
{
    assert_throw(cell_size != 0, "DIV/0");
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
