#include "loader_window.hpp"

#include "loader_renderer.hpp"

#include <windowsx.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
);

namespace loader {
namespace {

constexpr wchar_t kClassName[] = L"NexusLoaderWindow";
constexpr wchar_t kWindowTitle[] = L"Nexus";
constexpr int kWindowWidth = 620;
constexpr int kWindowHeight = 390;
constexpr int kDragRegionHeight = 38;

} // namespace

Window::~Window() {
	Destroy();
}

bool Window::Create() {
	if (m_Hwnd) {
		return true;
	}

	ImGui_ImplWin32_EnableDpiAwareness();

	m_Instance = GetModuleHandleW(nullptr);
	if (!m_Instance) {
		return false;
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = &Window::WndProc;
	wc.hInstance = m_Instance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.lpszClassName = kClassName;

	const ATOM atom = RegisterClassExW(&wc);
	if (!atom) {
		if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
			Destroy();
			return false;
		}
	} else {
		m_OwnsClass = true;
	}

	m_Hwnd = CreateWindowExW(
		WS_EX_APPWINDOW,
		kClassName,
		kWindowTitle,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		kWindowWidth,
		kWindowHeight,
		nullptr,
		nullptr,
		m_Instance,
		this
	);

	if (!m_Hwnd) {
		Destroy();
		return false;
	}

	Center();
	return true;
}

void Window::Destroy() {
	m_Renderer = nullptr;

	if (m_Hwnd) {
		if (IsWindow(m_Hwnd)) {
			DestroyWindow(m_Hwnd);
		}
		m_Hwnd = nullptr;
	}

	if (m_OwnsClass && m_Instance) {
		UnregisterClassW(kClassName, m_Instance);
		m_OwnsClass = false;
	}

	m_Instance = nullptr;
}

void Window::Show() {
	if (!m_Hwnd) {
		return;
	}

	ShowWindow(m_Hwnd, SW_SHOWDEFAULT);
	UpdateWindow(m_Hwnd);
}

void Window::AttachRenderer(Renderer* renderer) noexcept {
	m_Renderer = renderer;
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

void Window::Center() const {
	if (!m_Hwnd) {
		return;
	}

	POINT cursor{};
	if (!GetCursorPos(&cursor)) {
		cursor = POINT{0, 0};
	}

	const HMONITOR monitor = MonitorFromPoint(
		cursor,
		MONITOR_DEFAULTTOPRIMARY
	);

	MONITORINFO info{};
	info.cbSize = sizeof(info);

	if (!GetMonitorInfoW(monitor, &info)) {
		return;
	}

	const RECT& area = info.rcWork;
	const int areaWidth = area.right - area.left;
	const int areaHeight = area.bottom - area.top;

	SetWindowPos(
		m_Hwnd,
		HWND_TOP,
		area.left + (areaWidth - kWindowWidth) / 2,
		area.top + (areaHeight - kWindowHeight) / 2,
		kWindowWidth,
		kWindowHeight,
		SWP_NOACTIVATE
	);
}

LRESULT CALLBACK Window::WndProc(
	HWND hwnd,
	UINT message,
	WPARAM wparam,
	LPARAM lparam
) {
	Window* self = reinterpret_cast<Window*>(
		GetWindowLongPtrW(hwnd, GWLP_USERDATA)
	);

	if (message == WM_NCCREATE) {
		const auto* create =
			reinterpret_cast<const CREATESTRUCTW*>(lparam);
		self = static_cast<Window*>(create->lpCreateParams);
		SetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(self)
		);
	}

	if (
		ImGui::GetCurrentContext() &&
		ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)
	) {
		return TRUE;
	}

	switch (message) {
	case WM_NCHITTEST: {
		const LRESULT hit =
			DefWindowProcW(hwnd, message, wparam, lparam);

		if (hit == HTCLIENT) {
			POINT point{
				GET_X_LPARAM(lparam),
				GET_Y_LPARAM(lparam)
			};
			ScreenToClient(hwnd, &point);

			if (point.y >= 0 && point.y < kDragRegionHeight) {
				return HTCAPTION;
			}
		}

		return hit;
	}

	case WM_SIZE:
		if (
			self &&
			self->m_Renderer &&
			wparam != SIZE_MINIMIZED
		) {
			(void)self->m_Renderer->Resize(
				static_cast<UINT>(LOWORD(lparam)),
				static_cast<UINT>(HIWORD(lparam))
			);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace loader
