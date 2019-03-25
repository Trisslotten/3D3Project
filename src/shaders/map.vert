#version 450 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable


const vec2 quad[] = {
	vec2(0,0),
	vec2(1,0),
	vec2(0,1),
	vec2(1,1),
};

layout(location = 1) out vec2 uv;

void main()
{
	vec2 p = quad[gl_VertexIndex];
	uv = p;
	p = p*2.-1.;
	gl_Position = vec4(p, 0.5, 1);
}
