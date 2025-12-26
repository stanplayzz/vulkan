#version 450 core

layout (set = 1, binding = 0) uniform sampler2D textures[10];

layout (push_constant) uniform Push{
	uint textureIndex;
} pc;


layout (location = 1) in vec2 in_uv;
layout (location = 0) out vec4 out_color;

void main() {
	out_color = texture(textures[pc.textureIndex], in_uv);
}