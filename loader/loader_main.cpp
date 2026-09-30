#include "license_store.hpp"
#include "product_registry.hpp"

#include <auth.hpp>

#include <Windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dwmapi.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
);

namespace {

constexpr wchar_t kClassName[] = L"LuvkrimesLoaderWindow";
constexpr wchar_t kWindowTitle[] = L"luvkrimes loader";
constexpr int kWindowWidth = 620;
constexpr int kWindowHeight = 390;

enum class Screen {
	ProductSelect,
	Authentication
};

enum class AuthState {
	Connecting,
	Ready,
	Authenticating,
	Authenticated,
	Error,
	SessionInvalid
};

struct AuthSnapshot {
	AuthState State = AuthState::Connecting;
	std::string Status = "Connecting to authentication service...";
	std::string Username;
	std::string Subscription;
	std::string Expiry;
	bool Busy = true;
	bool Authenticated = false;
};

class AuthController final {
public:
	explicit AuthController(const loader::ProductDefinition& product)
		: m_Product(product),
		  m_App(
			std::string(product.KeyAuthName),
			std::string(product.KeyAuthOwnerId),
			std::string(product.KeyAuthVersion),
			std::string(product.KeyAuthUrl),
			std::string(product.KeyAuthPath)
		  ) {
	}

	~AuthController() {
		JoinWorker();
	}

	AuthController(const AuthController&) = delete;
	AuthController& operator=(const AuthController&) = delete;

	void Initialize() {
		Run(
			AuthState::Connecting,
			"Connecting to " + std::string(m_Product.DisplayName) + "...",
			[this] {
				m_App.init();

				std::scoped_lock lock(m_Mutex);
				if (!m_App.response.success) {
					m_Snapshot.State = AuthState::Error;
					m_Snapshot.Status = m_App.response.message.empty()
						? "Authentication initialization failed."
						: m_App.response.message;
					m_Snapshot.Authenticated = false;
					return;
				}

				m_Snapshot.State = AuthState::Ready;
				m_Snapshot.Status =
					"Ready. Enter your " +
					std::string(m_Product.DisplayName) +
					" license key.";
				m_Snapshot.Authenticated = false;
			}
		);
	}

	void Authenticate(std::string license, bool remember) {
		if (license.empty()) {
			std::scoped_lock lock(m_Mutex);
			m_Snapshot.State = AuthState::Error;
			m_Snapshot.Status = "Enter a license key.";
			return;
		}

		Run(
			AuthState::Authenticating,
			"Validating " + std::string(m_Product.DisplayName) + " license...",
			[this, license = std::move(license), remember] {
				// This KeyAuth::api instance was created from the selected
				// product's independent KeyAuth application configuration.
				m_App.license(license);

				std::scoped_lock lock(m_Mutex);
				if (!m_App.response.success) {
					m_Snapshot.State = AuthState::Error;
					m_Snapshot.Status = m_App.response.message.empty()
						? "License validation failed."
						: m_App.response.message;
					m_Snapshot.Authenticated = false;
					return;
				}

				m_Snapshot.State = AuthState::Authenticated;
				m_Snapshot.Status =
					std::string(m_Product.DisplayName) +
					" authentication successful.";
				m_Snapshot.Authenticated = true;
				m_Snapshot.Username = m_App.user_data.username;

				if (!m_App.user_data.subscriptions.empty()) {
					const auto& subscription =
						m_App.user_data.subscriptions.front();
					m_Snapshot.Subscription = subscription.name;
					m_Snapshot.Expiry = subscription.expiry;
				} else {
					m_Snapshot.Subscription.clear();
					m_Snapshot.Expiry.clear();
				}

				if (remember) {
					(void)loader::license_store::Save(
						m_Product.Slug,
						license
					);
				} else {
					loader::license_store::Clear(m_Product.Slug);
				}

				m_NextSessionCheck =
					std::chrono::steady_clock::now() +
					std::chrono::seconds(60);
			}
		);
	}

	void Tick() {
		const AuthSnapshot snapshot = Snapshot();

		if (
			!snapshot.Authenticated ||
			snapshot.Busy ||
			std::chrono::steady_clock::now() < m_NextSessionCheck
		) {
			return;
		}

		Run(
			AuthState::Authenticated,
			std::string(m_Product.DisplayName) + " session active.",
			[this] {
				m_App.check();

				std::scoped_lock lock(m_Mutex);
				if (!m_App.response.success) {
					m_Snapshot.State = AuthState::SessionInvalid;
					m_Snapshot.Status = m_App.response.message.empty()
						? "Session validation failed."
						: m_App.response.message;
					m_Snapshot.Authenticated = false;
					return;
				}

				m_Snapshot.State = AuthState::Authenticated;
				m_Snapshot.Status =
					std::string(m_Product.DisplayName) +
					" session active.";
				m_Snapshot.Authenticated = true;

				m_NextSessionCheck =
					std::chrono::steady_clock::now() +
					std::chrono::seconds(60);
			}
		);
	}

	[[nodiscard]] AuthSnapshot Snapshot() const {
		std::scoped_lock lock(m_Mutex);
		return m_Snapshot;
	}

private:
	template <typename Fn>
	void Run(
		AuthState state,
		std::string status,
		Fn&& fn
	) {
		if (m_Busy.exchange(true)) {
			return;
		}

		JoinWorker();

		{
			std::scoped_lock lock(m_Mutex);
			m_Snapshot.State = state;
			m_Snapshot.Status = std::move(status);
			m_Snapshot.Busy = true;
		}

		m_Worker = std::thread(
			[this, task = std::forward<Fn>(fn)]() mutable {
				try {
					task();
				} catch (const std::exception& exception) {
					std::scoped_lock lock(m_Mutex);
					m_Snapshot.State = AuthState::Error;
					m_Snapshot.Status = exception.what();
					m_Snapshot.Authenticated = false;
				} catch (...) {
					std::scoped_lock lock(m_Mutex);
					m_Snapshot.State = AuthState::Error;
					m_Snapshot.Status =
						"Unexpected authentication error.";
					m_Snapshot.Authenticated = false;
				}

				{
					std::scoped_lock lock(m_Mutex);
					m_Snapshot.Busy = false;
				}

				m_Busy.store(false);
			}
		);
	}

	void JoinWorker() {
		if (m_Worker.joinable()) {
			m_Worker.join();
		}
	}

	const loader::ProductDefinition& m_Product;
	KeyAuth::api m_App;

	mutable std::mutex m_Mutex;
	AuthSnapshot m_Snapshot{};
	std::thread m_Worker;
	std::atomic<bool> m_Busy{false};
	std::chrono::steady_clock::time_point m_NextSessionCheck{};
};

struct DxState {
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* Target = nullptr;
};

DxState g_Dx{};

void DestroyRenderTarget() {
	if (g_Dx.Target) {
		g_Dx.Target->Release();
		g_Dx.Target = nullptr;
	}
}

bool CreateRenderTarget() {
	ID3D11Texture2D* backBuffer = nullptr;

	if (
		FAILED(g_Dx.SwapChain->GetBuffer(
			0,
			IID_PPV_ARGS(&backBuffer)
		)) ||
		!backBuffer
	) {
		return false;
	}

	const HRESULT result = g_Dx.Device->CreateRenderTargetView(
		backBuffer,
		nullptr,
		&g_Dx.Target
	);

	backBuffer->Release();
	return SUCCEEDED(result);
}

bool CreateDevice(HWND hwnd) {
	DXGI_SWAP_CHAIN_DESC desc{};
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = hwnd;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevel{};
	const D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	const HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		levels,
		static_cast<UINT>(_countof(levels)),
		D3D11_SDK_VERSION,
		&desc,
		&g_Dx.SwapChain,
		&g_Dx.Device,
		&featureLevel,
		&g_Dx.Context
	);

	return SUCCEEDED(result) && CreateRenderTarget();
}

void CleanupDevice() {
	DestroyRenderTarget();
	if (g_Dx.SwapChain) g_Dx.SwapChain->Release();
	if (g_Dx.Context) g_Dx.Context->Release();
	if (g_Dx.Device) g_Dx.Device->Release();
	g_Dx.SwapChain = nullptr;
	g_Dx.Context = nullptr;
	g_Dx.Device = nullptr;
}

void CenterWindow(HWND hwnd) {
	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(
		hwnd,
		HWND_TOP,
		(screenWidth - kWindowWidth) / 2,
		(screenHeight - kWindowHeight) / 2,
		kWindowWidth,
		kWindowHeight,
		SWP_NOACTIVATE
	);
}

LRESULT CALLBACK WndProc(
	HWND hwnd,
	UINT message,
	WPARAM wparam,
	LPARAM lparam
) {
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

			if (point.y >= 0 && point.y < 38) {
				return HTCAPTION;
			}
		}

		return hit;
	}

	case WM_SIZE:
		if (
			g_Dx.Device &&
			wparam != SIZE_MINIMIZED &&
			g_Dx.SwapChain
		) {
			DestroyRenderTarget();

			const HRESULT resizeResult = g_Dx.SwapChain->ResizeBuffers(
				0,
				LOWORD(lparam),
				HIWORD(lparam),
				DXGI_FORMAT_UNKNOWN,
				0
			);

			if (SUCCEEDED(resizeResult)) {
				(void)CreateRenderTarget();
			}
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

void ApplyStyle() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 2.0f;
	style.PopupRounding = 0.0f;
	style.GrabRounding = 2.0f;
	style.WindowPadding = ImVec2(0.0f, 0.0f);
	style.FramePadding = ImVec2(10.0f, 7.0f);
	style.ItemSpacing = ImVec2(8.0f, 9.0f);
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

	ImVec4* colors = style.Colors;

	colors[ImGuiCol_WindowBg] =
		ImVec4(0.055f, 0.055f, 0.060f, 1.0f);
	colors[ImGuiCol_ChildBg] =
		ImVec4(0.075f, 0.075f, 0.082f, 1.0f);
	colors[ImGuiCol_Border] =
		ImVec4(0.17f, 0.17f, 0.19f, 1.0f);
	colors[ImGuiCol_FrameBg] =
		ImVec4(0.11f, 0.11f, 0.12f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] =
		ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
	colors[ImGuiCol_FrameBgActive] =
		ImVec4(0.17f, 0.17f, 0.19f, 1.0f);
	colors[ImGuiCol_Button] =
		ImVec4(0.36f, 0.18f, 0.62f, 1.0f);
	colors[ImGuiCol_ButtonHovered] =
		ImVec4(0.43f, 0.23f, 0.72f, 1.0f);
	colors[ImGuiCol_ButtonActive] =
		ImVec4(0.31f, 0.15f, 0.55f, 1.0f);
	colors[ImGuiCol_CheckMark] =
		ImVec4(0.67f, 0.42f, 0.95f, 1.0f);
	colors[ImGuiCol_Text] =
		ImVec4(0.88f, 0.88f, 0.90f, 1.0f);
	colors[ImGuiCol_TextDisabled] =
		ImVec4(0.48f, 0.48f, 0.52f, 1.0f);
}

bool LaunchConfiguredTarget(
	const loader::ProductDefinition& product,
	std::string& message
) {
	wchar_t target[32768]{};

	const DWORD count = GetEnvironmentVariableW(
		product.TargetEnvironmentVariable.data(),
		target,
		static_cast<DWORD>(_countof(target))
	);

	if (count == 0 || count >= _countof(target)) {
		message =
			"Authenticated. Configure " +
			std::string(product.Slug) +
			"'s launch target with its dedicated environment variable.";
		return false;
	}

	const std::filesystem::path path(target);

	if (!std::filesystem::is_regular_file(path)) {
		message = "Configured target executable was not found.";
		return false;
	}

	std::wstring command =
		L"\"" + path.wstring() + L"\"";

	std::vector<wchar_t> mutableCommand(
		command.begin(),
		command.end()
	);
	mutableCommand.push_back(L'\0');

	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);

	PROCESS_INFORMATION process{};

	if (!CreateProcessW(
		path.c_str(),
		mutableCommand.data(),
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		path.parent_path().c_str(),
		&startup,
		&process
	)) {
		message = "Failed to start configured target executable.";
		return false;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	message =
		std::string(product.DisplayName) +
		" target launched.";

	return true;
}

void DrawWindowChrome(bool& requestClose) {
	ImDrawList* draw = ImGui::GetWindowDrawList();

	const ImVec2 min = ImGui::GetWindowPos();
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const ImVec2 max(
		min.x + windowSize.x,
		min.y + windowSize.y
	);

	draw->AddRectFilled(
		min,
		ImVec2(max.x, min.y + 38.0f),
		IM_COL32(20, 20, 22, 255)
	);

	draw->AddLine(
		ImVec2(min.x, min.y + 38.0f),
		ImVec2(max.x, min.y + 38.0f),
		IM_COL32(63, 39, 90, 255)
	);

	ImGui::SetCursorPos(ImVec2(18.0f, 12.0f));
	ImGui::TextUnformatted("LUVKRIMES // MULTI LOADER");

	ImGui::SetCursorPos(ImVec2(588.0f, 8.0f));

	if (ImGui::Button("X", ImVec2(24.0f, 22.0f))) {
		requestClose = true;
	}
}

bool ProductCard(
	const loader::ProductDefinition& product,
	ImVec2 size
) {
	ImGui::PushID(product.Slug.data());

	const ImVec2 cursor = ImGui::GetCursorScreenPos();

	ImGui::BeginDisabled(!product.Configured);

	const bool pressed =
		ImGui::Button("##card", size);

	ImGui::EndDisabled();

	ImDrawList* draw = ImGui::GetWindowDrawList();

	const ImVec2 bottomRight(
		cursor.x + size.x,
		cursor.y + size.y
	);

	draw->AddRect(
		cursor,
		bottomRight,
		product.Configured
			? IM_COL32(89, 54, 128, 255)
			: IM_COL32(48, 48, 52, 255)
	);

	draw->AddText(
		ImVec2(cursor.x + 16.0f, cursor.y + 18.0f),
		product.Configured
			? IM_COL32(235, 235, 238, 255)
			: IM_COL32(120, 120, 125, 255),
		product.DisplayName.data()
	);

	draw->AddText(
		ImVec2(cursor.x + 16.0f, cursor.y + 46.0f),
		IM_COL32(140, 140, 148, 255),
		product.Subtitle.data()
	);

	draw->AddText(
		ImVec2(
			cursor.x + 16.0f,
			cursor.y + size.y - 30.0f
		),
		product.Configured
			? IM_COL32(171, 107, 242, 255)
			: IM_COL32(180, 100, 100, 255),
		product.Configured
			? "SELECT"
			: "CONFIGURATION REQUIRED"
	);

	ImGui::PopID();
	return pressed && product.Configured;
}

void DrawProductSelection(
	const loader::ProductDefinition*& selectedProduct,
	Screen& screen
) {
	ImGui::SetCursorPos(ImVec2(34.0f, 67.0f));

	ImGui::BeginChild(
		"##products",
		ImVec2(552.0f, 278.0f),
		true
	);

	ImGui::TextColored(
		ImVec4(0.67f, 0.42f, 0.95f, 1.0f),
		"SELECT PRODUCT"
	);

	ImGui::TextDisabled(
		"Each product uses its own independent license pool."
	);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec2 cardSize(250.0f, 148.0f);

	for (std::size_t index = 0; index < loader::Products.size(); ++index) {
		const auto& product = loader::Products[index];

		if (index > 0) {
			ImGui::SameLine();
		}

		if (ProductCard(product, cardSize)) {
			selectedProduct = &product;
			screen = Screen::Authentication;
		}
	}

	ImGui::Spacing();
	ImGui::TextDisabled(
		"More projects can be added to product_registry.hpp."
	);

	ImGui::EndChild();
}

void LoadRememberedLicense(
	const loader::ProductDefinition& product,
	std::array<char, 192>& license
) {
	license.fill('\0');

	std::string savedLicense;

	if (!loader::license_store::Load(product.Slug, savedLicense)) {
		return;
	}

	const std::size_t count = (std::min)(
		savedLicense.size(),
		license.size() - 1
	);

	std::memcpy(
		license.data(),
		savedLicense.data(),
		count
	);

	license[count] = '\0';
}

void DrawAuthentication(
	const loader::ProductDefinition& product,
	AuthController& auth,
	std::array<char, 192>& license,
	bool& remember,
	bool& goBack,
	bool& requestClose
) {
	const AuthSnapshot snapshot = auth.Snapshot();

	ImGui::SetCursorPos(ImVec2(34.0f, 61.0f));

	if (ImGui::Button("< PRODUCTS", ImVec2(112.0f, 28.0f))) {
		goBack = true;
	}

	ImGui::SetCursorPos(ImVec2(34.0f, 100.0f));

	ImGui::BeginChild(
		"##auth-card",
		ImVec2(552.0f, 245.0f),
		true
	);

	ImGui::TextColored(
		ImVec4(0.67f, 0.42f, 0.95f, 1.0f),
		"%s // AUTHENTICATION",
		product.DisplayName.data()
	);

	ImGui::TextDisabled(
		"Independent KeyAuth application // API 1.3 // v%s",
		product.KeyAuthVersion.data()
	);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (!snapshot.Authenticated) {
		ImGui::Text(
			"%s license key",
			product.DisplayName.data()
		);

		ImGui::SetNextItemWidth(-1.0f);

		ImGui::InputTextWithHint(
			"##license",
			"XXXXXX-XXXXXX-XXXXXX",
			license.data(),
			license.size(),
			ImGuiInputTextFlags_Password
		);

		ImGui::Checkbox(
			"Remember this product key on this Windows account",
			&remember
		);

		ImGui::Spacing();

		ImGui::BeginDisabled(snapshot.Busy);

		if (ImGui::Button(
			"AUTHENTICATE",
			ImVec2(-1.0f, 38.0f)
		)) {
			auth.Authenticate(
				std::string(license.data()),
				remember
			);
		}

		ImGui::EndDisabled();
	} else {
		ImGui::Text(
			"Product: %s",
			product.DisplayName.data()
		);

		ImGui::Text(
			"User: %s",
			snapshot.Username.empty()
				? "licensed user"
				: snapshot.Username.c_str()
		);

		if (!snapshot.Subscription.empty()) {
			ImGui::Text(
				"Subscription: %s",
				snapshot.Subscription.c_str()
			);
		}

		if (!snapshot.Expiry.empty()) {
			ImGui::Text(
				"Expiry: %s",
				snapshot.Expiry.c_str()
			);
		}

		ImGui::Spacing();

		static std::string launchStatus;

		if (ImGui::Button(
			"LAUNCH",
			ImVec2(-1.0f, 38.0f)
		)) {
			if (LaunchConfiguredTarget(product, launchStatus)) {
				requestClose = true;
			}
		}

		if (!launchStatus.empty()) {
			ImGui::TextWrapped(
				"%s",
				launchStatus.c_str()
			);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 statusColor =
		snapshot.Authenticated
			? ImVec4(0.45f, 0.85f, 0.55f, 1.0f)
			: snapshot.State == AuthState::Error ||
			  snapshot.State == AuthState::SessionInvalid
				? ImVec4(0.95f, 0.38f, 0.38f, 1.0f)
				: ImVec4(0.70f, 0.70f, 0.74f, 1.0f);

	ImGui::TextColored(
		statusColor,
		"%s",
		snapshot.Status.c_str()
	);

	ImGui::EndChild();
}

} // namespace

int WINAPI wWinMain(
	HINSTANCE instance,
	HINSTANCE,
	PWSTR,
	int
) {
	ImGui_ImplWin32_EnableDpiAwareness();

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = &WndProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.lpszClassName = kClassName;

	if (!RegisterClassExW(&wc)) {
		return 1;
	}

	HWND hwnd = CreateWindowExW(
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
		instance,
		nullptr
	);

	if (!hwnd) {
		UnregisterClassW(kClassName, instance);
		return 1;
	}

	CenterWindow(hwnd);

	if (!CreateDevice(hwnd)) {
		CleanupDevice();
		DestroyWindow(hwnd);
		UnregisterClassW(kClassName, instance);
		return 1;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	ApplyStyle();

	if (
		!ImGui_ImplWin32_Init(hwnd) ||
		!ImGui_ImplDX11_Init(g_Dx.Device, g_Dx.Context)
	) {
		ImGui::DestroyContext();
		CleanupDevice();
		DestroyWindow(hwnd);
		UnregisterClassW(kClassName, instance);
		return 1;
	}

	Screen screen = Screen::ProductSelect;
	const loader::ProductDefinition* selectedProduct = nullptr;

	std::unique_ptr<AuthController> auth;
	std::array<char, 192> license{};
	bool remember = true;

	bool running = true;

	while (running) {
		MSG msg{};

		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				running = false;
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (!running) {
			break;
		}

		if (auth) {
			auth->Tick();
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

		ImGui::Begin(
			"##loader",
			nullptr,
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings
		);

		bool requestClose = false;

		DrawWindowChrome(requestClose);

		if (screen == Screen::ProductSelect) {
			const loader::ProductDefinition* requestedProduct = nullptr;

			DrawProductSelection(
				requestedProduct,
				screen
			);

			if (requestedProduct) {
				selectedProduct = requestedProduct;
				remember = true;

				LoadRememberedLicense(
					*selectedProduct,
					license
				);

				auth =
					std::make_unique<AuthController>(
						*selectedProduct
					);

				auth->Initialize();
			}
		} else if (
			screen == Screen::Authentication &&
			selectedProduct &&
			auth
		) {
			bool goBack = false;

			DrawAuthentication(
				*selectedProduct,
				*auth,
				license,
				remember,
				goBack,
				requestClose
			);

			if (goBack) {
				auth.reset();
				selectedProduct = nullptr;
				license.fill('\0');
				screen = Screen::ProductSelect;
			}
		}

		ImGui::End();
		ImGui::Render();

		constexpr float clear[4] = {
			0.055f,
			0.055f,
			0.060f,
			1.0f
		};

		g_Dx.Context->OMSetRenderTargets(
			1,
			&g_Dx.Target,
			nullptr
		);

		g_Dx.Context->ClearRenderTargetView(
			g_Dx.Target,
			clear
		);

		ImGui_ImplDX11_RenderDrawData(
			ImGui::GetDrawData()
		);

		const HRESULT presentResult = g_Dx.SwapChain->Present(1, 0);
		if (FAILED(presentResult) && presentResult != DXGI_STATUS_OCCLUDED) {
			running = false;
		}

		if (requestClose) {
			running = false;
		}
	}

	auth.reset();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDevice();
	DestroyWindow(hwnd);
	UnregisterClassW(kClassName, instance);

	return 0;
}
