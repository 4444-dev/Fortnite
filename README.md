# luvkrimes base

Windows x64 C++20 project using Direct3D 11 and Dear ImGui.

## Build

- Visual Studio with the Desktop development with C++ workload
- Windows 10/11 SDK
- x64 configuration
- C++20-capable MSVC toolset

Open `luvkrimes base.slnx`, select `Debug | x64` or `Release | x64`, then build the solution.

## Project layout

- `main.cpp` — application startup and shutdown
- `workspace/driver` — low-level device/process interface
- `workspace/game/cache` — cached runtime state
- `workspace/game/features` — rendering-facing feature logic
- `workspace/interface` — Win32/D3D11/ImGui layer, split into window, renderer, runtime loop and menu
- `workspace/util` — shared utilities
- `thirdparty/imgui` — Dear ImGui sources

Generated build output, Visual Studio state and local offset dumps are ignored by Git.
