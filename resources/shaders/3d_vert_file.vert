#version 450
#extension GL_ARB_shader_draw_parameters : enable
struct InstanceData
{
    mat4 model;
    uint tex_id;
    uint pad0;
    uint pad1;
    uint pad2;
};

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

layout(std430, set = 0, binding = 1) readonly buffer instance_data
{
    InstanceData instances[];
} in_data;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec2 out_uv;
layout(location = 2) flat out uint out_model_id;
layout(location = 3) flat out uint out_tex_id;

layout(location = 4) out vec2 out_player_pos;

void main() 
{
    InstanceData instance = in_data.instances[gl_InstanceIndex];

    vec4 world_pos = instance.model * vec4(in_pos, 1.0);
    gl_Position = gubo.projView * world_pos;
    out_model_id = gl_InstanceIndex;
    out_world_pos = world_pos.xyz;
    out_uv = in_uv;
   
    uint submesh_local_index = gl_DrawIDARB - gl_BaseInstanceARB; 
    uint final_tex_id = instance.tex_id + submesh_local_index;
    out_tex_id = final_tex_id;

    out_player_pos = in_data.instances[1].model[3].xy;
}