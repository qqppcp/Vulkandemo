#include "bit_array.h"
#include <memory>

BitArray::BitArray(std::uint32_t size) {
    bits = new std::uint32_t[(size + 31) / 32];
    memset(bits, 0, (size + 31) / 32 * sizeof(std::uint32_t));
}

void BitArray::resize(std::uint32_t size) {
    free();
    bits = new std::uint32_t[(size + 31) / 32];
    memset(bits, 0, (size + 31) / 32 * sizeof(std::uint32_t));
}