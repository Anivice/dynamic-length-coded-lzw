#ifndef LZW_GENERAL_ERROR_H
#define LZW_GENERAL_ERROR_H

#include <stdexcept>
#include <string>
#include <sstream>
#include "utils.h"

namespace lzw::error
{
    class generic_error : public std::exception
    {
    protected:
        std::string message;

    public:
        explicit generic_error(const std::string& msg, bool include_backtrace_msg = false);
        ~generic_error() override = default;
        [[nodiscard]] const char* what() const noexcept override { return message.c_str(); }
    };
}

#define make_simple_error_class(name)                                                           \
namespace lzw::error                                                                            \
{                                                                                               \
    class name final : public generic_error {                                                   \
    private:                                                                                    \
        template < typename Type >                                                              \
        void msg_conj(const Type & msg) { oss << msg; }                                         \
                                                                                                \
        std::ostringstream oss;                                                                 \
                                                                                                \
    public:                                                                                     \
        template <typename... Args>                                                             \
        explicit name(const Args& ...args) : generic_error("<INSERT ERROR MESSAGE HERE>")       \
        {                                                                                       \
            msg_conj(#name ": ");                                                               \
            (msg_conj(args), ...);                                                              \
            lzw::utils::replace_all(message, "<INSERT ERROR MESSAGE HERE>", oss.str());         \
        }                                                                                       \
        name() : generic_error(#name) { }                                                 \
        ~name() override = default;                                                             \
    };                                                                                          \
}

#define make_simple_error_class_traceable(name)                                                         \
namespace lzw::error                                                                                    \
{                                                                                                       \
    class name final : public generic_error {                                                     \
    private:                                                                                            \
        template < typename Type >                                                                      \
        void msg_conj(const Type & msg) { oss << msg; }                                                 \
                                                                                                        \
        std::ostringstream oss;                                                                         \
                                                                                                        \
    public:                                                                                             \
        template <typename... Args>                                                                     \
        explicit name(const Args& ...args) : generic_error("<INSERT ERROR MESSAGE HERE>", true)         \
        {                                                                                               \
            msg_conj(#name ": ");                                                                       \
            (msg_conj(args), ...);                                                                      \
            lzw::utils::replace_all(message, "<INSERT ERROR MESSAGE HERE>", oss.str());                 \
        }                                                                                               \
        name() : generic_error(#name, true) { }                                                         \
        ~name() override = default;                                                                     \
    };                                                                                                  \
}

make_simple_error_class_traceable(assertion_failed);
#define assert_throw(condition, msg)                                                                    \
    if (!(condition)) {                                                                                 \
        elog("Assertion failed: " #condition "` failed\n");                                             \
        throw lzw::error::assertion_failed("Assertion `" #condition "` failed: " msg);                  \
    }
#define lzw_assert_simple(condition) assert_throw(condition, "")

#endif //LZW_GENERAL_ERROR_H