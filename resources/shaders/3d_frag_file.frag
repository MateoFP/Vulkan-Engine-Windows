#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_8bit_storage : enable
#define GRID_SIZE 101
#define TILE_SIZE 1

uint FLAG_STONE     = 1u << 0u;
uint FLAG_FOG       = 1u << 0u;
uint FLAG_VISITED   = 1u << 1u;

layout(set = 0, binding = 0) uniform Global_UBO 
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec4 dest;
    int is_debug;
    float time;
    float time_rb;
	float a_clicked;
} gubo;

layout(std430, set = 0, binding = 2) readonly buffer vertex_grid
{
    uint8_t data[GRID_SIZE * GRID_SIZE];
} grid;

layout(std430, set = 0, binding = 3) readonly buffer fog_buffer
{
    uint8_t data[100 * 100];
} fog;

uint get_FLAG(uint x, uint y, uint FLAG) 
{
    uint cx = clamp(x, 0, uint(GRID_SIZE));
    uint cy = clamp(y, 0, uint(GRID_SIZE));

    uint index = (cy * uint(GRID_SIZE)) + cx;

    bool is_on = (uint(grid.data[index]) & FLAG) != 0u;
    
    return uint(is_on);
}

uint hash2D(uvec2 p)
{
    p = p * uvec2(1597334677u, 3812015801u);
    p = (p.x ^ p.y) * uvec2(374761393u);
    return p.x ^ (p.x >> 16u);
}

int choose_tile(uvec2 tile_coord, int min_val, int max_val)
{
    uint h = hash2D(tile_coord);
    uint range = uint(max_val - min_val + 1);
    return min_val + int(h % range);
}

layout(set = 1, binding = 0) uniform sampler2D tex[];
layout(set = 1, binding = 1) uniform sampler2DArray tile;

layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) flat in uint in_model_id;
layout(location = 3) flat in uint in_tex_id;
layout(location = 4) in vec2 in_player_pos;

layout(location = 0) out vec4 out_color;

float get_fog_value(uint x, uint y) 
{
    uint cx = clamp(x, 0u, 100u);
    uint cy = clamp(y, 0u, 100u);
    uint index = (cy * 100u) + cx;
    uint fog_data = (uint(fog.data[index]));
    
    float max_fog_level = 1.0;

    if((fog_data & FLAG_VISITED) != 0u)
    {
        max_fog_level = 0.97;
    }

    vec2 corner_world_pos = vec2(float(x), 100 - float(y));

    float dist_to_player = distance(corner_world_pos.xy, in_player_pos);

    float first_r = 5.0;
    float second_r = 7.5;

    float active_fog = smoothstep(first_r, second_r, dist_to_player);

    return mix(0.0, max_fog_level, active_fog);
}

void main() 
{
    if(in_model_id > 0)
    {
        out_color = texture(tex[nonuniformEXT(in_tex_id)], in_uv);
        return;
    }

    vec2 map_uv = vec2(in_uv.x, in_uv.y);

    vec2 tiles_uv = map_uv * (GRID_SIZE-1.0);
    ivec2 tile_pos = ivec2(floor(tiles_uv));
    vec2 local_tile_uv  = 1.0 - fract(tiles_uv);

    uint v_bl = get_FLAG(uint(tile_pos.x),        uint(tile_pos.y), FLAG_STONE);
    uint v_br = get_FLAG(uint(tile_pos.x + 1),    uint(tile_pos.y), FLAG_STONE);
    uint v_tl = get_FLAG(uint(tile_pos.x),        uint(tile_pos.y + 1), FLAG_STONE);
    uint v_tr = get_FLAG(uint(tile_pos.x +1),     uint(tile_pos.y +1), FLAG_STONE);

    uint tile_mask = (v_tl * 1u) | (v_tr * 2u) | (v_bl * 4u) | (v_br * 8u);

    int stone_id        = choose_tile(uvec2(in_world_pos/TILE_SIZE), 0, 1);
    int dirt_id         = choose_tile(uvec2(in_world_pos/TILE_SIZE), 2, 4);
    vec3 stone_coords   = vec3(local_tile_uv, float(stone_id));
    vec3 dirt_coords    = vec3(local_tile_uv, float(dirt_id));
    vec4 stone_color    = texture(tile, stone_coords);
    vec4 dirt_color     = texture(tile, dirt_coords);

    if(tile_mask == 0)
    {
        out_color =  stone_color;
    }
    else if(tile_mask == 15)
    {
        out_color =  dirt_color;
    }
    else
    {
        vec2 slot_offset = vec2(float(tile_mask / 4u), float(tile_mask % 4u));
        vec2 alpha_uv = (slot_offset + local_tile_uv) * 0.25;
        ivec2 atlas_size = textureSize(tex[0], 0);
        ivec2 pixel_coord = ivec2(alpha_uv * vec2(atlas_size));

        float raw_mask = texelFetch(tex[0], pixel_coord, 0).r;
        out_color = mix(stone_color, dirt_color, raw_mask); 
    }
    
    if(gubo.is_debug == 0)
    {
        vec2 fog_uv = map_uv * 100;
        ivec2 fog_pos = ivec2(floor(fog_uv));
        vec2 local_fog_uv = fract(fog_uv);

        float f_bl = get_fog_value(uint(fog_pos.x),     uint(fog_pos.y));
        float f_br = get_fog_value(uint(fog_pos.x + 1), uint(fog_pos.y));
        float f_tl = get_fog_value(uint(fog_pos.x),     uint(fog_pos.y + 1));
        float f_tr = get_fog_value(uint(fog_pos.x + 1), uint(fog_pos.y + 1));

        float fog_top     = mix(f_tl, f_tr, local_fog_uv.x);
        float fog_bottom  = mix(f_bl, f_br, local_fog_uv.x);
        float fog_factor  = mix(fog_bottom, fog_top, local_fog_uv.y);
        vec3 fog_color = vec3(0.0, 0.0, 0.0);
        out_color.rgb = mix(out_color.rgb, fog_color, fog_factor);
    }
    else
    {
        float line_thickness = 0.01;
        if (local_tile_uv.x < line_thickness || local_tile_uv.x > (1.0 - line_thickness) ||
        local_tile_uv.y < line_thickness || local_tile_uv.y > (1.0 - line_thickness))
        {
            out_color = vec4(1.0, 0.1, 0.0, 1.0);
        }
    }

    //timed (fix pointer/circle/dest accuracy)
    float distance_to_click = distance(in_world_pos.xy, gubo.dest.xy);
    if(gubo.dest.x != 0)
    {
        float duration = 0.5;
        float max_r = 0.4;
        float age = gubo.time - gubo.time_rb;
        if(age >= 0.0 && age <= duration)
        {
            float progress = age / duration;
            float r = max_r - progress;
            float mask = 1.0 - smoothstep(0.0, 0.04, abs(distance_to_click - r));
            float fade = 0.8 - progress;
            out_color.rgb = mix(out_color.rgb, vec3(0.0, 1.0, 0.0), mask * fade);
        }
    }

    if(gubo.a_clicked == 1.0)
    {
        float distance_to_player = distance(in_world_pos.xy, in_player_pos.xy);
        float r = 3.7;
        float mask = 1.0 - smoothstep(0.0, 0.04, abs(distance_to_player - r));
        out_color.rgb = mix(out_color.rgb, vec3(0.9, 0.9, 1.0), mask * 0.5);
    }
}
