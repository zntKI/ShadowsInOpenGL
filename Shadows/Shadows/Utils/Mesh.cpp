#include "Mesh.hpp"

#include <GL/glew.h>

Mesh::Mesh( std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures )
	: vertices( vertices ), indices( indices ), textures( textures ),
	VAO( 0 ), VBO( 0 ), EBO( 0 )
{
	setupMesh();
}

void Mesh::setupMesh()
{
	glGenVertexArrays( 1, &VAO );
	glGenBuffers( 1, &VBO );
	glGenBuffers( 1, &EBO );

	glBindVertexArray( VAO );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );

	glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( Vertex ),
		&vertices[ 0 ], GL_STATIC_DRAW );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( unsigned int ),
		&indices[ 0 ], GL_STATIC_DRAW );

	// vertex position
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* ) 0 );

	// vertex normals
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* ) offsetof( Vertex, Normal ) );

	// vertex texture coordinates
	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* ) offsetof( Vertex, TexCoords ) );

	glBindVertexArray( 0 );
}

void Mesh::Draw( Shader& shader )
{
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	for ( unsigned int i = 0; i < textures.size(); i++ ) {
		// retrieve texture number (the N in diffuse_textureN)
		std::string number;
		std::string name = textures[ i ].type;
		if ( name == "texture_diffuse" )
			number = std::to_string( diffuseNr++ );
		else if ( name == "texture_specular" )
			number = std::to_string( specularNr++ );

		glActiveTexture( GL_TEXTURE0 + i );

		// tell the sampler2D in which texture unit to look at
		std::string str = name + number;
		std::string fullName = /*"u_Material." + */str;
		shader.setInt(fullName.c_str(), i );

		glBindTexture( GL_TEXTURE_2D, textures[ i ].id );
	}

	glActiveTexture( GL_TEXTURE0 );

	// draw mesh
	glBindVertexArray( VAO );
	glDrawElements( GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0 );
	glBindVertexArray( 0 );
}