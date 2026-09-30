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

std::filesystem::path LocalAppDataPath() {
	wchar_t buffer[32768]{};
	const DWORD count = GetEnvironmentVariableW(
		L"LOCALAPPDATA",
		buffer,
		static_cast<DWORD>(_countof(buffer))
	);

	if (count > 0 && count < _countof(buffer)) {
		return std::filesystem::path(buffer);
	}

	return L".";
}

std::filesystem::path RootPath() {
	return LocalAppDataPath() / L"Nexus" / L"licenses";
}

std::filesystem::path LegacyRootPath() {
	return LocalAppDataPath() / L"luvkrimes" / L"licenses";
}

std::filesystem::path StoragePath(
	const std::filesystem::path& root,
	std::string_view productSlug
) {
	return root /
		std::filesystem::path(std::string(productSlug) + ".dat");
}

std::filesystem::path StoragePath(std::string_view productSlug) {
	return StoragePath(RootPath(), productSlug);
}

std::filesystem::path LegacyStoragePath(std::string_view productSlug) {
	return StoragePath(LegacyRootPath(), productSlug);
}

std::wstring Description(
	std::string_view brand,
	std::string_view productSlug
) {
	const std::string text =
		std::string(brand) +
		"-license-" +
		std::string(productSlug);

	return std::wstring(text.begin(), text.end());
}

std::string Entropy(
	std::string_view brand,
	std::string_view productSlug
) {
	return
		std::string(brand) +
		"-license-entropy-v1:" +
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

bool TryUnprotect(
	DATA_BLOB& input,
	std::string_view productSlug,
	std::string_view brand,
	bool useEntropy,
	DATA_BLOB& output
) {
	std::string entropyText;
	DATA_BLOB entropy{};
	DATA_BLOB* entropyPointer = nullptr;

	if (useEntropy) {
		entropyText = Entropy(brand, productSlug);
		entropy.pbData =
			reinterpret_cast<BYTE*>(entropyText.data());
		entropy.cbData =
			static_cast<DWORD>(entropyText.size());
		entropyPointer = &entropy;
	}

	LPWSTR description = nullptr;
	const BOOL success = CryptUnprotectData(
		&input,
		&description,
		entropyPointer,
		nullptr,
		nullptr,
		CRYPTPROTECT_UI_FORBIDDEN,
		&output
	);

	if (!entropyText.empty()) {
		SecureZeroMemory(
			entropyText.data(),
			entropyText.size()
		);
	}

	if (!success) {
		if (description) {
			LocalFree(description);
		}
		output = {};
		return false;
	}

	const std::wstring expected =
		Description(brand, productSlug);
	const bool matches =
		description &&
		expected == description;

	if (description) {
		LocalFree(description);
	}

	if (!matches) {
		ClearPlaintextBlob(output);
		return false;
	}

	return true;
}

void RemoveStorageArtifacts(const std::filesystem::path& path) {
	std::error_code error;
	std::filesystem::remove(path, error);

	std::filesystem::path temporary = path;
	temporary += L".tmp";
	error.clear();
	std::filesystem::remove(temporary, error);
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

	std::string entropyText =
		Entropy("nexus", productSlug);
	DATA_BLOB entropy{};
	entropy.pbData =
		reinterpret_cast<BYTE*>(entropyText.data());
	entropy.cbData =
		static_cast<DWORD>(entropyText.size());

	const std::wstring description =
		Description("nexus", productSlug);

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
		SecureZeroMemory(
			entropyText.data(),
			entropyText.size()
		);
		return false;
	}

	const auto path = StoragePath(productSlug);
	std::error_code error;
	std::filesystem::create_directories(
		path.parent_path(),
		error
	);

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
					reinterpret_cast<const char*>(
						output.pbData
					),
					static_cast<std::streamsize>(
						output.cbData
					)
				);
				file.flush();
				success = file.good();
			}
		}

		if (success) {
			success = MoveFileExW(
				temporary.c_str(),
				path.c_str(),
				MOVEFILE_REPLACE_EXISTING |
					MOVEFILE_WRITE_THROUGH
			) != FALSE;
		}

		if (!success) {
			std::filesystem::remove(
				temporary,
				error
			);
		}
	}

	LocalFree(output.pbData);
	SecureZeroMemory(
		entropyText.data(),
		entropyText.size()
	);
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

	auto path = StoragePath(productSlug);
	bool loadedFromLegacyPath = false;

	std::ifstream file(
		path,
		std::ios::binary | std::ios::ate
	);

	if (!file) {
		path = LegacyStoragePath(productSlug);
		file.open(
			path,
			std::ios::binary | std::ios::ate
		);
		loadedFromLegacyPath = file.good();
	}

	if (!file) {
		return false;
	}

	const auto size = file.tellg();
	if (size <= 0 || size > 64 * 1024) {
		return false;
	}

	std::vector<BYTE> encrypted(
		static_cast<std::size_t>(size)
	);
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
	input.cbData =
		static_cast<DWORD>(encrypted.size());

	DATA_BLOB output{};
	bool legacyEncryption = false;

	if (!TryUnprotect(
		input,
		productSlug,
		"nexus",
		true,
		output
	)) {
		if (!TryUnprotect(
			input,
			productSlug,
			"luvkrimes",
			true,
			output
		)) {
			if (!TryUnprotect(
				input,
				productSlug,
				"luvkrimes",
				false,
				output
			)) {
				return false;
			}
		}

		legacyEncryption = true;
	}

	if (
		output.cbData == 0 ||
		output.cbData > kMaxLicenseLength
	) {
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

	if (legacyEncryption || loadedFromLegacyPath) {
		if (Save(productSlug, license)) {
			RemoveStorageArtifacts(
				LegacyStoragePath(productSlug)
			);
		}
	}

	return true;
}

void Clear(std::string_view productSlug) {
	if (!IsValidProductSlug(productSlug)) {
		return;
	}

	RemoveStorageArtifacts(
		StoragePath(productSlug)
	);
	RemoveStorageArtifacts(
		LegacyStoragePath(productSlug)
	);
}

} // namespace loader::license_store
