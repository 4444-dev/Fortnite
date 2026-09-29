#pragma once

#include <cstdio>
#include <utility>

namespace logger {

void Init();
void LineRaw(const char* text);

template <typename... Args>
void Log(const char* fmt, Args&&... args) {
    if (!fmt) {
        return;
    }

    char buffer[1024]{};
    const int written = std::snprintf(
        buffer,
        sizeof(buffer),
        fmt,
        std::forward<Args>(args)...
    );

    if (written < 0) {
        LineRaw("[logger] formatting error");
        return;
    }

    LineRaw(buffer);
}

} // namespace logger
