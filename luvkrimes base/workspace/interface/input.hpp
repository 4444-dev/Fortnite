#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <array>

namespace overlay {

class InputManager final {
public:
	[[nodiscard]] bool PressedOnce(int imguiKey);
	void Feed(HWND hwnd, bool keyboardEnabled);

private:
	[[nodiscard]] int VirtualKeyFromImGuiKey(int imguiKey) const;
	void FeedMouse(HWND hwnd) const;
	void FeedKeyboard();

	std::array<bool, 256> m_EdgeState{};
	std::array<bool, 256> m_KeyboardState{};
};

} // namespace overlay
