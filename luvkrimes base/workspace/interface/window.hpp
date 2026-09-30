#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

namespace overlay {

class Window final {
public:
	Window() = default;
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool Create();
	void Destroy();

	[[nodiscard]] HWND Handle() const noexcept;
	[[nodiscard]] bool PumpMessages() const;
	[[nodiscard]] bool SyncToVirtualDesktop();
	[[nodiscard]] SIZE ClientSize() const noexcept;
	[[nodiscard]] float DpiScale() const noexcept;

	void SetClickThrough(bool enabled) const;

private:
	static LRESULT CALLBACK WndProc(
		HWND hwnd,
		UINT message,
		WPARAM wparam,
		LPARAM lparam
	);

	[[nodiscard]] static RECT VirtualDesktopBounds() noexcept;

	HWND m_Hwnd = nullptr;
	HINSTANCE m_Instance = nullptr;
	bool m_OwnsClass = false;
};

} // namespace overlay
