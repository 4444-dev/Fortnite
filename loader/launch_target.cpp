#include "launch_target.hpp"

#include <Windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace loader {
namespace {

std::string WideToUtf8(const wchar_t* text) {
	if (!text || !*text) {
		return {};
	}

	const int required = WideCharToMultiByte(
		CP_UTF8,
		0,
		text,
		-1,
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (required <= 1) {
		return {};
	}

	std::string output(
		static_cast<std::size_t>(required),
		'\0'
	);

	const int converted = WideCharToMultiByte(
		CP_UTF8,
		0,
		text,
		-1,
		output.data(),
		required,
		nullptr,
		nullptr
	);

	if (converted <= 1) {
		return {};
	}

	output.resize(static_cast<std::size_t>(converted - 1));

	while (
		!output.empty() &&
		(output.back() == '\r' || output.back() == '\n' || output.back() == ' ')
	) {
		output.pop_back();
	}

	return output;
}

std::string WindowsErrorText(DWORD error) {
	wchar_t* buffer = nullptr;

	const DWORD length = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		error,
		0,
		reinterpret_cast<wchar_t*>(&buffer),
		0,
		nullptr
	);

	std::string text;
	if (length > 0 && buffer) {
		text = WideToUtf8(buffer);
	}

	if (buffer) {
		LocalFree(buffer);
	}

	if (text.empty()) {
		return "Windows error " + std::to_string(error);
	}

	return text + " (Windows error " + std::to_string(error) + ")";
}

} // namespace

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
			"Failed to start configured target: " +
			WindowsErrorText(error) +
			".";
		return false;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	message = std::string(product.DisplayName) + " target launched.";
	return true;
}

} // namespace loader
