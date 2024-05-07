#include "program.h"

GPUProgram::GPUProgram(std::string vertex, std::string fragment)
	: Vertex(vertex), Fragment(fragment)
{
	stages.resize(2);
	stages[0].setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(Vertex.value().module)
		.setPName("main");
	stages[1].setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(Fragment.value().module)
		.setPName("main");
}

GPUProgram::GPUProgram(std::string vertex, std::string geometry, std::string fragment)
	: Vertex(vertex), Geometry(geometry), Fragment(fragment)
{
	stages.resize(3);
	stages[0].setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(Vertex.value().module)
		.setPName("main");
	stages[0].setStage(vk::ShaderStageFlagBits::eGeometry)
		.setModule(Geometry.value().module)
		.setPName("main");
	stages[2].setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(Fragment.value().module)
		.setPName("main");
}

GPUProgram::~GPUProgram()
{
	if (Vertex.has_value())
	{
		Vertex->destroy();
	}
	if (TessellationControl.has_value())
	{
		TessellationControl->destroy();
	}
	if (TessellationEvaluation.has_value())
	{
		TessellationEvaluation->destroy();
	}
	if (Geometry.has_value())
	{
		Geometry->destroy();
	}
	if (Fragment.has_value())
	{
		Fragment->destroy();
	}
}
