#include "logger.hpp"

#include <Windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace logger {
namespace {

std::mutex g_LogMutex;
std::ofstream g_LogFile;

std::filesystem::path LocalAppDataPath() {
	wchar_t buffer[32768]{};
	const DWORD count = GetEnvironmentVariableW(
		L"LOCALAPPDATA",
		buffer,
		static_cast<DWORD>(_countof(buffer))
	);

	if (count > 0 && count < _countof(buffer)) {
		return std::filesystem::path(buffer);
	}

	std::error_code error;
	const auto current = std::filesystem::current_path(error);
	return error ? std::filesystem::path(L".") : current;
}

std::filesystem::path LogPath() {
	return LocalAppDataPath() / L"Nexus" / L"logs" / L"latest.log";
}

} // namespace

void Init() {
	const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	if (output && output != INVALID_HANDLE_VALUE) {
		DWORD mode = 0;
		if (GetConsoleMode(output, &mode)) {
			SetConsoleMode(
				output,
				mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING
			);
		}
	}

	std::error_code error;
	const auto path = LogPath();
	std::filesystem::create_directories(path.parent_path(), error);
	if (!error) {
		g_LogFile.open(path, std::ios::trunc);
	}
}

void LineRaw(const char* text) {
	if (!text) {
		return;
	}

	std::scoped_lock lock(g_LogMutex);

	SYSTEMTIME time{};
	GetLocalTime(&time);

	char line[1400]{};
	const int written = std::snprintf(
		line,
		sizeof(line),
		"[%02u:%02u:%02u] %s\n",
		static_cast<unsigned>(time.wHour),
		static_cast<unsigned>(time.wMinute),
		static_cast<unsigned>(time.wSecond),
		text
	);

	if (written <= 0) {
		return;
	}

	std::fputs(line, stdout);
	std::fflush(stdout);

	if (g_LogFile.is_open()) {
		g_LogFile << line;
		g_LogFile.flush();
	}
}

} // namespace logger
