#include <workspace/interface/settings_store.hpp>

#include <workspace/interface/menu.hpp>
#include <workspace/util/config/kv_config.hpp>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstdlib>

namespace app_settings {
namespace {

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

float ClampColor(float value) {
	return (std::clamp)(value, 0.0f, 1.0f);
}

void LoadColor(
	const util::KeyValueConfig& config,
	const char* prefix,
	float (&color)[4]
) {
	for (int index = 0; index < 4; ++index) {
		const std::string key =
			std::string(prefix) + "." + std::to_string(index);

		float value = color[index];
		if (config.TryGetFloat(key, value)) {
			color[index] = ClampColor(value);
		}
	}
}

void SaveColor(
	util::KeyValueConfig& config,
	const char* prefix,
	const float (&color)[4]
) {
	for (int index = 0; index < 4; ++index) {
		const std::string key =
			std::string(prefix) + "." + std::to_string(index);
		config.SetFloat(key, ClampColor(color[index]));
	}
}

} // namespace

std::filesystem::path Path() {
	return LocalAppDataPath() / L"luvkrimes" / L"settings.ini";
}

bool Load(menu::Settings& settings) {
	util::KeyValueConfig config;
	if (!config.Load(Path())) {
		return false;
	}

	(void)config.TryGetBool("visuals.box", settings.box);
	(void)config.TryGetBool("visuals.box_fill", settings.box_fill);
	(void)config.TryGetBool("visuals.skeleton", settings.skeleton);
	(void)config.TryGetBool("visuals.distance", settings.distance);
	(void)config.TryGetBool("visuals.snaplines", settings.snaplines);

	int boxStyle = settings.box_style;
	if (config.TryGetInt("visuals.box_style", boxStyle)) {
		settings.box_style = (std::clamp)(boxStyle, 0, 1);
	}

	int snaplineOrigin = settings.snapline_origin;
	if (config.TryGetInt("visuals.snapline_origin", snaplineOrigin)) {
		settings.snapline_origin = (std::clamp)(snaplineOrigin, 0, 2);
	}

	int menuKey = settings.menu_key;
	if (config.TryGetInt("menu.key", menuKey)) {
		settings.menu_key = menuKey;
	}

	LoadColor(config, "visuals.box_color", settings.box_color);
	LoadColor(config, "visuals.skeleton_color", settings.skeleton_color);
	LoadColor(config, "visuals.snapline_color", settings.snapline_color);
	LoadColor(config, "menu.accent", settings.accent);
	return true;
}

bool Save(const menu::Settings& settings) {
	util::KeyValueConfig config;
	config.SetBool("visuals.box", settings.box);
	config.SetInt("visuals.box_style", (std::clamp)(settings.box_style, 0, 1));
	config.SetBool("visuals.box_fill", settings.box_fill);
	config.SetBool("visuals.skeleton", settings.skeleton);
	config.SetBool("visuals.distance", settings.distance);
	config.SetBool("visuals.snaplines", settings.snaplines);
	config.SetInt(
		"visuals.snapline_origin",
		(std::clamp)(settings.snapline_origin, 0, 2)
	);
	config.SetInt("menu.key", settings.menu_key);

	SaveColor(config, "visuals.box_color", settings.box_color);
	SaveColor(config, "visuals.skeleton_color", settings.skeleton_color);
	SaveColor(config, "visuals.snapline_color", settings.snapline_color);
	SaveColor(config, "menu.accent", settings.accent);

	return config.Save(Path());
}

} // namespace app_settings
