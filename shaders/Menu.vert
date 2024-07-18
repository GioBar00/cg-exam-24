#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

void main() {
	gl_Position = mat4(1.0) * vec4(inPos, 0.0, 1.0);

	fragUV = inUV;
}
