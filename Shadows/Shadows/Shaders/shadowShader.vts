#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texCoords;

uniform mat4 u_Proj;
uniform mat4 u_View;
uniform mat4 u_Model;
uniform mat3 u_NormalMat;

uniform mat4 u_LightSpaceMatrix;

out VS_OUT {
	vec3 fragPos;
	vec3 normal;
	vec2 texCoords;

	vec4 fragPosLightSpace;
} o_vs;

void main()
{
	o_vs.fragPos = vec3(u_Model * vec4(a_pos, 1.0));
    o_vs.normal = u_NormalMat * a_normal;
    o_vs.texCoords = a_texCoords;

	o_vs.fragPosLightSpace = u_LightSpaceMatrix * vec4(o_vs.fragPos, 1.0);
	gl_Position = u_Proj * u_View * u_Model * vec4(a_pos, 1.0);
}