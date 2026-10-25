#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "../cfg/active_configuration.hpp"
#include "../mem/process_memory.hpp"

class Engine {
public:
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    static bool Init();
    static void Shutdown();

    static bool Running();
    static ProcessModule GetClient();
    static ProcessModule GetEngine();
    static std::shared_ptr<pProcess> GetProcess();
private:
    Engine() = default;

    static Engine& GetInstance()
    {
        static Engine i{};
        return i;
    }

    bool InitImpl();
    void ShutdownImpl();

    bool AwaitProcess();
    bool AwaitModules();

    void Thread();

private:
    std::shared_ptr<pProcess> process;
    ProcessModule client{ 0, 0 };
    ProcessModule engine{ 0, 0 };

    std::thread worker;
    std::atomic<bool> running{ false };
    int process_attempts = 0;
    int module_attempts = 0;
};
