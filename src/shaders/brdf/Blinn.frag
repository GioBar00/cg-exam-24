#version 450#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 1) uniform sampler2D tex;layout(set = 0, binding = 0) uniform LightUBO {
	vec3 lightPos;
	vec3 lightDir;
	vec4 lightCol;
	float cosIn;
	float cosOut;
	int TYPE;
	vec3 eyePos;} lubo;


vec3 directDir() {
	return lubo.lightDir;
}

vec3 directCol() {
	return lubo.lightCol.rgb;
}

vec3 pointDir(vec3 fragmentPos) {
	return normalize(lubo.lightPos - fragmentPos);
}

vec3 pointCol(vec3 fragmentPos) {
	return pow(lubo.lightCol.a / length(lubo.lightPos - fragmentPos), 2.0) * lubo.lightCol.rgb;
}

vec3 spotDir(vec3 fragmentPos) {
	return pointDir(fragmentPos);
}

vec3 spotCol(vec3 fragmentPos) {
	float ext = clamp((dot(normalize(lubo.lightPos - fragmentPos), lubo.lightDir) - lubo.cosOut) / (lubo.cosIn - lubo.cosOut), 0.0, 1.0); // Extended light model factor.
	return ext * pointCol(fragmentPos);
}


vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 mDiffuse, vec3 mSpecular) {
	vec3 Diffuse = mDiffuse * max(dot(N, L), 0.0f);
	//vec3 Diffuse = mDiffuse;
	vec3 Specular = mSpecular * vec3(pow(max(dot(N, normalize(V + L)), 0.0f), 200.0f));
	//vec3 Specular = vec3(pow(max(dot(N, normalize(V + L)), 0.0f), 200.0f));
	
	return Diffuse + Specular;
}

void main() {
	vec3 EyeDir = normalize(lubo.eyePos - fragPos);
	vec3 Norm = normalize(fragNorm);
	vec3 Albedo = texture(tex, fragUV).rgb;	vec3 L, lightCol;
	switch (lubo.TYPE) {
		case 0:
			L = directDir();
			lightCol = directCol();
			break;
		case 1:
			L = pointDir(fragPos);
			lightCol = pointCol(fragPos);
			break;
		case 2:
			L = spotDir(fragPos);
			lightCol = spotCol(fragPos);
			break;
	}	vec3 Fun = BRDF(EyeDir, Norm, L, Albedo, vec3(1.0f));	vec3 Ambient = vec3(0.1f);	outColor = vec4(Fun * lightCol.rgb + Ambient * Albedo, 1.0f);}