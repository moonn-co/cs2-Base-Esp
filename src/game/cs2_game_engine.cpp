#include "cs2_game_engine.hpp"

#include "../mem/signature_scanner.hpp"
#include "cache/entity_cache.hpp"

bool Engine::Init() {
    return GetInstance().InitImpl();
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

    std::thread(&Engine::Thread, this).detach();

    LOGF(INFO, "Successfully initialized engine...");
    return true;
}

void Engine::Thread() {
    
    

    while (true) {
        auto start = steady_clock::now();

        Cache::Refresh();

        if (cfg::settings::free_cpu)
            std::this_thread::sleep_until(start + 1ms);
    }
}

bool Engine::AwaitProcess() {
    if (!process || process->handle_) 
        return false;

    do {
        if (process->AttachProcess("cs2.exe"))
            break;

        if (process->pid_ && !process->handle_) {
            LOGF(FATAL, "Insufficient permissions to open a handle to the process. Try running as Administrator.");
            return false;
        }

        static int attempts = 0;

        if (!attempts)
            LOGF(INFO, "Waiting 50s for the game to open...");

        if (attempts > 10)
            return false;
        attempts++;

        std::this_thread::sleep_for(5s);
    } while (true);

    return true;
}

bool Engine::AwaitModules() {
    if (!process || !process->handle_) 
        return false;

    LOGF(INFO, "Waiting for the game to open...");

    do {
        this->client = process->GetModule("client.dll");
        this->engine = process->GetModule("engine2.dll");

        if (this->client.base && this->engine.base)
            break;

        static int attempts = 0;
        if (attempts > 10)
            return false;
        attempts++;

        std::this_thread::sleep_for(5s);
    } while (true);

    return true;
}
