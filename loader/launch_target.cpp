#include "launch_target.hpp"

#include <Windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace loader {

bool LaunchConfiguredTarget(
	const ProductDefinition& product,
	std::string& message
) {
	if (!product.Configured || product.TargetEnvironmentVariable.empty()) {
		message = "This product is not configured for launch.";
		return false;
	}

	const std::wstring variable(product.TargetEnvironmentVariable);
	wchar_t target[32768]{};

	SetLastError(ERROR_SUCCESS);
	const DWORD count = GetEnvironmentVariableW(
		variable.c_str(),
		target,
		static_cast<DWORD>(_countof(target))
	);

	if (count == 0) {
		message =
			"Authenticated. Configure " +
			std::string(product.Slug) +
			"'s launch target with its dedicated environment variable.";
		return false;
	}

	if (count >= _countof(target)) {
		message = "Configured target path exceeds the Windows path buffer.";
		return false;
	}

	const std::filesystem::path path(target);
	std::error_code fileError;
	const bool isFile = std::filesystem::is_regular_file(path, fileError);

	if (fileError) {
		message =
			"Configured target could not be accessed (error " +
			std::to_string(fileError.value()) +
			").";
		return false;
	}

	if (!isFile) {
		message = "Configured target executable was not found.";
		return false;
	}

	std::wstring command = L"\"" + path.wstring() + L"\"";
	std::vector<wchar_t> mutableCommand(command.begin(), command.end());
	mutableCommand.push_back(L'\0');

	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);

	PROCESS_INFORMATION process{};
	const std::filesystem::path workingDirectory = path.parent_path();
	const wchar_t* currentDirectory =
		workingDirectory.empty() ? nullptr : workingDirectory.c_str();

	if (!CreateProcessW(
		path.c_str(),
		mutableCommand.data(),
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		currentDirectory,
		&startup,
		&process
	)) {
		const DWORD error = GetLastError();
		message =
			"Failed to start configured target (Windows error " +
			std::to_string(error) +
			").";
		return false;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	message = std::string(product.DisplayName) + " target launched.";
	return true;
}

} // namespace loader
