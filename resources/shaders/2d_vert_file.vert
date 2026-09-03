#version 450

const vec2 quad_pos[6] = vec2[]
(
    vec2(0.0, 0.0), 
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),

    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

struct InstanceData2D
{
    vec2 pos;
    vec2 wh;
    uint tex_id;
    uint pad;
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

layout(std430, set = 0, binding = 4) readonly buffer instance_data
{
    InstanceData2D instances[];
} in_data;

layout(location = 0) out vec2 out_uv;
layout(location = 1) flat out uint out_tex_id;

void main() 
{
    vec2 unit_pos = quad_pos[gl_VertexIndex]; 
    InstanceData2D instance = in_data.instances[gl_InstanceIndex];
    vec2 screen_size = vec2(1920.0, 1080.0); 

    vec2 pixel_pos = instance.pos + (unit_pos * instance.wh);
    pixel_pos.x -= 50.0;
    pixel_pos.y -= 20.0;

    out_uv = vec2(unit_pos.x, unit_pos.y);

    out_tex_id = instance.tex_id;

    vec2 clip_pos;
    clip_pos.x = (pixel_pos.x / screen_size.x) * 2.0 - 1.0;
    clip_pos.y = (pixel_pos.y / screen_size.y) * 2.0 - 1.0;
    gl_Position = vec4(clip_pos, 0.0, 1.0);
}