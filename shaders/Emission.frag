#version 450#extension GL_ARB_separate_shader_objects : enablelayout(set = 0, binding = 0) uniform UBO {
	mat4 mvpMat;
	vec4 lightCol;
} ubo;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 1) uniform sampler2D tex;

void main() {
	vec3 Albedo = 0.2 * texture(tex, fragUV).rgb + 0.1 * ubo.lightCol.rgb;	outColor = vec4(Albedo, 1.0f);}