#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define NOMINMAX
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#undef far
#undef near

#pragma clang diagnostic pop

#include "common.h"

#define NAUHT_DLL_NAME L"nauht_game.dll"
#define NAUHT_DLL_LOADED_NAME L"nauht_game_loaded.dll"

static bool IsRunning = false;

typedef struct Game_Code
{
	HMODULE module;
	Game_Init_Func init;
	Game_Tick_Func tick;
	FILETIME timestamp;
} Game_Code;

static bool
LoadGameCode(Game_Code* game_code)
{
	bool succeeded = false;

	if (game_code->module != 0) FreeLibrary(game_code->module);
	*game_code = (Game_Code){0};

	if (CopyFileW(NAUHT_DLL_NAME, NAUHT_DLL_LOADED_NAME, FALSE))
	{
		HMODULE module = LoadLibraryW(NAUHT_DLL_LOADED_NAME);

		if (module != 0)
		{
			Game_Init_Func init = (Game_Init_Func)(void*)GetProcAddress(module, "Init");
			Game_Tick_Func tick = (Game_Tick_Func)(void*)GetProcAddress(module, "Tick");

			if (init == 0 || tick == 0) FreeLibrary(module);
			else
			{
				*game_code = (Game_Code){
					.module    = module,
					.init      = init,
					.tick      = tick,
					.timestamp = {0},
				};

				WIN32_FILE_ATTRIBUTE_DATA attrs;
				if (GetFileAttributesExW(NAUHT_DLL_NAME, GetFileExInfoStandard, &attrs)) game_code->timestamp = attrs.ftLastWriteTime;

				succeeded = true;
			}
		}
	}

	return succeeded;
}

static LRESULT
WndProc(HWND window, UINT msg_code, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  if (msg_code == WM_QUIT || msg_code == WM_CLOSE)
	{
		IsRunning = false;

    result = 0;
	}
	else
  {
    result = DefWindowProcW(window, msg_code, wparam, lparam);
  }

  return result;
}

int
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, wchar_t* cmdline, int show_cmd)
{
	bool encountered_errors = false;

	if (!encountered_errors)
	{
		wchar_t path[MAX_PATH + 1];
		DWORD r = GetModuleFileNameW(0, path, ARRAY_LEN(path));
		if (r == 0 || r == ARRAY_LEN(path))
		{
			//// ERROR: Failed to get path to executable
			encountered_errors = true;
		}
		else
		{
			wchar_t* past_last_slash = path;
			for (wchar_t* scan = path; *scan != 0; ++scan)
			{
				if (*scan == '/' || *scan == '\\') past_last_slash = scan + 1;
			}

			*past_last_slash = 0;

			if (!SetCurrentDirectoryW(path))
			{
				//// ERROR: Failed to set working directory
				encountered_errors = true;
			}
		}
	}

	HWND window = 0;
	if (!encountered_errors)
	{
		WNDCLASSEXW window_class = {
			.cbSize        = sizeof(window_class),
			.style         = 0, // TODO: Consider CS_*REDRAW
			.lpfnWndProc   = WndProc,
			.hInstance     = instance,
			.hIcon         = LoadIcon(0, IDI_QUESTION),
			.hCursor       = LoadCursor(0, IDC_ARROW),
			.lpszClassName = L"NAUHT",
		};

		if (RegisterClassExW(&window_class))
		{
			window = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP, window_class.lpszClassName, L"nauht",
															 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
															 0, 0, instance, 0);
		}

		if (window == 0)
		{
			//// ERROR: Failed to create window
			encountered_errors = true;
		}
	}

	// NOTE: based on mmozeiko's example
	//       https://gist.github.com/mmozeiko/5e727f845db182d468a34d524508ad5f#file-win32_d3d11-c-L69
	ID3D11Device* dx_device                = 0;
	ID3D11DeviceContext* dx_context        = 0;
	IDXGISwapChain1* swapchain             = 0;
	u32 swapchain_width                    = 0;
	u32 swapchain_height                   = 0;
	ID3D11RenderTargetView* swapchain_view = 0;
	if (!encountered_errors)
	{
		UINT flags = 0;
#ifdef NAUHT_DEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		if (!SUCCEEDED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, &(D3D_FEATURE_LEVEL){ D3D_FEATURE_LEVEL_11_0 }, 1, D3D11_SDK_VERSION, &dx_device, 0, &dx_context)))
		{
			//// ERROR: Failed to create direct x 11 context
			encountered_errors = true;
		}
		else
		{
#if NAUHT_DEBUG
			ID3D11InfoQueue* dx_info = 0;
			ID3D11Device_QueryInterface(dx_device, &IID_ID3D11InfoQueue, (void**)&dx_info);
			if (dx_info != 0)
			{
				ID3D11InfoQueue_SetBreakOnSeverity(dx_info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				ID3D11InfoQueue_SetBreakOnSeverity(dx_info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
				ID3D11InfoQueue_Release(dx_info);
			}

			IDXGIInfoQueue* dxgi_info = 0;
			IDXGIDevice_QueryInterface(dx_device, &IID_IDXGIInfoQueue, (void**)&dxgi_info);
			if (dxgi_info != 0)
			{
				IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
				IDXGIInfoQueue_Release(dxgi_info);
			}
#endif

			{
				bool succeeded = false;

				IDXGIDevice* dxgi_device = 0;
				if (SUCCEEDED(ID3D11Device_QueryInterface(dx_device, &IID_IDXGIDevice, (void**)&dxgi_device)))
				{
					IDXGIAdapter* adapter = 0;
					if (SUCCEEDED(IDXGIDevice_GetAdapter(dxgi_device, &adapter)))
					{
						IDXGIFactory2* factory = 0;
						if (SUCCEEDED(IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory2, (void**)&factory)))
						{
							DXGI_SWAP_CHAIN_DESC1 description = {
								// NOTE: 0,0 = same dimensions as window
								.Width       = 0,
								.Height      = 0,
								.Format      = DXGI_FORMAT_R8G8B8A8_UNORM,
								.SampleDesc  = { 1, 0 }, // NOTE: no msaa
								.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
								.BufferCount = 2,
								.Scaling     = DXGI_SCALING_NONE,
								.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,
								.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED,
							};

							if (SUCCEEDED(IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)dx_device, window, &description, 0, 0, &swapchain)))
							{
								IDXGIFactory_MakeWindowAssociation(factory, window, DXGI_MWA_NO_ALT_ENTER);

								succeeded = true;
							}

							IDXGIFactory2_Release(factory);
						}

						IDXGIAdapter_Release(adapter);
					}

					IDXGIDevice_Release(dxgi_device);
				}

				if (!succeeded)
				{
					//// ERROR: Failed to create dx swapchain
					encountered_errors = true;
				}
			}
		}
	}

	Game_Code game_code = {0};
	if (!encountered_errors)
	{
		if (!LoadGameCode(&game_code))
		{
			//// ERROR: Failed to load game code
			encountered_errors = true;
		}
		else
		{
			game_code.init(false);
		}
	}

	if (!encountered_errors)
	{
		ShowWindow(window, SW_SHOW);

		IsRunning = true;
		while (IsRunning)
		{
			WIN32_FILE_ATTRIBUTE_DATA attrs;
			if (GetFileAttributesExW(NAUHT_DLL_NAME, GetFileExInfoStandard, &attrs) && CompareFileTime(&game_code.timestamp, &attrs.ftLastWriteTime) == -1)
			{
				for (;;)
				{
					if (LoadGameCode(&game_code))
					{
						OutputDebugStringA("Successfully loaded game code");

						game_code.init(true);

						break;
					}
					else
					{
						OutputDebugStringA("Failed to load game code");
						Sleep(100);
					}
				}
			}

			for (MSG msg; PeekMessageW(&msg, 0, 0, 0, PM_REMOVE); )
			{
				WndProc(window, msg.message, msg.wParam, msg.lParam);
			}

			game_code.tick();

			RECT client_rect;
			GetClientRect(window, &client_rect);
			u32 width  = (u32)(client_rect.right  - client_rect.left);
			u32 height = (u32)(client_rect.bottom - client_rect.top);

			if (swapchain_view == 0 || swapchain_width != width || swapchain_height != height)
			{
				if (swapchain_view != 0)
				{
					ID3D11DeviceContext_ClearState(dx_context);
					ID3D11RenderTargetView_Release(swapchain_view);
					swapchain_view = 0;
				}

				swapchain_width  = width;
				swapchain_height = height;

				if (width != 0 && height != 0)
				{
					if (!SUCCEEDED(IDXGISwapChain1_ResizeBuffers(swapchain, 0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
					{
						//// ERROR
						// TODO: What should happen in this case?
						NOT_IMPLEMENTED;
					}
					else
					{
						ID3D11Texture2D* texture = 0;
						IDXGISwapChain1_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void**)&texture);
						ID3D11Device_CreateRenderTargetView(dx_device, (ID3D11Resource*)texture, 0, &swapchain_view);
						ID3D11Texture2D_Release(texture);
					}
				}
			}

			if (swapchain_view != 0)
			{
				ID3D11DeviceContext_ClearRenderTargetView(dx_context, swapchain_view, ((f32[]){ 1, 0, 1, 1 }));
			}

			HRESULT swap_result = IDXGISwapChain1_Present(swapchain, 1, 0);
			if (swap_result == DXGI_STATUS_OCCLUDED)
			{
				// TODO: What to do when the window is minimized?
				// NOTE: sleeping to avoid running too fast, because windows is slow and can't keep up
				Sleep(1);
			}
			else if (!SUCCEEDED(swap_result))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to present");
			}
		}
	}

  return 0;
}
