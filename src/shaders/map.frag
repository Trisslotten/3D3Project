#version 450 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

layout(location = 1) in vec2 uv;

float uhash12(uvec2 x)
{
	uvec2 q = 1103515245U * ((x >> 1U) ^ (uvec2(x.y, x.x)));
	uint  n = 1103515245U * ((q.x) ^ (q.y >> 3U));
	return float(n) * (1.0 / float(0xffffffffU));
}
float hash12(vec2 x) { return uhash12(uvec2(2000.*x)); }

vec3 wall(vec2 uv)
{
	float lt = 0.1;

	float hash = hash12(uv);
	
	uv*=vec2(80, 60);

	uv = fract(uv);

	vec3 result = 0.5*vec3(0.796, 0.255, 0.329);
	if((uv.x > lt && uv.y > lt && uv.y < 0.5) || 
		(uv.y > 0.5+lt && (uv.x < 0.5 || uv.x > 0.5 + lt)))
	{
		result = vec3(0.796, 0.255, 0.329);
	}

	return result + 0.1*hash;
}

vec3 ground(vec2 uv)
{
	vec3 color = vec3(0.15625);

	return color + 0.03*hash12(uv);
}

void main()
{
	float map = texture(tex, uv).r;
	vec3 color = wall(uv)*map + (1-map) * ground(uv);
	
	outColor = vec4(color, 1);
}