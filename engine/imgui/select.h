#pragma once
#include "ImGuiBase.h"
#include "layerFactory.h"
#include <memory>

class Layer;

class SelectAPP : public ImGuiBase
{
public:

	virtual void customUI() override;

private:
	std::unique_ptr<Layer> currentLayer;
	LayerFactory layerFactory;
	std::vector<std::string> appNames = { "ModelForward" };
	int selectedAppIndex = 0;
};