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

The same matrix builds and executes the standalone regression tests in `tests/`. CI also verifies that the application, loader and test executables were actually produced before running the test binary. First-party C++ warnings are treated as errors; bundled/vendor sources keep their own warning policy.

## Runtime architecture

- `main.cpp` — application startup and shutdown
- `workspace/driver` — low-level device/process interface
- `workspace/game/cache` — cached runtime state
- `workspace/game/features` — rendering-facing feature logic
- `workspace/interface/window.*` — DPI-aware Win32 virtual-desktop window lifetime
- `workspace/interface/renderer.*` — D3D11 + ImGui lifetime and swap-chain resizing
- `workspace/interface/input.*` — keyboard/mouse input routing
- `workspace/interface/settings_store.*` — persistent local settings
- `workspace/interface/interface.cpp` — runtime orchestration
- `workspace/interface/menu.*` — menu and diagnostics
- `workspace/util/config` — generic key/value configuration parser
- `workspace/util/crash` — local Windows minidump crash diagnostics
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


## Settings and display handling

Settings are loaded automatically from `%LOCALAPPDATA%\luvkrimes\settings.ini` and saved again on clean shutdown. Writes use a temporary file and replacement step so an interrupted write is less likely to leave a partially written settings file. The Config page also exposes explicit save/load/default controls.

The overlay window is DPI-aware and spans the Windows virtual desktop rather than only the primary display. Display-layout changes trigger a window resync and D3D11 swap-chain resize.

## Crash diagnostics

Unhandled process crashes write a `MiniDumpNormal` file into a `crashdumps` folder next to the executable. Runtime logs are also flushed to `%LOCALAPPDATA%\luvkrimes\logs\latest.log`, which helps correlate the dump with the last successful runtime stages. These files are intended for local debugging and are not uploaded automatically.


## Loader and authentication

A separate `loader/luvkrimes-loader.vcxproj` project provides a dark ImGui authentication window using KeyAuth API 1.3.

The loader is split into focused components rather than a single entry-point file:
- `loader_main.cpp` — runtime orchestration only.
- `loader_window.*` — Win32 window lifetime, message pump, dragging and resize forwarding.
- `loader_renderer.*` — D3D11/ImGui lifetime, swap-chain resize and frame presentation using RAII COM ownership.
- `loader_ui.*` — product selection and authentication UI state.
- `auth_controller.*` — asynchronous KeyAuth initialization, license authentication and session revalidation.
- `license_store.*` — product-scoped DPAPI persistence.
- `launch_target.*` — product-specific post-authentication process launch.
- `product_registry.hpp` — compile-time product definitions and registry validation.

The KeyAuth SDK is not vendored into this repository. The loader's pre-build step runs `scripts/bootstrap-keyauth.ps1`, which fetches the official KeyAuth 1.3 C++ library at pinned commit `486c83e6259f508ba0396f3156e50792a34a4576`. The optional upstream `Security.hpp` and `killEmulator.hpp` modules are removed after bootstrap, leaving normal authentication/session behavior without optional anti-analysis or emulator-killing logic.

The loader is now product-driven through `loader/product_registry.hpp`. Each product owns an independent KeyAuth configuration, an independent remembered-license file, and an independent launch target.

Current products:
- **Fortnite** — configured against KeyAuth application `Timocod18ytb's Application`, owner ID `ZOhORJsXc1`, version `1.0`.
- **Apex Legends** — present in the product selector but intentionally marked `configuration required` until its own separate KeyAuth application identifiers are supplied.

Fortnite and Apex must use different KeyAuth applications if their key pools must be isolated. A Fortnite key is therefore sent only to the Fortnite KeyAuth application; an Apex key will be sent only to the Apex KeyAuth application once Apex is configured.

The loader supports license-key authentication, 60-second session revalidation, and optional remembered-license storage using Windows DPAPI. Remembered license material is split by product under `%LOCALAPPDATA%\luvkrimes\licenses\<product>.dat`, encrypted for the current Windows account, and is never loaded across products.

Post-authentication launch targets are product-specific:
- `LUVKRIMES_TARGET_FORTNITE`
- `LUVKRIMES_TARGET_APEX`

Adding another project later only requires a new entry in `product_registry.hpp` with its own KeyAuth application configuration and target variable. Registry invariants reject duplicate IDs, slugs and target variables at compile time.

The regression suite covers configuration parsing/persistence, product-registry lookup and slug rules, DPAPI remembered-license save/load/clear behavior, disabled-product launch rejection, projection edge cases and transform/quaternion consistency.

No injection, process hiding, anti-debugging, VM detection, or driver concealment is performed by the loader.
