#include <workspace/interface/window.hpp>

#include <dwmapi.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>
#include <workspace/util/logger/logger.hpp>

#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
);

namespace overlay {
namespace {
constexpr wchar_t kWindowClassName[] = L"LuvkrimesOverlay";
constexpr wchar_t kWindowTitle[] = L"luvkrimes base";
}

Window::~Window() {
	Destroy();
}

bool Window::Create() {
	if (m_Hwnd) {
		return true;
	}

	m_Instance = GetModuleHandleW(nullptr);
	if (!m_Instance) {
		logger::Log("[window] GetModuleHandleW failed (%lu)", GetLastError());
		return false;
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &Window::WndProc;
	wc.hInstance = m_Instance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wc.lpszClassName = kWindowClassName;

	const ATOM atom = RegisterClassExW(&wc);
	if (!atom) {
		const DWORD error = GetLastError();
		if (error != ERROR_CLASS_ALREADY_EXISTS) {
			logger::Log("[window] RegisterClassExW failed (%lu)", error);
			return false;
		}
	} else {
		m_OwnsClass = true;
	}

	const int width = GetSystemMetrics(SM_CXSCREEN);
	const int height = GetSystemMetrics(SM_CYSCREEN);

	m_Hwnd = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
		kWindowClassName,
		kWindowTitle,
		WS_POPUP,
		0,
		0,
		width,
		height,
		nullptr,
		nullptr,
		m_Instance,
		nullptr
	);

	if (!m_Hwnd) {
		logger::Log("[window] CreateWindowExW failed (%lu)", GetLastError());
		Destroy();
		return false;
	}

	MARGINS margins{-1};
	if (FAILED(DwmExtendFrameIntoClientArea(m_Hwnd, &margins))) {
		logger::Log("[window] DwmExtendFrameIntoClientArea failed");
	}

	if (!SetLayeredWindowAttributes(m_Hwnd, 0, 255, LWA_ALPHA)) {
		logger::Log("[window] SetLayeredWindowAttributes failed (%lu)", GetLastError());
	}

	ShowWindow(m_Hwnd, SW_SHOW);
	UpdateWindow(m_Hwnd);
	return true;
}

void Window::Destroy() {
	if (m_Hwnd) {
		if (IsWindow(m_Hwnd)) {
			DestroyWindow(m_Hwnd);
		}
		m_Hwnd = nullptr;
	}

	if (m_OwnsClass && m_Instance) {
		UnregisterClassW(kWindowClassName, m_Instance);
		m_OwnsClass = false;
	}

	m_Instance = nullptr;
}

HWND Window::Handle() const noexcept {
	return m_Hwnd;
}

bool Window::PumpMessages() const {
	MSG msg{};
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return true;
}

void Window::SetClickThrough(bool enabled) const {
	if (!m_Hwnd) {
		return;
	}

	LONG_PTR style = GetWindowLongPtrW(m_Hwnd, GWL_EXSTYLE);
	if (enabled) {
		style |= WS_EX_TRANSPARENT;
	} else {
		style &= ~WS_EX_TRANSPARENT;
	}

	if (!SetWindowLongPtrW(m_Hwnd, GWL_EXSTYLE, style) && GetLastError() != ERROR_SUCCESS) {
		logger::Log("[window] SetWindowLongPtrW failed (%lu)", GetLastError());
	}
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)) {
		return TRUE;
	}

	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace overlay
