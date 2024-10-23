#pragma once
#include <glm/glm.hpp>

struct Bounds
{
	glm::vec3 pmin, pmax;
	Bounds() : pmin(1e9, 1e9, 1e9), pmax(-1e9, -1e9, -1e9) {}
	Bounds(glm::vec3 p) { pmin = p, pmax = p; }
	Bounds operator+(Bounds b);
	Bounds operator+(glm::vec3 b);
};

struct Sphere
{
	glm::vec3 center;
	float radius;

	Sphere operator+(Sphere b);
	static Sphere from_points(glm::vec3* pos, std::uint32_t size);
	static Sphere from_spheres(Sphere* spheres, std::uint32_t size);
};