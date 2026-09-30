#pragma once

#include <cstdint>

class PlayerCache;
class CameraCache;

namespace overlay {

bool run(PlayerCache& players, CameraCache& camera, std::uint32_t targetProcessId);

} // namespace overlay
