#include "signature_scanner.hpp"

#include <cstring>
#include <sstream>

#include "cs2_memory_offsets.hpp"
#include "../game/cs2_game_engine.hpp"

namespace
{
    // Resolves RIP-relative operands such as "48 8B 05 ?? ?? ?? ?? + disp32 + 7".
    // The instruction length is derived from the pattern instead of being
    // hardcoded, so a pattern with a different prefix length no longer resolves
    // to a wrong address.
    uint64_t ResolveRelative(pProcess& process, uint64_t match_address, const std::string& sig)
    {
        std::vector<uint64_t> wildcard_positions;

        std::istringstream iss(sig);
        std::string token;
        uint64_t position = 0;

        while (iss >> token)
        {
            if (token == "??" || token == "?")
                wildcard_positions.push_back(position);

            position++;
        }

        // A RIP-relative displacement is the first run of four wildcards.
        if (wildcard_positions.size() < 4)
            return 0;

        const uint64_t disp_position = wildcard_positions.front();
        const uint64_t instruction_end = disp_position + sizeof(int32_t);

        if (disp_position == 0 || disp_position > 0x0F)
            return 0;

        int32_t displacement = 0;
        if (!process.read_raw(match_address + disp_position, &displacement, sizeof(displacement)))
            return 0;

        return match_address + instruction_end + displacement;
    }
}

bool Dumper::Init() {
    auto process = Engine::GetProcess();
    auto client = Engine::GetClient();
    auto engine = Engine::GetEngine();

    if (!process || !client.base || !engine.base)
    {
        LOGF(FATAL, "Scanner was called before the process/modules were resolved");
        return false;
    }

    struct ScanEntry
    {
        const char* name;
        const std::string* signature;
        ProcessModule module;
        DWORD* out_offset;
    };

    const ScanEntry entries[] = {
        { "ViewMatrix",            &offsets::signatures::viewMatrix,            client, &offsets::viewMatrix },
        { "EntityList",            &offsets::signatures::entityList,            client, &offsets::entityList },
        { "LocalPlayerController", &offsets::signatures::localPlayerController, client, &offsets::localPlayerController },
        { "BuildNumber",           &offsets::signatures::buildNumber,           engine, &offsets::buildNumber },
    };

    for (const auto& entry : entries)
    {
        const auto match = Scan(*entry.signature, entry.module);

        if (!match)
        {
            LOGF(FATAL, "{} signature scan failed - outdated pattern", entry.name);
            return false;
        }

        *entry.out_offset = static_cast<DWORD>(match - entry.module.base);
    }

    // Cheap sanity check: an offset outside of its module cannot be a static
    // game address, which means the scan resolved to something meaningless.
    if (offsets::viewMatrix >= client.size || offsets::entityList >= client.size ||
        offsets::localPlayerController >= client.size || offsets::buildNumber >= engine.size)
    {
        LOGF(FATAL, "Resolved offsets are outside of their modules - patterns likely outdated");
        return false;
    }

    LOGF(INFO, "Successfully dumped offsets...");
    return true;
}

std::vector<uint16_t> Dumper::StrSigToArray(const std::string& sig) {
    std::istringstream iss(sig);
    std::vector<uint16_t> bytes;
    std::string byte_str;

    while (iss >> byte_str) {
        if (byte_str == "??" || byte_str == "?")
        {
            bytes.push_back(256);
            continue;
        }

        try
        {
            const auto value = std::stoul(byte_str, nullptr, 16);
            if (value > 0xFF)
                return {};

            bytes.push_back(static_cast<uint16_t>(value));
        }
        catch (const std::exception&)
        {
            // A malformed pattern used to throw out of the scanner and kill the
            // process during start-up.
            LOGF(FATAL, "Invalid signature token '{}'", byte_str);
            return {};
        }
    }
    return bytes;
}

void Dumper::ScanBlock(pProcess& process, const std::vector<uint16_t>& signature, uint64_t start, DWORD size, std::vector<uint64_t>& found_offsets)
{
    if (size == 0 || size > MAX_BLOCK_SIZE || signature.empty() || signature.size() > size)
        return;

    // The scratch buffer is reused between blocks instead of being allocated
    // with new/delete for every single block.
    static thread_local std::vector<uint8_t> buffer;
    buffer.resize(size);

    if (!process.read_raw(start, buffer.data(), size))
        return;

    const size_t length = signature.size();
    const uint8_t* data = buffer.data();
    const size_t last_start = size - length;

    const auto matches_at = [&](size_t i) {
        for (size_t k = 0; k < length; k++)
        {
            if (signature[k] != 256 && signature[k] != data[i + k])
                return false;
        }
        return true;
    };

    // Every shipped signature starts with a literal byte, so memchr skips over
    // the block instead of comparing each offset. Patterns that start with a
    // wildcard fall back to a linear pass.
    if (signature[0] != 256)
    {
        const uint8_t first = static_cast<uint8_t>(signature[0]);
        const uint8_t* cursor = data;
        const uint8_t* const block_end = data + size;

        while (cursor <= data + last_start)
        {
            const auto found = static_cast<const uint8_t*>(
                memchr(cursor, first, static_cast<size_t>(block_end - cursor))
            );

            if (!found)
                break;

            const size_t index = static_cast<size_t>(found - data);
            if (index > last_start)
                break;

            if (matches_at(index))
                found_offsets.push_back(start + index);

            cursor = found + 1;
        }

        return;
    }

    for (size_t i = 0; i <= last_start; i++)
    {
        if (matches_at(i))
            found_offsets.push_back(start + i);
    }
}

std::vector<uint64_t> Dumper::ScanMemory(const std::string& sig, uint64_t start, uint64_t end, size_t stop_after)
{
    std::vector<uint64_t> scan_results;

    auto process = Engine::GetProcess();

    if (!process || !process->handle_ || stop_after == 0)
        return scan_results;

    const auto signature = StrSigToArray(sig);
    if (signature.empty())
        return scan_results;

    MEMORY_BASIC_INFORMATION mbi{};

    while (start != 0 && VirtualQueryEx(process->handle_, reinterpret_cast<LPCVOID>(start), &mbi, sizeof(mbi)) != 0)
    {
        // Only committed, readable pages are scanned. The previous version let
        // the kernel read across unreadable regions and aborted the whole scan
        // on the first failure, so patterns behind a guard page were missed.
        const bool readable = mbi.State == MEM_COMMIT &&
            !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                            PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));

        if (readable && mbi.RegionSize > 0)
        {
            uint64_t address = reinterpret_cast<uint64_t>(mbi.BaseAddress);
            DWORD remaining = static_cast<DWORD>(mbi.RegionSize);

            while (remaining >= MAX_BLOCK_SIZE)
            {
                if (scan_results.size() >= stop_after || (end != 0 && address + MAX_BLOCK_SIZE > end))
                    return scan_results;

                ScanBlock(*process, signature, address, MAX_BLOCK_SIZE, scan_results);

                address += MAX_BLOCK_SIZE;
                remaining -= MAX_BLOCK_SIZE;
            }

            if (remaining > 0 && (end == 0 || address < end))
                ScanBlock(*process, signature, address, remaining, scan_results);
        }

        const auto region_end = reinterpret_cast<uint64_t>(mbi.BaseAddress) + mbi.RegionSize;

        // Guard against a zero-sized region that would loop forever.
        if (region_end <= start)
            start += 0x1000;
        else
            start = region_end;

        if (scan_results.size() >= stop_after || (end != 0 && start >= end))
            break;
    }

    return scan_results;
}

uint64_t Dumper::Scan(const std::string& sig, ProcessModule module)
{
    auto process = Engine::GetProcess();

    if (!process || !module.base || !module.size)
        return 0;

    const auto end = module.base + (module.size < 0x4000000 ? module.size : 0x4000000);
    const auto list = ScanMemory(sig, module.base, end, 1);

    if (list.empty())
        return 0;

    return ResolveRelative(*process, list.front(), sig);
}
