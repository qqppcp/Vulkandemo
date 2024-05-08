#include "backend.h"

#include "window.h"
#include "Context.h"
#include "ShaderPool.h"
#include "program.h"
#include "render_process.h"
#include "Render.h"
#include "imgui/imgui.h"

std::string shaderPath = R"(assets\shaders\)";
std::unique_ptr<RenderProcess> process;
std::unique_ptr<GPUProgram> triangle;
std::unique_ptr<Renderer> renderer;
std::unique_ptr<ImGuiRenderer> imguiRenderer;
std::vector<vk::Framebuffer> framebuffers;

void Init()
{
	Context::InitContext();
	Context::GetInstance().InitSwapchain();
	ShaderPool::Initialize();
	triangle.reset(new GPUProgram(shaderPath + "triangle.vert.spv", shaderPath + "triangle.frag.spv"));
	process.reset(new RenderProcess);
	process->InitPipelineLayout();
	process->InitRenderPass();
	process->InitPipeline(triangle.get());
	auto& swapchian = Context::GetInstance().swapchain;
	auto device = Context::GetInstance().device;
	auto [width, height] = GetWindowSize();
	for (int i = 0; i < swapchian->imageviews.size(); i++)
	{
		vk::FramebufferCreateInfo framebufferCI;
		framebufferCI.setAttachments(swapchian->imageviews[i])
			.setLayers(1)
			.setRenderPass(process->renderPass)
			.setWidth(width)
			.setHeight(height);
		framebuffers.emplace_back(device.createFramebuffer(framebufferCI));
	}
	renderer.reset(new Renderer(process.get(), framebuffers));

}

void Quit()
{
	auto device = Context::GetInstance().device;
	renderer.reset();
	for (auto framebuffer : framebuffers)
	{
		device.destroyFramebuffer(framebuffer);
	}
	process.reset();
	triangle.reset();
	ShaderPool::Quit();
	Context::GetInstance().DestroySwapchain();
	Context::Quit();
}

void Render()
{
	renderer->render();
}
