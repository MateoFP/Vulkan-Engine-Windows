#pragma once
#include "windows.h"
#include "imgui.h"
#include "stdio.h"
#include "mateo_math.h"

LRESULT CALLBACK Win32Callback(HWND	window, UINT message, WPARAM wParam, LPARAM lParam);

struct WinContext
{
	bool running;
	v2 size;
	RECT padded_rect;
	RECT client_rect;
};
struct DebugInfo
{
	bool show_imgui;
	float perf_count_frequency;
	float real_counter_elapsed;
	float real_sec_per_frame;
};

extern WinContext win;
extern DebugInfo debuginf;

void InitConsole();
void MessageLoop();

#ifdef DEBUG
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

