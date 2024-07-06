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
	vec3 eyePos;} lubo;vec3 directDir() {
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
}vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 mDiffuse, vec3 mSpecular) {	// Diffuse.	float dDot = dot(N, L), dShading;	float rMin, rMax, range;	float iLow, iHigh;	if (dDot <= 0.7) {		rMin = 0.0; rMax = 0.15; range = rMax - rMin;		iLow = 0.0; iHigh = 0.1;	}	if (0.1 < dDot) {		rMin = 0.15; rMax = 1.0; range = rMax - rMin;		iLow = 0.7; iHigh = 0.8;	}	dShading = clamp(rMin + range * ((dDot - iLow) / (iHigh - iLow)), rMin, rMax);	vec3 Diffuse = dShading * mDiffuse;	// Specular.	float sDot = dot(V, -reflect(L, N)), sShading;	if (sDot <= 0.9)		sShading = 0.0;	else if (0.9 < sDot && sDot <= 0.95) {		float min = 0.0, max = 1.0, range = max - min;				sShading = min + range * ((sDot - 0.9) / (0.95 - 0.9));	}	else if (0.95 < sDot)		sShading = 1.0;	vec3 Specular = sShading * mSpecular;		// Shader.	return (Diffuse + Specular);}

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