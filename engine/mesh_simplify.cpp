#include "mesh_simplify.h"
#include "bit_array.h"
#include "hash_table.h"
#include "heap.h"
#include "mesh_util.h"
#include <vector>
#include <algorithm>

using namespace std;
using namespace glm;

struct Quadric {
    double a2, b2, c2, d2;
    double ab, ac, ad;
    double bc, bd, cd;
    Quadric() { memset(this, 0, sizeof(double) * 10); }
    Quadric(dvec3 p0, dvec3 p1, dvec3 p2) {
        dvec3 n = normalize(cross(p1 - p0, p2 - p0));
        auto a = n.x, b = n.y, c = n.z;
        double d = -dot(n, p0);
        a2 = a * a, b2 = b * b, c2 = c * c, d2 = d * d;
        ab = a * b, ac = a * c, ad = a * d;
        bc = b * c, bd = b * d, cd = c * d;
    }
    void add(Quadric b) {
        double* t1 = (double*)this;
        double* t2 = (double*)&b;
        for (std::uint32_t i = 0; i < 10; i++) t1[i] += t2[i];
    }
    bool get(vec3& p) {
        dmat4 m, inv;
        m[0] = glm::dvec4(a2, ab, ac, 0);
        m[1] = glm::dvec4(ab, b2, bc, 0);
        m[2] = glm::dvec4(ac, bc, c2, 0);
        m[3] = glm::dvec4(ad, bd, cd, 1);
        inv = inverse(m);
        dvec4 v = inv[3];
        p = { (float)v.x,(float)v.y,(float)v.z };
        return true;
    }
    float evaluate(vec3 p) {
        float res = a2 * p.x * p.x + 2 * ab * p.x * p.y + 2 * ac * p.x * p.z + 2 * ad * p.x
            + b2 * p.y * p.y + 2 * bc * p.y * p.z + 2 * bd * p.y
            + c2 * p.z * p.z + 2 * cd * p.z + d2;
        return res <= 0.f ? 0.f : res;
    }
};

class MeshSimplifierImpl final : public MeshSimplifier {
public:
    std::uint32_t num_vert;
    std::uint32_t num_index;
    std::uint32_t num_tri;

    vec3* verts;
    std::uint32_t* indexes;

    HashTable vert_ht;
    HashTable corner_ht;
    vector<std::uint32_t> vert_refs;
    vector<u8> flags;
    BitArray tri_removed;

    enum flag {
        AdjMask = 1,
        LockMask = 2
    };

    vector<pair<vec3, vec3>> edges;
    HashTable edge0_ht;
    HashTable edge1_ht;
    Heap heap;

    vector<std::uint32_t> move_vert;
    vector<std::uint32_t> move_corner;
    vector<std::uint32_t> move_edge;
    vector<std::uint32_t> reevaluate_edge;

    vector<Quadric> tri_quadrics;

    float max_error;
    std::uint32_t remaining_num_vert;
    std::uint32_t remaining_num_tri;

    MeshSimplifierImpl(vec3* verts, std::uint32_t num_vert, std::uint32_t* indexes, std::uint32_t num_index);
    ~MeshSimplifierImpl() {}

    std::uint32_t hash(const vec3& v) {
        union { float f; std::uint32_t u; } x, y, z;
        x.f = (v.x == 0.f ? 0 : v.x);
        y.f = (v.y == 0.f ? 0 : v.y);
        z.f = (v.z == 0.f ? 0 : v.z);
        return murmur_mix(murmur_add(murmur_add(x.u, y.u), z.u));
    }
    void set_vert_idx(std::uint32_t corner, std::uint32_t idx);
    void remove_if_vert_duplicate(std::uint32_t corner);
    bool is_tri_duplicate(std::uint32_t tri_idx);
    void fixup_tri(std::uint32_t tri_idx);
    bool add_edge_ht(vec3& p0, vec3& p1, std::uint32_t idx);

    void calc_tri_quadric(std::uint32_t tri_idx);
    void gather_adj_tris(vec3 p, vector<std::uint32_t>& tris, bool& lock);
    float evaluate(vec3 p0, vec3 p1, bool merge);
    void lock_position(vec3 p);
    // bool is_position_locked(vec3 p);
    void simplify(std::uint32_t target_num_tri);
    void compact();

    void begin_merge(vec3 p);
    void end_merge();
};

MeshSimplifierImpl::MeshSimplifierImpl(vec3* _verts, std::uint32_t _num_vert, std::uint32_t* _indexes, std::uint32_t _num_index)
    :num_vert(_num_vert), num_index(_num_index), num_tri(num_index / 3)
    , verts(_verts), indexes(_indexes)
    , vert_ht(num_vert), vert_refs(num_vert), corner_ht(num_index)
    , tri_removed(num_tri), flags(num_index)
{
    remaining_num_vert = num_vert, remaining_num_tri = num_tri;
    for (std::uint32_t i = 0; i < num_vert; i++) {
        vert_ht.add(hash(verts[i]), i);
    }

    std::uint32_t exp_num_edge = std::min(std::min(num_index, 3 * num_vert - 6), num_tri + num_vert);
    edges.reserve(exp_num_edge);
    edge0_ht.resize(exp_num_edge);
    edge1_ht.resize(exp_num_edge);

    for (std::uint32_t corner = 0; corner < num_index; corner++) {
        std::uint32_t v_idx = indexes[corner];
        vert_refs[v_idx]++;
        const vec3& p = verts[v_idx];
        corner_ht.add(hash(p), corner);

        vec3 p0 = p;
        vec3 p1 = verts[indexes[cycle3(corner)]];
        if (add_edge_ht(p0, p1, edges.size())) {
            edges.push_back({ p0,p1 });
        }
    }
}

void MeshSimplifierImpl::lock_position(vec3 p) {
    for (std::uint32_t i : corner_ht[hash(p)]) {
        if (verts[indexes[i]] == p) {
            flags[i] |= LockMask;
        }
    }
}

// bool MeshSimplifierImpl::is_position_locked(vec3 p){
//     for(std::uint32_t i:corner_ht[hash(p)]){
//         if(verts[indexes[i]]==p){
//             if(flags[i]&LockMask) return true;
//             else return false;
//         }
//     }
//     return false;
// }

bool MeshSimplifierImpl::add_edge_ht(vec3& p0, vec3& p1, std::uint32_t idx) {
    std::uint32_t h0 = hash(p0), h1 = hash(p1);
    if (h0 > h1) swap(h0, h1), swap(p0, p1);
    for (std::uint32_t i : edge0_ht[h0]) {
        auto& e = edges[i];
        if (e.first == p0 && e.second == p1) return false;
    }
    edge0_ht.add(h0, idx);
    edge1_ht.add(h1, idx);
    return true;
}

void MeshSimplifierImpl::set_vert_idx(std::uint32_t corner, std::uint32_t idx) {
    std::uint32_t& v_idx = indexes[corner];
    assert(v_idx != ~0u);
    assert(vert_refs[v_idx] > 0);

    if (v_idx == idx) return;
    if (--vert_refs[v_idx] == 0) {
        vert_ht.remove(hash(verts[v_idx]), v_idx);
        remaining_num_vert--;
    }
    v_idx = idx;
    if (v_idx != ~0u) vert_refs[v_idx]++;
}

//将corner赋值为第一个遇到的相同点
void MeshSimplifierImpl::remove_if_vert_duplicate(std::uint32_t corner) {
    std::uint32_t v_idx = indexes[corner];
    vec3& v = verts[v_idx];
    for (std::uint32_t i : vert_ht[hash(v)]) {
        if (i == v_idx) break;
        if (v == verts[i]) {
            set_vert_idx(corner, i);
            break;
        }
    }
}

bool MeshSimplifierImpl::is_tri_duplicate(std::uint32_t tri_idx) {
    std::uint32_t i0 = indexes[tri_idx * 3 + 0], i1 = indexes[tri_idx * 3 + 1], i2 = indexes[tri_idx * 3 + 2];
    for (std::uint32_t i : corner_ht[hash(verts[i0])]) {
        if (i != tri_idx * 3) {
            if (i0 == indexes[i] && i1 == indexes[cycle3(i)] && i2 == indexes[(cycle3(i, 2))])
                return true;
        }
    }
    return false;
}

void MeshSimplifierImpl::fixup_tri(std::uint32_t tri_idx) {
    assert(!tri_removed[tri_idx]);

    const vec3& p0 = verts[indexes[tri_idx * 3 + 0]];
    const vec3& p1 = verts[indexes[tri_idx * 3 + 1]];
    const vec3& p2 = verts[indexes[tri_idx * 3 + 2]];

    bool is_removed = false;
    if (!is_removed) {
        is_removed = (p0 == p1) || (p1 == p2) || (p2 == p0);
    }
    if (!is_removed) {
        for (std::uint32_t k = 0; k < 3; k++) remove_if_vert_duplicate(tri_idx * 3 + k);
        is_removed = is_tri_duplicate(tri_idx);
    }
    if (is_removed) {
        tri_removed.set_true(tri_idx);
        remaining_num_tri--;
        for (std::uint32_t k = 0; k < 3; k++) {
            std::uint32_t corner = tri_idx * 3 + k;
            std::uint32_t v_idx = indexes[corner];
            corner_ht.remove(hash(verts[v_idx]), corner);
            set_vert_idx(corner, ~0u);
        }
    }
    else tri_quadrics[tri_idx] = Quadric(p0, p1, p2);
}

void MeshSimplifierImpl::gather_adj_tris(vec3 p, vector<std::uint32_t>& tris, bool& lock) {
    for (std::uint32_t i : corner_ht[hash(p)]) {
        if (verts[indexes[i]] == p) {
            std::uint32_t tri_idx = i / 3;
            if ((flags[tri_idx * 3] & AdjMask) == 0) {
                flags[tri_idx * 3] |= AdjMask;
                tris.push_back(tri_idx);
            }
            if (flags[i] & LockMask) {
                lock = true;
            }
        }
    }
}

float MeshSimplifierImpl::evaluate(vec3 p0, vec3 p1, bool merge) {
    if (p0 == p1) return 0.f;

    float error = 0;

    vector<std::uint32_t> adj_tris;
    bool lock0 = false, lock1 = false;
    gather_adj_tris(p0, adj_tris, lock0);
    gather_adj_tris(p1, adj_tris, lock1);
    if (adj_tris.size() == 0) return 0.f;
    if (adj_tris.size() > 24) {
        error += 0.5 * (adj_tris.size() - 24);
    }

    Quadric q;
    for (std::uint32_t i : adj_tris) {
        q.add(tri_quadrics[i]);
    }
    vec3 p = (p0 + p1) * 0.5f;

    auto is_valid_pos = [&](vec3 p)->bool {
        if (length(p - p0) + length(p - p1) > 2 * length(p0 - p1))
            return false;
        return true;
        };

    if (lock0 && lock1) error += 1e8;
    if (lock0 && !lock1) p = p0;
    else if (!lock0 && lock1) p = p1;
    else if (!q.get(p)) p = (p0 + p1) * 0.5f;
    if (!is_valid_pos(p)) {
        p = (p0 + p1) * 0.5f;
    }
    error += q.evaluate(p);

    if (merge) {
        begin_merge(p0), begin_merge(p1);
        for (std::uint32_t i : adj_tris) {
            for (std::uint32_t k = 0; k < 3; k++) {
                std::uint32_t corner = i * 3 + k;
                vec3& pos = verts[indexes[corner]];
                if (pos == p0 || pos == p1) {
                    pos = p;
                    if (lock0 || lock1) flags[corner] |= LockMask;
                }
            }
        }
        for (std::uint32_t i : move_edge) {
            auto& e = edges[i];
            if (e.first == p0 || e.first == p1) e.first = p;
            if (e.second == p0 || e.second == p1) e.second = p;
        }
        end_merge();

        vector<std::uint32_t> adj_verts;
        for (std::uint32_t i : adj_tris) {
            for (std::uint32_t k = 0; k < 3; k++) {
                adj_verts.push_back(indexes[i * 3 + k]);
            }
        }
        sort(adj_verts.begin(), adj_verts.end());
        adj_verts.erase(unique(adj_verts.begin(), adj_verts.end()), adj_verts.end());

        for (std::uint32_t v_idx : adj_verts) {
            std::uint32_t h = hash(verts[v_idx]);
            for (std::uint32_t i : edge0_ht[h]) {
                if (edges[i].first == verts[v_idx]) {
                    if (heap.is_present(i)) {
                        heap.remove(i);
                        reevaluate_edge.push_back(i);
                    }
                }
            }
            for (std::uint32_t i : edge1_ht[h]) {
                if (edges[i].second == verts[v_idx]) {
                    if (heap.is_present(i)) {
                        heap.remove(i);
                        reevaluate_edge.push_back(i);
                    }
                }
            }
        }
        for (std::uint32_t i : adj_tris) {
            fixup_tri(i);
        }
    }
    for (std::uint32_t i : adj_tris) {
        flags[i * 3] &= (~AdjMask);
    }
    return error;
}

void MeshSimplifierImpl::begin_merge(vec3 p) {
    std::uint32_t h = hash(p);
    for (std::uint32_t i : vert_ht[h]) {
        if (verts[i] == p) {
            vert_ht.remove(h, i);
            move_vert.push_back(i);
        }
    }
    for (std::uint32_t i : corner_ht[h]) {
        if (verts[indexes[i]] == p) {
            corner_ht.remove(h, i);
            move_corner.push_back(i);
        }
    }
    for (std::uint32_t i : edge0_ht[h]) {
        if (edges[i].first == p) {
            edge0_ht.remove(hash(edges[i].first), i);
            edge1_ht.remove(hash(edges[i].second), i);
            move_edge.push_back(i);
        }
    }
    for (std::uint32_t i : edge1_ht[h]) {
        if (edges[i].second == p) {
            edge0_ht.remove(hash(edges[i].first), i);
            edge1_ht.remove(hash(edges[i].second), i);
            move_edge.push_back(i);
        }
    }
}

void MeshSimplifierImpl::end_merge() {
    for (std::uint32_t i : move_vert) {
        vert_ht.add(hash(verts[i]), i);
    }
    for (std::uint32_t i : move_corner) {
        corner_ht.add(hash(verts[indexes[i]]), i);
    }
    for (std::uint32_t i : move_edge) {
        auto& e = edges[i];
        if (e.first == e.second || !add_edge_ht(e.first, e.second, i)) {
            heap.remove(i);
        }
    }
    move_vert.clear();
    move_corner.clear();
    move_edge.clear();
}

void MeshSimplifierImpl::simplify(std::uint32_t target_num_tri) {
    tri_quadrics.resize(num_tri);
    for (std::uint32_t i = 0; i < num_tri; i++) fixup_tri(i);
    if (remaining_num_tri <= target_num_tri) {
        compact();
        return;
    }
    heap.resize(edges.size());
    std::uint32_t i = 0;
    for (auto& e : edges) {
        float error = evaluate(e.first, e.second, false);
        heap.add(error, i);
        i++;
    }

    max_error = 0;
    while (!heap.empty()) {
        std::uint32_t e_idx = heap.top();
        if (heap.get_key(e_idx) >= 1e6) break;

        heap.pop();

        auto& e = edges[e_idx];
        edge0_ht.remove(hash(e.first), e_idx);
        edge1_ht.remove(hash(e.second), e_idx);

        float error = evaluate(e.first, e.second, true);
        if (error > max_error) max_error = error;

        if (remaining_num_tri <= target_num_tri) break;

        for (std::uint32_t i : reevaluate_edge) {
            auto& e = edges[i];
            float error = evaluate(e.first, e.second, false);
            heap.add(error, i);
        }
        reevaluate_edge.clear();
    }
    compact();
}

void MeshSimplifierImpl::compact() {
    std::uint32_t v_cnt = 0;
    for (std::uint32_t i = 0; i < num_vert; i++) {
        if (vert_refs[i] > 0) {
            if (i != v_cnt) verts[v_cnt] = verts[i];
            //重用作下标
            vert_refs[i] = v_cnt++;
        }
    }
    assert(v_cnt == remaining_num_vert);

    std::uint32_t t_cnt = 0;
    for (std::uint32_t i = 0; i < num_tri; i++) {
        if (!tri_removed[i]) {
            for (std::uint32_t k = 0; k < 3; k++) {
                indexes[t_cnt * 3 + k] = vert_refs[indexes[i * 3 + k]];
            }
            t_cnt++;
        }
    }
    assert(t_cnt == remaining_num_tri);
}

MeshSimplifier::MeshSimplifier(vec3* verts, std::uint32_t num_vert, std::uint32_t* indexes, std::uint32_t num_index) {
    impl = new MeshSimplifierImpl(verts, num_vert, indexes, num_index);
}

MeshSimplifier::~MeshSimplifier() {
    if (impl) delete (MeshSimplifierImpl*)impl;
}

void MeshSimplifier::lock_position(vec3 p) {
    ((MeshSimplifierImpl*)impl)->lock_position(p);
}

// bool MeshSimplifier::is_position_locked(vec3 p){
//     return ((MeshSimplifierImpl*)impl)->is_position_locked(p);
// }

void MeshSimplifier::simplify(std::uint32_t target_num_tri) {
    ((MeshSimplifierImpl*)impl)->simplify(target_num_tri);
}

std::uint32_t MeshSimplifier::remaining_num_vert() {
    return ((MeshSimplifierImpl*)impl)->remaining_num_vert;
}

std::uint32_t MeshSimplifier::remaining_num_tri() {
    return ((MeshSimplifierImpl*)impl)->remaining_num_tri;
}

float MeshSimplifier::max_error() {
    return ((MeshSimplifierImpl*)impl)->max_error;
}