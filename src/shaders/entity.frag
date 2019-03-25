#version 450 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

layout(location = 1) in vec2 uv;
layout(location = 2) in float count;

void main()
{
	vec3 color = vec3(1,0,0);
	if(count == 0)
	{
		color = vec3(0,0.5,1);
	} else {
		float threshold = (count-1) / 10.0;
		threshold = 0.3;
		float w = step(threshold, 1-uv.y);
		color = w * color + (1-w) * vec3(1,1,0);
	}
	outColor = vec4(color,1);
}