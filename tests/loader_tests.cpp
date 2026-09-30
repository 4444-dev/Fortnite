#include "test_common.hpp"
#include "test_suites.hpp"

#include "../loader/launch_target.hpp"
#include "../loader/license_store.hpp"
#include "../loader/product_registry.hpp"

#include <string>
#include <string_view>

namespace tests {
namespace {

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

	return ok;
}

bool RunLicenseStoreTests() {
	bool ok = true;
	constexpr std::string_view slug = "luvkrimes-ci-test";
	const std::string value = "test-license-value";

	loader::license_store::Clear(slug);

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

	loader::license_store::Clear(slug);
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
