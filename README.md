# Nexus

Windows x64 C++20 project using Direct3D 11 and Dear ImGui.

## Build

- Visual Studio with the Desktop development with C++ workload
- Windows 10/11 SDK
- x64 configuration
- C++20-capable MSVC toolset

Open `luvkrimes base.slnx`, select `Debug | x64` or `Release | x64`, then build the solution. The solution/project filenames retain their legacy internal names for source-tree stability; distributed binaries and user-facing branding are Nexus.

The project uses warning level `/W4` for first-party code. Dear ImGui sources keep their vendor warning level so project warnings remain actionable.

## Continuous integration

GitHub Actions builds both:

- `Debug | x64`
- `Release | x64`

on pull requests targeting `main`, and on pushes to `main`.

The same matrix builds and executes the standalone regression tests in `tests/` twice per configuration to catch state leakage between runs. CI also verifies that the application, loader and test executables were actually produced before running the test binary. First-party C++ warnings and all linker warnings are treated as errors; the current Debug and Release builds complete with zero compiler/linker warnings.

## Packaged Apex radar

Nexus packages the Apex radar from the separate public repository `4444-dev/Apex`, pinned to commit `c7df9610d51efa2c236b07ccd04b74e660f08e30` for reproducible client releases.

The installed/portable payload contains:

- `projects\apex\Nexus-Apex-Radar.exe`
- `projects\apex\web\radar.html`
- `projects\apex\web\maps\...`

The radar runtime is not modified by Nexus; the loader only packages and launches the committed binary/assets. Apex remains unavailable for authenticated launch until its own separate KeyAuth application name and Owner ID are configured in `loader/product_registry.hpp`.

## Distribution

Client distribution is produced from tested `Release | x64` binaries. The packaging pipeline creates:

- `Nexus-Setup-<version>.exe` — per-user Windows installer
- `Nexus-Portable-<version>.zip` — portable package with the same runtime layout
- `SHA256SUMS.txt` — SHA-256 checksums for both downloadable packages

The installer uses NSIS 3.12.0, installs under `%LOCALAPPDATA%\Programs\Nexus`, creates Nexus Start Menu and desktop shortcuts, registers a normal Windows uninstaller, and does not require administrator rights. When upgrading from v1.0.0 it removes the known legacy Luvkrimes program files/shortcuts while preserving local user data for automatic migration. Uninstalling Nexus removes the installed program but intentionally preserves settings, logs and remembered-license data under `%LOCALAPPDATA%\Nexus`.

The packaged loader resolves Fortnite automatically from `projects\fortnite\Nexus-Fortnite.exe` next to the loader. `NEXUS_TARGET_FORTNITE` remains available as an explicit development override, so customers do not need to configure environment variables manually.

CI builds the package on every Release configuration and performs a silent install/verify/uninstall round-trip before the build can pass.

`.github/workflows/release.yml` publishes automatically when `release/VERSION` changes on `main`. The workflow builds, tests, packages, installs and uninstalls the client package before publishing the matching semantic-version release. The published assets are the Nexus installer, portable ZIP and checksum file.

Authenticode signing is optional. If repository secrets `WINDOWS_CERTIFICATE_BASE64` and `WINDOWS_CERTIFICATE_PASSWORD` are configured, the release workflow signs the release binaries and installer before publication. Without those secrets, the packages are still built and published but Windows may identify the publisher as unknown.
## Multi-monitor overlay behavior

The in-game overlay is bound to the monitor containing the visible Fortnite process window instead of spanning the entire Windows virtual desktop. Nexus re-checks the target monitor periodically so moving Fortnite to another display updates the overlay without restarting it. The in-game menu starts open and interactive on that same monitor; the configured menu key (Insert by default) toggles click-through mode.

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

Settings are loaded automatically from `%LOCALAPPDATA%\Nexus\settings.ini` and saved again on clean shutdown. If an older `%LOCALAPPDATA%\luvkrimes\settings.ini` exists, Nexus imports it automatically on first load. Writes use a temporary file and replacement step so an interrupted write is less likely to leave a partially written settings file. The Config page also exposes explicit save/load/default controls.

The overlay window is DPI-aware and spans the Windows virtual desktop rather than only the primary display. Display-layout changes trigger a window resync and D3D11 swap-chain resize.

## Crash diagnostics

Unhandled process crashes write a `MiniDumpNormal` file into a `crashdumps` folder next to the executable. Runtime logs are also flushed to `%LOCALAPPDATA%\Nexus\logs\latest.log`, which helps correlate the dump with the last successful runtime stages. These files are intended for local debugging and are not uploaded automatically.


## Loader and authentication

The loader project (`loader/luvkrimes-loader.vcxproj`, retained as an internal source filename) builds the user-facing `Nexus.exe` authentication client using KeyAuth API 1.3.

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

The loader supports license-key authentication, 60-second session revalidation, and optional remembered-license storage using Windows DPAPI. Remembered license material is split by product under `%LOCALAPPDATA%\Nexus\licenses\<product>.dat`, encrypted for the current Windows account and cryptographically bound to the product slug through DPAPI optional entropy. Existing `%LOCALAPPDATA%\luvkrimes\licenses\<product>.dat` files and both historical DPAPI formats are accepted once and transparently migrated to Nexus storage.

Post-authentication launch targets are product-specific. Packaged products use their relative installed/portable executable automatically; environment variables remain optional overrides for development:
- `NEXUS_TARGET_FORTNITE`
- `NEXUS_TARGET_APEX`

For compatibility with v1.0.0 development setups, the former `LUVKRIMES_TARGET_FORTNITE` and `LUVKRIMES_TARGET_APEX` variables are still accepted as fallbacks.

Adding another project later only requires a new entry in `product_registry.hpp` with its own KeyAuth application configuration and target variable. Registry invariants reject duplicate IDs, slugs and target variables at compile time.

The regression suite covers configuration parsing/persistence, product-registry lookup and slug rules, isolated DPAPI remembered-license save/load/clear behavior, maximum-size and corruption handling, cryptographic cross-product rejection, disabled-product launch rejection, projection edge cases and transform/quaternion consistency.

No injection, process hiding, anti-debugging, VM detection, or driver concealment is performed by the loader.
