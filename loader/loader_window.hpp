#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

namespace loader {

class Renderer;

class Window final {
public:
	Window() = default;
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	[[nodiscard]] bool Create();
	void Destroy();
	void Show();
	void Minimize();

	void AttachRenderer(Renderer* renderer) noexcept;

	[[nodiscard]] HWND Handle() const noexcept;
	[[nodiscard]] bool PumpMessages() const;

private:
	static LRESULT CALLBACK WndProc(
		HWND hwnd,
		UINT message,
		WPARAM wparam,
		LPARAM lparam
	);

	void Center() const;

	HWND m_Hwnd = nullptr;
	HINSTANCE m_Instance = nullptr;
	Renderer* m_Renderer = nullptr;
	bool m_OwnsClass = false;
};

} // namespace loader
