#pragma once
#include "Shader.h"

class ShaderPool final {
public:
	static vk::Result Initialize();
	static void Quit();
	static ShaderPool& GetInstance() { return *instance; }
	Shader CreateShader(const std::string filename, const std::string_view name = "");
	~ShaderPool();
private:
	static std::unique_ptr<ShaderPool> instance;
	ShaderPool();
	std::vector<Shader> m_Shaders;
};