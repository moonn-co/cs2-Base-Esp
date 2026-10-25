#ifndef _PPROCESS_HPP_
#define _PPROCESS_HPP_

#include <vector>
#include <cmath>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>
#include <Psapi.h>
#include "utils/debug_logger.hpp"

typedef NTSTATUS(WINAPI* pNtReadVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesRead);

// The project is read-only by design: only the read primitive is resolved here.
// NtWriteVirtualMemory is intentionally not imported anymore.
class pMemory {

public:
	static pMemory& Get()
	{
		static pMemory instance;
		return instance;
	}

	bool valid() const { return pfnNtReadVirtualMemory != nullptr; }

	pNtReadVirtualMemory pfnNtReadVirtualMemory = nullptr;

private:
	pMemory()
	{
		HMODULE ntdll = GetModuleHandleA("ntdll.dll");
		if (!ntdll)
			return;

		pfnNtReadVirtualMemory = reinterpret_cast<pNtReadVirtualMemory>(GetProcAddress(ntdll, "NtReadVirtualMemory"));
	}
};

struct ProcessModule
{
	uintptr_t base, size;
};

class pProcess
{
public:
	DWORD		  pid_ = 0;
	HANDLE		  handle_ = nullptr;
	HWND		  hwnd_ = nullptr;
	ProcessModule base_client_{ 0, 0 };

public:
	bool AttachProcess(const char* process_name);
	bool UpdateHWND();
	void Close();

public:
	ProcessModule GetModule(const char* module_name);
	uintptr_t     FindSignature(std::vector<uint8_t> signature);
	uintptr_t     FindSignature(ProcessModule target_module, std::vector<uint8_t> signature);

	template<class T>
	uintptr_t ReadOffsetFromSignature(std::vector<uint8_t> signature, uint8_t offset)
	{
		uintptr_t pattern_address = this->FindSignature(signature);
		if (!pattern_address)
			return 0x0;

		T offset_value = this->read<T>(pattern_address + offset);
		return pattern_address + offset_value + offset + sizeof(T);
	}

	bool read_raw(uintptr_t address, void* buffer, size_t size)
	{
		if (!handle_ || !address || !buffer || !size)
			return false;

		auto& memory = pMemory::Get();
		if (!memory.valid())
			return false;

		ULONG bytes_read = 0;
		NTSTATUS status = memory.pfnNtReadVirtualMemory(
			handle_,
			reinterpret_cast<PVOID>(address),
			buffer,
			static_cast<ULONG>(size),
			&bytes_read
		);

		return status == 0 && bytes_read == static_cast<ULONG>(size);
	}

	template<class T>
	T read(uintptr_t address)
	{
		// Value-initialised buffer: a failed read yields a zeroed value instead of garbage.
		T buffer{};
		this->read_raw(address, &buffer, sizeof(T));
		return buffer;
	}

	uintptr_t read_multi_address(uintptr_t ptr, std::vector<uintptr_t> offsets)
	{
		uintptr_t buffer = ptr;
		for (const auto offset : offsets)
		{
			if (!buffer)
				return 0;

			buffer = this->read<uintptr_t>(buffer + offset);
		}

		return buffer;
	}

	template <typename T>
	T read_multi(uintptr_t base, std::vector<uintptr_t> offsets) {
		// An empty offset list used to underflow offsets.size() - 1 and loop over a huge range.
		if (offsets.empty())
			return T{};

		uintptr_t buffer = base;
		for (size_t i = 0; i + 1 < offsets.size(); i++)
		{
			if (!buffer)
				return T{};

			buffer = this->read<uintptr_t>(buffer + offsets[i]);
		}
		return this->read<T>(buffer + offsets.back());
	}

private:
	uint32_t FindProcessIdByProcessName(const char* process_name);
	HWND GetWindowHandleFromProcessId(DWORD ProcessId);
};
#endif
