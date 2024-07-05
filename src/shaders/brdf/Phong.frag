#version 450#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 1) uniform sampler2D tex;layout(set = 0, binding = 0) uniform LightUBO {
	vec3 lightDir;
	vec4 lightCol;
	vec3 eyePos;} lubo;vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 mDiffuse, vec3 mSpecular) {
	vec3 Diffuse = mDiffuse * max(dot(N, L), 0.0f);
	//vec3 Diffuse = mDiffuse;
	vec3 Specular = mSpecular * vec3(pow(max(dot(V, -reflect(L, N)), 0.0f), 150.0f));
	//vec3 Specular = vec3(pow(max(dot(V, -reflect(L, N)), 0.0f), 150.0f));
	
	return Diffuse + Specular;
}

void main() {
	vec3 EyeDir = normalize(lubo.eyePos - fragPos);
	vec3 Norm = normalize(fragNorm);
	vec3 L = lubo.lightDir;
	vec3 Albedo = texture(tex, fragUV).rgb;	vec3 Fun = BRDF(EyeDir, Norm, L, Albedo, vec3(1.0f));	vec3 lightCol = lubo.lightCol.rgb;	outColor = vec4(Fun * lightCol.rgb, 1.0f);}