#pragma once
#include "mateo_math.h"
#include "vk_backend.h"

struct ButtonState
{
	uint32_t half_transition_count;
	bool ended_down;
};
struct Camera3D
{
	v3 look_at;
	v3 position;
	float speed;
};
struct BoundingBox
{
	v3 max;
	v3 min;
};
struct Ray
{
	v3 position;
	v3 direction;
};
struct RayCollision
{
	bool hit;
	float distance;
	v3 point;
	v3 normal;
};
struct GameContext
{
	Camera3D cam;
	mat4 model[MODEL_NUM];
	BoundingBox world_box;
};
struct GameInput
{
	int32_t x, y;
	RayCollision collision;
	Ray ray;

	union
	{
		ButtonState buttons[2];
		struct
		{
			ButtonState spacebar;
			ButtonState mouse_rb;
		};
	};
};
struct PlayerInfo
{
	v3 position;
	v3 destination;
	v3 movement;
	float angle;
	float speed;
	bool moving;
};

extern GameContext gc;
extern GameInput input;
extern PlayerInfo p1;
extern float render_dt;
extern float physics_dt;
extern uint32_t current_frame;

v2 WorldToScreen(v3 world_pos, const mat4 view_proj);
void UpdateVertexGrid(float x, float y);
void SetGridDirt();
void UpdateFogGrid();
void HpToScreen(v3 pos, mat4 viewproj, float char_height);
void PrintGrid();
v3 RayCast(double x, double y, mat4 view, mat4 projection, float width, float height);
RayCollision GetRayCollisionBox(Ray ray, BoundingBox box);

void InitGame();
void UpdatePositions();
void UploadData();
void UpdateGame(float frame_delta);
