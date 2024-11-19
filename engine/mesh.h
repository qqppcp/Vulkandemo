#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Vertex.h"

class Texture;

struct Material {
	glm::vec4 Ka_illum;
	glm::vec4 Kd_dissolve;
	glm::vec4 Ks_shininess;
	glm::vec4 emission_ior;
	glm::vec4 baseColorFactor;
	int diffuseTextureId = -1;
	int normalTextureId = -1;
	int specularTextureId = -1;
	int emissiveTextureId = -1;
	int occlusionTexture = -1;
	int reflectTextureId = -1;
	float roughnessFactor;
	float metallicFactor;
	int alphaMode = 0;
	float alphaCutoff;
	glm::vec2 padding;
};

struct IndirectCommandAndMeshData
{
	vk::DrawIndexedIndirectCommand command;

	uint32_t meshId;
	uint32_t materialIndex;
};

struct AABB
{
	glm::vec3 minPos;
	glm::vec3 maxPos;
	glm::vec3 center;
	glm::vec3 extent;
};
struct Node;
struct Skin {
	std::string name;
	Node* skeletonRoot = nullptr;
	std::vector<glm::mat4> inverseBindMatrices;
	std::vector<Node*> joints;
};

struct Node {
	Node* parent;
	uint32_t index;
	std::vector<Node*> children;
	glm::mat4 matrix;
	std::string name;
	Skin* skin;
	int32_t skinIndex = -1;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};
	glm::mat4 localMatrix();
	glm::mat4 getMatrix();
	~Node();
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
	std::vector<std::shared_ptr<Texture>> textures;
	std::vector<glm::mat4> transforms;
	std::vector<Material> materials;
	std::vector<AABB> aabbs;
	std::vector<IndirectCommandAndMeshData> indirectDrawData;
	std::vector<Node*> linearNodes;
	std::vector<Node*> nodes;
	std::string directory;
	~Mesh();
	void loadobj(std::string path);
	void loadgltf(std::string path);

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

