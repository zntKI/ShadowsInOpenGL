#pragma once

#include "Mesh.hpp"
#include "Shader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <string>

unsigned int TextureFromFile( const char* path, const std::string& directory, bool gamma = false );

class Model
{
public:
	Model( const char* path );
	void Draw( Shader& shader );

private:
	std::vector<Mesh> meshes;

	// model data
	std::vector<Texture> textures_loaded;
	std::string directory;

	void loadModel( std::string path );
	void processNode( aiNode* node, const aiScene* scene );
	Mesh processMesh( aiMesh* mesh, const aiScene* scene );
	std::vector<Texture> loadMaterialTextures( aiMaterial* mat,
		aiTextureType type, std::string typeName );
};