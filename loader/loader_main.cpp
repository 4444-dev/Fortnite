#include "loader_renderer.hpp"
#include "loader_ui.hpp"
#include "loader_window.hpp"

#include <Windows.h>

int WINAPI wWinMain(
	HINSTANCE,
	HINSTANCE,
	PWSTR,
	int
) {
	loader::Renderer renderer;
	loader::Window window;

	if (!window.Create()) {
		MessageBoxW(
			nullptr,
			L"Unable to create the loader window.",
			L"Nexus",
			MB_OK | MB_ICONERROR
		);
		return 1;
	}

	window.AttachRenderer(&renderer);

	if (!renderer.Initialize(window.Handle())) {
		MessageBoxW(
			window.Handle(),
			L"Unable to initialize the Direct3D 11 renderer.",
			L"Nexus",
			MB_OK | MB_ICONERROR
		);
		return 1;
	}

	loader::ApplyLoaderStyle();
	window.Show();

	bool running = true;
	bool rendererFailed = false;

	{
		loader::UiController ui;

		while (running && window.PumpMessages()) {
			ui.Tick();

			if (!renderer.IsReady()) {
				rendererFailed = true;
				break;
			}

			renderer.BeginFrame();

			bool requestClose = false;
			bool requestMinimize = false;
			ui.Draw(
				requestClose,
				requestMinimize
			);

			if (requestMinimize) {
				window.Minimize();
			}

			if (!renderer.EndFrame()) {
				rendererFailed = true;
				running = false;
			} else if (requestClose) {
				running = false;
			}
		}
	}

	if (rendererFailed) {
		MessageBoxW(
			window.Handle(),
			L"The renderer stopped unexpectedly. Restart the loader and check your graphics driver if the problem persists.",
			L"Nexus",
			MB_OK | MB_ICONERROR
		);
	}


	window.AttachRenderer(nullptr);
	renderer.Shutdown();
	window.Destroy();

	return rendererFailed ? 1 : 0;
}
