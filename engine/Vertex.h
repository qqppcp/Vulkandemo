#pragma once

#include "vulkan/vulkan.hpp"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec4 Tangent;
	glm::vec2 TexCoords;
	glm::vec2 TexCoords2;
	int32_t materialId;
	void applyTransform(const glm::mat4& m) {
		auto newp = m * glm::vec4(Position, 1.0);
		Position = glm::vec3(newp.x, newp.y, newp.z);
		glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(m));
		Normal = normalMatrix * Normal;
		Tangent = glm::inverseTranspose(m) * Tangent;
	}
	static std::array<vk::VertexInputAttributeDescription, 5> GetAttribute()
	{
		std::array<vk::VertexInputAttributeDescription, 5> attribs;
		attribs[0].setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(0);
		attribs[1].setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, Normal.r));
		attribs[2].setBinding(0)
			.setLocation(2)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(offsetof(Vertex, TexCoords.r));
		attribs[3].setBinding(0)
			.setLocation(3)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, Tangent.r));
		attribs[4].setBinding(0)
			.setLocation(5)
			.setFormat(vk::Format::eR32Sint)
			.setOffset(offsetof(Vertex, materialId));
		return attribs;
	}
	static vk::VertexInputBindingDescription GetBinding()
	{
		vk::VertexInputBindingDescription binding;
		binding.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(sizeof(Vertex));
		return binding;
	}
};

//class Vertex final {
//public:
//
//	float x, y;
//	float r, g, b, a;
//	static std::array<vk::VertexInputAttributeDescription, 2> GetAttribute()
//	{
//		std::array<vk::VertexInputAttributeDescription, 2> attribs;
//		attribs[0].setBinding(0)
//			.setLocation(0)
//			.setFormat(vk::Format::eR32G32Sfloat)
//			.setOffset(0);
//		attribs[1].setBinding(0)
//			.setLocation(1)
//			.setFormat(vk::Format::eR32G32B32A32Sfloat)
//			.setOffset(offsetof(Vertex, r));
//		return attribs;
//	}
//	static vk::VertexInputBindingDescription GetBinding()
//	{
//		vk::VertexInputBindingDescription binding;
//		binding.setBinding(0)
//			.setInputRate(vk::VertexInputRate::eVertex)
//			.setStride(sizeof(Vertex));
//		return binding;
//	}
//};

//class VertexPos final
//{
//public:
//	glm::vec3 pos;
//	glm::vec3 normal;
//	glm::vec2 uv;
//	VertexPos(glm::vec3 _pos, glm::vec3 _nor, glm::vec2 _uv) { pos = _pos, normal = _nor, uv = _uv; }
//	VertexPos(float x, float y, float z) { pos = glm::vec3{ x,y,z }; normal = glm::vec3{ 0 }; uv = glm::vec2{ 0 }; }
//	VertexPos(double x, double y, double z) { pos = glm::vec3{ x,y,z }; normal = glm::vec3{ 0 }; uv = glm::vec2{ 0 }; }
//	VertexPos() {};
//	static std::array<vk::VertexInputAttributeDescription, 2> GetAttribute()
//	{
//		std::array<vk::VertexInputAttributeDescription, 2> attribs;
//		attribs[0].setBinding(0)
//			.setLocation(0)
//			.setFormat(vk::Format::eR32G32B32Sfloat)
//			.setOffset(0);
//		attribs[1].setBinding(0)
//			.setLocation(1)
//			.setFormat(vk::Format::eR32G32Sfloat)
//			.setOffset(offsetof(VertexPos, uv));
//		return attribs;
//	}
//	static vk::VertexInputBindingDescription GetBinding()
//	{
//		vk::VertexInputBindingDescription binding;
//		binding.setBinding(0)
//			.setInputRate(vk::VertexInputRate::eVertex)
//			.setStride(sizeof(VertexPos));
//		return binding;
//	}
//};