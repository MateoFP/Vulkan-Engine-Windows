#version 450

layout(set = 0, binding = 0) uniform Global_UBO 
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec4 dest;
    int is_debug;
    uint frame_num;
} gubo;
layout(set = 0, binding = 1) uniform Model_UBO
{
    mat4 model[3];
} mubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in uint inModelIndex;
layout(location = 3) in uint in_tex_id;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outUv;
layout(location = 2) flat out uint outModelIndex;
layout(location = 3) flat out uint out_tex_id;

layout(location = 4) out vec2 outPlayerPos;

void main() 
{
    vec4 world_pos = mubo.model[inModelIndex] * vec4(inPos, 1.0);
    gl_Position = gubo.projView * world_pos;
    outModelIndex = inModelIndex;
    out_tex_id = in_tex_id;
    outWorldPos = world_pos.xyz;
    outUv = inUv;
    
    outPlayerPos.x = mubo.model[1][3][0];
    outPlayerPos.y = mubo.model[1][3][1]; 
}