#include "loader_renderer.hpp"
#include "loader_ui.hpp"
#include "loader_window.hpp"

int WINAPI wWinMain(
	HINSTANCE,
	HINSTANCE,
	PWSTR,
	int
) {
	loader::Renderer renderer;
	loader::Window window;

	if (!window.Create()) {
		return 1;
	}

	window.AttachRenderer(&renderer);

	if (!renderer.Initialize(window.Handle())) {
		return 1;
	}

	loader::ApplyLoaderStyle();
	window.Show();

	bool running = true;

	{
		loader::UiController ui;

		while (running && window.PumpMessages()) {
			ui.Tick();

			if (!renderer.IsReady()) {
				break;
			}

			renderer.BeginFrame();

			bool requestClose = false;
			ui.Draw(requestClose);

			if (!renderer.EndFrame() || requestClose) {
				running = false;
			}
		}
	}

	window.AttachRenderer(nullptr);
	renderer.Shutdown();
	window.Destroy();

	return 0;
}
