#include "lzw6.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    std::vector<uint8_t> data = {
        'A', 'A', 'B', 'B', 'B', 'C', 'C', 'C', 'C', 'C', 'C', 'D', 'E', 'F', 'G', 'G'
    }, out;

    lzw::Huffman huffman(data, out);

    return 0;
}
