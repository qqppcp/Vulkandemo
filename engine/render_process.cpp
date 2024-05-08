#include "render_process.h"
#include "Context.h"
#include "program.h"
#include "window.h"
#include "Vertex.h"

void RenderProcess::InitPipelineLayout()
{
	vk::PipelineLayoutCreateInfo layoutCI;
	layout = Context::GetInstance().device.createPipelineLayout(layoutCI);
}
void RenderProcess::InitRenderPass()
{
	vk::RenderPassCreateInfo renderpassCI;
	vk::AttachmentDescription attachment;
	attachment.setFormat(Context::GetInstance().swapchain->info.surfaceFormat.format)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::SubpassDescription subpass;
	vk::AttachmentReference colorRederence;
	colorRederence.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	subpass.setColorAttachments(colorRederence)
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	vk::SubpassDependency dependency;
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	renderpassCI.setAttachments(attachment)
		.setSubpasses(subpass)
		.setDependencies(dependency);

	renderPass = Context::GetInstance().device.createRenderPass(renderpassCI);
}
void RenderProcess::InitPipeline(GPUProgram* program)
{
	vk::GraphicsPipelineCreateInfo pipelineCI;

	vk::PipelineVertexInputStateCreateInfo inputSCI;
	auto attrib = Vertex::GetAttribute();
	auto binding = Vertex::GetBinding();
	inputSCI.setVertexAttributeDescriptions(attrib)
		.setVertexBindingDescriptions(binding);
	pipelineCI.setPVertexInputState(&inputSCI);

	vk::PipelineInputAssemblyStateCreateInfo assemblySCI;
	assemblySCI.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(false);
	pipelineCI.setPInputAssemblyState(&assemblySCI);

	pipelineCI.setStages(program->stages);

	vk::PipelineViewportStateCreateInfo viewportSCI;
	auto [width, height] = GetWindowSize();
	vk::Viewport viewport(0, 0, width, height, 0, 1);
	vk::Rect2D scissors({0,0}, {width, height});
	viewportSCI.setScissors(scissors)
		.setViewports(viewport);
	pipelineCI.setPViewportState(&viewportSCI);

	vk::PipelineRasterizationStateCreateInfo rasterizerSCI;
	rasterizerSCI.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f);
	pipelineCI.setPRasterizationState(&rasterizerSCI);

	vk::PipelineMultisampleStateCreateInfo multisampleSCI;
	multisampleSCI.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);
	pipelineCI.setPMultisampleState(&multisampleSCI);

	vk::PipelineDepthStencilStateCreateInfo depthSCI;
	// ø™∆Ù…Ó∂»≤‚ ‘
	depthSCI.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(false)
		.setStencilTestEnable(false);
	pipelineCI.setPDepthStencilState(&depthSCI);
	
	vk::PipelineColorBlendStateCreateInfo blendSCI;
	vk::PipelineColorBlendAttachmentState attachment;
	attachment.setBlendEnable(false)
		.setColorWriteMask(vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR);
	blendSCI.setAttachments(attachment)
		.setLogicOpEnable(false);
	pipelineCI.setPColorBlendState(&blendSCI);

	pipelineCI.setLayout(layout);
	pipelineCI.setRenderPass(renderPass);

	auto result = Context::GetInstance().device.createGraphicsPipeline(nullptr, pipelineCI);
	if (result.result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Create graphics pipeline failed.");
	}
	pipeline = result.value;
}

RenderProcess::~RenderProcess()
{
	vk::Device device = Context::GetInstance().device;
	device.destroyRenderPass(renderPass);
	device.destroyPipelineLayout(layout);
	device.destroyPipeline(pipeline);
}

void RenderProcess::DestroyPipeline()
{
	Context::GetInstance().device.destroyPipeline(pipeline);
}
