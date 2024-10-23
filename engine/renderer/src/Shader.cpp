#include "Shader.h"
#include "Context.h"
#include "log.h"

#include <filesystem>
#include <fstream>

Shader::Shader(const std::string& filename)
{
	if (!std::filesystem::exists(filename))
	{
		DEMO_LOG(Error, std::format("Failed to find external shader file: {}", filename));
	}
	std::vector<uint32_t> shaderData;
	std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if (!file.is_open())
		DEMO_LOG(Error, std::format("Failed to open external shader file: {}", filename));
	std::streampos size = file.tellg();
	if (size % 4)
		DEMO_LOG(Error, std::format("File Size is {} bytes, it isn't 4 times", (int)size));

	name = "main";
	file.seekg(0);
	shaderData.resize(size / 4);
	file.read(reinterpret_cast<char*>(shaderData.data()), size);
	vk::ShaderModuleCreateInfo shaderCI;
	shaderCI.setCode(shaderData);
	module = Context::GetInstance().device.createShaderModule(shaderCI);
}

Shader::~Shader()
{
}

void Shader::destroy()
{
	Context::GetInstance().device.destroyShaderModule(module);
}
