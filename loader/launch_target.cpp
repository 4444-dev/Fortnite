#include "launch_target.hpp"

#include <Windows.h>

#include <filesystem>
#include <vector>

namespace loader {

bool LaunchConfiguredTarget(
	const ProductDefinition& product,
	std::string& message
) {
	wchar_t target[32768]{};

	const DWORD count = GetEnvironmentVariableW(
		product.TargetEnvironmentVariable.data(),
		target,
		static_cast<DWORD>(_countof(target))
	);

	if (count == 0 || count >= _countof(target)) {
		message =
			"Authenticated. Configure " +
			std::string(product.Slug) +
			"'s launch target with its dedicated environment variable.";
		return false;
	}

	const std::filesystem::path path(target);

	if (!std::filesystem::is_regular_file(path)) {
		message = "Configured target executable was not found.";
		return false;
	}

	std::wstring command = L"\"" + path.wstring() + L"\"";
	std::vector<wchar_t> mutableCommand(command.begin(), command.end());
	mutableCommand.push_back(L'\0');

	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);

	PROCESS_INFORMATION process{};

	if (!CreateProcessW(
		path.c_str(),
		mutableCommand.data(),
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		path.parent_path().c_str(),
		&startup,
		&process
	)) {
		message = "Failed to start configured target executable.";
		return false;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	message = std::string(product.DisplayName) + " target launched.";
	return true;
}

} // namespace loader
