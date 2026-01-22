#include "lzw6.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    int fd = ::open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat st = { };
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return 1;
    }

    // Map the entire file into virtual address space
    const char * data_ = static_cast<char*>(::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    if (data_ == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    std::vector<uint8_t> data(data_, data_ + st.st_size), out, in;
    lzw::lzw<12> Compressor(data, in);
    lzw::lzw<12> Decompressor(in, out);

    Compressor.compress();
    Decompressor.decompress();

    int ofd = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (ofd == -1) {
        perror("open");
        return 1;
    }

    write(ofd, in.data(), in.size());
    return 0;
}
