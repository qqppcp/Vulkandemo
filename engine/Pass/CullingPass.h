#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <memory>

class GPUProgram;
struct Mesh;
class Buffer;
class Pipeline;

class CullingPass
{
public:
	struct MeshBoundBoxBuffer
	{
		glm::vec4 centerPos;
		glm::vec4 extents;
	};

	struct GPUCullingPassPushConstants
	{
		uint32_t drawCount;
	};

	struct ViewBuffer
	{
		alignas(16) glm::vec4 frustumPlanes[6];
	};

	struct IndirectDrawCount
	{
		uint32_t count;
	};
	
	CullingPass() = default;
	~CullingPass();

	void init(std::shared_ptr<Mesh> mesh, std::shared_ptr<Buffer> inputIndirectBuffer);

	void upload();

	void cull(vk::CommandBuffer cmdbuf, int frameIndex);

	void addBarrierForCulledBuffers(vk::CommandBuffer cmdbuf, vk::PipelineStageFlags dstStage,
		uint32_t computeFamilyIndex, uint32_t graphicsFamilyIndex);

	std::shared_ptr<Buffer> culledIndirectDrawBuffer()
	{
		return outputIndirectDrawBuffer;
	}

	std::shared_ptr<Buffer> culledIndirectDrawCountBuffer()
	{
		return outputIndirectDrawCountBuffer;
	}

private:
	std::shared_ptr<GPUProgram> shader;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Buffer> stageBuffer;
	std::shared_ptr<Buffer> camFrustumBuffer;
	std::shared_ptr<Buffer> meshBboxBuffer;
	std::shared_ptr<Buffer> inputIndirectDrawBuffer;
	std::shared_ptr<Buffer> outputIndirectDrawBuffer;
	std::shared_ptr<Buffer> outputIndirectDrawCountBuffer;

	std::vector<MeshBoundBoxBuffer> meshBBosData;
	ViewBuffer frustum;
};