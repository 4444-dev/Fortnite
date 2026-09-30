#include "license_store.hpp"
#include "product_registry.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "Crypt32.lib")

namespace loader::license_store {
namespace {

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

std::string Entropy(std::string_view productSlug) {
	return
		"luvkrimes-license-entropy-v1:" +
		std::string(productSlug);
}

void ClearPlaintextBlob(DATA_BLOB& blob) noexcept {
	if (blob.pbData && blob.cbData > 0) {
		SecureZeroMemory(blob.pbData, blob.cbData);
	}

	if (blob.pbData) {
		LocalFree(blob.pbData);
	}

	blob = {};
}

} // namespace

bool Save(
	std::string_view productSlug,
	const std::string& license
) {
	if (
		!IsValidProductSlug(productSlug) ||
		license.empty() ||
		license.size() > kMaxLicenseLength
	) {
		return false;
	}

	DATA_BLOB input{};
	input.pbData = reinterpret_cast<BYTE*>(
		const_cast<char*>(license.data())
	);
	input.cbData = static_cast<DWORD>(license.size());

	std::string entropyText = Entropy(productSlug);
	DATA_BLOB entropy{};
	entropy.pbData = reinterpret_cast<BYTE*>(entropyText.data());
	entropy.cbData = static_cast<DWORD>(entropyText.size());

	const std::wstring description = Description(productSlug);

	DATA_BLOB output{};
	if (!CryptProtectData(
		&input,
		description.c_str(),
		&entropy,
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
	SecureZeroMemory(entropyText.data(), entropyText.size());
	return success;
}

bool Load(
	std::string_view productSlug,
	std::string& license
) {
	license.clear();

	if (!IsValidProductSlug(productSlug)) {
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
	file.close();

	DATA_BLOB input{};
	input.pbData = encrypted.data();
	input.cbData = static_cast<DWORD>(encrypted.size());

	std::string entropyText = Entropy(productSlug);
	DATA_BLOB entropy{};
	entropy.pbData = reinterpret_cast<BYTE*>(entropyText.data());
	entropy.cbData = static_cast<DWORD>(entropyText.size());

	DATA_BLOB output{};
	LPWSTR description = nullptr;
	bool legacyFormat = false;

	if (!CryptUnprotectData(
		&input,
		&description,
		&entropy,
		nullptr,
		nullptr,
		CRYPTPROTECT_UI_FORBIDDEN,
		&output
	)) {
		output = {};
		description = nullptr;

		if (!CryptUnprotectData(
			&input,
			&description,
			nullptr,
			nullptr,
			nullptr,
			CRYPTPROTECT_UI_FORBIDDEN,
			&output
		)) {
			SecureZeroMemory(entropyText.data(), entropyText.size());
			return false;
		}

		legacyFormat = true;
	}

	SecureZeroMemory(entropyText.data(), entropyText.size());

	const std::wstring expected = Description(productSlug);
	const bool descriptionMatches =
		description &&
		expected == description;

	if (description) {
		LocalFree(description);
	}

	if (!descriptionMatches) {
		ClearPlaintextBlob(output);
		return false;
	}

	if (output.cbData == 0 || output.cbData > kMaxLicenseLength) {
		ClearPlaintextBlob(output);
		return false;
	}

	license.assign(
		reinterpret_cast<const char*>(output.pbData),
		static_cast<std::size_t>(output.cbData)
	);
	ClearPlaintextBlob(output);

	if (license.empty()) {
		return false;
	}

	if (legacyFormat) {
		(void)Save(productSlug, license);
	}

	return true;
}

void Clear(std::string_view productSlug) {
	if (!IsValidProductSlug(productSlug)) {
		return;
	}

	std::error_code error;
	std::filesystem::remove(StoragePath(productSlug), error);
}

} // namespace loader::license_store
