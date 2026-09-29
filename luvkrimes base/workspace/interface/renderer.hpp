#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

struct ImFont;

namespace overlay {

class Renderer final {
public:
	Renderer() = default;
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	bool Initialize(HWND hwnd);
	void Shutdown();

	void BeginFrame();
	[[nodiscard]] bool EndFrame();

	[[nodiscard]] ImFont* EspFont() const noexcept;
	[[nodiscard]] float Fps() const noexcept;
	[[nodiscard]] bool IsReady() const noexcept;

private:
	Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_DeviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_RenderTargetView;

	ImFont* m_EspFont = nullptr;
	bool m_ImGuiContextCreated = false;
	bool m_Win32BackendReady = false;
	bool m_Dx11BackendReady = false;
};

} // namespace overlay
