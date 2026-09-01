#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform sampler2D tex[];

layout(location = 1) in vec2 in_uv;
layout(location = 2) flat in uint in_model_id;

layout(location = 0) out vec4 out_color;

void main() 
{
	out_color = texture(tex[nonuniformEXT(in_model_id)], in_uv);
}
