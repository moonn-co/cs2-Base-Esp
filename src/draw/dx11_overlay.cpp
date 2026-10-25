#include "dx11_overlay.hpp"

#include "utils/debug_logger.hpp"

ID3D11Device* Window::device = nullptr;
ID3D11DeviceContext* Window::device_context = nullptr;
IDXGISwapChain* Window::swap_chain = nullptr;
ID3D11RenderTargetView* Window::render_targetview = nullptr;

bool Window::vsync = false;
HWND Window::hwnd = nullptr;
HWND Window::viewport = nullptr;
WNDCLASSEX Window::wc = { };

extern LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

static bool CreateSwapChainForAdapter(const D3D_DRIVER_TYPE driver_type, IDXGISwapChain** out_swap_chain, ID3D11Device** out_device, ID3D11DeviceContext** out_context)
{
	DXGI_SWAP_CHAIN_DESC sd{};

	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = Window::hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
	const D3D_FEATURE_LEVEL feature_levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

	return D3D11CreateDeviceAndSwapChain(
		nullptr,
		driver_type,
		nullptr,
		0U,
		feature_levels,
		2,
		D3D11_SDK_VERSION,
		&sd,
		out_swap_chain,
		out_device,
		&feature_level,
		out_context
	) == S_OK;
}

bool Window::CreateDevice()
{
	if (!CreateSwapChainForAdapter(D3D_DRIVER_TYPE_HARDWARE, &swap_chain, &device, &device_context))
	{
		// Software rasteriser keeps the overlay usable on machines without a
		// working D3D11 driver instead of aborting the whole process.
		LOGF(WARNING, "Hardware device creation failed - retrying with WARP");

		if (!CreateSwapChainForAdapter(D3D_DRIVER_TYPE_WARP, &swap_chain, &device, &device_context))
		{
			LOGF(FATAL, "DirectX device creation failed");
			return false;
		}
	}

	if (!swap_chain || !device || !device_context)
	{
		LOGF(FATAL, "DirectX objects are missing after device creation");
		return false;
	}

	ID3D11Texture2D* back_buffer = nullptr;
	if (FAILED(swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer))) || !back_buffer)
	{
		LOGF(FATAL, "Failed to acquire the back buffer");
		return false;
	}

	const auto result = device->CreateRenderTargetView(back_buffer, nullptr, &render_targetview);
	back_buffer->Release();

	if (FAILED(result) || !render_targetview)
	{
		LOGF(FATAL, "Failed to create the render target view");
		return false;
	}

	return true;
}

void Window::DestroyDevice()
{
	if (!device)
	{
		LOGF(WARNING, "No DirectX device available for destruction");
		return;
	}

	// The old code dereferenced every pointer unconditionally, which crashed
	// when initialisation stopped half way through.
	if (render_targetview)
	{
		render_targetview->Release();
		render_targetview = nullptr;
	}

	if (swap_chain)
	{
		swap_chain->Release();
		swap_chain = nullptr;
	}

	if (device_context)
	{
		device_context->Release();
		device_context = nullptr;
	}

	device->Release();
	device = nullptr;
}

bool Window::SpawnWindow()
{
	ImGui_ImplWin32_EnableDpiAwareness();

	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = "wa";
	wc.lpfnWndProc = window_procedure;

	if (!RegisterClassEx(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		LOGF(FATAL, "Failed to register the overlay window class");
		return false;
	}

	const int width = GetSystemMetrics(SM_CXSCREEN);
	const int height = GetSystemMetrics(SM_CYSCREEN);

	hwnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		"wa",
		"wa",
		WS_POPUP | WS_VISIBLE,
		0, 0, width, height,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	if (hwnd == nullptr)
	{
		LOGF(FATAL, "Failed to create Window");
		return false;
	}

	// Layered colour key: pure black pixels are transparent.
	if (!SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), BYTE(255), LWA_ALPHA))
	{
		LOGF(FATAL, "Failed to set layered window attributes");
		DestroyWindow(hwnd);
		hwnd = nullptr;
		return false;
	}

	const MARGINS margins = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	return true;
}

void Window::DespawnWindow()
{
	if (!hwnd)
		return;

	DestroyWindow(hwnd);
	hwnd = nullptr;

	if (wc.lpszClassName)
		UnregisterClass(wc.lpszClassName, wc.hInstance);

	LOGF(VERBOSE, "Window Despawned");
}

bool Window::CreateImGui()
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigViewportsNoTaskBarIcon = true;
	io.ConfigViewportsNoAutoMerge = true;

	if (!ImGui_ImplWin32_Init(hwnd))
	{
		LOGF(FATAL, "Failed ImGui_ImplWin32_Init");
		return false;
	}

	if (!ImGui_ImplDX11_Init(device, device_context))
	{
		LOGF(FATAL, "Failed ImGui_ImplDX11_Init");
		return false;
	}

	LOGF(VERBOSE, "ImGui Initialized");
	return true;
}

void Window::DestroyImGui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	LOGF(VERBOSE, "Imgui Destroyed");
}

void Window::StartRender()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			shouldRun = false;
	}

	if (!shouldRun)
		return;

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Window::EndRender()
{
	if (!shouldRun || !render_targetview || !device_context || !swap_chain)
		return;

	ImGui::Render();

	const float color[4]{ 0, 0, 0, 0 };

	device_context->OMSetRenderTargets(1, &render_targetview, nullptr);
	device_context->ClearRenderTargetView(render_targetview, color);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	const HRESULT present = swap_chain->Present(vsync ? 1U : 0U, 0U);

	// A lost device means the overlay can no longer draw; stop instead of
	// spinning on a dead swap chain.
	if (present == DXGI_ERROR_DEVICE_REMOVED || present == DXGI_ERROR_DEVICE_RESET)
	{
		LOGF(FATAL, "DirectX device was removed - stopping the renderer");
		shouldRun = false;
	}
}

void Window::SetTopMost(HWND window, bool up_down) {
	SetWindowPos(
		window,
		up_down ? HWND_TOPMOST : HWND_NOTOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER
	);
}

void Window::SetClickthrough(HWND window, bool clickthrough)
{
	LONG_PTR style = GetWindowLongPtr(window, GWL_EXSTYLE);

	style |= WS_EX_LAYERED;

	if (clickthrough)
		style |= WS_EX_TRANSPARENT;
	else
		style &= ~WS_EX_TRANSPARENT;

	SetWindowLongPtr(window, GWL_EXSTYLE, style);
}

void Window::SetBounds(HWND window, const RECT& new_bounds) {
	SetWindowPos(
		window,
		nullptr,
		new_bounds.left, new_bounds.top,
		new_bounds.right - new_bounds.left,
		new_bounds.bottom - new_bounds.top,
		SWP_NOZORDER | SWP_NOACTIVATE
	);

	// The previous version contained "bounds = bounds", which assigned the
	// parameter to itself and left the cached rect untouched.
	Window::bounds = new_bounds;

	UpdateWindow(window);
}

bool Window::SetAffinity(HWND window, WindowAffinity afi) {
	auto mode = WDA_NONE;
	[[maybe_unused]] const char* mode_str = "Disabled";

	switch (afi) {
	case WindowAffinity::Black:
		mode = WDA_MONITOR;
		mode_str = "Black";
		break;
	case WindowAffinity::Invisible:
		mode = WDA_EXCLUDEFROMCAPTURE;
		mode_str = "Invisible";
		break;
	default:
		mode = WDA_NONE;
		mode_str = "Disabled";
		break;
	}

	const auto status = SetWindowDisplayAffinity(window, mode);

	if (status)
		LOGF(VERBOSE, "Set Window Affinity to {}", mode_str);
	else
		LOGF(WARNING, "Failed to set Window Affinity to {}", mode_str);

	return status != 0;
}

void Window::SetForeground(HWND window) {
	if (!IsWindowInForeground(window))
		BringToForeground(window);
}

void Window::SetVSync(bool enable) {
	vsync = enable;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
		return 1;

	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		LOGF(VERBOSE, "Window procedure WM_DESTROY event triggered");
		Window::shouldRun = false;
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		LOGF(VERBOSE, "Window procedure WM_CLOSE event triggered");
		Window::shouldRun = false;
		DestroyWindow(window);
		break;
	case WM_SIZE:
		if (Window::device != nullptr && wParam != SIZE_MINIMIZED)
		{
			if (Window::render_targetview != nullptr)
			{
				Window::render_targetview->Release();
				Window::render_targetview = nullptr;
			}

			Window::swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* back_buffer = nullptr;
			Window::swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

			if (back_buffer)
			{
				Window::device->CreateRenderTargetView(back_buffer, nullptr, &Window::render_targetview);
				back_buffer->Release();

				return 1;
			}
		}
		return 0;
	case WM_DPICHANGED:
		LOGF(VERBOSE, "Window procedure WM_DPICHANGED event triggered");
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
		{
			const RECT* suggested_rect = reinterpret_cast<RECT*>(lParam);

			SetWindowPos(
				Window::hwnd,
				nullptr,
				suggested_rect->left, suggested_rect->top,
				suggested_rect->right - suggested_rect->left,
				suggested_rect->bottom - suggested_rect->top,
				SWP_NOZORDER | SWP_NOACTIVATE
			);
		}
		break;
	}

	return DefWindowProc(window, msg, wParam, lParam);
}
