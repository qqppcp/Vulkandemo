#include "layerFactory.h"
#include "modelforwardLayer.h"

LayerFactory::LayerFactory()
{
	//Register("ModelForward", []() {return std::make_unique<ModelForwardLayer>(); });
}

void LayerFactory::Register(const std::string& name, LayerCreateFunc createFunc)
{
	registry[name] = createFunc;
}

std::unique_ptr<Layer> LayerFactory::CreateLayer(const std::string& name)
{
	auto it = registry.find(name);
	if (it != registry.end())
	{
		return it->second();
	}
	return nullptr;
}
