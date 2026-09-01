#pragma comment(lib, "winmm.lib")
#include "win32_backend.h"
#include "vk_backend.h"
#include "game.h"
#ifdef DEBUG
#include "imgui.h"
#endif

WinContext win = {};
DebugInfo debuginfo = {};
float render_dt = 1.0f / 60.0f;

int CALLBACK WinMain
(HINSTANCE	instance,
	HINSTANCE	prevInstance,
	LPSTR		commandline,
	int			showCode)
{
	LARGE_INTEGER perf_count_frequency_result;
	QueryPerformanceFrequency(&perf_count_frequency_result);
	debuginfo.perf_count_frequency = perf_count_frequency_result.QuadPart;

	InitConsole();

	#ifdef DEBUG
	ImGui::CreateContext();
	#endif
	
	WNDCLASSEXW win32_class = {};
	win32_class.lpfnWndProc = Win32Callback;
	win32_class.hInstance = instance;
	win32_class.lpszClassName = L"win32_class";
	win32_class.hCursor = LoadCursor(0, IDC_HAND);
	win32_class.cbSize = sizeof(WNDCLASSEXW);

	if (RegisterClassExW(&win32_class))
	{
		win.size = { 1920.0, 1080.0 };

		HWND window_handle = CreateWindowExW(0, win32_class.lpszClassName, 
								L"Earthworm Applications", WS_VISIBLE,
								0, 0, win.size.x, win.size.y, 
								0, 0, instance, 0);

		if (window_handle)
		{
			InitVulkan(window_handle);
			#ifdef DEBUG
			InitImgui(window_handle);
			#endif
			InitGame();
			win.running = true;

			LARGE_INTEGER last_counter;
			QueryPerformanceCounter(&last_counter);
			timeBeginPeriod(1);
			
			float frame_delta = render_dt;

			while (win.running)
			{
				MessageLoop();
				UpdateGame(frame_delta);

				LARGE_INTEGER end_counter;
				QueryPerformanceCounter(&end_counter);

				float counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
				float sec_per_frame = counter_elapsed / debuginfo.perf_count_frequency;
				
				if (sec_per_frame < render_dt)
				{
					while (sec_per_frame < (render_dt - 0.001f))
					{
						DWORD SleepMS = (DWORD)(1000.0f * (render_dt - sec_per_frame));
						if (SleepMS > 0)
						{
							Sleep(SleepMS);
						}
						QueryPerformanceCounter(&end_counter);
						sec_per_frame = (end_counter.QuadPart - last_counter.QuadPart)
							/ debuginfo.perf_count_frequency;
					}
					while (sec_per_frame < render_dt)
					{
						QueryPerformanceCounter(&end_counter);
						sec_per_frame = (end_counter.QuadPart - last_counter.QuadPart)
							/ debuginfo.perf_count_frequency;
					}
				}
				LARGE_INTEGER real_counter;
				QueryPerformanceCounter(&real_counter);
				debuginfo.real_counter_elapsed = real_counter.QuadPart - last_counter.QuadPart;
				debuginfo.real_sec_per_frame = debuginfo.real_counter_elapsed / debuginfo.perf_count_frequency;

				frame_delta = debuginfo.real_sec_per_frame;
				gubo.time += debuginfo.real_sec_per_frame;
				last_counter = real_counter;
			}
		}
	}

	return 0;
}