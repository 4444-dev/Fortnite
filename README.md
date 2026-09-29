# luvkrimes base

Windows x64 C++20 project using Direct3D 11 and Dear ImGui.

## Build

- Visual Studio with the Desktop development with C++ workload
- Windows 10/11 SDK
- x64 configuration
- C++20-capable MSVC toolset

Open `luvkrimes base.slnx`, select `Debug | x64` or `Release | x64`, then build the solution.

The project uses warning level `/W4` for first-party code. Dear ImGui sources keep their vendor warning level so project warnings remain actionable.

## Continuous integration

GitHub Actions builds both:

- `Debug | x64`
- `Release | x64`

on pull requests targeting `main`, and on pushes to `main`.

## Runtime architecture

- `main.cpp` — application startup and shutdown
- `workspace/driver` — low-level device/process interface
- `workspace/game/cache` — cached runtime state
- `workspace/game/features` — rendering-facing feature logic
- `workspace/interface/window.*` — Win32 window lifetime
- `workspace/interface/renderer.*` — D3D11 + ImGui lifetime
- `workspace/interface/interface.cpp` — runtime/input orchestration
- `workspace/interface/menu.*` — menu and diagnostics
- `workspace/util` — shared utilities
- `thirdparty/imgui` — Dear ImGui sources

## Cache and diagnostics

Player snapshots use two buffers: the cache thread prepares the inactive buffer and atomically publishes it when complete. The renderer reads a stable snapshot under a shared lock instead of copying the full player vector every frame.

The Config page exposes runtime health and timing information:

- world / camera state
- actor and player counts
- FPS and frame time
- engine-cache time
- actor-scan time
- player-cache time

Generated build output, Visual Studio state and local offset dumps are ignored by Git.
