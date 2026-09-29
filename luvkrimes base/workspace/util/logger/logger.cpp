#include "logger.hpp"

#include <Windows.h>

#include <cstdio>
#include <mutex>

namespace logger {
namespace {
std::mutex g_LogMutex;
}

void Init() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!output || output == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (GetConsoleMode(output, &mode)) {
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

void LineRaw(const char* text) {
    if (!text) {
        return;
    }

    std::scoped_lock lock(g_LogMutex);

    SYSTEMTIME time{};
    GetLocalTime(&time);

    std::printf(
        "[%02u:%02u:%02u] %s\n",
        static_cast<unsigned>(time.wHour),
        static_cast<unsigned>(time.wMinute),
        static_cast<unsigned>(time.wSecond),
        text
    );
    std::fflush(stdout);
}

} // namespace logger
