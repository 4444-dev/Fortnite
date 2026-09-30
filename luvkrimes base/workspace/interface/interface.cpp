#include <includes.hpp>
#include <workspace/interface/interface.hpp>

#include <workspace/interface/input.hpp>
#include <workspace/interface/menu.hpp>
#include <workspace/interface/renderer.hpp>
#include <workspace/interface/settings_store.hpp>
#include <workspace/interface/window.hpp>
#include <workspace/util/logger/logger.hpp>

#include <chrono>

namespace overlay {
namespace {

player::Config BuildPlayerConfig(
	const menu::Settings& settings,
	ImFont* espFont
) {
	return {
		settings.box ? settings.box_style + 1 : 0,
		settings.box_fill,
		{
			settings.box_color[0],
			settings.box_color[1],
			settings.box_color[2],
			settings.box_color[3]
		},
		settings.distance,
		settings.skeleton,
		{
			settings.skeleton_color[0],
			settings.skeleton_color[1],
			settings.skeleton_color[2],
			settings.skeleton_color[3]
		},
		settings.snaplines,
		settings.snapline_origin,
		{
			settings.snapline_color[0],
			settings.snapline_color[1],
			settings.snapline_color[2],
			settings.snapline_color[3]
		},
		espFont
	};
}

bool RunLoop(
	Window& window,
	Renderer& renderer,
	PlayerCache& players,
	CameraCache& camera
) {
	InputManager input;
	bool menuOpen = true;
	window.SetClickThrough(false);

	auto nextMonitorSync =
		std::chrono::steady_clock::now();

	while (window.PumpMessages()) {
		if (input.PressedOnce(menu::cfg.menu_key)) {
			menuOpen = !menuOpen;
			window.SetClickThrough(!menuOpen);

			if (!menuOpen) {
				ImGui::GetIO().AddFocusEvent(false);
			}
		}

		const auto now =
			std::chrono::steady_clock::now();

		if (now >= nextMonitorSync) {
			nextMonitorSync =
				now + std::chrono::milliseconds(250);

			if (window.SyncToTargetMonitor()) {
				const SIZE size = window.ClientSize();
				if (
					size.cx > 0 &&
					size.cy > 0 &&
					!renderer.Resize(
						static_cast<UINT>(size.cx),
						static_cast<UINT>(size.cy)
					)
				) {
					return false;
				}
			}
		}

		if (players.ImageBase()) {
			camera.Update(players.World());
			if (camera.Valid()) {
				players.SetCameraLocation(camera.Location());
			}
		}

		input.Feed(window.Handle(), menuOpen);
		renderer.BeginFrame();

		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const player::Config playerConfig = BuildPlayerConfig(
			menu::cfg,
			renderer.EspFont()
		);

		player::Draw(
			players,
			camera,
			ImGui::GetBackgroundDrawList(),
			display.x,
			display.y,
			playerConfig
		);

		if (menuOpen) {
			const PlayerCacheStats cacheStats = players.Stats();
			const float fps = renderer.Fps();

			menu::RuntimeStatus status{};
			status.world_valid = players.World() != nullptr;
			status.camera_valid = camera.Valid();
			status.actor_count = static_cast<int>(cacheStats.ActorCount);
			status.player_count = static_cast<int>(cacheStats.PlayerCount);
			status.fps = fps;
			status.frame_ms = fps > 0.0f ? 1000.0f / fps : 0.0f;
			status.engine_ms = static_cast<float>(cacheStats.EngineMs);
			status.actors_ms = static_cast<float>(cacheStats.ActorsMs);
			status.players_ms = static_cast<float>(cacheStats.PlayersMs);
			status.dpi_scale = window.DpiScale();

			menu::render(status);
		}

		if (!renderer.EndFrame()) {
			return false;
		}
	}

	return true;
}

} // namespace

bool run(PlayerCache& players, CameraCache& camera, std::uint32_t targetProcessId) {
	if (app_settings::Load(menu::cfg)) {
		logger::Log(
			"[settings] loaded from %ls",
			app_settings::Path().c_str()
		);
	} else {
		logger::Log("[settings] using defaults");
	}

	Window window;
	if (!window.Create(static_cast<DWORD>(targetProcessId))) {
		logger::Log("[overlay] window creation failed");
		return false;
	}

	Renderer renderer;
	if (!renderer.Initialize(window.Handle())) {
		logger::Log("[overlay] renderer initialization failed");
		return false;
	}

	logger::Log("[overlay] runtime initialized");
	const bool result = RunLoop(window, renderer, players, camera);

	if (!app_settings::Save(menu::cfg)) {
		logger::Log("[settings] save failed");
	}

	logger::Log("[overlay] runtime stopped");
	return result;
}

} // namespace overlay
