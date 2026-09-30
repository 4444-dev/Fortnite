#include "license_store.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "Crypt32.lib")

namespace loader::license_store {
namespace {

constexpr std::size_t kMaxLicenseLength = 4096;

bool ValidSlug(std::string_view slug) {
	if (slug.empty() || slug.size() > 64) {
		return false;
	}

	for (const char ch : slug) {
		const bool valid =
			(ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9') ||
			ch == '-' ||
			ch == '_';

		if (!valid) {
			return false;
		}
	}

	return true;
}

std::filesystem::path RootPath() {
	wchar_t buffer[32768]{};
	const DWORD count = GetEnvironmentVariableW(
		L"LOCALAPPDATA",
		buffer,
		static_cast<DWORD>(_countof(buffer))
	);

	std::filesystem::path root = L".";
	if (count > 0 && count < _countof(buffer)) {
		root = buffer;
	}

	return root / L"luvkrimes" / L"licenses";
}

std::filesystem::path StoragePath(std::string_view productSlug) {
	return RootPath() /
		std::filesystem::path(std::string(productSlug) + ".dat");
}

std::wstring Description(std::string_view productSlug) {
	const std::string text =
		"luvkrimes-license-" + std::string(productSlug);

	return std::wstring(text.begin(), text.end());
}

} // namespace

bool Save(
	std::string_view productSlug,
	const std::string& license
) {
	if (!ValidSlug(productSlug) || license.empty() || license.size() > kMaxLicenseLength) {
		return false;
	}

	DATA_BLOB input{};
	input.pbData = reinterpret_cast<BYTE*>(
		const_cast<char*>(license.data())
	);
	input.cbData = static_cast<DWORD>(license.size());

	const std::wstring description = Description(productSlug);

	DATA_BLOB output{};
	if (!CryptProtectData(
		&input,
		description.c_str(),
		nullptr,
		nullptr,
		nullptr,
		CRYPTPROTECT_UI_FORBIDDEN,
		&output
	)) {
		return false;
	}

	const auto path = StoragePath(productSlug);
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);

	bool success = false;
	if (!error) {
		std::filesystem::path temporary = path;
		temporary += L".tmp";

		{
			std::ofstream file(
				temporary,
				std::ios::binary | std::ios::trunc
			);

			if (file) {
				file.write(
					reinterpret_cast<const char*>(output.pbData),
					static_cast<std::streamsize>(output.cbData)
				);
				file.flush();
				success = file.good();
			}
		}

		if (success) {
			success = MoveFileExW(
				temporary.c_str(),
				path.c_str(),
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
			) != FALSE;
		}

		if (!success) {
			std::filesystem::remove(temporary, error);
		}
	}

	LocalFree(output.pbData);
	return success;
}

bool Load(
	std::string_view productSlug,
	std::string& license
) {
	license.clear();

	if (!ValidSlug(productSlug)) {
		return false;
	}

	const auto path = StoragePath(productSlug);
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		return false;
	}

	const auto size = file.tellg();
	if (size <= 0 || size > 64 * 1024) {
		return false;
	}

	std::vector<BYTE> encrypted(static_cast<std::size_t>(size));
	file.seekg(0, std::ios::beg);
	file.read(
		reinterpret_cast<char*>(encrypted.data()),
		static_cast<std::streamsize>(encrypted.size())
	);
	if (!file) {
		return false;
	}

	DATA_BLOB input{};
	input.pbData = encrypted.data();
	input.cbData = static_cast<DWORD>(encrypted.size());

	DATA_BLOB output{};
	LPWSTR description = nullptr;

	if (!CryptUnprotectData(
		&input,
		&description,
		nullptr,
		nullptr,
		nullptr,
		CRYPTPROTECT_UI_FORBIDDEN,
		&output
	)) {
		return false;
	}

	const std::wstring expected = Description(productSlug);
	const bool descriptionMatches =
		description &&
		expected == description;

	if (description) {
		LocalFree(description);
	}

	if (!descriptionMatches) {
		LocalFree(output.pbData);
		return false;
	}

	if (output.cbData == 0 || output.cbData > kMaxLicenseLength) {
		LocalFree(output.pbData);
		return false;
	}

	license.assign(
		reinterpret_cast<const char*>(output.pbData),
		static_cast<std::size_t>(output.cbData)
	);
	LocalFree(output.pbData);

	return !license.empty();
}

void Clear(std::string_view productSlug) {
	if (!ValidSlug(productSlug)) {
		return;
	}

	std::error_code error;
	std::filesystem::remove(StoragePath(productSlug), error);
}

} // namespace loader::license_store
