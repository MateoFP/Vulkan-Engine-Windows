#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform sampler2D tex[];

layout(location = 1) in vec2 inUv;
layout(location = 2) flat in uint inModelIndex;

layout(location = 0) out vec4 out_color;

void main() 
{
	out_color = texture(tex[nonuniformEXT(inModelIndex)], inUv);
}
