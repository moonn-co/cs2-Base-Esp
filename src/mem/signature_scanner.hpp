#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "process_memory.hpp"

inline constexpr DWORD MAX_BLOCK_SIZE = 409600;

// Stateless signature scanner. The old implementation kept a singleton and a
// hidden "next" table on the instance, which made it impossible to scan two
// patterns concurrently (the engine thread and the renderer both did).
class Dumper {
public:
    static bool Init();

    Dumper() = default;
    Dumper(const Dumper&) = delete;
    Dumper(Dumper&&) = delete;
    Dumper& operator=(const Dumper&) = delete;
    Dumper& operator=(Dumper&&) = delete;

private:
    // Converts "48 8B 0D ?? ?? ??" into bytes; 256 marks a wildcard.
    static std::vector<uint16_t> StrSigToArray(const std::string& sig);
    static void ScanBlock(pProcess& process, const std::vector<uint16_t>& signature, uint64_t start, DWORD size, std::vector<uint64_t>& found_offsets);
    static std::vector<uint64_t> ScanMemory(const std::string& sig, uint64_t start, uint64_t end, size_t stop_after = 1);
    static uint64_t Scan(const std::string& sig, ProcessModule module);
};
