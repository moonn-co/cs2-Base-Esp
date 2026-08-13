#include "signature_scanner.hpp"

#include "../game/cs2_game_engine.hpp"
#include "cs2_memory_offsets.hpp"

bool Dumper::Init() {
    return GetInstance().InitImpl();
}

bool Dumper::InitImpl() {
    auto process = Engine::GetProcess();
    auto client = Engine::GetClient();
    auto engine = Engine::GetEngine();

    DWORD64 scanned_address = 0;

    if (!(scanned_address = Scan(offsets::signatures::viewMatrix, client))) {
        LOGF(FATAL, "ViewMatrix signature scan failed - outdated pattern");
        return false;
    }

    offsets::viewMatrix = static_cast<DWORD>(scanned_address - client.base);

    
    if (!(scanned_address = Scan(offsets::signatures::entityList, client))) {
        LOGF(FATAL, "EntityList signature scan failed - outdated pattern");
        return false;
    }

    offsets::entityList = static_cast<DWORD>(scanned_address - client.base);

    
    if (!(scanned_address = Scan(offsets::signatures::localPlayerController, client))) {
        LOGF(FATAL, "LocalPlayerController signature scan failed - outdated pattern");
        return false;
    }

    offsets::localPlayerController = static_cast<DWORD>(scanned_address - client.base);

    

    
    if (!(scanned_address = Scan(offsets::signatures::buildNumber, engine))) {
        LOGF(FATAL, "BuildNumber signature scan failed - outdated pattern");
        return false;
    }

    offsets::buildNumber = static_cast<DWORD>(scanned_address - engine.base);

    LOGF(INFO, "Successfully dumped offsets...");

    return true;
}

DWORD64 Dumper::Scan(const std::string sig, ProcessModule module) {
    auto process = Engine::GetProcess();

    if (!process)
        return 0;

    DWORD offsets = 0;
    DWORD64 address = 0;
    std::vector<DWORD64> list;

    
    list = ScanMemory(sig, module.base, module.base + 0x4000000);

    if (!list.size())
        return 0;

    if (!process->read_raw(list.at(0) + 3, &offsets, sizeof(DWORD)))
        return 0;

    address = list.at(0) + offsets + 7;
    return address;
}

std::vector<WORD> Dumper::StrSigToArray(const std::string& sig) {
    std::istringstream iss(sig);
    std::vector<WORD> bytes;
    std::string byte_str;

    while (iss >> byte_str) {
        if (byte_str == "??" || byte_str == "?")
            bytes.push_back(256);
        else
            bytes.push_back(static_cast<WORD>(std::stoul(byte_str, nullptr, 16)));
    }
    return bytes;
}

void Dumper::GetNextArray(std::vector<short>& next, const std::vector<WORD>& signature)
{
    auto size = signature.size();
    for (int i = 0; i < size; i++)
        next[signature[i]] = i;
}

void Dumper::ScanBlock(byte* buffer, const std::vector<short>& next, const std::vector<WORD>& signature, DWORD64 start, DWORD size, std::vector<DWORD64>& found_offsets)
{
    auto process = Engine::GetProcess();

    if (!process->read_raw(start, buffer, size))
        return;

    int length = static_cast<int>(signature.size());

    for (int i = 0, j, k; i < static_cast<int>(size);)
    {
        j = i; k = 0;

        for (; k < length && j < static_cast<int>(size) && (signature[k] == buffer[j] || signature[k] == 256); k++, j++);

        if (k == length)
            found_offsets.push_back(start + i);

        if ((i + length) >= static_cast<int>(size))
            return;

        int Num = next[buffer[i + length]];
        if (Num == -1)
            i += (length - next[256]);
        else
            i += (length - Num);
    }
}

std::vector<DWORD64> Dumper::ScanMemory(const std::string& sig, DWORD64 start, DWORD64 end, int number)
{
    std::vector<DWORD64> scan_results;
    std::vector<short> next(260, -1);

    auto process = Engine::GetProcess();

    if (!process)
        return scan_results;

    byte* buffer = new byte[MAX_BLOCK_SIZE];

    auto signature = StrSigToArray(sig);
    if (!signature.size())
        return scan_results;

    GetNextArray(next, signature);

    MEMORY_BASIC_INFORMATION mbi;
    while (VirtualQueryEx(process->handle_, reinterpret_cast<LPCVOID>(start), &mbi, sizeof(mbi)) != 0)
    {
        int searches = 0;
        auto size = mbi.RegionSize;

        while (size >= MAX_BLOCK_SIZE)
        {
            if (scan_results.size() >= number) {
                delete[] buffer;
	            return scan_results;
            }

            ScanBlock(buffer, next, signature, start + (static_cast<DWORD64>(MAX_BLOCK_SIZE) * searches), MAX_BLOCK_SIZE, scan_results);

            size -= MAX_BLOCK_SIZE;
            searches++;
        }

        ScanBlock(buffer, next, signature, start + (static_cast<DWORD64>(MAX_BLOCK_SIZE) * searches), static_cast<DWORD>(size), scan_results);

        start += mbi.RegionSize;

        if (scan_results.size() >= number || end != 0 && start > end)
            break;
    }

	delete[] buffer;
	return scan_results;
}