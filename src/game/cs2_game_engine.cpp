#include "cs2_game_engine.hpp"

#include "../mem/signature_scanner.hpp"
#include "cache/entity_cache.hpp"

using namespace std::chrono_literals;

namespace
{
    constexpr int kMaxAttempts = 10;
    constexpr auto kAttemptDelay = 5s;
}

bool Engine::Init() {
    return GetInstance().InitImpl();
}

void Engine::Shutdown() {
    GetInstance().ShutdownImpl();
}

bool Engine::Running() {
    return GetInstance().running.load();
}

ProcessModule Engine::GetClient() {
    return GetInstance().client;
}

ProcessModule Engine::GetEngine() {
    return GetInstance().engine;
}

std::shared_ptr<pProcess> Engine::GetProcess() {
    return GetInstance().process;
}

bool Engine::InitImpl() {
    process = std::make_shared<pProcess>();

    if (!this->AwaitProcess()) {
        LOGF(FATAL, "CS2 process not found - ensure game is running");
        return false;
    }

    if (!this->AwaitModules()) {
        LOGF(FATAL, "Game took too long to load, please open me again once its fully loaded");
        return false;
    }

    if (!Dumper::Init()) {
        LOGF(FATAL, "Failed to dump game offsets");
        return false;
    }

    running = true;
    worker = std::thread(&Engine::Thread, this);

    LOGF(INFO, "Successfully initialized engine...");
    return true;
}

void Engine::ShutdownImpl() {
    running = false;

    if (worker.joinable())
        worker.join();

    Cache::Reset();

    if (process)
        process->Close();
}

void Engine::Thread() {
    // A single tick rate drives the cache so the loop never busy-spins.
    const auto interval = std::chrono::milliseconds(cfg::settings::free_cpu ? cfg::settings::cache_refresh_ms : 0);

    while (running.load()) {
        const auto start = std::chrono::steady_clock::now();

        // Stop cleanly when the game is closed instead of reading a dead handle.
        if (!process || !process->handle_ || WaitForSingleObject(process->handle_, 0) != WAIT_TIMEOUT) {
            LOGF(INFO, "Game process is gone - stopping the engine thread");
            running = false;
            break;
        }

        Cache::Refresh();

        if (interval.count() > 0) {
            const auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed < interval)
                std::this_thread::sleep_for(interval - elapsed);
        }
        else {
            std::this_thread::yield();
        }
    }
}

bool Engine::AwaitProcess() {
    if (!process)
        return false;

    if (process->handle_)
        return true;

    LOGF(INFO, "Waiting up to 50s for the game to open...");

    do {
        if (process->AttachProcess("cs2.exe"))
            break;

        if (process->pid_ && !process->handle_) {
            LOGF(FATAL, "Insufficient permissions to open a handle to the process. Try running as Administrator.");
            return false;
        }

        if (++process_attempts > kMaxAttempts)
            return false;

        std::this_thread::sleep_for(kAttemptDelay);
    } while (true);

    return true;
}

bool Engine::AwaitModules() {
    if (!process || !process->handle_)
        return false;

    LOGF(INFO, "Waiting for the game modules...");

    do {
        this->client = process->GetModule("client.dll");
        this->engine = process->GetModule("engine2.dll");

        if (this->client.base && this->engine.base)
            break;

        if (++module_attempts > kMaxAttempts)
            return false;

        std::this_thread::sleep_for(kAttemptDelay);
    } while (true);

    return true;
}
