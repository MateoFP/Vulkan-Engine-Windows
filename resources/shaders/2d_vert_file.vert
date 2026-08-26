#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in uint inModelIndex;

layout(location = 1) out vec2 outUv;
layout(location = 2) flat out uint outModelIndex;

void main() 
{
	vec2 screen_size = vec2(1920.0, 1080.0);
	vec2 clip_space;
	clip_space.x = (inPos.x / screen_size.x) * 2.0 - 1.0;
    clip_space.y = 1.0 - (inPos.y / screen_size.y) * 2.0;

	gl_Position = vec4(clip_space, 0.0, 1.0);
	outModelIndex = inModelIndex;
	outUv = inUv;
}