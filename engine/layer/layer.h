#pragma once
#include <string>

class Layer
{
public:
	Layer(const std::string& name = "Layer")
		: m_LayerName(name) {}
	virtual ~Layer() {}

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate(float _deltatime) {}
	virtual void OnRender() {}
	virtual void OnEvent() {}

	inline const std::string& GetName() const { return m_LayerName; }
private:
	std::string m_LayerName;
};