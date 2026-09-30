#include <workspace/interface/input.hpp>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>

extern ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

namespace overlay {

int InputManager::VirtualKeyFromImGuiKey(int imguiKey) const {
	switch (imguiKey) {
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
		if (ImGui_ImplWin32_KeyEventToImGuiKey(vk, 0) == imguiKey) {
			return vk;
		}
	}

	return VK_INSERT;
}

bool InputManager::PressedOnce(int imguiKey) {
	const int vk = VirtualKeyFromImGuiKey(imguiKey);
	if (vk < 0 || vk >= static_cast<int>(m_EdgeState.size())) {
		return false;
	}

	const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
	const bool pressed = down && !m_EdgeState[static_cast<std::size_t>(vk)];
	m_EdgeState[static_cast<std::size_t>(vk)] = down;
	return pressed;
}

void InputManager::Feed(HWND hwnd, bool keyboardEnabled) {
	FeedMouse(hwnd);
	if (keyboardEnabled) {
		FeedKeyboard();
	}
}

void InputManager::FeedMouse(HWND hwnd) const {
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

void InputManager::FeedKeyboard() {
	ImGuiIO& io = ImGui::GetIO();
	BYTE state[256]{};

	for (int vk = 0; vk < 256; ++vk) {
		state[vk] = (GetAsyncKeyState(vk) & 0x8000) ? 0x80 : 0;
	}

	if (GetKeyState(VK_CAPITAL) & 1) {
		state[VK_CAPITAL] |= 1;
	}

	io.AddKeyEvent(ImGuiMod_Ctrl, state[VK_CONTROL] != 0);
	io.AddKeyEvent(ImGuiMod_Shift, state[VK_SHIFT] != 0);
	io.AddKeyEvent(ImGuiMod_Alt, state[VK_MENU] != 0);

	for (int vk = 8; vk < 256; ++vk) {
		if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) {
			continue;
		}

		const bool down = state[vk] != 0;
		const std::size_t index = static_cast<std::size_t>(vk);
		if (down == m_KeyboardState[index]) {
			continue;
		}
		m_KeyboardState[index] = down;

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
			state,
			characters,
			static_cast<int>(_countof(characters)),
			0
		);

		for (int indexCharacter = 0; indexCharacter < count; ++indexCharacter) {
			if (characters[indexCharacter] >= 32) {
				io.AddInputCharacterUTF16(characters[indexCharacter]);
			}
		}
	}
}

} // namespace overlay
