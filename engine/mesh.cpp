#include "mesh.h"
#include "log.h"
#include "Texture.h"

#include <iostream>
#include <memory>
#include <format>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace std;

namespace {
	void processNode(aiNode* node, const aiScene* scene, Mesh* _mesh);
	void processMesh(aiMesh* mesh, const aiScene* scene, Mesh* _mesh);
	//std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode* node, const aiScene* scene, Mesh* _mesh)
	{
		// process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			processMesh(mesh, scene, _mesh);
		}
		// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene, _mesh);
		}

	}

	void processMesh(aiMesh* mesh, const aiScene* scene, Mesh* _mesh)
	{
		// data to fill
		uint32_t vertexOffset = _mesh->vertices.size();
		vector<Texture> textures;

		// walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if (mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
				// tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.Tangent = vector;
				// bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.Bitangent = vector;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			_mesh->vertices.push_back(vertex);
		}
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				_mesh->indices.push_back(face.mIndices[j] + vertexOffset);
		}
		// process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN

		//// 1. diffuse maps
		//vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		//if (diffuseMaps.size() == 0)
		//{
		//	diffuseMaps.push_back(Texture{ textureID , "texture_diffuse" , "" });
		//}
		//textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		//// 2. specular maps
		//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		//if (specularMaps.size() == 0)
		//{
		//	specularMaps.push_back(Texture{ textureID , "texture_specular" , "" });
		//}
		//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		//// 3. normal maps
		//std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		//if (normalMaps.size() == 0)
		//{
		//	normalMaps.push_back(Texture{ textureID , "texture_normal" , "" });
		//}
		//textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		//// 4. height maps
		//vector<Texture> reflectionMaps = this->loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_reflection");
		//if (reflectionMaps.size() == 0)
		//{
		//	reflectionMaps.push_back(Texture{ textureID , "texture_reflection" , "" });
		//}
		//textures.insert(textures.end(), reflectionMaps.begin(), reflectionMaps.end());

		//// return a mesh object created from the extracted mesh data
		//return Mesh(vertices, indices, textures);
	}

	//// checks all material textures of a given type and loads the textures if they're not loaded yet.
	//// the required info is returned as a Texture struct.
	//vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
	//{
	//	vector<Texture> textures;
	//	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	//	{
	//		aiString str;
	//		mat->GetTexture(type, i, &str);
	//		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
	//		bool skip = false;
	//		for (unsigned int j = 0; j < textures_loaded.size(); j++)
	//		{
	//			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
	//			{
	//				textures.push_back(textures_loaded[j]);
	//				skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
	//				break;
	//			}
	//		}
	//		if (!skip)
	//		{   // if texture hasn't been loaded already, load it
	//			Texture texture;
	//			texture.id = TextureFromFile(str.C_Str(), this->directory);
	//			texture.type = typeName;
	//			texture.path = str.C_Str();
	//			textures.push_back(texture);
	//			textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
	//		}
	//	}
	//	return textures;
	//}
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

			glm::vec3 bitangent = {
				(f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x)),
				(f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y)),
				(f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)) };

			bitangent = glm::normalize(bitangent);

			vertices[i0].Tangent = tangent;
			vertices[i1].Tangent = tangent;
			vertices[i2].Tangent = tangent;
			vertices[i0].Bitangent = bitangent;
			vertices[i1].Bitangent = bitangent;
			vertices[i2].Bitangent = bitangent;
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

void Mesh::load(std::string path)
{
	Assimp::Importer importer;
	const auto scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		DEMO_LOG(Error, std::format("ASSIMP:: {}", importer.GetErrorString()));
	}
	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene, this);
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
					vertices.push_back(Vertex{ glm::vec3{ vx,  vy, vz }, glm::vec3{ nx,  ny, nz }, glm::vec2{ tx, ty }, });
				}
				else
				{
					vertices.push_back(Vertex{ glm::vec3{ vx,  vy, vz }, glm::vec3{ 0 }, glm::vec2{ 0, 0 } });
				}
				vertices.back().materialId = shapes[s].mesh.material_ids[f];
				this->indices.push_back(this->indices.size());
			}

			index_offset += fv;
		}
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
			std::shared_ptr<Texture> reflectTex =  TextureManager::Instance().Load(directory + '/' + materials[i].ambient_texname);
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