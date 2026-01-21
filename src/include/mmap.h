#ifndef LZW_MMAP_H
#define LZW_MMAP_H

#include <stdexcept>

#include "error.h"
#include <sys/mman.h>

/// Cannot open file
namespace lzw::error {
    class BasicIOcannotOpenFile : std::runtime_error {
        public:
        explicit BasicIOcannotOpenFile(const std::string & msg) : std::runtime_error{msg} {}
    };
}

namespace lzw::basic_io
{
    /// Map file to address
    class mmap
    {
        int fd = -1;
        void * data_ = MAP_FAILED;
        unsigned long long int size_ = 0;
        bool read_only_ = false;

    public:
        mmap() noexcept = default;

        mmap(const mmap &) = delete;
        mmap(mmap &&) = delete;
        mmap & operator=(const mmap &) = delete;
        mmap & operator=(mmap &&) = delete;

        /// open the file
        /// @param file path to file
        /// @param read_only If file is should be opened as read-only
        /// @throws lzw::error::BasicIOcannotOpenFile Cannot mmap file
        explicit mmap(const std::string & file, const bool read_only = false) { open(file, read_only); }

        ~mmap() noexcept;

        /// open the file
        /// @param file path to file
        /// @param read_only If file is should be opened as read-only
        /// @throws lzw::error::BasicIOcannotOpenFile Cannot mmap file
        void open(const std::string & file, bool read_only = false);

        /// close the file
        /// @throws lzw::error::assertion_failed Can't sync or unmap
        void close();

        /// Get mapped file array
        /// @return File array
        [[nodiscard]] char * data() const noexcept { return (char*)data_; }

        /// Get array size
        /// @return array size
        [[nodiscard]] unsigned long long int size() const noexcept { return size_; }

        /// Get file descriptor for this disk file
        /// @return file descriptor for this disk file
        [[nodiscard]] int get_fd() const noexcept { return fd; }

        /// sync data
        /// @throws lzw::error::assertion_failed Can't sync or unmap
        void sync() const;
    };
}

#endif //LZW_MMAP_H
