#version 450 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

layout(location = 1) in vec2 uv;

float brick(vec2 uv)
{
	float lt = 0.1;
	
	uv*=50;
	uv = fract(uv);

	float result = 0.0;
	if(uv.x > lt && uv.y > lt && uv.y < 0.5)
		result = 1.0;
	if(uv.y > 0.5+lt && (uv.x < 0.5 || uv.x > 0.5 + lt))
		result = 1.0;
	return result;
}

void main()
{
	float color = texture(tex, uv).r;
	color = brick(uv)*color;

	outColor = vec4(vec3(color),1);
}