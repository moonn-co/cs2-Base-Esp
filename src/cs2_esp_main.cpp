#include "game/cs2_game_engine.hpp"
#include "draw/directx_renderer.hpp"

#include "utils/exception_handler.hpp"

int main()
{
    c_exception_handler::setup();
    LogHelper::Init();

    LOGF(INFO, "CS2 ESP Simple - Starting...");

    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        LOGF(WARNING, "Failed to set high priority class for application process");

    if (!Engine::Init()) {
        LOGF(FATAL, "Engine initialization failed - terminating application");
        goto exit;
    }

    if (!Renderer::Init()) {
        LOGF(FATAL, "Renderer initialization failed - terminating application");
        goto exit;
    }

    LOGF(INFO, "ESP initialized - Box, Skeleton, Health, Weapon Name activated");
    LOGF(INFO, "Press END to quit");

    Renderer::Thread();

exit:
    LOGF(INFO, "CS2 ESP Simple - Shutting down");
    LogHelper::Destroy();
    return 0;
}