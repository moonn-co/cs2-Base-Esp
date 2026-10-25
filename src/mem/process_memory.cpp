#include "process_memory.hpp"
#include <tlhelp32.h>

uint32_t pProcess::FindProcessIdByProcessName(const char* ProcessName)
{
	if (!ProcessName || !*ProcessName)
		return 0;

	std::wstring wideProcessName;
	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, ProcessName, -1, nullptr, 0);
	if (wideCharLength > 0)
	{
		wideProcessName.resize(wideCharLength);
		MultiByteToWideChar(CP_UTF8, 0, ProcessName, -1, &wideProcessName[0], wideCharLength);
	}

	HANDLE hPID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hPID == INVALID_HANDLE_VALUE)
	{
		LOGF(FATAL, "Failed to create a process snapshot");
		return 0;
	}

	PROCESSENTRY32W process_entry_{ };
	process_entry_.dwSize = sizeof(PROCESSENTRY32W);

	DWORD pid = 0;
	if (Process32FirstW(hPID, &process_entry_))
	{
		do
		{
			if (!wcscmp(process_entry_.szExeFile, wideProcessName.c_str()))
			{
				pid = process_entry_.th32ProcessID;
				break;
			}
		} while (Process32NextW(hPID, &process_entry_));
	}
	CloseHandle(hPID);
	return pid;
}

HWND pProcess::GetWindowHandleFromProcessId(DWORD ProcessId) {
	HWND hwnd = NULL;
	do {
		hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == ProcessId) {
			TCHAR windowTitle[MAX_PATH];
			GetWindowText(hwnd, windowTitle, MAX_PATH);
			if (IsWindowVisible(hwnd) && windowTitle[0] != '\0') {
				return hwnd;
			}
		}
	} while (hwnd != NULL);
	return NULL; 
}

bool pProcess::AttachProcess(const char* process_name)
{
	auto target_pid = FindProcessIdByProcessName(process_name);

	if (!target_pid) {
		LOGF(INFO, "Process not found");
		return false;
	}

	pid_ = target_pid;

	// Read-only access. The project never writes into the target process, so
	// PROCESS_VM_WRITE / PROCESS_VM_OPERATION are not requested anymore.
	handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid_);

	if (!handle_) {
		LOGF(FATAL, "Failed to open process handle");
		return false;
	}

	base_client_ = GetModule("client.dll");
	if (!base_client_.base) {
		LOGF(FATAL, "Failed to get client.dll module");
		return false;
	}

	hwnd_ = GetWindowHandleFromProcessId(pid_);

	return true;
}

bool pProcess::UpdateHWND()
{
	hwnd_ = this->GetWindowHandleFromProcessId(pid_);
	return hwnd_ != nullptr;
}

ProcessModule pProcess::GetModule(const char* lModule)
{
	if (!lModule || !*lModule || !pid_)
		return { 0, 0 };

	std::wstring wideModule;
	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, lModule, -1, nullptr, 0);
	if (wideCharLength > 0)
	{
		wideModule.resize(wideCharLength);
		MultiByteToWideChar(CP_UTF8, 0, lModule, -1, &wideModule[0], wideCharLength);
	}

	HANDLE handle_module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid_);
	if (handle_module == INVALID_HANDLE_VALUE)
		return { 0, 0 };

	ProcessModule result{ 0, 0 };

	MODULEENTRY32W module_entry_{};
	module_entry_.dwSize = sizeof(MODULEENTRY32W);

	// Module32FirstW must be called once before Module32NextW, otherwise the
	// first (zeroed) entry is compared and the API sequence is invalid.
	if (Module32FirstW(handle_module, &module_entry_))
	{
		do
		{
			if (!wcscmp(module_entry_.szModule, wideModule.c_str()))
			{
				result = { reinterpret_cast<uintptr_t>(module_entry_.modBaseAddr), static_cast<uintptr_t>(module_entry_.modBaseSize) };
				break;
			}
		} while (Module32NextW(handle_module, &module_entry_));
	}

	CloseHandle(handle_module);
	return result;
}

uintptr_t pProcess::FindSignature(std::vector<uint8_t> signature)
{
	return FindSignature(base_client_, signature);
}

uintptr_t pProcess::FindSignature(ProcessModule target_module, std::vector<uint8_t> signature)
{
	if (!handle_ || !target_module.base || !target_module.size || signature.empty())
		return 0x0;

	if (signature.size() > target_module.size)
		return 0x0;

	std::vector<uint8_t> data(target_module.size);

	if (!read_raw(target_module.base, data.data(), data.size()))
		return 0x0;

	// A zero byte in the pattern is treated as a wildcard.
	const size_t last_start = data.size() - signature.size();

	for (size_t i = 0; i <= last_start; i++)
	{
		bool found = true;

		for (size_t j = 0; j < signature.size(); j++)
		{
			const uint8_t expected = signature[j];
			if (expected != 0x00 && data[i + j] != expected)
			{
				found = false;
				break;
			}
		}

		if (found)
			return target_module.base + i;
	}

	return 0x0;
}

void pProcess::Close()
{
	if (!handle_)
		return;

	CloseHandle(handle_);
	handle_ = nullptr;
	pid_ = 0;
	hwnd_ = nullptr;
	base_client_ = { 0, 0 };
}
