#include "lzw6.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    std::vector<uint8_t> data, out, out2;
    const int fd = open(argv[1], O_RDONLY);
    struct stat st { };
    fstat(fd, &st);

    data.resize(st.st_size);
    (void)read(fd, data.data(), st.st_size);

    lzw::Huffman huffman(data, out);
    huffman.compress();

    lzw::Huffman huffman2(out, out2);
    huffman2.decompress();
    return 0;
}
