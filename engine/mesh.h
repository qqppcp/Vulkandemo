#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "Vertex.h"

class Texture;

struct Material {
	glm::vec4 Ka_illum;
	glm::vec4 Kd_dissolve;
	glm::vec4 Ks_shininess;
	glm::vec4 emission_ior;
	int diffuseTextureId = -1;
	int normalTextureId = -1;
	int specularTextureId = -1;
	int reflectTextureId = -1;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
	std::vector<std::shared_ptr<Texture>> textures;
	std::vector<Material> materials;
	std::string directory;
	~Mesh();
	void load(std::string path);
	void loadobj(std::string path);

	std::vector<glm::vec3> flatten()
	{
		std::vector<glm::vec3> _vertices;
		for (auto idx : indices)
		{
			_vertices.push_back(vertices[idx].Position);
		}
		return _vertices;
	}
};

