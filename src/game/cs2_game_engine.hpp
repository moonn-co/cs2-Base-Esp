#pragma once
#include "../cfg/active_configuration.hpp"
#include "../mem/process_memory.hpp"

class Engine {
public:
    ~Engine()                           = default;
    Engine(const Engine&)            = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&)      = delete;

   static bool Init();
   static ProcessModule GetClient();
   static ProcessModule GetEngine();
   static std::shared_ptr<pProcess> GetProcess(); 
private:
    Engine() {};

    static Engine& GetInstance()
    {
        static Engine i{};
        return i;
    }

    bool InitImpl();

    bool AwaitProcess();
    bool AwaitModules();

    void Thread();

private:
    std::shared_ptr<pProcess> process;
    ProcessModule client;
    ProcessModule engine;
};