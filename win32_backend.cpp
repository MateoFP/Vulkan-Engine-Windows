#include "stdio.h"
#include "windows.h"
#include "win32_backend.h"
#include "game.h"

HCURSOR my_cursor = (HCURSOR)LoadImageW(NULL, L"resources/textures/orc.cur", IMAGE_CURSOR, 38, 38, LR_LOADFROMFILE | LR_DEFAULTSIZE);
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InitConsole()
{
	AllocConsole();

	FILE* fp_stdout;
	freopen_s(&fp_stdout, "CONOUT$", "w", stdout);

	FILE* fp_stderr;
	freopen_s(&fp_stderr, "CONOUT$", "w", stderr);

	printf("And\n");
}

LRESULT CALLBACK Win32Callback
(HWND	window,
	UINT	message,
	WPARAM	wParam,
	LPARAM	lParam)
{
	LRESULT result = 0;

	#ifdef DEBUG
	if (ImGui::GetCurrentContext() != nullptr)
	{
		if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam) && message != WM_SETCURSOR)
		{
			return true;
		}
	}
	#endif
	switch (message)
	{
		case WM_KEYUP:
		{
			if (wParam == ' ')
			{
				input.spacebar.ended_down = false;
			}
		} break;
		case WM_KEYDOWN:
		{
			if (wParam == 'S')
			{
				p1.moving = false;
			}
			if (wParam == 'P')
			{
				#ifdef DEBUG
				if (debuginf.show_imgui)
				{
					debuginf.show_imgui = false;
				}
				else
				{
					debuginf.show_imgui = true;
				}
				#endif
			}
			if (wParam == ' ')
			{
				input.spacebar.ended_down = true;
			}
		}break;
		case WM_RBUTTONDOWN:
		{
			input.ray.direction = RayCast(input.x, input.y,
				gubo.view, gubo.proj, (float)win.client_rect.right, (float)win.client_rect.bottom);

			input.collision = GetRayCollisionBox(input.ray, gc.world_box);
			if (!input.collision.hit) break;

			p1.destination = input.collision.point;

			p1.movement.x = p1.destination.x - p1.position.x;
			p1.movement.y = p1.destination.y - p1.position.y;

			p1.movement = v3_normalize(p1.movement);

			float look_x = p1.movement.x;
			float look_y = p1.movement.y;

			p1.movement *= p1.speed;

			p1.angle = (atan2f(look_y, look_x)) + 1.570796f;
			p1.moving = true;		

			#ifdef DEBUG
			if (gubo.is_debug)
			{
				UpdateVertexGrid(p1.destination.x, p1.destination.y);
			}
			#endif
		} break;
		case WM_MOUSEWHEEL:
		{
			int32_t wheel_pos = (int32_t)wParam;
			if (wheel_pos < 0)
			{
				gc.cam.position.z += 0.5;
			}
			else
			{
				gc.cam.position.z -= 0.5;
			}
		} break;
		case WM_MOUSEMOVE:
		{
			POINT mouse_pos;
			GetCursorPos(&mouse_pos);
			ScreenToClient(window, &mouse_pos);
			input.x = mouse_pos.x;
			input.y = mouse_pos.y;
		} break;
		case WM_QUIT:
		{
			win.running = false;
		} break;
		case WM_CLOSE:
		{
			win.running = false;
		} break;
		case WM_DESTROY:
		{
			win.running = false;
		} break;
		case WM_ACTIVATEAPP:
		{
			printf("WM_ACTIVATEAPP\n");
		} break;
		case WM_SETFOCUS:
		{
			GetClientRect(window, &win.client_rect);
			win.padded_rect = win.client_rect; //probably bug 
			MapWindowPoints(window, NULL, (LPPOINT)&win.padded_rect, 2);
			InflateRect(&win.padded_rect, -20, -20);
			ClipCursor(&win.padded_rect);
			printf("WM_SETFOCUS\n");
		} break;
		case WM_KILLFOCUS:
		{
			printf("WM_KILLFOCUS\n");
			ClipCursor(NULL);
		} break;
		case WM_SETCURSOR:
		{
			SetCursor(my_cursor);
			return TRUE;
		} break;
		default:
		{
			result = DefWindowProcW(window, message, wParam, lParam);
		} break;
	}
	return result;
}

void MessageLoop()
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
		}
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}