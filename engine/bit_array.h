#pragma once
#include <cstdint>

class BitArray {
    std::uint32_t* bits;
public:
    BitArray() { bits = nullptr; }
    BitArray(std::uint32_t size);
    ~BitArray() { free(); }
    void resize(std::uint32_t size);
    void free() {
        if (bits) delete[] bits;
    }
    void set_false(std::uint32_t idx) {
        std::uint32_t x = idx >> 5;
        std::uint32_t y = idx & 31;
        bits[x] &= ~(1 << y);
    }
    void set_true(std::uint32_t idx) {
        std::uint32_t x = idx >> 5;
        std::uint32_t y = idx & 31;
        bits[x] |= (1 << y);
    }
    bool operator[](std::uint32_t idx) {
        std::uint32_t x = idx >> 5;
        std::uint32_t y = idx & 31;
        return (bool)(bits[x] >> y & 1);
    }
};