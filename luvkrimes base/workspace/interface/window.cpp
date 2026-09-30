#include <workspace/interface/window.hpp>

#include <dwmapi.h>
#include <thirdparty/imgui/imgui.h>
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
constexpr wchar_t kWindowClassName[] = L"NexusOverlay";
constexpr wchar_t kWindowTitle[] = L"Nexus";

struct WindowSearchContext {
	DWORD ProcessId = 0;
	HWND BestWindow = nullptr;
	long long BestArea = 0;
};

BOOL CALLBACK FindProcessWindow(
	HWND hwnd,
	LPARAM parameter
) {
	auto* context =
		reinterpret_cast<WindowSearchContext*>(parameter);

	if (!context || !IsWindowVisible(hwnd)) {
		return TRUE;
	}

	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);
	if (processId != context->ProcessId) {
		return TRUE;
	}

	if (GetWindow(hwnd, GW_OWNER) != nullptr) {
		return TRUE;
	}

	RECT rect{};
	if (!GetWindowRect(hwnd, &rect)) {
		return TRUE;
	}

	const long long width =
		static_cast<long long>(rect.right) - rect.left;
	const long long height =
		static_cast<long long>(rect.bottom) - rect.top;
	const long long area = width * height;

	if (width <= 0 || height <= 0 || area <= context->BestArea) {
		return TRUE;
	}

	context->BestArea = area;
	context->BestWindow = hwnd;
	return TRUE;
}
}

Window::~Window() {
	Destroy();
}

HWND Window::TargetWindow() const noexcept {
	if (m_TargetProcessId == 0) {
		return nullptr;
	}

	WindowSearchContext context{};
	context.ProcessId = m_TargetProcessId;
	EnumWindows(
		&FindProcessWindow,
		reinterpret_cast<LPARAM>(&context)
	);

	return context.BestWindow;
}

RECT Window::TargetMonitorBounds() const noexcept {
	HMONITOR monitor = nullptr;

	if (const HWND target = TargetWindow()) {
		monitor = MonitorFromWindow(
			target,
			MONITOR_DEFAULTTONEAREST
		);
	}

	if (!monitor) {
		POINT origin{0, 0};
		monitor = MonitorFromPoint(
			origin,
			MONITOR_DEFAULTTOPRIMARY
		);
	}

	MONITORINFO info{};
	info.cbSize = sizeof(info);

	if (monitor && GetMonitorInfoW(monitor, &info)) {
		return info.rcMonitor;
	}

	return RECT{
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN)
	};
}

bool Window::Create(DWORD targetProcessId) {
	if (m_Hwnd) {
		return true;
	}

	m_TargetProcessId = targetProcessId;
	ImGui_ImplWin32_EnableDpiAwareness();

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

	const RECT bounds = TargetMonitorBounds();
	const int width = bounds.right - bounds.left;
	const int height = bounds.bottom - bounds.top;

	m_Hwnd = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
		kWindowClassName,
		kWindowTitle,
		WS_POPUP,
		bounds.left,
		bounds.top,
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

	logger::Log(
		"[window] target monitor %dx%d at (%d,%d), dpi %.2f",
		width,
		height,
		bounds.left,
		bounds.top,
		static_cast<double>(DpiScale())
	);
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
	m_TargetProcessId = 0;
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

bool Window::SyncToTargetMonitor() {
	if (!m_Hwnd) {
		return false;
	}

	const RECT expected = TargetMonitorBounds();
	RECT current{};
	if (!GetWindowRect(m_Hwnd, &current)) {
		return false;
	}

	if (
		current.left == expected.left &&
		current.top == expected.top &&
		current.right == expected.right &&
		current.bottom == expected.bottom
	) {
		return false;
	}

	const int width = expected.right - expected.left;
	const int height = expected.bottom - expected.top;

	if (!SetWindowPos(
		m_Hwnd,
		nullptr,
		expected.left,
		expected.top,
		width,
		height,
		SWP_NOACTIVATE | SWP_NOZORDER
	)) {
		logger::Log("[window] target monitor resize failed (%lu)", GetLastError());
		return false;
	}

	logger::Log(
		"[window] target monitor changed -> %dx%d at (%d,%d)",
		width,
		height,
		expected.left,
		expected.top
	);
	return true;
}

SIZE Window::ClientSize() const noexcept {
	SIZE size{};
	if (!m_Hwnd) {
		return size;
	}

	RECT client{};
	if (GetClientRect(m_Hwnd, &client)) {
		size.cx = client.right - client.left;
		size.cy = client.bottom - client.top;
	}
	return size;
}

float Window::DpiScale() const noexcept {
	if (!m_Hwnd) {
		return 1.0f;
	}

	const float scale = ImGui_ImplWin32_GetDpiScaleForHwnd(m_Hwnd);
	return scale > 0.0f ? scale : 1.0f;
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

	SetLastError(ERROR_SUCCESS);
	const LONG_PTR previousStyle = SetWindowLongPtrW(m_Hwnd, GWL_EXSTYLE, style);
	const DWORD error = GetLastError();
	if (previousStyle == 0 && error != ERROR_SUCCESS) {
		logger::Log("[window] SetWindowLongPtrW failed (%lu)", error);
	}
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	if (
		ImGui::GetCurrentContext() &&
		ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)
	) {
		return TRUE;
	}

	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace overlay
