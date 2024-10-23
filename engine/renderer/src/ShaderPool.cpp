#include "ShaderPool.h"

std::unique_ptr<ShaderPool> ShaderPool::instance = nullptr;

vk::Result ShaderPool::Initialize()
{
	if (!instance)
	{
		instance.reset(new ShaderPool);
	}
	return vk::Result::eSuccess;
}

void ShaderPool::Quit()
{
	instance.reset();
}

Shader ShaderPool::CreateShader(const std::string filename, const std::string_view name)
{
	Shader shader(filename);
	for (Shader _shader : m_Shaders)
	{
		if (name == _shader.name)
			return std::move(_shader);
	}
	shader.name = name;
	if (name == "")
	{
		shader.name = std::to_string(m_Shaders.size());
	}
	m_Shaders.emplace_back(shader);
	return std::move(shader);
}

ShaderPool::~ShaderPool()
{
	for (auto shader : m_Shaders)
	{
		shader.destroy();
	}
}

ShaderPool::ShaderPool()
{
}
