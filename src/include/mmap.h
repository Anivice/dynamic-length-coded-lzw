#ifndef LZW_MMAP_H
#define LZW_MMAP_H

#include "error.h"
#include <sys/mman.h>

/// Cannot open file
make_simple_error_class(BasicIOcannotOpenFile);

namespace lzw::basic_io
{
    /// Map file to address
    class mmap
    {
        int fd = -1;
        void * data_ = MAP_FAILED;
        unsigned long long int size_ = 0;
    public:
        mmap() noexcept = default;

        mmap(const mmap &) = delete;
        mmap(mmap &&) = delete;
        mmap & operator=(const mmap &) = delete;
        mmap & operator=(mmap &&) = delete;

        /// open the file
        /// @param file path to file
        /// @throws lzw::error::BasicIOcannotOpenFile Cannot mmap file
        explicit mmap(const std::string & file) { open(file); }

        ~mmap() noexcept;

        /// open the file
        /// @param file path to file
        /// @throws lzw::error::BasicIOcannotOpenFile Cannot mmap file
        void open(const std::string & file);

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
