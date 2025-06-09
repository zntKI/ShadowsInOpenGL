#version 330 core

in VS_OUT {
	vec3 fragPos;
	vec3 normal;
	vec2 texCoords;

	vec4 fragPosLightSpace;
} i_fs;

uniform sampler2D u_TexDiffuse;

uniform sampler2D u_ShadowMap;

uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;

out vec4 o_fragColor;

float ShadowCalculation(vec3 fragmentNormal, vec3 lightDir);

void main()
{
	vec3 color = texture(u_TexDiffuse, i_fs.texCoords).rgb;
	vec3 normal = normalize(i_fs.normal);
	vec3 lightColor = vec3(1.0);

	// ambient
	vec3 ambient = 0.15 * color;

	//diffuse
	vec3 lightDir = normalize(u_LightPos - i_fs.fragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;

	//specular
	vec3 viewDir = normalize(u_ViewPos - i_fs.fragPos);
	float spec = 0.0;
	vec3 halfwayDir = normalize(lightDir + viewDir);
	spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
	vec3 specular = lightColor * spec;

	// Calculate shadow
	float shadowAmount = ShadowCalculation(normal, lightDir);
	vec3 lighting = ( ambient + (1.0 - shadowAmount) * (diffuse + specular) ) * color;

	o_fragColor = vec4(lighting, 1.0);
}

float ShadowCalculation(vec3 fragmentNormal, vec3 lightDir)
{
	vec4 fragPosLightSpace = i_fs.fragPosLightSpace;

	// perform perspective divide
	// because we don't pass 'fragPosLightSpace' through 'gl_Position', this step is skipped so we have to do it manually
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	float currentDepth = projCoords.z;
	if (currentDepth > 1.0)
		return 0.0;

	// Simple way to reduce shadow acne
	// float bias = 0.005;
	// Better way adapting the value based on the angle of the light source
	float bias = max(0.05 * (1.0 - dot(fragmentNormal, lightDir)), 0.005);

	// PCF:
	float shadowAmount = 0.0;
	vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadowAmount += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadowAmount /= 9.0;

	return shadowAmount;
}