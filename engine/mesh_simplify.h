#pragma once
#include <glm/glm.hpp>


class MeshSimplifier {
    MeshSimplifier* impl;
public:
    MeshSimplifier() { impl = nullptr; }
    MeshSimplifier(glm::vec3* verts, std::uint32_t num_vert, std::uint32_t* indexes, std::uint32_t num_index);
    ~MeshSimplifier();

    void lock_position(glm::vec3 p);
    // bool is_position_locked(vec3 p);
    void simplify(std::uint32_t target_num_tri);
    std::uint32_t remaining_num_vert();
    std::uint32_t remaining_num_tri();
    float max_error();
};