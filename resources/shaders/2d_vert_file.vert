#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in uint in_model_id;

layout(location = 1) out vec2 out_uv;
layout(location = 2) flat out uint out_model_id;

void main() 
{
	vec2 screen_size = vec2(1920.0, 1080.0);
	vec2 clip_space;
	clip_space.x = (in_pos.x / screen_size.x) * 2.0 - 1.0;
    clip_space.y = 1.0 - (in_pos.y / screen_size.y) * 2.0;

	gl_Position = vec4(clip_space, 0.0, 1.0);
	out_model_id = in_model_id;
	out_uv = in_uv;
}