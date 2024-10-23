#include "cluster.h"
#include "hash_table.h"
#include "partitioner.h"
#include "mesh_util.h"
#include "mesh_simplify.h"
#include <unordered_map>
#include <span>

using namespace std;

inline std::uint32_t hash(glm::vec3 v)
{
	union { float f; std::uint32_t u; } x, y, z;
	x.f = (v.x == 0.f ? 0 : v.x);
	y.f = (v.y == 0.f ? 0 : v.y);
	z.f = (v.z == 0.f ? 0 : v.z);
	return murmur_mix(murmur_add(murmur_add(x.u, y.u), z.u));
}

inline u32 hash(pair<glm::vec3, glm::vec3> e)
{
	u32 h0 = ::hash(e.first);
	u32 h1 = ::hash(e.second);
	return murmur_mix(murmur_add(h0, h1));
}

//将原来的数位以2个0分隔：10111->1000001001001，用于生成莫顿码 
inline u32 expand_bits(u32 v) {
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}
//莫顿码，要求 0<=x,y,z<=1
u32 morton3D(glm::vec3 p) {
	u32 x = p.x * 1023, y = p.y * 1023, z = p.z * 1023;
	x = expand_bits(x);
	y = expand_bits(y);
	z = expand_bits(z);
	return (x << 2) | (y << 1) | (z << 1);
}

//边哈希，找到共享顶点且相反的边，代表两三角形相邻
void build_adjacency_edge_link(const vector<glm::vec3>& verts,
	const vector<u32>& indices, Graph& edge_link)
{
	HashTable edge_ht(indices.size());
	edge_link.init(indices.size());

	for (size_t i = 0; i < indices.size(); i++)
	{
		glm::vec3 p0 = verts[indices[i]];
		glm::vec3 p1 = verts[indices[cycle3(i)]];
		edge_ht.add(::hash({ p0, p1 }), i);

		for (auto j : edge_ht[::hash({p1, p0})])
		{
			if (p1 == verts[indices[j]] && p0 == verts[indices[cycle3(j)]])
			{
				edge_link.increase_edge_cost(i, j, 1);
				edge_link.increase_edge_cost(j, i, 1);
			}
		}
	}
}

void build_adjacency_graph(const Graph& edge_link, Graph& graph)
{
	graph.init(edge_link.g.size() / 3);
	u32 u = 0;
	for (const auto& mp : edge_link.g)
	{
		for (auto [v, w] : mp)
		{
			graph.increase_edge_cost(u / 3, v / 3, 1);
		}
		u++;
	}
}

void cluster_triangles(const vector<glm::vec3>& verts,
	const vector<u32>& indices, vector<Cluster>& clusters)
{
	Graph edge_link, graph;
	build_adjacency_edge_link(verts, indices, edge_link);
	build_adjacency_graph(edge_link, graph);

	Partitioner partitioner;
	partitioner.partition(graph, Cluster::cluster_size - 4, Cluster::cluster_size);

	// 根据划分结果构建clusters
	for (auto [l, r] : partitioner.ranges)
	{
		clusters.push_back({});
		Cluster& cluster = clusters.back();

		unordered_map<u32, u32> mp;
		for (u32 i = l; i < r; i++)
		{
			u32 t_idx = partitioner.node_id[i];
			for (u32 k = 0; k < 3; k++)
			{
				u32 e_idx = t_idx * 3 + k;
				u32 v_idx = indices[e_idx];
				if (mp.find(v_idx) == mp.end()) //重映射顶点下标
				{
					mp[v_idx] = cluster.verts.size();
					cluster.verts.push_back(verts[v_idx]);
				}
				bool is_external = false;
				for (auto [adj_edge, _] : edge_link.g[e_idx])
				{
					u32 adj_tri = partitioner.sort_to[adj_edge / 3];
					if (adj_tri < l || adj_tri >= r)
					{
						is_external = true;
						break;
					}
				}
				if (is_external)
				{
					cluster.external_edges.push_back(cluster.indices.size());
				}
				cluster.indices.push_back(mp[v_idx]);
			}
		}
		cluster.mip_level = 0;
		cluster.lod_error = 0;
		cluster.sphere_bounds = Sphere::from_points(cluster.verts.data(), cluster.verts.size());
		cluster.lod_bounds = cluster.sphere_bounds;
		cluster.box_bounds = cluster.verts[0];
		for (glm::vec3 p : cluster.verts) cluster.box_bounds = cluster.box_bounds + p;
	}
}

void build_clusters_edge_link(
	span<const Cluster> clusters,
	const vector<pair<u32, u32>>& ext_edges,
	Graph& edge_link
) {
	HashTable edge_ht(ext_edges.size());
	edge_link.init(ext_edges.size());

	u32 i = 0;
	for (auto [c_id, e_id] : ext_edges) {
		auto& pos = clusters[c_id].verts;
		auto& idx = clusters[c_id].indices;
		glm::vec3 p0 = pos[idx[e_id]];
		glm::vec3 p1 = pos[idx[cycle3(e_id)]];
		edge_ht.add(::hash({ p0,p1 }), i);
		for (u32 j : edge_ht[::hash({ p1,p0 })]) {
			auto [c_id1, e_id1] = ext_edges[j];
			auto& pos1 = clusters[c_id1].verts;
			auto& idx1 = clusters[c_id1].indices;

			if (pos1[idx1[e_id1]] == p1 && pos1[idx1[cycle3(e_id1)]] == p0) {
				edge_link.increase_edge_cost(i, j, 1);
				edge_link.increase_edge_cost(j, i, 1);
			}
		}
		i++;
	}
}

void build_clusters_graph(
	const Graph& edge_link,
	const vector<u32>& mp,
	u32 num_cluster,
	Graph& graph
) {
	graph.init(num_cluster);
	u32 u = 0;
	for (const auto& emp : edge_link.g) {
		for (auto [v, w] : emp) {
			graph.increase_edge_cost(mp[u], mp[v], 1);
		}
		u++;
	}
}

void group_clusters(
	vector<Cluster>& clusters,
	u32 offset,
	u32 num_cluster,
	vector<ClusterGroup>& cluster_groups,
	u32 mip_level
) {
	span<const Cluster> clusters_view(clusters.begin() + offset, num_cluster);

	//取出每个cluster的边界，并建立边id到簇id的映射
	vector<u32> mp; //edge_id to cluster_id
	vector<u32> mp1; //cluster_id to first_edge_id
	vector<pair<u32, u32>> ext_edges;
	u32 i = 0;
	for (auto& cluster : clusters_view) {
		assert(cluster.mip_level == mip_level);
		mp1.push_back(mp.size());
		for (u32 e : cluster.external_edges) {
			ext_edges.push_back({ i,e });
			mp.push_back(i);
		}
		i++;
	}
	Graph edge_link, graph;
	build_clusters_edge_link(clusters_view, ext_edges, edge_link);
	build_clusters_graph(edge_link, mp, num_cluster, graph);

	Partitioner partitioner;
	partitioner.partition(graph, ClusterGroup::group_size - 4, ClusterGroup::group_size);

	//todo: 包围盒
	for (auto [l, r] : partitioner.ranges) {
		cluster_groups.push_back({});
		auto& group = cluster_groups.back();
		group.mip_level = mip_level;

		for (u32 i = l; i < r; i++) {
			u32 c_id = partitioner.node_id[i];
			clusters[c_id + offset].group_id = cluster_groups.size() - 1;
			group.clusters.push_back(c_id + offset);
			for (u32 e_idx = mp1[c_id]; e_idx < mp.size() && mp[e_idx] == c_id; e_idx++) {
				bool is_external = false;
				for (auto [adj_e, _] : edge_link.g[e_idx]) {
					u32 adj_cl = partitioner.sort_to[mp[adj_e]];
					if (adj_cl < l || adj_cl >= r) {
						is_external = true;
						break;
					}
				}
				if (is_external) {
					u32 e = ext_edges[e_idx].second;
					group.external_edges.push_back({ c_id + offset,e });
				}
			}
		}
	}
}

void build_parent_clusters(
	ClusterGroup& cluster_group,
	std::vector<Cluster>& clusters
) {
	vector<glm::vec3> pos;
	vector<u32> idx;
	vector<Sphere> lod_bounds;
	f32 max_parent_lod_error = 0;
	u32 i_ofs = 0;
	for (u32 c : cluster_group.clusters) {
		auto& cluster = clusters[c];
		for (glm::vec3 p : cluster.verts) pos.push_back(p);
		for (u32 i : cluster.indices) idx.push_back(i + i_ofs);
		i_ofs += cluster.verts.size();
		lod_bounds.push_back(cluster.lod_bounds);
		max_parent_lod_error = max(max_parent_lod_error, cluster.lod_error); //强制父节点的error大于等于子节点
	}
	Sphere parent_lod_bound = Sphere::from_spheres(lod_bounds.data(), lod_bounds.size());

	MeshSimplifier simplifier(pos.data(), pos.size(), idx.data(), idx.size());
	HashTable edge_ht(cluster_group.external_edges.size());
	u32 i = 0;

	for (auto [c, e] : cluster_group.external_edges) {
		auto& pos = clusters[c].verts;
		auto& idx = clusters[c].indices;
		glm::vec3 p0 = pos[idx[e]], p1 = pos[idx[cycle3(e)]];
		edge_ht.add(::hash({ p0,p1 }), i);
		simplifier.lock_position(p0);
		simplifier.lock_position(p1);
		i++;
	}

	simplifier.simplify((Cluster::cluster_size - 2) * (cluster_group.clusters.size() / 2));
	pos.resize(simplifier.remaining_num_vert());
	idx.resize(simplifier.remaining_num_tri() * 3);

	max_parent_lod_error = max(max_parent_lod_error, sqrt(simplifier.max_error()));

	Graph edge_link, graph;
	build_adjacency_edge_link(pos, idx, edge_link);
	build_adjacency_graph(edge_link, graph);

	Partitioner partitioner;
	partitioner.partition(graph, Cluster::cluster_size - 4, Cluster::cluster_size);

	for (auto [l, r] : partitioner.ranges) {
		clusters.push_back({});
		Cluster& cluster = clusters.back();

		unordered_map<u32, u32> mp;
		for (u32 i = l; i < r; i++) {
			u32 t_idx = partitioner.node_id[i];
			for (u32 k = 0; k < 3; k++) {
				u32 e_idx = t_idx * 3 + k;
				u32 v_idx = idx[e_idx];
				if (mp.find(v_idx) == mp.end()) { //重映射顶点下标
					mp[v_idx] = cluster.verts.size();
					cluster.verts.push_back(pos[v_idx]);
				}
				bool is_external = false;
				for (auto [adj_edge, _] : edge_link.g[e_idx]) {
					u32 adj_tri = partitioner.sort_to[adj_edge / 3];
					if (adj_tri < l || adj_tri >= r) { //出点在不同划分说明是边界
						is_external = true;
						break;
					}
				}
				glm::vec3 p0 = pos[v_idx], p1 = pos[idx[cycle3(e_idx)]]; //
				if (!is_external) {
					for (u32 j : edge_ht[::hash({ p0,p1 })]) {
						auto [c, e] = cluster_group.external_edges[j];
						auto& pos = clusters[c].verts;
						auto& idx = clusters[c].indices;
						if (p0 == pos[idx[e]] && p1 == pos[idx[cycle3(e)]]) {
							is_external = true;
							break;
						}
					}
				}

				if (is_external) {
					cluster.external_edges.push_back(cluster.indices.size());
				}
				cluster.indices.push_back(mp[v_idx]);
			}
		}

		cluster.mip_level = cluster_group.mip_level + 1;
		cluster.sphere_bounds = Sphere::from_points(cluster.verts.data(), cluster.verts.size());
		//强制父节点的lod包围盒覆盖所有子节点lod包围盒
		cluster.lod_bounds = parent_lod_bound;
		cluster.lod_error = max_parent_lod_error;
		cluster.box_bounds = cluster.verts[0];
		for (auto p : cluster.verts) cluster.box_bounds = cluster.box_bounds + p;
	}
	cluster_group.lod_bounds = parent_lod_bound;
	cluster_group.max_parent_lod_error = max_parent_lod_error;
}
