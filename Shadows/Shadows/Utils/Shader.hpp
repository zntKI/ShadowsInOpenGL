#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

class Shader
{
public:
	// program ID
	unsigned int ID;

	Shader( const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr );
private:
	// for debugging
	int success;
	char infoLog[ 512 ];

	void readCodeFromFile( const char* path, std::string& shaderCode );
	void createCompileShader( GLenum SHADER_TYPE, unsigned int& shaderID, const char* shaderCode );

public:
	~Shader();

	void use();

	void setBool( const std::string& name, bool value ) const;
	void setInt( const std::string& name, int value ) const;
	void setFloat( const std::string& name, float value ) const;

	void setVec2( const std::string& name, glm::vec2 vec ) const;
	void setVec3( const std::string& name, glm::vec3 vec ) const;
	void setVec3( const std::string& name, float x, float y, float z ) const;
	void setVec4( const std::string& name, glm::vec4 vec ) const;
	void setMatrix4( const std::string& name, glm::mat4 matrix ) const;
	void setMatrix3( const std::string& name, glm::mat3 matrix ) const;
};