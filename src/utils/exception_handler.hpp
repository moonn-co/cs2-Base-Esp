#pragma once

#include <Windows.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include "debug_logger.hpp"

class c_exception_handler
{
public:
	static void log_file(const std::string& output)
	{
		const std::string file_name = "crashlog.txt";

		std::error_code ec;
		if (std::filesystem::exists(file_name, ec))
			std::filesystem::remove(file_name, ec);

		std::ofstream output_file(file_name, std::ios::app);
		if (!output_file)
			return;

		output_file << output << std::endl;
	}

	static long __stdcall handler(EXCEPTION_POINTERS* info)
	{
		if (!info || !info->ExceptionRecord || !info->ContextRecord)
			return EXCEPTION_CONTINUE_SEARCH;

		const auto code = info->ExceptionRecord->ExceptionCode;

		// Only crash-class exceptions are reported; C++ exceptions and other
		// handled codes keep travelling to their normal handlers.
		const bool fatal =
			code == EXCEPTION_ACCESS_VIOLATION ||
			code == EXCEPTION_ILLEGAL_INSTRUCTION ||
			code == EXCEPTION_STACK_OVERFLOW ||
			code == EXCEPTION_IN_PAGE_ERROR;

		if (!fatal)
			return EXCEPTION_CONTINUE_SEARCH;

		const auto address = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
		const auto rip = static_cast<uintptr_t>(info->ContextRecord->Rip);

		MEMORY_BASIC_INFORMATION memory_info{};
		uintptr_t alloc_base = 0;

		if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &memory_info, sizeof(memory_info)) && memory_info.AllocationBase)
			alloc_base = reinterpret_cast<uintptr_t>(memory_info.AllocationBase);

		const auto output = std::format(
			"crash: code {:#010x} at {:#x} (module_base+{:#x}), rip {:#x}",
			static_cast<uint32_t>(code),
			address,
			address - alloc_base,
			rip
		);

		log_file(output);

		return EXCEPTION_CONTINUE_SEARCH;
	}

	static bool setup()
	{
		const auto handle = AddVectoredExceptionHandler(0, handler);
		if (!handle)
		{
			LOGF(FATAL, "Failed to add vectored exception handler");
			return false;
		}

		return true;
	}
};
