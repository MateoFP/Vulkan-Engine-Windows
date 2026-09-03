#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 1, binding = 0) uniform sampler2D tex[];

layout(location = 0) in vec2 in_uv;
layout(location = 1) flat in uint tex_id;

layout(location = 0) out vec4 out_color;

void main() 
{
	out_color = texture(tex[nonuniformEXT(tex_id)], in_uv);
}
