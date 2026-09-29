#include <includes.hpp>
#include <workspace/interface/interface.hpp>

#include <thirdparty/imgui/backends/imgui_impl_win32.h>
#include <workspace/interface/menu.hpp>
#include <workspace/interface/renderer.hpp>
#include <workspace/interface/window.hpp>
#include <workspace/util/logger/logger.hpp>

extern ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

namespace overlay {
namespace {

int VkFromKey(int key) {
	switch (key) {
	case ImGuiKey_MouseRight:
		return VK_RBUTTON;
	case ImGuiKey_MouseMiddle:
		return VK_MBUTTON;
	case ImGuiKey_MouseX1:
		return VK_XBUTTON1;
	case ImGuiKey_MouseX2:
		return VK_XBUTTON2;
	default:
		break;
	}

	for (int vk = 8; vk < 256; ++vk) {
		if (ImGui_ImplWin32_KeyEventToImGuiKey(vk, 0) == key) {
			return vk;
		}
	}

	return VK_INSERT;
}

void FeedKeyboard() {
	static bool previousState[256]{};

	ImGuiIO& io = ImGui::GetIO();
	BYTE keyboardState[256]{};

	for (int vk = 0; vk < 256; ++vk) {
		keyboardState[vk] = (GetAsyncKeyState(vk) & 0x8000) ? 0x80 : 0;
	}

	if (GetKeyState(VK_CAPITAL) & 1) {
		keyboardState[VK_CAPITAL] |= 1;
	}

	io.AddKeyEvent(ImGuiMod_Ctrl, keyboardState[VK_CONTROL] != 0);
	io.AddKeyEvent(ImGuiMod_Shift, keyboardState[VK_SHIFT] != 0);
	io.AddKeyEvent(ImGuiMod_Alt, keyboardState[VK_MENU] != 0);

	for (int vk = 8; vk < 256; ++vk) {
		if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) {
			continue;
		}

		const bool down = keyboardState[vk] != 0;
		if (down == previousState[vk]) {
			continue;
		}
		previousState[vk] = down;

		const ImGuiKey key = ImGui_ImplWin32_KeyEventToImGuiKey(vk, 0);
		if (key != ImGuiKey_None) {
			io.AddKeyEvent(key, down);
		}

		if (!down) {
			continue;
		}

		wchar_t characters[4]{};
		const int count = ToUnicode(
			vk,
			MapVirtualKeyW(vk, MAPVK_VK_TO_VSC),
			keyboardState,
			characters,
			static_cast<int>(std::size(characters)),
			0
		);

		for (int index = 0; index < count; ++index) {
			if (characters[index] >= 32) {
				io.AddInputCharacterUTF16(characters[index]);
			}
		}
	}
}

void FeedMouse(HWND hwnd) {
	ImGuiIO& io = ImGui::GetIO();

	POINT cursor{};
	if (GetCursorPos(&cursor) && hwnd) {
		ScreenToClient(hwnd, &cursor);
		io.AddMousePosEvent(
			static_cast<float>(cursor.x),
			static_cast<float>(cursor.y)
		);
	}

	io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
	io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
	io.AddMouseButtonEvent(2, (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);
	io.AddMouseButtonEvent(3, (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0);
	io.AddMouseButtonEvent(4, (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0);
}

player::Config BuildPlayerConfig(const menu::Settings& settings, ImFont* espFont) {
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

bool RunLoop(Window& window, Renderer& renderer, PlayerCache& players, CameraCache& camera) {
	bool menuOpen = true;
	bool toggleWasDown = false;

	while (window.PumpMessages()) {
		const bool toggleDown =
			(GetAsyncKeyState(VkFromKey(menu::cfg.menu_key)) & 0x8000) != 0;

		if (toggleDown && !toggleWasDown) {
			menuOpen = !menuOpen;
			window.SetClickThrough(!menuOpen);

			if (!menuOpen) {
				ImGui::GetIO().AddFocusEvent(false);
			}
		}
		toggleWasDown = toggleDown;

		if (players.ImageBase()) {
			camera.Update(players.World());
			if (camera.Valid()) {
				players.SetCameraLocation(camera.Location());
			}
		}

		FeedMouse(window.Handle());
		if (menuOpen) {
			FeedKeyboard();
		}

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
			const menu::RuntimeStatus status{
				players.World() != nullptr,
				camera.Valid(),
				static_cast<int>(players.Count()),
				renderer.Fps()
			};
			menu::render(status);
		}

		if (!renderer.EndFrame()) {
			return false;
		}
	}

	return true;
}

} // namespace

bool run(PlayerCache& players, CameraCache& camera) {
	Window window;
	if (!window.Create()) {
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
	logger::Log("[overlay] runtime stopped");
	return result;
}

} // namespace overlay
