#version 450#extension GL_ARB_separate_shader_objects : enablelayout(set = 0, binding = 1) uniform Param {
	bool isOn;
} pubo;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D before;layout(set = 0, binding = 2) uniform sampler2D after;

void main() {	if (!pubo.isOn)		outColor = texture(before, fragUV);	else		outColor = texture(after, fragUV);}