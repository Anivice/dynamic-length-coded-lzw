#include "mmap.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils.h"
#include <iostream>

namespace lzw::basic_io
{
    mmap::~mmap() noexcept
    {
        try {
            close();
        }
        catch (std::exception & e) {
            std::cerr << e.what() << "\n";
        }
        catch (...) {
            std::cerr << "Cannot sync file\n";
        }
    }

    void mmap::open(const std::string& file, const bool read_only)
    {
        fd = ::open(file.c_str(), read_only ? O_RDONLY : O_RDWR);
        if (fd == -1) {
            throw error::BasicIOcannotOpenFile("invalid fd returned by ::open(\"" + file + "\", O_RDWR)");
        }

        struct stat st = { };
        if (fstat(fd, &st) == -1) {
            throw error::BasicIOcannotOpenFile("fstat failed for file " + file);
        }

        // Map the entire file into virtual address space
        data_ = static_cast<char*>(::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, read_only ? MAP_PRIVATE : MAP_SHARED, fd, 0));

        if (data_ == MAP_FAILED) {
            throw error::BasicIOcannotOpenFile("mmap failed for file " + file);
        }

        size_ = st.st_size;
        read_only_ = read_only;
    }

    void mmap::close()
    {
        if (data_ != MAP_FAILED)
        {
            sync();
            if (::munmap(data_, size_) != 0) {
                throw std::runtime_error("munmap failed");
            }
            ::close(fd);
            data_ = MAP_FAILED;
        }
    }

    void mmap::sync() const
    {
        if (read_only_) return;

        if (msync(data_, size_, MS_SYNC) != 0) {
            throw std::runtime_error("msync failed");
        }

        if (fsync(fd) != 0) {
            throw std::runtime_error("fsync failed");
        }
    }
}
