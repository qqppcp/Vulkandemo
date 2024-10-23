#include "heap.h"
#include <memory>
#include <cassert>

Heap::Heap() {
    heap = nullptr, keys = nullptr, heap_indexes = nullptr;
    heap_size = 0, num_index = 0;
}

Heap::Heap(std::uint32_t _num_index) {
    heap_size = 0;
    num_index = _num_index;
    heap = new std::uint32_t[num_index];
    keys = new float[num_index];
    heap_indexes = new std::uint32_t[num_index];
    memset(heap_indexes, 0xff, num_index * sizeof(std::uint32_t));
}

void Heap::resize(std::uint32_t _num_index) {
    free();
    heap_size = 0;
    num_index = _num_index;
    heap = new std::uint32_t[num_index];
    keys = new float[num_index];
    heap_indexes = new std::uint32_t[num_index];
    memset(heap_indexes, 0xff, num_index * sizeof(std::uint32_t));
}

void Heap::push_up(std::uint32_t i) {
    std::uint32_t idx = heap[i];
    std::uint32_t fa = (i - 1) >> 1;
    while (i > 0 && keys[idx] < keys[heap[fa]]) {
        heap[i] = heap[fa];
        heap_indexes[heap[i]] = i;
        i = fa, fa = (i - 1) >> 1;
    }
    heap[i] = idx;
    heap_indexes[heap[i]] = i;
}

void Heap::push_down(std::uint32_t i) {
    std::uint32_t idx = heap[i];
    std::uint32_t ls = (i << 1) + 1;
    std::uint32_t rs = ls + 1;
    while (ls < heap_size) {
        std::uint32_t t = ls;
        if (rs < heap_size && keys[heap[rs]] < keys[heap[ls]])
            t = rs;
        if (keys[heap[t]] < keys[idx]) {
            heap[i] = heap[t];
            heap_indexes[heap[i]] = i;
            i = t, ls = (i << 1) + 1, rs = ls + 1;
        }
        else break;
    }
    heap[i] = idx;
    heap_indexes[heap[i]] = i;
}

void Heap::clear() {
    heap_size = 0;
    memset(heap_indexes, 0xff, num_index * sizeof(std::uint32_t));
}

std::uint32_t Heap::top() {
    assert(heap_size > 0);
    return heap[0];
}

void Heap::pop() {
    assert(heap_size > 0);

    std::uint32_t idx = heap[0];
    heap[0] = heap[--heap_size];
    heap_indexes[heap[0]] = 0;
    heap_indexes[idx] = ~0u;
    push_down(0);
}

void Heap::add(float key, std::uint32_t idx) {
    assert(!is_present(idx));

    std::uint32_t i = heap_size++;
    heap[i] = idx;
    keys[idx] = key;
    heap_indexes[idx] = i;
    push_up(i);
}

void Heap::update(float key, std::uint32_t idx) {
    keys[idx] = key;
    std::uint32_t i = heap_indexes[idx];
    if (i > 0 && key < keys[heap[(i - 1) >> 1]]) push_up(i);
    else push_down(i);
}

void Heap::remove(std::uint32_t idx) {
    //if(!is_present(idx)) return;
    assert(is_present(idx));

    float key = keys[idx];
    std::uint32_t i = heap_indexes[idx];

    if (i == heap_size - 1) {
        --heap_size;
        heap_indexes[idx] = ~0u;
        return;
    }

    heap[i] = heap[--heap_size];
    heap_indexes[heap[i]] = i;
    heap_indexes[idx] = ~0u;
    if (key < keys[heap[i]]) push_down(i);
    else push_up(i);
}

float Heap::get_key(std::uint32_t idx) {
    return keys[idx];
}