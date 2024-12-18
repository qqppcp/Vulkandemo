#include "mesh.h"
#include "log.h"
#include "Texture.h"
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>
#include <format>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

using namespace std;

namespace {
	std::string getFileExtension(const std::string& filePath) {
		size_t pos = filePath.find_last_of('.');
		if (pos != std::string::npos && pos != filePath.length() - 1) {
			return filePath.substr(pos + 1);
		}
		return ""; // 没有后缀时返回空字符串
	}
	void loadNode(Node* parent, const tinygltf::Node& node,uint32_t nodeIndex, const tinygltf::Model& model, Mesh* _mesh)
	{
		Node* newNode = new Node{};
		newNode->index = nodeIndex;
		newNode->parent = parent;
		newNode->name = node.name;
		newNode->skinIndex = node.skin;
		newNode->matrix = glm::mat4(1.0f);

		// Generate local node matrix
		glm::vec3 translation = glm::vec3(0.0f);
		if (node.translation.size() == 3) {
			translation = glm::make_vec3(node.translation.data());
			newNode->translation = translation;
		}
		glm::mat4 rotation = glm::mat4(1.0f);
		if (node.rotation.size() == 4) {
			glm::quat q = glm::make_quat(node.rotation.data());
			rotation = glm::mat4_cast(q);
			newNode->rotation = glm::mat4(q);
		}
		glm::vec3 scale = glm::vec3(1.0f);
		if (node.scale.size() == 3) {
			scale = glm::make_vec3(node.scale.data());
			newNode->scale = scale;
		}
		glm::mat4 matrix = glm::mat4(1.0f);
		if (node.matrix.size() == 16) {
			newNode->matrix = glm::make_mat4x4(node.matrix.data());
			matrix = glm::make_mat4x4(node.matrix.data());
		}
		else
		{
			matrix = glm::translate(matrix, translation);
			matrix *= rotation;
			matrix = glm::scale(matrix, scale);
		}
		_mesh->transforms.push_back(matrix);

		// Node with children
		if (node.children.size() > 0)
		{
			for (auto i = 0; i < node.children.size(); i++)
			{
				loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, _mesh);
			}
		}

		// Node contains mesh data
		if (node.mesh > -1)
		{
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			/*if (mesh.primitives.size() != 1)
			{
				std::abort();
			}*/
			for (size_t j = 0; j < mesh.primitives.size(); j++)
			{
				const tinygltf::Primitive& primitive = mesh.primitives[j];
				if (primitive.indices < 0)
				{
					continue;
				}
				uint32_t indexStart = static_cast<uint32_t>(_mesh->indices.size());
				uint32_t vertexStart = static_cast<uint32_t>(_mesh->vertices.size());
				uint32_t indexCount = 0;
				uint32_t vertexCount = 0;
				glm::vec3 posMin{};
				glm::vec3 posMax{};
				bool hasSkin = false;
				// Vertices
				{
					const float* bufferPos = nullptr;
					const float* bufferNormals = nullptr;
					const float* bufferTexCoords = nullptr;
					const float* bufferColors = nullptr;
					const float* bufferTangents = nullptr;
					uint32_t numColorComponents;
					const uint16_t* bufferJoints = nullptr;
					const float* bufferWeights = nullptr;

					// Position attribute is required
					assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

					const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
					bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
					posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
					posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

					if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
					{
						const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
						bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
					}

					if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
						bufferTexCoords = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					}

					if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
					{
						const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
						const tinygltf::BufferView& colorView = model.bufferViews[colorAccessor.bufferView];
						// Color buffer are either of type vec3 or vec4
						numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
						bufferColors = reinterpret_cast<const float*>(&(model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
					}

					if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
					{
						const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
						const tinygltf::BufferView& tangentView = model.bufferViews[tangentAccessor.bufferView];
						bufferTangents = reinterpret_cast<const float*>(&(model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
					}

					// Skinning
					// Joints
					if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
						const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
						bufferJoints = reinterpret_cast<const uint16_t*>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
					}

					if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
						const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
						bufferWeights = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					}

					hasSkin = (bufferJoints && bufferWeights);
					vertexCount = static_cast<uint32_t>(posAccessor.count);
					for (size_t v = 0; v < posAccessor.count; v++)
					{
						Vertex vert{};
						vert.Position = glm::make_vec3(&bufferPos[v * 3]);
						vert.Normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
						vert.TexCoords = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec2(0.0f);
						vert.Tangent = bufferTangents ? glm::vec4(glm::make_vec4(&bufferTangents[v * 4])) : glm::vec4(0.0f);
						vert.materialId = primitive.material;
						//vert.applyTransform(matrix);
						_mesh->vertices.push_back(vert);
					}
				}
				// Indices
				{
					const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

					indexCount = static_cast<uint32_t>(accessor.count);

					switch (accessor.componentType)
					{
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
					{
						uint32_t* buf = new uint32_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
						for (size_t index = 0; index < accessor.count; index++)
						{
							_mesh->indices.push_back(buf[index] /*+ vertexStart*/);
						}
						delete[] buf;
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
					{
						uint16_t* buf = new uint16_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
						for (size_t index = 0; index < accessor.count; index++) {
							_mesh->indices.push_back(buf[index] /*+ vertexStart*/);
						}
						delete[] buf;
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
					{
						uint8_t* buf = new uint8_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
						for (size_t index = 0; index < accessor.count; index++) {
							_mesh->indices.push_back(buf[index] /*+ vertexStart*/);
						}
						delete[] buf;
						break;
					}
					default:
						DEMO_LOG(Error, std::format("Index component type {} not supported!", accessor.componentType));
						return;
					}
				}
				AABB aabb;
				aabb.minPos = posMin;
				aabb.maxPos = posMax;
				aabb.extent = (posMax - posMin) * 0.5f;
				aabb.center = posMin + aabb.extent;
				_mesh->aabbs.push_back(aabb);
				IndirectCommandAndMeshData indirectData;
				indirectData.command.setFirstIndex(indexStart)
					.setFirstInstance(0)
					.setIndexCount(indexCount)
					.setInstanceCount(1)
					.setVertexOffset(vertexStart);
				indirectData.meshId = _mesh->indirectDrawData.size();
				indirectData.materialIndex = primitive.material;
				_mesh->indirectDrawData.push_back(indirectData);
				_mesh->nodes.push_back(newNode);
			}
		}
		if (parent)
		{
			parent->children.push_back(newNode);
		}
		_mesh->linearNodes.push_back(newNode);
	}
}

namespace
{
	void generate_tangents(std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
		uint32_t index_count = indices.size();
		for (size_t i = 0; i < index_count; i += 3) {
			uint32_t i0 = indices[i + 0];
			uint32_t i1 = indices[i + 1];
			uint32_t i2 = indices[i + 2];

			glm::vec3 edge1 = vertices[i1].Position - vertices[i0].Position;
			glm::vec3 edge2 = vertices[i2].Position - vertices[i0].Position;
			glm::vec2 deltaUV1 = vertices[i1].TexCoords - vertices[i0].TexCoords;
			glm::vec2 deltaUV2 = vertices[i2].TexCoords - vertices[i0].TexCoords;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent = {
				(f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x)),
				(f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y)),
				(f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)) };

			tangent = glm::normalize(tangent);
			//float w = ((deltaUV1.y * deltaUV2.x - deltaUV2.y * deltaUV1.x) < 0.0f) ? -1.0f : 1.0f;

			glm::vec3 bitangent = {
				(f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x)),
				(f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y)),
				(f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)) };

			bitangent = glm::normalize(bitangent);
			float w = (glm::dot(glm::cross(vertices[i0].Normal, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;
			vertices[i0].Tangent = glm::vec4(tangent, w);
			vertices[i1].Tangent = glm::vec4(tangent, w);
			vertices[i2].Tangent = glm::vec4(tangent, w);
		}
	}
}

Mesh::~Mesh()
{
	for (auto texture : textures)
	{
		TextureManager::Instance().Destroy(texture);
		texture.reset();
	}
}


void Mesh::loadobj(string path)
{
	directory = path.substr(0, path.find_last_of('/'));
	tinyobj::ObjReaderConfig reader_config;
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path, reader_config))
	{
		if (!reader.Error().empty())
		{
			return;
		}
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();
	int total_vtx_num = 0;

	for (size_t s = 0; s < shapes.size(); s++)
	{
		total_vtx_num += shapes[s].mesh.num_face_vertices.size() * 3;
	}
	vertices.reserve(total_vtx_num);
	for (size_t s = 0; s < shapes.size(); s++)
	{
		int IndexStart = indices.size();
		int VertexStart = vertices.size();

		glm::vec3 posMin{10000.0f};
		glm::vec3 posMax{-10000.0f};

		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
			if (fv != 3)
			{
				return;
			}

			for (size_t v = 0; v < fv; v++)
			{
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				if (idx.texcoord_index >= 0 && idx.normal_index >= 0)
				{
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					Vertex vert;
					vert.Position = glm::vec3{ vx,  vy, vz };
					vert.Normal = glm::vec3{ nx,  ny, nz };
					vert.TexCoords = glm::vec2{ tx, ty };
					vertices.push_back(vert);
				}
				else
				{
					Vertex vert;
					vert.Position = glm::vec3{ vx,  vy, vz };
					vert.Normal = glm::vec3{ 0 };
					vert.TexCoords = glm::vec2{ 0 };
					vertices.push_back(vert);
				}
				posMin = glm::vec3(std::min(posMin.x, vertices.back().Position.x), std::min(posMin.y, vertices.back().Position.y), std::min(posMin.z, vertices.back().Position.z));
				posMax = glm::vec3(std::max(posMin.x, vertices.back().Position.x), std::max(posMin.y, vertices.back().Position.y), std::max(posMin.z, vertices.back().Position.z));
				vertices.back().materialId = shapes[s].mesh.material_ids[f];
				this->indices.push_back(this->indices.size());
			}
			index_offset += fv;
		}
		AABB aabb;
		aabb.minPos = posMin;
		aabb.maxPos = posMax;
		aabb.extent = (posMax - posMin) * 0.5f;
		aabb.center = posMin + aabb.extent;
		aabbs.push_back(aabb);
		IndirectCommandAndMeshData indirectData;
		indirectData.command.setFirstIndex(IndexStart)
			.setFirstInstance(0)
			.setIndexCount(index_offset)
			.setInstanceCount(1)
			.setVertexOffset(VertexStart);
		indirectData.meshId = indirectDrawData.size();
		indirectData.materialIndex = vertices.back().materialId;
		indirectDrawData.push_back(indirectData);
	}
	generate_tangents(vertices, indices);
	for (size_t i = 0; i < materials.size(); i++)
	{
		Material material;
		material.Ka_illum = glm::vec4(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2], materials[i].illum);
		material.Kd_dissolve = glm::vec4(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2], materials[i].dissolve);
		material.Ks_shininess = glm::vec4(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2], materials[i].shininess);
		material.emission_ior = glm::vec4(materials[i].emission[0], materials[i].emission[1], materials[i].emission[2], materials[i].ior);
		if (materials[i].ambient_texname != "")
		{
			material.reflectTextureId = textures.size();
			std::shared_ptr<Texture> reflectTex = TextureManager::Instance().Load(directory + '/' + materials[i].ambient_texname);
			textures.push_back(reflectTex);
		}
		if (materials[i].diffuse_texname != "")
		{
			material.diffuseTextureId = textures.size();
			std::shared_ptr<Texture> diffuseTex = TextureManager::Instance().Load(directory + '/' + materials[i].diffuse_texname);
			textures.push_back(diffuseTex);
		}
		if (materials[i].specular_texname != "")
		{
			material.specularTextureId = textures.size();
			std::shared_ptr<Texture> specularTex = TextureManager::Instance().Load(directory + '/' + materials[i].specular_texname);
			textures.push_back(specularTex);
		}
		if (materials[i].bump_texname != "")
		{
			material.normalTextureId = textures.size();
			std::shared_ptr<Texture> normalTex = TextureManager::Instance().Load(directory + '/' + materials[i].bump_texname);
			textures.push_back(normalTex);
		}
		this->materials.push_back(material);
	}
}

void Mesh::loadgltf(std::string path)
{
	stbi_set_flip_vertically_on_load(false);
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret = false;
	auto ext = getFileExtension(path);
	if (ext == "glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
	}
	if (ext == "gltf")
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	} 
	if (!warn.empty())
	{
		DEMO_LOG(Warning, "Warn: " + warn);
	}
	if (!err.empty())
	{
		DEMO_LOG(Error, "Err: " + err);
	}
	if (!ret)
	{
		DEMO_LOG(Error, "Failed to parse glTF");
		return;
	}

	for (auto& texture : model.textures)
	{
		vk::Format format = vk::Format::eR8G8B8A8Unorm;
		if (model.images[texture.source].bits == 16)
		{
			format = vk::Format::eR16G16B16A16Unorm;
		}
		auto t = TextureManager::Instance().Create(model.images[texture.source].image.data(), 
			model.images[texture.source].width, model.images[texture.source].height, 
			model.images[texture.source].bits / 8 * model.images[texture.source].component, format);
		textures.push_back(t);
	}
	//使用image中的数据有问题，还是使用index从bufferview中读取图像数据用stbimage读取，设置stbi_set_flip_vertically_on_load(false);
	// 20241114：发现在加载模型前设置非翻转就不用重新加载了
	//for (auto& texture : model.textures)
	//{
	//	auto image = model.images[texture.source];
	//	auto imageBufferView = image.bufferView;
	//	std::vector<uint8_t> imageData(model.bufferViews[imageBufferView].byteLength);
	//	memcpy(imageData.data(), model.buffers[0].data.data() + (int)model.bufferViews[imageBufferView].byteOffset, model.bufferViews[imageBufferView].byteLength);
	//	int width, height, channles;
	//	stbi_set_flip_vertically_on_load(false);
	//	auto date = stbi_load_from_memory(imageData.data(), imageData.size(), &width, &height, &channles, STBI_rgb_alpha);
	//	auto t = TextureManager::Instance().Create(date, width, height, vk::Format::eR8G8B8A8Unorm);
	//	textures.push_back(t);
	//}
	for (auto& mat : model.materials)
	{
		Material currentMat;
		currentMat.diffuseTextureId = mat.pbrMetallicRoughness.baseColorTexture.index;
		currentMat.specularTextureId = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
		currentMat.normalTextureId = mat.normalTexture.index;
		currentMat.emissiveTextureId = mat.emissiveTexture.index;

		if (currentMat.diffuseTextureId == -1 && mat.extensions["KHR_materials_pbrSpecularGlossiness"].Has("diffuseTexture"))
		{
			currentMat.diffuseTextureId = mat.extensions["KHR_materials_pbrSpecularGlossiness"].Get("diffuseTexture").Get("index").GetNumberAsInt();
		}
		if (currentMat.specularTextureId == -1 && mat.extensions["KHR_materials_pbrSpecularGlossiness"].Has("specularGlossinessTexture"))
		{
			currentMat.specularTextureId = mat.extensions["KHR_materials_pbrSpecularGlossiness"].Get("specularGlossinessTexture").Get("index").GetNumberAsInt();
		}
		if (mat.extensions["KHR_materials_pbrSpecularGlossiness"].Has("diffuseFactor"))
		{
			auto& factor = mat.extensions["KHR_materials_pbrSpecularGlossiness"].Get("diffuseFactor");
			currentMat.Kd_dissolve.x = factor.Get(0).GetNumberAsDouble();
			currentMat.Kd_dissolve.y = factor.Get(1).GetNumberAsDouble();
			currentMat.Kd_dissolve.z = factor.Get(2).GetNumberAsDouble();
		}
		if (mat.extensions["KHR_materials_pbrSpecularGlossiness"].Has("specularFactor"))
		{
			auto& factor = mat.extensions["KHR_materials_pbrSpecularGlossiness"].Get("specularFactor");
			currentMat.Ks_shininess.x = factor.Get(0).GetNumberAsDouble();
			currentMat.Ks_shininess.y = factor.Get(1).GetNumberAsDouble();
			currentMat.Ks_shininess.z = factor.Get(2).GetNumberAsDouble();
			currentMat.Ks_shininess.w = 32;
		}

		currentMat.baseColorFactor = glm::vec4(
			mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1],
			mat.pbrMetallicRoughness.baseColorFactor[2], mat.pbrMetallicRoughness.baseColorFactor[3]);

		currentMat.metallicFactor = mat.pbrMetallicRoughness.metallicFactor;
		currentMat.roughnessFactor = mat.pbrMetallicRoughness.roughnessFactor;
		currentMat.alphaCutoff = mat.alphaCutoff;
		if (mat.alphaMode == "OPAQUE")
		{
			currentMat.alphaMode = 0;
		}
		else if (mat.alphaMode == "BLEND")
		{
			currentMat.alphaMode = 1;
		}
		else if (mat.alphaMode == "MASK")
		{
			currentMat.alphaMode = 2;
		}
		materials.push_back(currentMat);
	}
	const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		const auto node = model.nodes[scene.nodes[i]];
		loadNode(nullptr,node, scene.nodes[i], model, this);
	}
	for (int i = 0; i < nodes.size(); i++)
	{
		const glm::mat4 localMatrix = nodes[i]->getMatrix();
		int start = indirectDrawData[i].command.vertexOffset;
		int end = vertices.size();
		glm::vec3 posMin{ 1000 };
		glm::vec3 posMax{ -1000 };
		if (i != nodes.size() - 1)
		{
			end = indirectDrawData[i + 1].command.vertexOffset;
		}
		for (int j = start; j < end; j++)
		{
			Vertex& vertex = vertices[j];
			vertex.Position = glm::vec3(localMatrix * glm::vec4(vertex.Position, 1.0f));
			vertex.Normal = glm::normalize(glm::mat3(localMatrix) * vertex.Normal);
			posMin = glm::vec3(std::min(posMin.x, vertex.Position.x), std::min(posMin.y, vertex.Position.y), std::min(posMin.z, vertex.Position.z));
			posMax = glm::vec3(std::max(posMax.x, vertex.Position.x), std::max(posMax.y, vertex.Position.y), std::max(posMax.z, vertex.Position.z));
		}
		AABB& aabb = aabbs[i];
		aabb.minPos = posMin;
		aabb.maxPos = posMax;
		aabb.extent = (posMax - posMin) * 0.5f;
		aabb.center = posMin + aabb.extent;
		//AABB& aabb = aabbs[i];
		//aabb.minPos = glm::vec3(glm::vec4(aabb.minPos, 1.0f) * localMatrix);
		//aabb.maxPos = glm::vec3(glm::vec4(aabb.maxPos, 1.0f) * localMatrix);
		//aabb.extent = (aabb.maxPos - aabb.minPos) * 0.5f;
		//aabb.center = aabb.minPos + aabb.extent;
	}
}

glm::mat4 Node::localMatrix()
{
	return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 Node::getMatrix()
{
	glm::mat4 m = localMatrix();
	Node* p = parent;
	while (p) {
		m = p->localMatrix() * m;
		p = p->parent;
	}
	return m;
}

Node::~Node()
{
	for (auto& child : children) {
		delete child;
	}
}

