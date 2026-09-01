#include "game.h"
#include "vk_backend.h"
#include "win32_backend.h"
#ifdef DEBUG
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_win32.h"
#endif

GameContext gc = {};
GameInput input = {};
PlayerInfo p1 = {};
GUBO gubo = {};
uint32_t current_frame = 0;
float physics_dt = 1.0f / 60.0f;
float accumulator = 0;

v2 WorldToScreen(v3 world_pos, mat4 view_proj)
{
	float clip_x = world_pos.x * view_proj.column[0].x + world_pos.y * view_proj.column[1].x + world_pos.z * view_proj.column[2].x + view_proj.column[3].x;
	float clip_y = world_pos.x * view_proj.column[0].y + world_pos.y * view_proj.column[1].y + world_pos.z * view_proj.column[2].y + view_proj.column[3].y;
	float clip_w = world_pos.x * view_proj.column[0].w + world_pos.y * view_proj.column[1].w + world_pos.z * view_proj.column[2].w + view_proj.column[3].w;

	float ndc_x = clip_x / clip_w;
	float ndc_y = clip_y / clip_w;

	v2 position_2d;
	position_2d.x = (ndc_x + 1.0f) * 0.5f * 1920.0;
	position_2d.y = (1.0f - ndc_y) * 0.5f * 1080.0;

	return position_2d;
}
void UpdateVertexGrid(float x, float y)
{
	int raw_x = round(x / TILE_SIZE);
	int raw_y = round(y / TILE_SIZE);

	if (raw_x < 0 || raw_x >= GRID_SIZE || raw_y < 0 || raw_y >= GRID_SIZE)
	{
		return;
	}

	uint8_t flipped_y = (GRID_SIZE - 1) - raw_y;

	uint8_t x2 = static_cast<uint8_t>(raw_x);
	uint8_t y2 = static_cast<uint8_t>(flipped_y);

	bool is_stone = (vk.vertex_grid[y2][x2] & FLAG_STONE) != 0;

	if (is_stone)
	{
		vk.vertex_grid[y2][x2] &= ~FLAG_STONE;
	}
	else 
	{
		vk.vertex_grid[y2][x2] |= FLAG_STONE;
	} 

	memcpy(vk.vertex_grid_mapped, &vk.vertex_grid[0], sizeof(vk.vertex_grid));
}

void SetGridDirt()
{
	for (uint32_t w = 0; w < 101; w++)
	{
		for (uint32_t h = 0; h < 101; h++)
		{
			vk.vertex_grid[w][h] |= FLAG_STONE;
		}
	}

	int radius = 3; 
	int length = 97;

	for (int i = 0; i < length; ++i) 
	{
		int cx = i;
		int cz = i;

		for (int dz = -radius; dz <= radius; ++dz) 
		{
			for (int dx = -radius; dx <= radius; ++dx) 
			{
				int x = cx + dx;
				int z = 101 - (cz + dz);

				if (x >= 0 && x < 101 && z >= 0 && z < 101) 
				{
					vk.vertex_grid[z][x] &= ~FLAG_STONE;
				}
			}
		}
	}

	memcpy(vk.vertex_grid_mapped, &vk.vertex_grid[0], sizeof(vk.vertex_grid));
}
void UpdateFogGrid()
{
	int px = (int)floorf(p1.position.x);
	int py = (100) - (int)floorf(p1.position.y);

	int radius = 7.5;

	int x = 0;
	int y = 0;

	x = clamp(x, 0, 100);
	y = clamp(y, 0, 100);

	for (y = py - radius; y <= py + radius; ++y)
	{
		for (x = px - radius; x <= px + radius; ++x)
		{
			if (x >= 0 && x < 100 && y >= 0 && y < 100)
			{
				v2 player = { px,py };
				v2 fog_pos = { x,y };
				if(v2_distance(player, fog_pos) < radius)
				{
					vk.fog_map[y][x] |= FLAG_VISITED;
				}
			}
		}
	}

	memcpy(vk.fog_map_mapped, &vk.fog_map, sizeof(vk.fog_map));
}
void HpToScreen(v3 pos, mat4 viewproj, float char_height)
{
	pos.z = pos.z + char_height;

	v2 pos_2d = WorldToScreen(pos, viewproj);

	float bar_w = 100.0f;
	float bar_h = 18.0f;

	float bar_x = pos_2d.x - (bar_w / 2.0f);
	float bar_y = pos_2d.y - (bar_h / 2.0f);

	CreateQuad(bar_x, bar_y + 20.0f, bar_w, bar_h, 4);
	CreateQuad(bar_x, bar_y + 20.0f, bar_w, bar_h, 5); //health bar
	CreateQuad(bar_x, bar_y + 20.0f, bar_w, bar_h, 6); //mana bar
}
void PrintGrid() {
	for (int y = 0; y < GRID_SIZE; ++y) {
		for (int x = 0; x < GRID_SIZE; ++x)
		{
			printf("%d", static_cast<int>(vk.vertex_grid[y][x]));
			printf(",");
		}
		printf("\n");
	}
}
v3 RayCast(double xpos, double ypos, mat4 view, mat4 projection, float width, float height)
{
	if (view.element[0][0] == 0.0f)
	{
		printf("No valid view matrix.\n");
		return { 0,0,0 };
	}

	float x = (2.0f * (float)xpos) / width - 1.0f;
	float y = (2.0f * (float)ypos) / height - 1.0f;

	v4 ray_clip = { x, y, 0.0f, 1.0f };

	mat4 invProjMatrix = mat4_inv_perspective(projection);
	v4 ray_eye = mat4_v4_linear_combine(ray_clip, invProjMatrix);

	ray_eye.z = -1.0f;
	ray_eye.w = 0.0f;

	mat4 invViewMatrix = mat4_inv_lookat(view);
	v4 inv_ray_wor = mat4_v4_linear_combine(ray_eye, invViewMatrix);

	v3 ray_wor = { inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z };
	ray_wor = v3_normalize(ray_wor);

	return ray_wor;
}
RayCollision GetRayCollisionBox(Ray ray, BoundingBox box)
{
	RayCollision collision = { 0 };

	if (ray.position.x == 0)
	{
		printf("No valid ray.\n");
		return collision = { 0 };
	}	

	float inv_dx = 1.0f / ray.direction.x;
	float inv_dy = 1.0f / ray.direction.y;
	float inv_dz = 1.0f / ray.direction.z;

	float t1 = (box.min.x - ray.position.x) * inv_dx;
	float t2 = (box.max.x - ray.position.x) * inv_dx;
	float t3 = (box.min.y - ray.position.y) * inv_dy;
	float t4 = (box.max.y - ray.position.y) * inv_dy;
	float t5 = (box.min.z - ray.position.z) * inv_dz;
	float t6 = (box.max.z - ray.position.z) * inv_dz;

	float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
	float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));

	if (tmax < 0.0f || tmin > tmax)
	{
		collision.hit = false;
		return collision;
	}

	collision.hit = true;

	bool insideBox = (tmin < 0.0f);
	collision.distance = insideBox ? tmax : tmin;

	collision.point = (ray.position + (collision.distance * ray.direction));

	v3 box_center = v3_lerp(box.min, box.max, 0.5f);
	v3 box_extents = box.max - box.min;

	collision.normal = collision.point - box_center;
	collision.normal *= 2.001f;
	collision.normal = v3_divide(collision.normal, box_extents);

	collision.normal.x = roundf(collision.normal.x);
	collision.normal.y = roundf(collision.normal.y);
	collision.normal.z = roundf(collision.normal.z);
	collision.normal = v3_normalize(collision.normal);

	if (insideBox)
	{
		collision.normal = -collision.normal;
	}

	return collision;
}
mat4 TSA(v3 translate, float scale, float angle)
{
	mat4 result = mat4_diagonal(1.0f);

	mat4 scale_mat = mat4_scale({ scale, scale, scale });
	mat4 rotate_mat = mat4_rotate_RH(angle, { 0.0f, 0.0f, 1.0f });
	mat4 translate_mat = mat4_translate({ translate.x, translate.y, 0.0f });

	result = mat4_multiply(translate_mat, mat4_multiply(rotate_mat, scale_mat));

	return result;
}

void InitGame()
{
	gc.cam.look_at =  { 10.0f, 11.0f, 0.0f };
	gc.cam.position = { 10.0f, 3.0f, 15.0f };
	gc.cam.speed = 75.0f * physics_dt;

	gc.world_box.max = { 100,100,0 };
	gc.world_box.min = { 0,0,0 };

	p1.position = { 10, 10, 0 };
	p1.speed = 2.0f * physics_dt;
	p1.angle = 0.0f;
	p1.moving = false;
}
void UpdatePositions()
{
	//p1
	if ((p1.moving) && (v2_distance({ p1.position.x, p1.position.y },
		{ p1.destination.x, p1.destination.y }) > 0.1f))
	{
		p1.position += p1.movement;
	}
	else{ p1.moving = false; }

	//camera
	uint16_t edge_margin = 50;

	if (input.x < win.padded_rect.left)
	{
		gc.cam.position.x -= gc.cam.speed;
		gc.cam.look_at.x -= gc.cam.speed;
	}
	else if (input.x > win.padded_rect.right - edge_margin)
	{
		gc.cam.position.x += gc.cam.speed;
		gc.cam.look_at.x += gc.cam.speed;
	}
	if (input.y < win.padded_rect.top)
	{
		gc.cam.position.y += gc.cam.speed;
		gc.cam.look_at.y += gc.cam.speed;
	}
	else if (input.y > win.padded_rect.bottom - edge_margin)
	{
		gc.cam.position.y -= gc.cam.speed;
		gc.cam.look_at.y -= gc.cam.speed;
	}

	input.ray.position = { gc.cam.position };

	if (input.spacebar.ended_down)
	{
		gc.cam.look_at = { p1.position.x, p1.position.y + 1.0f, 0.0f };
		gc.cam.position = { p1.position.x, p1.position.y - 7.0f, 15.0f };
	}
}
void UploadData()
{
	gubo.model = mat4_diagonal(1.0f);
	gubo.view = mat4_look_at(gc.cam.position, gc.cam.look_at, { 0.0f, 0.0f, 1.0f });
	gubo.proj = mat4_perspective(DegToRad(40.0f), (float)win.client_rect.right / (float)win.client_rect.bottom, 1.0f);
	gubo.proj.element[1][1] *= -1.0f;
	gubo.projView = mat4_multiply(gubo.proj, gubo.view);
	gubo.dest = { p1.destination.x, p1.destination.y, 0,0 };
	memcpy(vk.global_uniform_buffer_mapped[current_frame], &gubo, sizeof(gubo));

	vk.instance_data[0].model_mat = TSA({ 50.0,50.0,0.0 },	1.0, 0.0);
	vk.instance_data[0].tex_id = 0;
	vk.instance_data[1].model_mat = TSA(p1.position, 0.5, p1.angle);
	vk.instance_data[1].tex_id = 1;
	vk.instance_data[2].model_mat = TSA({ 10.0,10.0,0.0 }, 0.4, 0.0);
	vk.instance_data[2].tex_id = 2;

	VkDeviceSize upload = sizeof(InstanceData) * 3;
	memcpy(vk.instance_buffer_mapped[current_frame], vk.instance_data, upload);

	vk.global_vert_2d.clear();
	HpToScreen(p1.position, gubo.projView, 1.556);
	VkDeviceSize global_vert_2d_buffer_size = sizeof(vk.global_vert_2d[0]) * vk.global_vert_2d.size();
	memcpy(vk.global_vert_2d_buffer_mapped, vk.global_vert_2d.data(), global_vert_2d_buffer_size);
}
void UpdateGame(float frame_delta)
{
	#ifdef DEBUG
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	if (debuginfo.show_imgui)
	{
		gubo.is_debug = 1;
		ImGui::Begin("DEBUG");
		ImGui::Text("time %.2f\n", gubo.time);
		ImGui::Text("ms/f %.2f\n", ((debuginfo.real_counter_elapsed * 1000.0f) / debuginfo.perf_count_frequency));
		ImGui::Text("fps  %.2f \n", debuginfo.perf_count_frequency / debuginfo.real_counter_elapsed);
		ImGui::End();
	}
	else(gubo.is_debug = 0);
	ImGui::Render();
	#else
	gubo.is_debug = 0;
	#endif

	accumulator += frame_delta;

	while (accumulator >= physics_dt)
	{
		UpdatePositions();
		accumulator -= physics_dt;
	}
	UpdateFogGrid();
	UploadData();
	DrawFrame();
}