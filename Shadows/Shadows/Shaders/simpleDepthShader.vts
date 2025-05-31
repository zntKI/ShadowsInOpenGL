#version 330 core

layout (location = 0) in vec3 a_pos;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

void main()
{
	gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_pos, 1.0);
}