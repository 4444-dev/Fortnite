#pragma once

#include <filesystem>

namespace menu {
struct Settings;
}

namespace app_settings {

[[nodiscard]] std::filesystem::path Path();
bool Load(menu::Settings& settings);
bool Save(const menu::Settings& settings);

} // namespace app_settings
