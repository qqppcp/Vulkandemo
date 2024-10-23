#pragma once
#include <cstdint>
class Heap {
    std::uint32_t heap_size;
    std::uint32_t num_index;
    std::uint32_t* heap;
    float* keys;
    std::uint32_t* heap_indexes;

    void push_up(std::uint32_t i);
    void push_down(std::uint32_t i);
public:
    Heap();
    Heap(std::uint32_t _num_index);
    ~Heap() { free(); }

    void free() {
        heap_size = 0, num_index = 0;
        delete[] heap;
        delete[] keys;
        delete[] heap_indexes;
        heap = nullptr, keys = nullptr, heap_indexes = nullptr;
    }
    void resize(std::uint32_t _num_index);

    float get_key(std::uint32_t idx);
    void clear();
    bool empty() { return heap_size == 0; }
    bool is_present(std::uint32_t idx) { return heap_indexes[idx] != ~0u; }
    std::uint32_t top();
    void pop();
    void add(float key, std::uint32_t idx);
    void update(float key, std::uint32_t idx);
    void remove(std::uint32_t idx);
};