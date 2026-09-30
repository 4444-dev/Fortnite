#include <workspace/interface/renderer.hpp>

#include <dxgi.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>
#include <workspace/interface/menu.hpp>
#include <workspace/util/logger/logger.hpp>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace overlay {

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
		logger::Log(
			"[renderer] swap-chain GetBuffer failed (hr=0x%08lX)",
			static_cast<unsigned long>(bufferResult)
		);
		return false;
	}

	const HRESULT rtvResult = m_Device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		m_RenderTargetView.GetAddressOf()
	);

	if (FAILED(rtvResult)) {
		logger::Log(
			"[renderer] CreateRenderTargetView failed (hr=0x%08lX)",
			static_cast<unsigned long>(rtvResult)
		);
		return false;
	}

	return true;
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
		m_DeviceContext.GetAddressOf()
	);

	if (FAILED(createResult)) {
		logger::Log(
			"[renderer] D3D11CreateDeviceAndSwapChain failed (hr=0x%08lX)",
			static_cast<unsigned long>(createResult)
		);
		return false;
	}

	if (!CreateRenderTarget()) {
		Shutdown();
		return false;
	}

	ImGui::CreateContext();
	m_ImGuiContextCreated = true;

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	ImGui::StyleColorsDark();

	menu::setup();

	m_EspFont = io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\verdanab.ttf",
		14.0f,
		nullptr,
		io.Fonts->GetGlyphRangesDefault()
	);
	if (!m_EspFont) {
		m_EspFont = io.Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\seguisb.ttf",
			14.0f,
			nullptr,
			io.Fonts->GetGlyphRangesDefault()
		);
	}
	if (!m_EspFont && !io.Fonts->Fonts.empty()) {
		m_EspFont = io.Fonts->Fonts.back();
	}

	if (!ImGui_ImplWin32_Init(hwnd)) {
		logger::Log("[renderer] ImGui Win32 backend initialization failed");
		Shutdown();
		return false;
	}
	m_Win32BackendReady = true;

	if (!ImGui_ImplDX11_Init(m_Device.Get(), m_DeviceContext.Get())) {
		logger::Log("[renderer] ImGui DX11 backend initialization failed");
		Shutdown();
		return false;
	}
	m_Dx11BackendReady = true;

	logger::Log(
		"[renderer] initialized (feature level 0x%X)",
		static_cast<unsigned>(selectedLevel)
	);
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
	m_EspFont = nullptr;

	m_RenderTargetView.Reset();
	m_SwapChain.Reset();
	m_DeviceContext.Reset();
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

	constexpr float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	ImGui::Render();

	ID3D11RenderTargetView* target = m_RenderTargetView.Get();
	m_DeviceContext->OMSetRenderTargets(1, &target, nullptr);
	m_DeviceContext->ClearRenderTargetView(target, clearColor);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	const HRESULT result = m_SwapChain->Present(1, 0);
	if (result == DXGI_STATUS_OCCLUDED) {
		Sleep(16);
		return true;
	}
	if (FAILED(result)) {
		logger::Log(
			"[renderer] Present failed (hr=0x%08lX)",
			static_cast<unsigned long>(result)
		);
		return false;
	}

	return true;
}

bool Renderer::Resize(UINT width, UINT height) {
	if (!m_SwapChain || !m_DeviceContext || width == 0 || height == 0) {
		return false;
	}

	m_DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_RenderTargetView.Reset();

	const HRESULT result = m_SwapChain->ResizeBuffers(
		0,
		width,
		height,
		DXGI_FORMAT_UNKNOWN,
		0
	);

	if (FAILED(result)) {
		logger::Log(
			"[renderer] ResizeBuffers failed (hr=0x%08lX)",
			static_cast<unsigned long>(result)
		);
		return false;
	}

	if (!CreateRenderTarget()) {
		return false;
	}

	logger::Log("[renderer] resized to %ux%u", width, height);
	return true;
}

ImFont* Renderer::EspFont() const noexcept {
	return m_EspFont;
}

float Renderer::Fps() const noexcept {
	if (!m_ImGuiContextCreated || !ImGui::GetCurrentContext()) {
		return 0.0f;
	}
	return ImGui::GetIO().Framerate;
}

bool Renderer::IsReady() const noexcept {
	return
		m_Device &&
		m_DeviceContext &&
		m_SwapChain &&
		m_RenderTargetView &&
		m_ImGuiContextCreated &&
		m_Win32BackendReady &&
		m_Dx11BackendReady;
}

} // namespace overlay
