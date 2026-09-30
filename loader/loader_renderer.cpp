#include "loader_renderer.hpp"

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace loader {

Renderer::~Renderer() {
	Shutdown();
}

bool Renderer::CreateRenderTarget() {
	if (!m_SwapChain || !m_Device) {
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	const HRESULT bufferResult = m_SwapChain->GetBuffer(
		0,
		IID_PPV_ARGS(backBuffer.GetAddressOf())
	);

	if (FAILED(bufferResult) || !backBuffer) {
		return false;
	}

	const HRESULT targetResult = m_Device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		m_Target.GetAddressOf()
	);

	return SUCCEEDED(targetResult) && m_Target;
}

bool Renderer::Initialize(HWND hwnd) {
	if (!hwnd) {
		return false;
	}

	Shutdown();

	DXGI_SWAP_CHAIN_DESC desc{};
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = hwnd;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	const D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};
	D3D_FEATURE_LEVEL selectedLevel{};

	const HRESULT createResult = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		levels,
		static_cast<UINT>(_countof(levels)),
		D3D11_SDK_VERSION,
		&desc,
		m_SwapChain.GetAddressOf(),
		m_Device.GetAddressOf(),
		&selectedLevel,
		m_Context.GetAddressOf()
	);

	if (FAILED(createResult) || !CreateRenderTarget()) {
		Shutdown();
		return false;
	}

	ImGui::CreateContext();
	m_ImGuiContextCreated = true;

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	ImFont* uiFont = io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\segoeui.ttf",
		16.0f,
		nullptr,
		io.Fonts->GetGlyphRangesDefault()
	);
	if (!uiFont) {
		uiFont = io.Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\seguisb.ttf",
			16.0f,
			nullptr,
			io.Fonts->GetGlyphRangesDefault()
		);
	}
	if (uiFont) {
		io.FontDefault = uiFont;
	}

	if (!ImGui_ImplWin32_Init(hwnd)) {
		Shutdown();
		return false;
	}
	m_Win32BackendReady = true;

	if (!ImGui_ImplDX11_Init(m_Device.Get(), m_Context.Get())) {
		Shutdown();
		return false;
	}
	m_Dx11BackendReady = true;

	return true;
}

void Renderer::Shutdown() {
	if (m_Dx11BackendReady) {
		ImGui_ImplDX11_Shutdown();
		m_Dx11BackendReady = false;
	}

	if (m_Win32BackendReady) {
		ImGui_ImplWin32_Shutdown();
		m_Win32BackendReady = false;
	}

	if (m_ImGuiContextCreated && ImGui::GetCurrentContext()) {
		ImGui::DestroyContext();
	}
	m_ImGuiContextCreated = false;

	m_Target.Reset();
	m_SwapChain.Reset();
	m_Context.Reset();
	m_Device.Reset();
}

void Renderer::BeginFrame() {
	if (!IsReady()) {
		return;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

bool Renderer::EndFrame() {
	if (!IsReady()) {
		return false;
	}

	ImGui::Render();

	ID3D11RenderTargetView* target = m_Target.Get();
	m_Context->OMSetRenderTargets(1, &target, nullptr);

	constexpr float clearColor[4] = {
		0.055f,
		0.055f,
		0.060f,
		1.0f
	};
	m_Context->ClearRenderTargetView(target, clearColor);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	const HRESULT presentResult = m_SwapChain->Present(1, 0);
	if (presentResult == DXGI_STATUS_OCCLUDED) {
		return true;
	}

	return SUCCEEDED(presentResult);
}

bool Renderer::Resize(UINT width, UINT height) {
	if (!m_SwapChain || !m_Context || width == 0 || height == 0) {
		return false;
	}

	m_Context->OMSetRenderTargets(0, nullptr, nullptr);
	m_Target.Reset();

	const HRESULT resizeResult = m_SwapChain->ResizeBuffers(
		0,
		width,
		height,
		DXGI_FORMAT_UNKNOWN,
		0
	);

	if (FAILED(resizeResult)) {
		return false;
	}

	return CreateRenderTarget();
}

bool Renderer::IsReady() const noexcept {
	return
		m_Device &&
		m_Context &&
		m_SwapChain &&
		m_Target &&
		m_ImGuiContextCreated &&
		m_Win32BackendReady &&
		m_Dx11BackendReady;
}

} // namespace loader
