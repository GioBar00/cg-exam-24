#version 450#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 1) uniform sampler2D tex;const int MAX_LIGHTS = 256;layout(set = 0, binding = 0) uniform LightUBO {
	vec3 TYPE[MAX_LIGHTS];
	vec3 lightPos[MAX_LIGHTS];
	vec3 lightDir[MAX_LIGHTS];
	vec4 lightCol[MAX_LIGHTS];
	float cosIn;
	float cosOut;
	uint NUMBER;
	vec3 eyeDir;} lubo;vec3 directDir(int idx) {
	return lubo.lightDir[idx];
}

vec3 directCol(int idx) {
	return lubo.lightCol[idx].rgb;
}

vec3 pointDir(int idx, vec3 fragmentPos) {
	return normalize(lubo.lightPos[idx] - fragmentPos);
}

vec3 pointCol(int idx, vec3 fragmentPos) {
	return pow(lubo.lightCol[idx].a / length(lubo.lightPos[idx] - fragmentPos), 2.0) * lubo.lightCol[idx].rgb;
}

vec3 spotDir(int idx, vec3 fragmentPos) {
	return pointDir(idx, fragmentPos);
}

vec3 spotCol(int idx, vec3 fragmentPos) {
	float ext = clamp((dot(normalize(lubo.lightPos[idx] - fragmentPos), lubo.lightDir[idx]) - lubo.cosOut) / (lubo.cosIn - lubo.cosOut), 0.0, 1.0); // Extended light model factor.
	return ext * pointCol(idx, fragmentPos);
}vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 mDiffuse, vec3 mSpecular, bool specular) {
	vec3 Diffuse = mDiffuse * max(dot(N, L), 0.0f);
	//vec3 Diffuse = mDiffuse;
	vec3 Specular = mSpecular * vec3(pow(max(dot(V, -reflect(L, N)), 0.0f), 150.0f));
	//vec3 Specular = vec3(pow(max(dot(V, -reflect(L, N)), 0.0f), 150.0f));
	
	return (Diffuse + (specular ? Specular : vec3(0)));
}

void main() {
	vec3 EyeDir = normalize(lubo.eyeDir);
	vec3 Norm = normalize(fragNorm);
	vec3 Albedo = texture(tex, fragUV).rgb;	vec3 L, lightCol, Fun = vec3(0.0f);
	uint maskD, maskP, maskS;
	for (int i = 0; i < lubo.NUMBER; i++) {		maskD = uint(lubo.TYPE[i].x); maskP = uint(lubo.TYPE[i].y); maskS = uint(lubo.TYPE[i].z);		if (maskD == 1) {			L = directDir(i);			lightCol = directCol(i);		} else if (maskP == 1) {			L = pointDir(i, fragPos);			lightCol = pointCol(i, fragPos);		} else if (maskS == 1) {			L = spotDir(i, fragPos);			lightCol = spotCol(i, fragPos);		}		Fun += BRDF(EyeDir, Norm, L, Albedo, vec3(1.0f), false) * lightCol.rgb;	}	vec3 Ambient = vec3(0.01f);	outColor = vec4(Fun + Ambient * Albedo, 1.0f);}