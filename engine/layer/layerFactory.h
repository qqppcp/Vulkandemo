#pragma once
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
	
class Layer;
// TODO:应该设置AppFactory，Layer现在只涉及一次drawcall，还不是完整的流程。
class LayerFactory
{
public:
	using LayerCreateFunc = std::function<std::unique_ptr<Layer>()>;
	LayerFactory();
	void Register(const std::string& name, LayerCreateFunc createFunc);
	std::unique_ptr<Layer> CreateLayer(const std::string& name);
private:
	std::unordered_map<std::string, LayerCreateFunc> registry;
};