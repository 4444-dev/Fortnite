#include <workspace/util/crash/crash_handler.hpp>

#include <Windows.h>
#include <DbgHelp.h>

#include <filesystem>
#include <string>

#pragma comment(lib, "Dbghelp.lib")

namespace crash_handler {
namespace {

std::filesystem::path ExecutableDirectory() {
	wchar_t buffer[32768]{};
	const DWORD length = GetModuleFileNameW(
		nullptr,
		buffer,
		static_cast<DWORD>(_countof(buffer))
	);

	if (length == 0 || length >= _countof(buffer)) {
		return std::filesystem::path(L".");
	}

	return std::filesystem::path(buffer).parent_path();
}

std::filesystem::path DumpPath() {
	SYSTEMTIME time{};
	GetLocalTime(&time);

	wchar_t filename[128]{};
	swprintf_s(
		filename,
		L"crash-%04u%02u%02u-%02u%02u%02u-%lu.dmp",
		static_cast<unsigned>(time.wYear),
		static_cast<unsigned>(time.wMonth),
		static_cast<unsigned>(time.wDay),
		static_cast<unsigned>(time.wHour),
		static_cast<unsigned>(time.wMinute),
		static_cast<unsigned>(time.wSecond),
		static_cast<unsigned long>(GetCurrentProcessId())
	);

	const auto directory = ExecutableDirectory() / L"crashdumps";
	std::error_code error;
	std::filesystem::create_directories(directory, error);
	return directory / filename;
}

LONG WINAPI HandleUnhandledException(EXCEPTION_POINTERS* exceptionPointers) {
	const auto path = DumpPath();

	HANDLE file = CreateFileW(
		path.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (file != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
		exceptionInfo.ThreadId = GetCurrentThreadId();
		exceptionInfo.ExceptionPointers = exceptionPointers;
		exceptionInfo.ClientPointers = FALSE;

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			file,
			MiniDumpNormal,
			exceptionPointers ? &exceptionInfo : nullptr,
			nullptr,
			nullptr
		);

		CloseHandle(file);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

void Install() {
	SetUnhandledExceptionFilter(&HandleUnhandledException);
}

} // namespace crash_handler
