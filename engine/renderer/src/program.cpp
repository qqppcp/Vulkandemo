#include "program.h"

GPUProgram::GPUProgram(std::string comp)
{
	Compute.reset(new Shader(comp));
	stages.resize(1);
	stages[0].setStage(vk::ShaderStageFlagBits::eCompute)
		.setModule(Compute->module)
		.setPName("main");
}

GPUProgram::GPUProgram(std::string vertex, std::string fragment)
{
	Vertex.reset(new Shader(vertex));
	Fragment.reset(new Shader(fragment));
	stages.resize(2);
	stages[0].setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(Vertex->module)
		.setPName("main");
	stages[1].setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(Fragment->module)
		.setPName("main");
}

GPUProgram::GPUProgram(std::string vertex, std::string geometry, std::string fragment)
{
	Vertex.reset(new Shader(vertex));
	Geometry.reset(new Shader(geometry));
	Fragment.reset(new Shader(fragment));
	stages.resize(3);
	stages[0].setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(Vertex->module)
		.setPName("main");
	stages[0].setStage(vk::ShaderStageFlagBits::eGeometry)
		.setModule(Geometry->module)
		.setPName("main");
	stages[2].setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(Fragment->module)
		.setPName("main");
}

GPUProgram::~GPUProgram()
{
	if (Vertex.get())
	{
		Vertex->destroy();
	}
	if (TessellationControl.get())
	{
		TessellationControl->destroy();
	}
	if (TessellationEvaluation.get())
	{
		TessellationEvaluation->destroy();
	}
	if (Geometry.get())
	{
		Geometry->destroy();
	}
	if (Fragment.get())
	{
		Fragment->destroy();
	}
	if (Compute.get())
	{
		Compute->destroy();
	}
}
