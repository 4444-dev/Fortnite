#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>
#include <d3d11.h>
#include <dxgi.h>
#include <algorithm>

using uptr = uintptr_t;
using u64  = uint64_t;
using u32  = uint32_t;
using u16  = uint16_t;
using u8   = uint8_t;
using i64  = int64_t;
using i32  = int32_t;
using i16  = int16_t;
using i8   = int8_t;

#include <workspace/game/unreal/offsets.hpp>
#include <workspace/game/unreal/structures.hpp>
#include <workspace/game/unreal/enums.hpp>
#include <workspace/game/unreal/classes.hpp>

#include <workspace/driver/driver.hpp>

#include <workspace/game/cache/cache.hpp>
#include <workspace/game/cache/camera.hpp>

#include <workspace/game/features/player/player.hpp>
#include <workspace/interface/interface.hpp>
