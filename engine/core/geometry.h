#pragma once
#include <unordered_map>
#include <string>
#include <memory>

struct Mesh;

class GeometryManager
{
public:
	~GeometryManager();
	static void Init();
	static void Quit();
	static GeometryManager& GetInstance();
	std::shared_ptr<Mesh> loadobj(std::string name);
	std::shared_ptr<Mesh> getMesh(std::string name);
	
private:
	GeometryManager();
	static std::unique_ptr<GeometryManager> instance;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Contain;
};
