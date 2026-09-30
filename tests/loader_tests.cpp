#include "test_common.hpp"
#include "test_suites.hpp"

#include "../loader/launch_target.hpp"
#include "../loader/license_store.hpp"
#include "../loader/product_registry.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace tests {
namespace {

class ScopedLocalAppData final {
public:
	ScopedLocalAppData() {
		wchar_t buffer[32768]{};
		const DWORD count = GetEnvironmentVariableW(
			L"LOCALAPPDATA",
			buffer,
			static_cast<DWORD>(_countof(buffer))
		);

		if (count > 0 && count < _countof(buffer)) {
			m_HadOriginal = true;
			m_Original.assign(buffer, count);
		}

		m_Root =
			std::filesystem::temp_directory_path() /
			(
				L"nexus-loader-tests-" +
				std::to_wstring(GetCurrentProcessId())
			);

		std::error_code error;
		std::filesystem::remove_all(m_Root, error);
		error.clear();
		std::filesystem::create_directories(m_Root, error);

		m_Ready =
			!error &&
			SetEnvironmentVariableW(
				L"LOCALAPPDATA",
				m_Root.c_str()
			) != FALSE;
	}

	~ScopedLocalAppData() {
		if (m_HadOriginal) {
			(void)SetEnvironmentVariableW(
				L"LOCALAPPDATA",
				m_Original.c_str()
			);
		} else {
			(void)SetEnvironmentVariableW(
				L"LOCALAPPDATA",
				nullptr
			);
		}

		std::error_code error;
		std::filesystem::remove_all(m_Root, error);
	}

	ScopedLocalAppData(const ScopedLocalAppData&) = delete;
	ScopedLocalAppData& operator=(const ScopedLocalAppData&) = delete;

	[[nodiscard]] bool Ready() const noexcept {
		return m_Ready;
	}

	[[nodiscard]] std::filesystem::path LicensePath(
		std::string_view slug
	) const {
		return
			m_Root /
			L"Nexus" /
			L"licenses" /
			std::filesystem::path(std::string(slug) + ".dat");
	}
	[[nodiscard]] std::filesystem::path LegacyLicensePath(
		std::string_view slug
	) const {
		return
			m_Root /
			L"luvkrimes" /
			L"licenses" /
			std::filesystem::path(std::string(slug) + ".dat");
	}

private:
	std::filesystem::path m_Root;
	std::wstring m_Original;
	bool m_HadOriginal = false;
	bool m_Ready = false;
};

bool WriteLegacyLicenseBlob(
	const std::filesystem::path& path,
	std::string_view slug,
	const std::string& value
) {
	std::error_code error;
	std::filesystem::create_directories(
		path.parent_path(),
		error
	);
	if (error) {
		return false;
	}

	DATA_BLOB input{};
	input.pbData = reinterpret_cast<BYTE*>(
		const_cast<char*>(value.data())
	);
	input.cbData = static_cast<DWORD>(value.size());

	std::string entropyText =
		"luvkrimes-license-entropy-v1:" +
		std::string(slug);
	DATA_BLOB entropy{};
	entropy.pbData =
		reinterpret_cast<BYTE*>(entropyText.data());
	entropy.cbData =
		static_cast<DWORD>(entropyText.size());

	const std::string descriptionText =
		"luvkrimes-license-" + std::string(slug);
	const std::wstring description(
		descriptionText.begin(),
		descriptionText.end()
	);

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

	std::ofstream file(
		path,
		std::ios::binary | std::ios::trunc
	);
	if (file) {
		file.write(
			reinterpret_cast<const char*>(output.pbData),
			static_cast<std::streamsize>(output.cbData)
		);
		file.flush();
	}

	const bool success = file.good();
	LocalFree(output.pbData);
	return success;
}

bool RunProductRegistryTests() {
	bool ok = true;

	ok &= Check(loader::RegistryIsValid(), "product registry invariants");

	const auto* fortnite =
		loader::FindProduct(loader::ProductId::Fortnite);
	const auto* apex =
		loader::FindProduct(loader::ProductId::ApexLegends);

	ok &= Check(
		fortnite == &loader::Fortnite,
		"find Fortnite product"
	);
	ok &= Check(
		apex == &loader::ApexLegends,
		"find Apex product"
	);
	ok &= Check(
		loader::Fortnite.Configured,
		"Fortnite configured"
	);
	ok &= Check(
		loader::Fortnite.KeyAuthUrl.starts_with("https://"),
		"configured KeyAuth endpoint uses HTTPS"
	);
	ok &= Check(
		loader::Fortnite.PackagedTargetRelativePath ==
			L"projects\\fortnite\\Nexus-Fortnite.exe",
		"Fortnite packaged target path"
	);
	ok &= Check(
		loader::ApexLegends.PackagedTargetRelativePath.empty(),
		"Apex has no packaged target until configured"
	);
	ok &= Check(
		!loader::ApexLegends.Configured,
		"Apex remains disabled until configured"
	);
	ok &= Check(
		loader::Fortnite.Slug != loader::ApexLegends.Slug,
		"product slugs isolated"
	);
	ok &= Check(
		loader::Fortnite.TargetEnvironmentVariable !=
			loader::ApexLegends.TargetEnvironmentVariable,
		"product launch targets isolated"
	);
	ok &= Check(
		loader::Fortnite.TargetEnvironmentVariable ==
			L"NEXUS_TARGET_FORTNITE",
		"Nexus Fortnite launch override"
	);
	ok &= Check(
		loader::Fortnite.LegacyTargetEnvironmentVariable ==
			L"LUVKRIMES_TARGET_FORTNITE",
		"legacy Fortnite launch override retained"
	);
	ok &= Check(
		loader::ApexLegends.TargetEnvironmentVariable ==
			L"NEXUS_TARGET_APEX",
		"Nexus Apex launch override"
	);
	ok &= Check(
		loader::IsValidProductSlug("fortnite"),
		"accept valid product slug"
	);
	ok &= Check(
		loader::IsValidProductSlug("apex_2-test"),
		"accept slug separators"
	);
	ok &= Check(
		!loader::IsValidProductSlug(""),
		"reject empty product slug"
	);
	ok &= Check(
		!loader::IsValidProductSlug("Fortnite"),
		"reject uppercase product slug"
	);
	ok &= Check(
		!loader::IsValidProductSlug("../fortnite"),
		"reject path-like product slug"
	);
	ok &= Check(
		loader::IsValidProductSlug(std::string(64, 'a')),
		"accept maximum-length product slug"
	);
	ok &= Check(
		!loader::IsValidProductSlug(std::string(65, 'a')),
		"reject oversized product slug"
	);

	return ok;
}

bool RunLicenseStoreTests() {
	bool ok = true;
	ScopedLocalAppData localAppData;
	ok &= Check(
		localAppData.Ready(),
		"isolate remembered-license test storage"
	);
	if (!localAppData.Ready()) {
		return false;
	}

	constexpr std::string_view slug = "nexus-ci-test";
	constexpr std::string_view otherSlug = "nexus-ci-other";
	const std::string value = "test-license-value";

	loader::license_store::Clear(slug);
	loader::license_store::Clear(otherSlug);
	constexpr std::string_view legacySlug =
		"nexus-legacy-migration";
	const std::string legacyValue =
		"legacy-license-value";

	loader::license_store::Clear(legacySlug);

	ok &= Check(
		WriteLegacyLicenseBlob(
			localAppData.LegacyLicensePath(legacySlug),
			legacySlug,
			legacyValue
		),
		"create legacy remembered-license blob"
	);

	std::string migratedLegacy;
	ok &= Check(
		loader::license_store::Load(
			legacySlug,
			migratedLegacy
		) &&
			migratedLegacy == legacyValue,
		"load legacy remembered-license blob"
	);
	ok &= Check(
		std::filesystem::exists(
			localAppData.LicensePath(legacySlug)
		) &&
			!std::filesystem::exists(
				localAppData.LegacyLicensePath(legacySlug)
			),
		"migrate legacy remembered-license storage to Nexus"
	);

	loader::license_store::Clear(legacySlug);

	ok &= Check(
		!loader::license_store::Save("../invalid", value),
		"reject unsafe license-store slug"
	);
	ok &= Check(
		!loader::license_store::Save(slug, ""),
		"reject empty remembered license"
	);
	ok &= Check(
		!loader::license_store::Save(
			slug,
			std::string(
				loader::license_store::kMaxLicenseLength + 1,
				'x'
			)
		),
		"reject oversized remembered license"
	);
	ok &= Check(
		loader::license_store::Save(slug, value),
		"save remembered license"
	);

	std::string loaded;
	ok &= Check(
		loader::license_store::Load(slug, loaded) &&
			loaded == value,
		"remembered license roundtrip"
	);

	std::error_code copyError;
	std::filesystem::copy_file(
		localAppData.LicensePath(slug),
		localAppData.LicensePath(otherSlug),
		std::filesystem::copy_options::overwrite_existing,
		copyError
	);
	ok &= Check(
		!copyError,
		"copy encrypted license blob for isolation test"
	);

	std::string crossProduct;
	ok &= Check(
		!loader::license_store::Load(otherSlug, crossProduct) &&
			crossProduct.empty(),
		"reject remembered license copied across product slugs"
	);

	{
		std::fstream encrypted(
			localAppData.LicensePath(slug),
			std::ios::binary | std::ios::in | std::ios::out
		);

		char first = 0;
		if (encrypted.read(&first, 1)) {
			first = static_cast<char>(
				static_cast<unsigned char>(first) ^ 0x5A
			);
			encrypted.seekp(0, std::ios::beg);
			encrypted.write(&first, 1);
			encrypted.flush();
		}

		ok &= Check(
			encrypted.good(),
			"corrupt encrypted license blob for integrity test"
		);
	}

	std::string corrupted;
	ok &= Check(
		!loader::license_store::Load(slug, corrupted) &&
			corrupted.empty(),
		"reject corrupted remembered license blob"
	);

	const std::string maximumLicense(
		loader::license_store::kMaxLicenseLength,
		'm'
	);
	ok &= Check(
		loader::license_store::Save(slug, maximumLicense),
		"accept maximum-length remembered license"
	);

	loaded.clear();
	ok &= Check(
		loader::license_store::Load(slug, loaded) &&
			loaded == maximumLicense,
		"maximum-length remembered license roundtrip"
	);

	std::filesystem::path staleTemporary =
		localAppData.LicensePath(slug);
	staleTemporary += L".tmp";

	{
		std::ofstream stale(
			staleTemporary,
			std::ios::binary | std::ios::trunc
		);
		stale << "stale";
	}

	loader::license_store::Clear(slug);
	loader::license_store::Clear(otherSlug);
	loaded = "sentinel";

	ok &= Check(
		!std::filesystem::exists(localAppData.LicensePath(slug)) &&
			!std::filesystem::exists(staleTemporary),
		"clear remembered license artifacts"
	);

	ok &= Check(
		!loader::license_store::Load(slug, loaded) &&
			loaded.empty(),
		"clear remembered license"
	);

	return ok;
}

bool RunLaunchContractTests() {
	std::string message;

	const bool rejected =
		!loader::LaunchConfiguredTarget(
			loader::ApexLegends,
			message
		);

	return
		Check(
			rejected,
			"reject launch for unconfigured product"
		) &&
		Check(
			!message.empty(),
			"launch rejection includes diagnostic"
		);
}

} // namespace

bool RunLoaderTests() {
	return
		RunProductRegistryTests() &&
		RunLicenseStoreTests() &&
		RunLaunchContractTests();
}

} // namespace tests
