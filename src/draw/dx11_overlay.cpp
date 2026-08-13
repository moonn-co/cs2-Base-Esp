#include "dx11_overlay.hpp"

ID3D11Device* Window::device = nullptr;
ID3D11DeviceContext* Window::device_context = nullptr;
IDXGISwapChain* Window::swap_chain = nullptr;
ID3D11RenderTargetView* Window::render_targetview = nullptr;

bool Window::vsync = false;
HWND Window::hwnd = nullptr;
HWND Window::viewport = nullptr;
WNDCLASSEX Window::wc = { };

extern LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

bool Window::CreateDevice()
{
	
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));

	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

	HRESULT device_status = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		featureLevelArray,
		2,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&featureLevel,
		&device_context);

	if (device_status == DXGI_ERROR_UNSUPPORTED) {
		device_status = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_WARP,
			nullptr,
			0U,
			featureLevelArray,
			2, D3D11_SDK_VERSION,
			&sd,
			&swap_chain,
			&device,
			&featureLevel,
			&device_context);

		LOGF(FATAL, "DXGI_ERROR | Created with D3D_DRIVER_TYPE_WARP");
	}

	if (device_status != S_OK) {
		LOGF(FATAL, "DirectX device creation failed");
		return false;
	}

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	if (back_buffer)
	{
		device->CreateRenderTargetView(back_buffer, nullptr, &render_targetview);
		back_buffer->Release();

		return true;
	}

	LOGF(FATAL, "Failed to Create Device");
	return false;
}

void Window::DestroyDevice()
{
	if (device)
	{
		device->Release();
		device_context->Release();
		swap_chain->Release();
		render_targetview->Release();

		device = nullptr;

	}
	else
		LOGF(WARNING, "No DirectX device available for destruction");
}

bool Window::SpawnWindow()
{
	ImGui_ImplWin32_EnableDpiAwareness();
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = "wa";
	wc.lpfnWndProc = window_procedure;

	RegisterClassEx(&wc);

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	hwnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		"wa",
		"wa",
		WS_POPUP | WS_VISIBLE,
		0, 0, width, height,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	if (hwnd == NULL) {
		LOGF(FATAL, "Failed to create Window");
		return false;
	}
	
	if (!SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), BYTE(255), LWA_ALPHA)) {
		LOGF(FATAL, "Failed to set layered window attributes");
		return false;
	}

	MARGINS margins = { -1 };
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetForeground(hwnd);

	return true;
}

void Window::DespawnWindow()
{
	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	LOGF(VERBOSE, "Window Despawned");
}

bool Window::CreateImGui()
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	
	io.IniFilename = nullptr;

	io.ConfigViewportsNoTaskBarIcon = true;
	io.ConfigViewportsNoAutoMerge = true;

	if (!ImGui_ImplWin32_Init(hwnd)) {
		LOGF(FATAL, "Failed ImGui_ImplWin32_Init");
		return false;
	}

	if (!ImGui_ImplDX11_Init(device, device_context)) {
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

	
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Window::EndRender()
{
	
	ImGui::Render();

	
	float color[4]{ 0, 0, 0, 0 };

	
	device_context->OMSetRenderTargets(1, &render_targetview, nullptr);
	device_context->ClearRenderTargetView(render_targetview, color);

	
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	auto io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	
	if (vsync) 
		swap_chain->Present(1U, 0U);
	else 
		swap_chain->Present(0U, 0U);
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

void Window::SetBounds(HWND window, RECT bounds) {
	SetWindowPos(
		window,
		nullptr,
		bounds.left, bounds.top,
		bounds.right - bounds.left,
		bounds.bottom - bounds.top,
		SWP_NOZORDER | SWP_NOACTIVATE
	);

	bounds = bounds;
	UpdateWindow(window);
}

bool Window::SetAffinity(HWND window, WindowAffinity afi) {
	auto mode = WDA_NONE;
	std::string mode_str = "Disabled";

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

	auto status = SetWindowDisplayAffinity(window, mode);

	if (status)
		LOGF(VERBOSE, "Set Window Affinity to {}", mode_str);
	else
		LOGF(FATAL, "Failed to set Window Affinity to {}", mode_str);

	return status;
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
		return true;

	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_KEYUP:
	{
		bool toggle_key = wParam == VK_INSERT || wParam == VK_SHIFT && ((wParam >> 16) & 0xFF) == 0x36;

		if (toggle_key) {
			// ESP is always on - no menu toggle needed
		}
		break;
	}
	case WM_DESTROY: 
		LOGF(VERBOSE, "Window procedure WM_DESTROY event triggered"); 
		break;
	case WM_CLOSE:
		LOGF(VERBOSE, "Window procedure WM_CLOSE event triggered");
		
		Window::shouldRun = false; 
		break;
	case WM_SIZE:
		
		if (Window::device != nullptr && wParam != SIZE_MINIMIZED)
		{
			if (Window::render_targetview != nullptr) {
				Window::render_targetview->Release();
				Window::render_targetview = nullptr;
			}

			
			Window::swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

			
			ID3D11Texture2D* back_buffer{ nullptr };
			Window::swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

			
			if (back_buffer)
			{
				Window::device->CreateRenderTargetView(back_buffer, nullptr, &Window::render_targetview);
				back_buffer->Release();

				return true;
			}
		}
		return 0;
	case WM_DPICHANGED:
		LOGF(VERBOSE, "Window procedure WM_DPICHANGED event triggered");
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
		{
			const RECT* suggested_rect = (RECT*)lParam;

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
