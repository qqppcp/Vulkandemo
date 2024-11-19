#include "geometry.h"
#include "mesh.h"

std::unique_ptr<GeometryManager> GeometryManager::instance = nullptr;

GeometryManager::~GeometryManager()
{
	for (auto m : m_Contain)
	{
		m.second.reset();
	}
	m_Contain.clear();
}

void GeometryManager::Init()
{
	instance.reset(new GeometryManager);
}

void GeometryManager::Quit()
{
	instance.reset();
}

GeometryManager& GeometryManager::GetInstance()
{
	if (!instance)
	{
		throw("No Init Geometry Manager.");
	}
	return *instance;
}

std::shared_ptr<Mesh> GeometryManager::loadobj(std::string name)
{
	std::shared_ptr<Mesh> mesh;
	mesh.reset(new Mesh());
	mesh->loadobj(name);
	m_Contain[name] = mesh;
	return mesh;
}

std::shared_ptr<Mesh> GeometryManager::loadgltf(std::string name)
{
	std::shared_ptr<Mesh> mesh;
	mesh.reset(new Mesh());
	mesh->loadgltf(name);
	m_Contain[name] = mesh;
	return mesh;
}

std::shared_ptr<Mesh> GeometryManager::getMesh(std::string name)
{
	if (m_Contain.find(name) == m_Contain.end())
	{
		throw("Geometry No Exise.");
	}
	return m_Contain[name];
}

GeometryManager::GeometryManager()
{
	std::shared_ptr<Mesh> mesh;
	mesh.reset(new Mesh());
	mesh->vertices.resize(36);
	{
		mesh->vertices[0].Position = { -1.0f, -1.0f, -1.0f }; mesh->vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh->vertices[0].TexCoords = { 0.0f, 0.0f };
		mesh->vertices[1].Position = { 1.0f,  1.0f, -1.0f }; mesh->vertices[1].Normal = { 0.0f,  0.0f, -1.0f }; mesh->vertices[1].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[2].Position = { 1.0f, -1.0f, -1.0f }; mesh->vertices[2].Normal = { 0.0f,  0.0f, -1.0f }; mesh->vertices[2].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[3].Position = { 1.0f,  1.0f, -1.0f }; mesh->vertices[3].Normal = { 0.0f,  0.0f, -1.0f }; mesh->vertices[3].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[4].Position = { -1.0f, -1.0f, -1.0f }; mesh->vertices[4].Normal = { 0.0f,  0.0f, -1.0f }; mesh->vertices[4].TexCoords = { 0.0f, 0.0f };
		mesh->vertices[5].Position = { -1.0f,  1.0f, -1.0f }; mesh->vertices[5].Normal = { 0.0f,  0.0f, -1.0f }; mesh->vertices[5].TexCoords = { 0.0f, 1.0f };

		mesh->vertices[6].Position = { -1.0f, -1.0f,  1.0f }; mesh->vertices[6].Normal = { 0.0f,  0.0f, 1.0f }; mesh->vertices[6].TexCoords = { 0.0f, 0.0f };
		mesh->vertices[7].Position = { 1.0f, -1.0f,  1.0f }; mesh->vertices[7].Normal = { 0.0f,  0.0f, 1.0f }; mesh->vertices[7].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[8].Position = { 1.0f,  1.0f,  1.0f }; mesh->vertices[8].Normal = { 0.0f,  0.0f, 1.0f }; mesh->vertices[8].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[9].Position = { 1.0f,  1.0f,  1.0f }; mesh->vertices[9].Normal = { 0.0f,  0.0f, 1.0f }; mesh->vertices[9].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[10].Position = { -1.0f,  1.0f,  1.0f }; mesh->vertices[10].Normal = { 0.0f,  0.0f, 1.0f }; mesh->vertices[10].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[11].Position = { -1.0f, -1.0f,  1.0f }; mesh->vertices[11].Normal = { 0.0f,  0.0f, 1.0f }; mesh->vertices[11].TexCoords = { 0.0f, 0.0f };

		mesh->vertices[12].Position = { -1.0f,  1.0f,  1.0f }; mesh->vertices[12].Normal = { -1.0f,  0.0f,  0.0f }; mesh->vertices[12].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[13].Position = { -1.0f,  1.0f, -1.0f }; mesh->vertices[13].Normal = { -1.0f,  0.0f,  0.0f }; mesh->vertices[13].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[14].Position = { -1.0f, -1.0f, -1.0f }; mesh->vertices[14].Normal = { -1.0f,  0.0f,  0.0f }; mesh->vertices[14].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[15].Position = { -1.0f, -1.0f, -1.0f }; mesh->vertices[15].Normal = { -1.0f,  0.0f,  0.0f }; mesh->vertices[15].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[16].Position = { -1.0f, -1.0f,  1.0f }; mesh->vertices[16].Normal = { -1.0f,  0.0f,  0.0f }; mesh->vertices[16].TexCoords = { 0.0f, 0.0f };
		mesh->vertices[17].Position = { -1.0f,  1.0f,  1.0f }; mesh->vertices[17].Normal = { -1.0f,  0.0f,  0.0f }; mesh->vertices[17].TexCoords = { 1.0f, 0.0f };

		mesh->vertices[18].Position = { 1.0f,  1.0f,  1.0f }; mesh->vertices[18].Normal = { 1.0f,  0.0f,  0.0f }; mesh->vertices[18].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[19].Position = { 1.0f, -1.0f, -1.0f }; mesh->vertices[19].Normal = { 1.0f,  0.0f,  0.0f }; mesh->vertices[19].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[20].Position = { 1.0f,  1.0f, -1.0f }; mesh->vertices[20].Normal = { 1.0f,  0.0f,  0.0f }; mesh->vertices[20].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[21].Position = { 1.0f, -1.0f, -1.0f }; mesh->vertices[21].Normal = { 1.0f,  0.0f,  0.0f }; mesh->vertices[21].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[22].Position = { 1.0f,  1.0f,  1.0f }; mesh->vertices[22].Normal = { 1.0f,  0.0f,  0.0f }; mesh->vertices[22].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[23].Position = { 1.0f, -1.0f,  1.0f }; mesh->vertices[23].Normal = { 1.0f,  0.0f,  0.0f }; mesh->vertices[23].TexCoords = { 0.0f, 0.0f };

		mesh->vertices[24].Position = { -1.0f, -1.0f, -1.0f }; mesh->vertices[24].Normal = { 0.0f, -1.0f,  0.0f }; mesh->vertices[24].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[25].Position = { 1.0f, -1.0f, -1.0f }; mesh->vertices[25].Normal = { 0.0f, -1.0f,  0.0f }; mesh->vertices[25].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[26].Position = { 1.0f, -1.0f,  1.0f }; mesh->vertices[26].Normal = { 0.0f, -1.0f,  0.0f }; mesh->vertices[26].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[27].Position = { 1.0f, -1.0f,  1.0f }; mesh->vertices[27].Normal = { 0.0f, -1.0f,  0.0f }; mesh->vertices[27].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[28].Position = { -1.0f, -1.0f,  1.0f }; mesh->vertices[28].Normal = { 0.0f, -1.0f,  0.0f }; mesh->vertices[28].TexCoords = { 0.0f, 0.0f };
		mesh->vertices[29].Position = { -1.0f, -1.0f, -1.0f }; mesh->vertices[29].Normal = { 0.0f, -1.0f,  0.0f }; mesh->vertices[29].TexCoords = { 0.0f, 1.0f };

		mesh->vertices[30].Position = { -1.0f,  1.0f, -1.0f }; mesh->vertices[30].Normal = { 0.0f,  1.0f,  0.0f }; mesh->vertices[30].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[31].Position = { 1.0f,  1.0f , 1.0f }; mesh->vertices[31].Normal = { 0.0f,  1.0f,  0.0f }; mesh->vertices[31].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[32].Position = { 1.0f,  1.0f, -1.0f }; mesh->vertices[32].Normal = { 0.0f,  1.0f,  0.0f }; mesh->vertices[32].TexCoords = { 1.0f, 1.0f };
		mesh->vertices[33].Position = { 1.0f,  1.0f,  1.0f }; mesh->vertices[33].Normal = { 0.0f,  1.0f,  0.0f }; mesh->vertices[33].TexCoords = { 1.0f, 0.0f };
		mesh->vertices[34].Position = { -1.0f,  1.0f, -1.0f }; mesh->vertices[34].Normal = { 0.0f,  1.0f,  0.0f }; mesh->vertices[34].TexCoords = { 0.0f, 1.0f };
		mesh->vertices[35].Position = { -1.0f,  1.0f,  1.0f }; mesh->vertices[35].Normal = { 0.0f,  1.0f,  0.0f }; mesh->vertices[35].TexCoords = { 0.0f, 0.0f };
	}
	for (int i = 0; i < 36; i++)
	{
		mesh->indices.push_back(i);
	}
	m_Contain["cube"] = mesh;
	mesh.reset();
	std::shared_ptr<Mesh> sphere;
	sphere.reset(new Mesh());
	const uint32_t X_SEGMENTS = 64;
	const uint32_t Y_SEGMENTS = 64;
	const float PI = 3.14159265359f;
	for (int x = 0; x <= X_SEGMENTS; x++)
	{
		for (int y = 0; y <= Y_SEGMENTS; y++)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			Vertex vert;
			vert.Position = glm::vec3(xPos, yPos, zPos);
			vert.Normal = glm::vec3(xPos, yPos, zPos);
			vert.TexCoords = glm::vec2(xSegment, ySegment);
			sphere->vertices.push_back(vert);
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				sphere->indices.push_back(y * (X_SEGMENTS + 1) + x);
				sphere->indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				sphere->indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				sphere->indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
	// TODO: 画圆需要设置图元为triangle strip
}
