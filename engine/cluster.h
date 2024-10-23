#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "bounds.h"

struct Cluster
{
	static constexpr std::uint32_t cluster_size = 128;

	std::vector<glm::vec3> verts;
	std::vector<std::uint32_t> indices;
	std::vector<std::uint32_t> external_edges;

	Bounds box_bounds;
	Sphere sphere_bounds;
	Sphere lod_bounds;
	float lod_error;
	std::uint32_t mip_level;
	std::uint32_t group_id;
};

struct ClusterGroup
{
	static constexpr std::uint32_t group_size = 32;

	Sphere bounds;
	Sphere lod_bounds;
	float min_lod_error;
	float max_parent_lod_error;
	std::uint32_t mip_level;
	std::vector<std::uint32_t> clusters; //对cluster数组的下标
	std::vector<std::pair<std::uint32_t, std::uint32_t>> external_edges;//first: cluster id, second: edge id
};

void cluster_triangles(const std::vector<glm::vec3>& verts,
	const std::vector<std::uint32_t>& indices,
	std::vector<Cluster>& clusters);

void group_clusters(std::vector<Cluster>& clusters,
	std::uint32_t offset, std::uint32_t num_cluster,
	std::vector<ClusterGroup>& cluster_groups, std::uint32_t mip_level);

void build_parent_clusters(ClusterGroup& cluster_group, std::vector<Cluster>& clusters);