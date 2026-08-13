#pragma once


#ifdef _DEBUG
    #include <fstream>
    #include <iostream>
    #include <string>
    #include <chrono>
    #include <ctime>
    #include <format>

    class LogHelper {
    public:
        static bool Init() { 
            AllocConsole(); 
            SetConsoleTitleA("CS2 ESP Debug");
            return true; 
        }
        static void Destroy() { FreeConsole(); }
        static void Free() { FreeConsole(); }
        static void Log(const std::string& level, const std::string& message) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &time_t);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &tm);
            
            std::cout << "[" << timestamp << "][" << level << "] " << message << std::endl;
        }
    };

    #define LOGF(level, ...) LogHelper::Log(#level, std::format(__VA_ARGS__))
#else
    
    class LogHelper {
    public:
        static bool Init() { return true; }
        static void Destroy() {}
        static void Free() {}
        static void Log(const std::string&, const std::string&) {}
    };

    #define LOGF(level, ...) ((void)0)
#endif