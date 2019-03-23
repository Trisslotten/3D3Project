#version 450 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

const vec2 quad[] = {
	vec2(0,0),
	vec2(1,0),
	vec2(0,1),
	vec2(1,1)
};

void main()
{
	gl_Position = vec4(quad[gl_VertexIndex], 0.5, 1);
}
