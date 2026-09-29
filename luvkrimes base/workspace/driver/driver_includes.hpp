#pragma warning (disable: 4091 6328 6031)
#pragma once

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <assert.h>
#include "xor.hpp"

#define FILENAMES_ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
(strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#define LOG_INFO(fmt, ...) \
printf(skCrypt("[Usermode] - [%s:%d] " fmt "\n"), FILENAMES_, __LINE__, ##__VA_ARGS__)

#define LOG_NEW_LINE(fmt) \
printf(skCrypt("[%s:%d] " fmt "\n"), FILENAMES_, __LINE__)
