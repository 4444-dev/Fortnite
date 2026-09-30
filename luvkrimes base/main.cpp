#include <includes.hpp>
#include <workspace/util/logger/logger.hpp>
#include <workspace/util/crash/crash_handler.hpp>

#include <chrono>
#include <thread>

namespace {

class ScopedHandle final {
public:
	explicit ScopedHandle(HANDLE handle = nullptr) noexcept
		: m_Handle(handle) {
	}

	~ScopedHandle() {
		Reset();
	}

	ScopedHandle(const ScopedHandle&) = delete;
	ScopedHandle& operator=(const ScopedHandle&) = delete;

	[[nodiscard]] HANDLE Get() const noexcept {
		return m_Handle;
	}

	[[nodiscard]] bool IsValid() const noexcept {
		return m_Handle && m_Handle != INVALID_HANDLE_VALUE;
	}

	void Reset(HANDLE handle = nullptr) noexcept {
		if (IsValid()) {
			CloseHandle(m_Handle);
		}
		m_Handle = handle;
	}

private:
	HANDLE m_Handle = nullptr;
};

u32 FindPidByName(const wchar_t* name) {
	if (!name || !*name) {
		return 0;
	}

	ScopedHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (!snapshot.IsValid()) {
		return 0;
	}

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(entry);

	if (!Process32FirstW(snapshot.Get(), &entry)) {
		return 0;
	}

	do {
		if (_wcsicmp(entry.szExeFile, name) == 0) {
			return entry.th32ProcessID;
		}
	} while (Process32NextW(snapshot.Get(), &entry));

	return 0;
}

u32 WaitForProcess(const wchar_t* name, const char* prettyName) {
	if (const u32 pid = FindPidByName(name)) {
		logger::Log("%s already running (pid %u)", prettyName, pid);
		return pid;
	}

	logger::Log("waiting for %s...", prettyName);

	u32 pid = 0;
	while (!(pid = FindPidByName(name))) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	logger::Log("%s launched (pid %u)", prettyName, pid);
	return pid;
}

void PressKeyToExit() {
	logger::Log("press any key to exit...");
	(void)getchar();
}

} // namespace

i32 main(i32, char**) {
	crash_handler::Install();
	logger::Init();
	SetConsoleTitleW(L"Nexus");

	constexpr const wchar_t* processName = L"FortniteClient-Win64-Shipping.exe";

	const u32 processId = WaitForProcess(processName, "fortnite");

	logger::Log("attaching driver");

	uptr imageBase = 0;
	if (!AttachDriver(processName, &imageBase)) {
		logger::Log("attach failed (win err %lu)", GetLastError());
		logger::Log("check that the required device is available");
		PressKeyToExit();
		return 1;
	}

	logger::Log(
		"attached, image 0x%llx",
		static_cast<unsigned long long>(imageBase)
	);

	PlayerCache players;
	CameraCache camera;

	players.SetImageBase(imageBase);
	players.Start();
	logger::Log("cache threads running");

	logger::Log("starting overlay");
	const bool ok = overlay::run(players, camera, processId);

	players.Stop();

	if (!ok) {
		logger::Log("overlay exited with error");
		PressKeyToExit();
		return 1;
	}

	logger::Log("clean exit");
	return 0;
}
