#pragma once

#include <string>

#ifdef _DEBUG
    #include <iostream>
    #include <chrono>
    #include <ctime>
    #include <format>
    #include <Windows.h>

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
            const auto now = std::chrono::system_clock::now();
            const auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time_t);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &tm);

            std::cout << "[" << timestamp << "][" << level << "] " << message << std::endl;
        }
    };

    #define LOGF(level, ...) LogHelper::Log(#level, std::format(__VA_ARGS__))
#else
    // Release builds stay silent, but the variadic arguments are still parsed,
    // so the level name never shadows a real identifier.
    class LogHelper {
    public:
        static bool Init() { return true; }
        static void Destroy() {}
        static void Free() {}
        static void Log(const std::string&, const std::string&) {}
    };

    #define LOGF(level, ...) ((void)0)
#endif
