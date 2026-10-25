#include "directx_renderer.hpp"

#include <chrono>
#include <thread>

#include "dx11_overlay.hpp"
#include "../game/cs2_game_engine.hpp"
#include "visual_esp.hpp"

using namespace std::chrono_literals;

bool Renderer::Init() {
    return GetInstance().InitImpl();
}

void Renderer::Thread() {
    GetInstance().ThreadImpl();
}

void Renderer::Destroy() {
    GetInstance().DestroyImpl();
}

bool Renderer::IsOpen() {
    return GetInstance().isRunning.load() && Window::hwnd != nullptr;
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

    if (!Esp::Init()) {
        LOGF(FATAL, "Failed to initialize ESP renderer");
        return false;
    }

    Window::SetClickthrough(Window::hwnd, true);

    auto process = Engine::GetProcess();
    if (process && process->hwnd_)
        SetForegroundWindow(process->hwnd_);

    LOGF(INFO, "Successfully initialized ESP renderer...");
    return true;
}

void Renderer::DestroyImpl() {
    isRunning = false;
    Window::shouldRun = false;
}

void Renderer::ThreadImpl() {
    // The overlay is hidden while the game is in the background, so the loop
    // parks instead of presenting an invisible frame at full speed.
    while (isRunning.load()) {
        // Polled on every iteration, regardless of which window owns focus.
        HandleState();

        if (!isRunning.load())
            break;

        if (!HandleWindowOrder()) {
            if (!isRunning.load())
                break;

            std::this_thread::sleep_for(10ms);
            continue;
        }

        if (!overlay_visible) {
            std::this_thread::sleep_for(100ms);
            continue;
        }

        Render();
    }

    Window::DestroyImGui();
    Window::DestroyDevice();
    Window::DespawnWindow();

    LOGF(INFO, "Renderer stopped");
}

void Renderer::Render() {
    Window::StartRender();

    if (!Window::shouldRun)
    {
        isRunning = false;
        return;
    }

    Esp::Render();

    Window::EndRender();
}

bool Renderer::HandleState() {
    // The quit key has to be polled on every iteration. The old version only
    // checked it while the game window was focused, so END did nothing when the
    // overlay or another window had focus.
    if (GetAsyncKeyState(VK_END) & 0x8000)
        isRunning = false;

    if (!Window::shouldRun)
        isRunning = false;

    return false;
}

bool Renderer::HandleWindowOrder() {
    auto p = Engine::GetProcess();

    if (!p)
        return false;

    if (!p->hwnd_ && !p->UpdateHWND())
        return false;

    if (!IsWindow(p->hwnd_)) {
        LOGF(INFO, "Game window is gone - closing the overlay");
        isRunning = false;
        return false;
    }

    const auto foreground = GetForegroundWindow();
    this->isFocused = (foreground == Window::hwnd || foreground == p->hwnd_);

    if (!this->isFocused) {
        if (overlay_visible) {
            ShowWindow(Window::hwnd, SW_HIDE);
            overlay_visible = false;
        }

        return true;
    }

    if (!overlay_visible) {
        ShowWindow(Window::hwnd, SW_SHOW);
        overlay_visible = true;
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
