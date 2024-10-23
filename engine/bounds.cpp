#include "bounds.h"


Bounds Bounds::operator+(Bounds b) {
	Bounds bounds;
	bounds.pmin = {
		std::min(pmin.x,b.pmin.x),
		std::min(pmin.y,b.pmin.y),
		std::min(pmin.z,b.pmin.z),
	};
	bounds.pmax = {
		std::max(pmax.x,b.pmax.x),
		std::max(pmax.y,b.pmax.y),
		std::max(pmax.z,b.pmax.z),
	};
	return bounds;
}

Bounds Bounds::operator+(glm::vec3 b)
{
	Bounds bounds;
	bounds.pmin = {
		std::min(pmin.x,b.x),
		std::min(pmin.y,b.y),
		std::min(pmin.z,b.z),
	};
	bounds.pmax = {
		std::max(pmax.x,b.x),
		std::max(pmax.y,b.y),
		std::max(pmax.z,b.z),
	};
	return bounds;
}

Sphere Sphere::from_points(glm::vec3* pos, std::uint32_t size)
{
	std::uint32_t min_idx[3] = {};
	std::uint32_t max_idx[3] = {};
	for (size_t i = 0; i < size; i++)
	{
		for (size_t k = 0; k < 3; k++)
		{
			if (pos[i][k] < pos[min_idx[k]][k]) min_idx[k] = i;
			if (pos[i][k] > pos[max_idx[k]][k]) max_idx[k] = i;
		}
	}
	float max_len = 0;
	std::uint32_t max_axis = 0;
	for (size_t k = 0; k < 3; k++)
	{
		glm::vec3 pmin = pos[min_idx[k]];
		glm::vec3 pmax = pos[max_idx[k]];
		float tlen = glm::length(pmax - pmin);
		if (tlen > max_len)
		{
			max_len = tlen, max_axis = k;
		}
	}
	glm::vec3 pmin = pos[min_idx[max_axis]];
	glm::vec3 pmax = pos[max_idx[max_axis]];

	Sphere sphere;
	sphere.center = (pmin + pmax) * 0.5f;
	sphere.radius = 0.5 * max_len;
	max_len = sphere.radius * sphere.radius;
	for (size_t i = 0; i < size; i++)
	{
		float len = glm::length(pos[i] - sphere.center) * glm::length(pos[i] - sphere.center);
		if (len > max_len)
		{
			len = sqrt(len);
			float t = 0.5 - 0.5 * (sphere.radius / len);
			sphere.center = sphere.center + (pos[i] - sphere.center) * t;
			sphere.radius = (sphere.radius + len) * 0.5;
			max_len = sphere.radius * sphere.radius;
		}
	}

	for (size_t i = 0; i < size; i++)
	{
		float len = glm::length(pos[i] - sphere.center);
		assert(len - 1e-6 <= sphere.radius);
	}
	return sphere;
}

inline float sqr(float x) { return x * x; }

Sphere Sphere::operator+(Sphere b)
{
	glm::vec3 t = b.center - center;
	float tlen2 = sqr(glm::length(t));
	if (sqr(radius - b.radius) >= tlen2)
	{
		return radius < b.radius ? b : *this;
	}
	Sphere sphere;
	float tlen = glm::length(t);
	sphere.radius = (tlen + radius + b.radius) * 0.5;
	sphere.center = center + t * ((sphere.radius - radius) / tlen);
	return sphere;
}

Sphere Sphere::from_spheres(Sphere* spheres, std::uint32_t size)
{
	std::uint32_t min_idx[3] = {};
	std::uint32_t max_idx[3] = {};
	for (std::uint32_t i = 0; i < size; i++) {
		for (std::uint32_t k = 0; k < 3; k++) {
			if (spheres[i].center[k] - spheres[i].radius < spheres[min_idx[k]].center[k] - spheres[min_idx[k]].radius)
				min_idx[k] = i;
			if (spheres[i].center[k] + spheres[i].radius < spheres[max_idx[k]].center[k] + spheres[max_idx[k]].radius)
				max_idx[k] = i;
		}
	}
	float max_len = 0;
	std::uint32_t max_axis = 0;
	for (std::uint32_t k = 0; k < 3; k++) {
		Sphere spmin = spheres[min_idx[k]];
		Sphere spmax = spheres[max_idx[k]];
		float tlen = length(spmax.center - spmin.center) + spmax.radius + spmin.radius;
		if (tlen > max_len) max_len = tlen, max_axis = k;
	}
	Sphere sphere = spheres[min_idx[max_axis]];
	sphere = sphere + spheres[max_idx[max_axis]];
	for (std::uint32_t i = 0; i < size; i++) {
		sphere = sphere + spheres[i];
	}
	//
	for (std::uint32_t i = 0; i < size; i++) {
		float t1 = sqr(sphere.radius - spheres[i].radius);
		float t2 = glm::length(sphere.center - spheres[i].center) * glm::length(sphere.center - spheres[i].center);
		assert(t1 + 1e-6 >= t2);
	}
	return sphere;
}