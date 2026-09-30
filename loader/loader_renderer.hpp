#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace loader {

class Renderer final {
public:
	Renderer() = default;
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	[[nodiscard]] bool Initialize(HWND hwnd);
	void Shutdown();

	void BeginFrame();
	[[nodiscard]] bool EndFrame();
	[[nodiscard]] bool Resize(UINT width, UINT height);
	[[nodiscard]] bool IsReady() const noexcept;

private:
	[[nodiscard]] bool CreateRenderTarget();

	Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_Target;

	bool m_ImGuiContextCreated = false;
	bool m_Win32BackendReady = false;
	bool m_Dx11BackendReady = false;
};

} // namespace loader
