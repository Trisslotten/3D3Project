#version 450 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Uniforms {
	vec2 pos;
	float count;
} uniforms;

const vec2 quad[] = {
	vec2(0,0),
	vec2(1,0),
	vec2(0,1),
	vec2(1,1),
};

layout(binding = 1) uniform sampler2D tex;

layout(location = 1) out vec2 uv;
layout(location = 2) out float count;

void main()
{
	vec2 mapSize = vec2(textureSize(tex, 0));
	vec2 p = quad[gl_VertexIndex];
	uv = p;

	vec2 normPos = uniforms.pos / mapSize;
	normPos = normPos*2.0-1.0;

	vec2 tileSize = 2.0/mapSize;

	p = p*tileSize + normPos;

	count = uniforms.count;

	gl_Position = vec4(p, 0.5, 1);
}
