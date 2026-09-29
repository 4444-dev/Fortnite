#include "license_store.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <filesystem>
#include <fstream>
#include <vector>

#pragma comment(lib, "Crypt32.lib")

namespace loader::license_store {
namespace {

std::filesystem::path StoragePath() {
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

	return root / L"luvkrimes" / L"license.dat";
}

} // namespace

bool Save(const std::string& license) {
	if (license.empty()) {
		return false;
	}

	DATA_BLOB input{};
	input.pbData = reinterpret_cast<BYTE*>(
		const_cast<char*>(license.data())
	);
	input.cbData = static_cast<DWORD>(license.size());

	DATA_BLOB output{};
	if (!CryptProtectData(
		&input,
		L"luvkrimes-license",
		nullptr,
		nullptr,
		nullptr,
		CRYPTPROTECT_UI_FORBIDDEN,
		&output
	)) {
		return false;
	}

	const auto path = StoragePath();
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);

	bool success = false;
	if (!error) {
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (file) {
			file.write(
				reinterpret_cast<const char*>(output.pbData),
				static_cast<std::streamsize>(output.cbData)
			);
			success = file.good();
		}
	}

	LocalFree(output.pbData);
	return success;
}

bool Load(std::string& license) {
	const auto path = StoragePath();
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
	if (!CryptUnprotectData(
		&input,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		CRYPTPROTECT_UI_FORBIDDEN,
		&output
	)) {
		return false;
	}

	license.assign(
		reinterpret_cast<const char*>(output.pbData),
		static_cast<std::size_t>(output.cbData)
	);
	LocalFree(output.pbData);

	return !license.empty();
}

void Clear() {
	std::error_code error;
	std::filesystem::remove(StoragePath(), error);
}

} // namespace loader::license_store
