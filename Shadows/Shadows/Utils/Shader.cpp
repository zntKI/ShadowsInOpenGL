#include "Shader.hpp"

#include <fstream>
#include <sstream>

#include <GL/glew.h>

Shader::Shader( const char* vertexPath, const char* fragmentPath, const char* geometryPath )
{
	// 1. read code from file

	std::string vertexCode;
	readCodeFromFile( vertexPath, vertexCode );
	const char* vShaderCode = vertexCode.c_str();

	std::string fragmentCode;
	readCodeFromFile( fragmentPath, fragmentCode );
	const char* fShaderCode = fragmentCode.c_str();

	// 2. create and compile shaders

	unsigned int vertex;
	createCompileShader( GL_VERTEX_SHADER, vertex, vShaderCode );
	unsigned int fragment;
	createCompileShader( GL_FRAGMENT_SHADER, fragment, fShaderCode );

	// 3. shader program

	this->ID = glCreateProgram();
	glAttachShader( this->ID, vertex );
	glAttachShader( this->ID, fragment );

	// geometry shader
	if ( geometryPath ) {
		std::string geometryCode;
		readCodeFromFile( geometryPath, geometryCode );
		const char* gShaderCode = geometryCode.c_str();

		unsigned int geometry;
		createCompileShader( GL_GEOMETRY_SHADER, geometry, gShaderCode );

		glAttachShader( this->ID, geometry );

		glDeleteShader( geometry );
	}

	glLinkProgram( this->ID );

	// print linking errors if any
	glGetProgramiv( this->ID, GL_LINK_STATUS, &success );
	if ( !success ) {
		glGetProgramInfoLog( this->ID, 512, NULL, infoLog );
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
			infoLog << std::endl;
	}

	this->use();


	// 4. delete shaders
	glDeleteShader( vertex );
	glDeleteShader( fragment );
}

void Shader::readCodeFromFile( const char* path, std::string& shaderCode )
{
	std::ifstream shaderFile;

	// ensure ifstream objects can throw exceptions:
	shaderFile.exceptions( std::ifstream::failbit | std::ifstream::badbit );

	try {
		shaderFile.open( path );

		std::stringstream shaderStream;

		shaderStream << shaderFile.rdbuf();

		shaderFile.close();

		shaderCode = shaderStream.str();
	}
	catch ( std::ifstream::failure e ) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSSFULLY_READ" << std::endl;
	}
}

void Shader::createCompileShader( GLenum SHADER_TYPE, unsigned int& shaderID, const char* shaderCode )
{
	switch ( SHADER_TYPE ) {
	case GL_VERTEX_SHADER:
		shaderID = glCreateShader( GL_VERTEX_SHADER );
		break;
	case GL_FRAGMENT_SHADER:
		shaderID = glCreateShader( GL_FRAGMENT_SHADER );
		break;
	case GL_GEOMETRY_SHADER:
		shaderID = glCreateShader( GL_GEOMETRY_SHADER );
		break;
	default:
		std::cout << "ERROR::SHADER::CREATE::INCORRECT_SHADER_TYPE" << std::endl;
		return;
	}

	glShaderSource( shaderID, 1, &shaderCode, nullptr );
	glCompileShader( shaderID );

	glGetShaderiv( shaderID, GL_COMPILE_STATUS, &success );
	if ( success == GL_FALSE ) {
		glGetShaderInfoLog( shaderID, 512, NULL, infoLog );
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" <<
			infoLog << std::endl;
	}
}

Shader::~Shader()
{
	glDeleteProgram( this->ID );
}

void Shader::use()
{
	glUseProgram( this->ID );
}

void Shader::setBool( const std::string& name, bool value ) const
{
	glUniform1i( glGetUniformLocation( this->ID, name.c_str() ), ( int ) value );
}

void Shader::setInt( const std::string& name, int value ) const
{
	glUniform1i( glGetUniformLocation( this->ID, name.c_str() ), value );
}

void Shader::setFloat( const std::string& name, float value ) const
{
	glUniform1f( glGetUniformLocation( this->ID, name.c_str() ), value );
}

void Shader::setVec2( const std::string& name, glm::vec2 vec ) const
{
	glUniform2fv( glGetUniformLocation( this->ID, name.c_str() ), 1, glm::value_ptr( vec ) );
}

void Shader::setVec3( const std::string& name, glm::vec3 vec ) const
{
	glUniform3fv( glGetUniformLocation( this->ID, name.c_str() ), 1, glm::value_ptr( vec ) );
}

void Shader::setVec3( const std::string& name, float x, float y, float z ) const
{
	glUniform3f( glGetUniformLocation( this->ID, name.c_str() ), x, y, z );
}

void Shader::setVec4( const std::string& name, glm::vec4 vec ) const
{
	glUniform4fv( glGetUniformLocation( this->ID, name.c_str() ), 1, glm::value_ptr( vec ) );
}

void Shader::setMatrix4( const std::string& name, glm::mat4 matrix ) const
{
	glUniformMatrix4fv( glGetUniformLocation( this->ID, name.c_str() ), 1, GL_FALSE, glm::value_ptr( matrix ) );
}

void Shader::setMatrix3( const std::string& name, glm::mat3 matrix ) const
{
	glUniformMatrix3fv( glGetUniformLocation( this->ID, name.c_str() ), 1, GL_FALSE, glm::value_ptr( matrix ) );
}