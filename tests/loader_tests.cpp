#include "test_common.hpp"
#include "test_suites.hpp"

#include "../loader/launch_target.hpp"
#include "../loader/license_store.hpp"
#include "../loader/product_registry.hpp"

#include <Windows.h>

#include <filesystem>
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
				L"luvkrimes-loader-tests-" +
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

	constexpr std::string_view slug = "luvkrimes-ci-test";
	constexpr std::string_view otherSlug = "luvkrimes-ci-other";
	const std::string value = "test-license-value";

	loader::license_store::Clear(slug);
	loader::license_store::Clear(otherSlug);

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

	loader::license_store::Clear(slug);
	loader::license_store::Clear(otherSlug);
	loaded = "sentinel";

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
