#include "CullingPass.h"
#include "Pipeline.h"
#include "Context.h"
#include "define.h"
#include "mesh.h"
#include "Buffer.h"
#include "program.h"
#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

constexpr uint32_t MESH_BBOX_SET = 0;
constexpr uint32_t INPUT_INDIRECT_BUFFER_SET = 1;
constexpr uint32_t OUTPUT_INDIRECT_BUFFER_SET = 2;
constexpr uint32_t OUTPUT_INDIRECT_COUNT_BUFFER_SET = 3;
constexpr uint32_t CAMERA_FRUSTUM_SET = 4;
constexpr uint32_t BINDING_0 = 0;

namespace
{
	void* p;
}

void CullingPass::init(std::shared_ptr<Mesh> mesh, std::shared_ptr<Buffer> inputIndirectBuffer)
{
	inputIndirectDrawBuffer = inputIndirectBuffer;
	camFrustumBuffer.reset(new Buffer(sizeof(ViewBuffer), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
	stageBuffer.reset(new Buffer(sizeof(ViewBuffer), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	p = Context::GetInstance().device.mapMemory(stageBuffer->memory, 0, sizeof(ViewBuffer));
	for (auto aabb : mesh->aabbs)
	{
		meshBBosData.emplace_back(MeshBoundBoxBuffer{
			.centerPos = glm::vec4(aabb.center, 1.0f),
			.extents = glm::vec4(aabb.extent, 1.0f),
			});
	}

	const auto totalSize = sizeof(MeshBoundBoxBuffer) * meshBBosData.size();

	meshBboxBuffer.reset(new Buffer(totalSize, vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, meshBboxBuffer, meshBBosData.size() * sizeof(MeshBoundBoxBuffer), meshBBosData.data());

	outputIndirectDrawBuffer.reset(new Buffer(sizeof(IndirectCommandAndMeshData) * mesh->indirectDrawData.size(),
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));

	outputIndirectDrawCountBuffer.reset(new Buffer(sizeof(IndirectDrawCount), vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));

	shader.reset(new GPUProgram(shaderPath + "culling.comp.spv"));

	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = MESH_BBOX_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = INPUT_INDIRECT_BUFFER_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(50)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = OUTPUT_INDIRECT_BUFFER_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = OUTPUT_INDIRECT_COUNT_BUFFER_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = CAMERA_FRUSTUM_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> ranges(1);
	ranges[0].setOffset(0)
		.setSize(sizeof(GPUCullingPassPushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eCompute);

	const Pipeline::ComputePipelineDescriptor desc = {
		.sets = setLayouts,
		.computerShader = shader->Compute,
		.pushConstants = ranges,
	};
	m_pipeline.reset(new Pipeline(desc, "main"));
	m_pipeline->allocateDescriptors({
		{.set = MESH_BBOX_SET, .count = 1},
		{.set = INPUT_INDIRECT_BUFFER_SET, .count = 1},
		{.set = OUTPUT_INDIRECT_BUFFER_SET, .count = 1},
		{.set = OUTPUT_INDIRECT_COUNT_BUFFER_SET, .count = 1},
		{.set = CAMERA_FRUSTUM_SET, .count = 3},
		});
	m_pipeline->bindResource(MESH_BBOX_SET, BINDING_0, 0, meshBboxBuffer, 0, meshBboxBuffer->size, 
		vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(INPUT_INDIRECT_BUFFER_SET, BINDING_0, 0, inputIndirectDrawBuffer, 0, 
		inputIndirectDrawBuffer->size, vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(OUTPUT_INDIRECT_BUFFER_SET, BINDING_0, 0, outputIndirectDrawBuffer, 0,
		outputIndirectDrawBuffer->size, vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(OUTPUT_INDIRECT_COUNT_BUFFER_SET, BINDING_0, 0, outputIndirectDrawCountBuffer,
		0, outputIndirectDrawCountBuffer->size, vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(CAMERA_FRUSTUM_SET, BINDING_0, 0, camFrustumBuffer, 0,
		camFrustumBuffer->size, vk::DescriptorType::eUniformBuffer);

}

CullingPass::~CullingPass()
{
	Context::GetInstance().device.unmapMemory(stageBuffer->memory);
	m_pipeline.reset();
	stageBuffer.reset();
	camFrustumBuffer.reset();
	meshBboxBuffer.reset();
	outputIndirectDrawBuffer.reset();
	outputIndirectDrawCountBuffer.reset();
	shader.reset();
}


void CullingPass::upload()
{
}

void CullingPass::cull(vk::CommandBuffer cmdbuf, int frameIndex)
{
	GPUCullingPassPushConstants pushConst{
		.drawCount = uint32_t(meshBBosData.size()),
	};
	auto camera = CameraManager::mainCamera;
	float fov = 45.0f;
	float nearP = 0.1f;
	float farP = 1.0f;
	float aspect = 1280.0 / 720;
	const auto tanFovYHalf = glm::tan(glm::radians(fov) * 0.5);
	const float nearPlaneHalfHeight = nearP * tanFovYHalf;
	const float farPlaneHalfHeight = farP * tanFovYHalf;
	const glm::vec3 nearPlaneHalfHeightVec = nearPlaneHalfHeight * camera->Up;
	const glm::vec3 nearPlaneHalfWidthVec = nearPlaneHalfHeight * aspect * camera->Right;

	const glm::vec3 farPlaneHalfHeightVec = farPlaneHalfHeight * camera->Up;
	const glm::vec3 farPlaneHalfWidthVec = farPlaneHalfHeight * aspect * camera->Right;

	const glm::vec3 nearCameraPosition = camera->Position + camera->Front * nearP;
	const glm::vec3 farCameraPosition = camera->Position + camera->Front * farP;

	const glm::vec3 nearTopRight =
		nearCameraPosition + nearPlaneHalfWidthVec + nearPlaneHalfHeightVec;
	const glm::vec3 nearBottomRight =
		nearCameraPosition + nearPlaneHalfWidthVec - nearPlaneHalfHeightVec;
	const glm::vec3 nearTopLeft =
		nearCameraPosition - nearPlaneHalfWidthVec + nearPlaneHalfHeightVec;
	const glm::vec3 nearBottomLeft =
		nearCameraPosition - nearPlaneHalfWidthVec - nearPlaneHalfHeightVec;

	const glm::vec3 farTopRight =
		farCameraPosition + farPlaneHalfWidthVec + farPlaneHalfHeightVec;
	const glm::vec3 farBottomRight =
		farCameraPosition + farPlaneHalfWidthVec - farPlaneHalfHeightVec;
	const glm::vec3 farTopLeft =
		farCameraPosition - farPlaneHalfWidthVec + farPlaneHalfHeightVec;
	const glm::vec3 farBottomLeft =
		farCameraPosition - farPlaneHalfWidthVec - farPlaneHalfHeightVec;

	auto getNormal = [](const glm::vec3& corner, const glm::vec3& point1,
		const glm::vec3& point2) {
			const glm::vec3 dir0 = point1 - corner;
			const glm::vec3 dir1 = point2 - corner;
			const glm::vec3 crossDir = glm::cross(dir0, dir1);
			return glm::normalize(crossDir);
		};

	// left
	const glm::vec3 leftNormal = getNormal(farBottomLeft, nearBottomLeft, farTopLeft);
	frustum.frustumPlanes[0] = glm::vec4(leftNormal, -glm::dot(leftNormal, farBottomLeft));
	// down
	const glm::vec3 downNormal = getNormal(farBottomRight, nearBottomRight, farBottomLeft);
	frustum.frustumPlanes[1] = glm::vec4(downNormal, -glm::dot(downNormal, farBottomRight));
	// right
	const glm::vec3 rightNormal = getNormal(farTopRight, nearTopRight, farBottomRight);
	frustum.frustumPlanes[2] = glm::vec4(rightNormal, -glm::dot(rightNormal, farTopRight));
	// top
	const glm::vec3 topNormal = getNormal(farTopLeft, nearTopLeft, farTopRight);
	frustum.frustumPlanes[3] = glm::vec4(topNormal, -glm::dot(topNormal, farTopLeft));
	// front
	const glm::vec3 frontNormal = getNormal(nearTopRight, nearTopLeft, nearBottomRight);
	frustum.frustumPlanes[4] = glm::vec4(frontNormal, -glm::dot(frontNormal, nearTopRight));
	// back
	const glm::vec3 backNormal = getNormal(farTopRight, farBottomRight, farTopLeft);
	frustum.frustumPlanes[5] = glm::vec4(backNormal, -glm::dot(backNormal, farTopRight));

	memcpy(p, &frustum, sizeof(ViewBuffer));
	CopyBuffer(stageBuffer->buffer, camFrustumBuffer->buffer, sizeof(ViewBuffer), 0, 0);

	m_pipeline->bind(cmdbuf);
	m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eCompute,
		sizeof(GPUCullingPassPushConstants), &pushConst);
	m_pipeline->bindDescriptorSets(cmdbuf, {
		{.set = MESH_BBOX_SET, .bindIdx = 0},
		{.set = INPUT_INDIRECT_BUFFER_SET, .bindIdx = 0},
		{.set = OUTPUT_INDIRECT_BUFFER_SET, .bindIdx = 0},
		{.set = OUTPUT_INDIRECT_COUNT_BUFFER_SET, .bindIdx = 0},
		{.set = CAMERA_FRUSTUM_SET, .bindIdx = 0},
		});
	m_pipeline->updateDescriptorSets();

	cmdbuf.dispatch((pushConst.drawCount / 256) + 1, 1, 1);

}

void CullingPass::addBarrierForCulledBuffers(vk::CommandBuffer cmdbuf, vk::PipelineStageFlags dstStage, uint32_t computeFamilyIndex, uint32_t graphicsFamilyIndex)
{
	std::array<vk::BufferMemoryBarrier, 2> barriers;
	barriers[0].setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
		.setDstAccessMask(vk::AccessFlagBits::eIndirectCommandRead)
		.setSrcQueueFamilyIndex(computeFamilyIndex)
		.setDstQueueFamilyIndex(graphicsFamilyIndex)
		.setBuffer(outputIndirectDrawBuffer->buffer)
		.setSize(outputIndirectDrawBuffer->size);
	barriers[1].setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
		.setDstAccessMask(vk::AccessFlagBits::eIndirectCommandRead)
		.setSrcQueueFamilyIndex(computeFamilyIndex)
		.setDstQueueFamilyIndex(graphicsFamilyIndex)
		.setBuffer(outputIndirectDrawCountBuffer->buffer)
		.setSize(outputIndirectDrawCountBuffer->size);

	cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, dstStage, {}, {}, barriers, {});
}
