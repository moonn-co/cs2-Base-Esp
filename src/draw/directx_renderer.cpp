#include "directx_renderer.hpp"
#include "dx11_overlay.hpp"

#include "../game/cs2_game_engine.hpp"
#include "visual_esp.hpp"

bool Renderer::Init() {
    return GetInstance().InitImpl();
}

void Renderer::Thread() {
    return GetInstance().ThreadImpl();
}

void Renderer::Destroy() {
    return GetInstance().DestroyImpl();
}   

bool Renderer::IsOpen() {
    return false;
}

bool Renderer::InitImpl() {
    if (!Window::SpawnWindow()) {
        LOGF(FATAL, "Failed to create window");
        return false;
    }

    if (!Window::CreateDevice()) {
        LOGF(FATAL, "Failed to create device");
        return false;
    }

    if (!Window::CreateImGui()) {
        LOGF(FATAL, "Failed to create ImGui");
        return false;
    }

    Esp::Init();

    Window::SetClickthrough(Window::hwnd, true);

    SetForegroundWindow(Engine::GetProcess()->hwnd_);

    LOGF(INFO, "Successfully initialized ESP renderer...");
    return true;
}

void Renderer::DestroyImpl() {
    isRunning = false;
}

void Renderer::ThreadImpl() {
    while (isRunning) {
        Render();

        if (this->isFocused && HandleState())
            continue;

        HandleWindowOrder();
    }

    Window::DestroyImGui();
    Window::DestroyDevice();
    Window::DespawnWindow();
}

void Renderer::Render() {
    Window::StartRender();

    Esp::Render();

    Window::EndRender();
}

bool Renderer::HandleState() {
    isRunning = Window::shouldRun;

    bool pressed_end = (GetAsyncKeyState(VK_END) & 0x8000);

    if (pressed_end)
        this->isRunning = false;

    return false;
}

bool Renderer::HandleWindowOrder() {
    auto p = Engine::GetProcess();

    if (!p || (!p->hwnd_ && !p->UpdateHWND()))
        return false;

    if (!IsWindow(p->hwnd_))
        this->isRunning = false;

    static bool overlay_visible = true;
    auto foreground = GetForegroundWindow();
    this->isFocused = (foreground == Window::hwnd || foreground == p->hwnd_);

    if (!this->isFocused && overlay_visible) {
        ShowWindow(Window::hwnd, SW_HIDE);
        overlay_visible = false;
        return true;
    }

    if (!overlay_visible && this->isFocused) {  
        ShowWindow(Window::hwnd, SW_SHOW);
        overlay_visible = true;
        return true;
    }

    static RECT last_rect = { 0, 0, 0, 0 };

    RECT window_rect;
    if (!GetWindowRect(p->hwnd_, &window_rect))
        return false;

    if (memcmp(&window_rect, &last_rect, sizeof(RECT)) == 0)
        return true;

    RECT client_rect;
    if (!GetClientRect(p->hwnd_, &client_rect))
        return false;

    POINT top_left = { client_rect.left, client_rect.top };
    POINT bottom_right = { client_rect.right, client_rect.bottom };

    ClientToScreen(p->hwnd_, &top_left);
    ClientToScreen(p->hwnd_, &bottom_right);

    RECT screen_rect = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

    SetWindowPos(
        Window::hwnd,
        HWND_TOPMOST,
        screen_rect.left,
        screen_rect.top,
        screen_rect.right - screen_rect.left,
        screen_rect.bottom - screen_rect.top,
        SWP_NOACTIVATE | SWP_SHOWWINDOW
    );

    last_rect = window_rect;

    return true;
}