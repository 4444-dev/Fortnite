#include <workspace/util/config/kv_config.hpp>

#include <cstdint>
using uptr = uintptr_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i32 = int32_t;
#include <workspace/game/unreal/structures.hpp>
#include "../loader/launch_target.hpp"
#include "../loader/license_store.hpp"
#include "../loader/product_registry.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

namespace {

bool Check(bool condition, const char* message) {
	if (!condition) {
		std::cerr << "FAILED: " << message << '\n';
		return false;
	}
	return true;
}

bool NearlyEqual(double a, double b, double epsilon = 1e-9) {
	return std::fabs(a - b) <= epsilon;
}

bool RunProductRegistryTests() {
	bool ok = true;
	ok &= Check(loader::RegistryIsValid(), "product registry invariants");
	const auto* fortnite = loader::FindProduct(loader::ProductId::Fortnite);
	const auto* apex = loader::FindProduct(loader::ProductId::ApexLegends);
	ok &= Check(fortnite == &loader::Fortnite, "find Fortnite product");
	ok &= Check(apex == &loader::ApexLegends, "find Apex product");
	ok &= Check(loader::Fortnite.Configured, "Fortnite configured");
	ok &= Check(!loader::ApexLegends.Configured, "Apex remains disabled until configured");
	ok &= Check(loader::Fortnite.Slug != loader::ApexLegends.Slug, "product slugs isolated");
	ok &= Check(loader::Fortnite.TargetEnvironmentVariable != loader::ApexLegends.TargetEnvironmentVariable,
		"product launch targets isolated");
	ok &= Check(loader::IsValidProductSlug("fortnite"), "accept valid product slug");
	ok &= Check(loader::IsValidProductSlug("apex_2-test"), "accept slug separators");
	ok &= Check(!loader::IsValidProductSlug(""), "reject empty product slug");
	ok &= Check(!loader::IsValidProductSlug("Fortnite"), "reject uppercase product slug");
	ok &= Check(!loader::IsValidProductSlug("../fortnite"), "reject path-like product slug");
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
			std::string(loader::license_store::kMaxLicenseLength + 1, 'x')
		),
		"reject oversized remembered license"
	);

	ok &= Check(
		loader::license_store::Save(slug, value),
		"save remembered license"
	);

	std::string loaded;
	ok &= Check(
		loader::license_store::Load(slug, loaded) && loaded == value,
		"remembered license roundtrip"
	);

	loader::license_store::Clear(slug);
	loaded = "sentinel";
	ok &= Check(
		!loader::license_store::Load(slug, loaded) && loaded.empty(),
		"clear remembered license"
	);

	return ok;
}

bool RunLaunchContractTests() {
	bool ok = true;
	std::string message;
	ok &= Check(
		!loader::LaunchConfiguredTarget(loader::ApexLegends, message),
		"reject launch for unconfigured product"
	);
	ok &= Check(!message.empty(), "launch rejection includes diagnostic");
	return ok;
}

bool RunMathTests() {
	bool ok = true;

	FMatrix identity;
	for (int i = 0; i < 4; ++i) identity.M[i][i] = 1.0;

	FVector2D screen {};
	ok &= Check(identity.WorldToScreen({0.0, 0.0, 0.0}, 1920.0, 1080.0, screen) &&
		NearlyEqual(screen.X, 960.0) && NearlyEqual(screen.Y, 540.0), "identity projection center");
	ok &= Check(identity.WorldToScreen({1.0, 1.0, 0.0}, 1920.0, 1080.0, screen) &&
		NearlyEqual(screen.X, 1920.0) && NearlyEqual(screen.Y, 0.0), "identity projection upper-right");
	ok &= Check(!identity.WorldToScreen({0.0, 0.0, 0.0}, 0.0, 1080.0, screen), "reject zero-width viewport");
	ok &= Check(!identity.WorldToScreen({0.0, 0.0, 0.0}, 1920.0, 0.0, screen), "reject zero-height viewport");

	FMatrix nonFinite = identity;
	nonFinite.M41 = std::numeric_limits<double>::infinity();
	ok &= Check(!nonFinite.WorldToScreen({0.0, 0.0, 0.0}, 1920.0, 1080.0, screen),
		"reject infinite projected coordinate");

	nonFinite = identity;
	nonFinite.M44 = std::numeric_limits<double>::quiet_NaN();
	ok &= Check(!nonFinite.WorldToScreen({0.0, 0.0, 0.0}, 1920.0, 1080.0, screen),
		"reject non-finite projection W");

	FMatrix invalidW = identity;
	invalidW.M14 = std::numeric_limits<double>::quiet_NaN();
	ok &= Check(!invalidW.WorldToScreen({1.0, 0.0, 0.0}, 1920.0, 1080.0, screen),
		"reject non-finite clip W");

	FMatrix invalidX = identity;
	invalidX.M11 = std::numeric_limits<double>::infinity();
	ok &= Check(!invalidX.WorldToScreen({1.0, 0.0, 0.0}, 1920.0, 1080.0, screen),
		"reject non-finite projected coordinate");

	FMatrix behind = identity;
	behind.M44 = -1.0;
	ok &= Check(!behind.WorldToScreen({0.0, 0.0, 0.0}, 1920.0, 1080.0, screen), "reject point behind camera");

	FTransform transform;
	transform.Translation = {10.0, 20.0, 30.0};
	transform.Scale3D = {2.0, 3.0, 4.0};
	const FVector transformed = transform.TransformPosition({1.0, 1.0, 1.0});
	ok &= Check(NearlyEqual(transformed.X, 12.0) && NearlyEqual(transformed.Y, 23.0) &&
		NearlyEqual(transformed.Z, 34.0), "transform scale and translation");

	const double halfSqrtTwo = std::sqrt(0.5);
	FTransform rotated;
	rotated.Rotation = FQuat(0.0, 0.0, halfSqrtTwo, halfSqrtTwo);
	const FVector rotatedVector = rotated.TransformPosition({1.0, 0.0, 0.0});
	ok &= Check(NearlyEqual(rotatedVector.X, 0.0, 1e-8) &&
		NearlyEqual(rotatedVector.Y, 1.0, 1e-8) &&
		NearlyEqual(rotatedVector.Z, 0.0, 1e-8), "quaternion rotates 90 degrees around Z");

	const FMatrix rotatedMatrix = rotated.ToMatrixWithScale();
	const FVector matrixRotated = rotatedMatrix.TransformPosition({1.0, 0.0, 0.0});
	ok &= Check(NearlyEqual(matrixRotated.X, rotatedVector.X, 1e-8) &&
		NearlyEqual(matrixRotated.Y, rotatedVector.Y, 1e-8) &&
		NearlyEqual(matrixRotated.Z, rotatedVector.Z, 1e-8),
		"rotation matrix agrees with quaternion transform");

	const FMatrix transformMatrix = transform.ToMatrixWithScale();
	const FVector matrixTransformed = transformMatrix.TransformPosition({1.0, 1.0, 1.0});
	ok &= Check(NearlyEqual(matrixTransformed.X, transformed.X) &&
		NearlyEqual(matrixTransformed.Y, transformed.Y) && NearlyEqual(matrixTransformed.Z, transformed.Z),
		"transform matrix agrees with direct transform");

	return ok;
}

} // namespace

int main() {
	if (!RunProductRegistryTests()) return 1;
	if (!RunLicenseStoreTests()) return 1;
	if (!RunLaunchContractTests()) return 1;
	if (!RunMathTests()) return 1;

	const auto path =
		std::filesystem::temp_directory_path() /
		"luvkrimes-config-tests.ini";

	util::KeyValueConfig output;
	output.SetBool("enabled", true);
	output.SetInt("count", 42);
	output.SetFloat("scale", 1.25f);
	output.SetString("name", "test value");

	if (!Check(output.Save(path), "save config")) {
		return 1;
	}

	util::KeyValueConfig input;
	if (!Check(input.Load(path), "load config")) {
		return 1;
	}

	bool enabled = false;
	int count = 0;
	float scale = 0.0f;

	if (!Check(input.TryGetBool("enabled", enabled) && enabled, "bool roundtrip")) {
		return 1;
	}
	if (!Check(input.TryGetInt("count", count) && count == 42, "int roundtrip")) {
		return 1;
	}
	if (!Check(
		input.TryGetFloat("scale", scale) &&
		std::fabs(scale - 1.25f) < 0.0001f,
		"float roundtrip"
	)) {
		return 1;
	}

	const auto name = input.GetString("name");
	if (!Check(name && *name == "test value", "string roundtrip")) {
		return 1;
	}

	output.SetInt("count", 84);
	if (!Check(output.Save(path), "replace existing config")) {
		return 1;
	}

	util::KeyValueConfig replaced;
	int replacedCount = 0;
	if (!Check(
		replaced.Load(path) &&
		replaced.TryGetInt("count", replacedCount) &&
		replacedCount == 84,
		"atomic replacement roundtrip"
	)) {
		return 1;
	}

	std::filesystem::path temporary = path;
	temporary += ".tmp";
	if (!Check(!std::filesystem::exists(temporary), "temporary file cleanup")) {
		return 1;
	}

	{
		std::ofstream manual(path, std::ios::trunc);
		manual
			<< "# comment\n"
			<< " valid = 7 \n"
			<< "flag=false\n"
			<< "broken-line\n"
			<< "invalid_int=nope\n"
			<< "invalid_float=nan\n"
			<< "trailing_float=1.25oops\n"
			<< "invalid_bool=yes\n";
	}

	util::KeyValueConfig parsed;
	if (!Check(parsed.Load(path), "load hand-written config")) {
		return 1;
	}

	int valid = 0;
	int invalid = 123;
	int duplicate = 0;
	float invalidFloat = 9.0f;
	bool flag = true;
	bool invalidBool = false;
	float invalidFloat = 0.0f;
	float trailingFloat = 0.0f;

	if (!Check(parsed.TryGetInt("valid", valid) && valid == 7, "trim whitespace")) {
		return 1;
	}
	if (!Check(parsed.TryGetBool("flag", flag) && !flag, "parse false")) {
		return 1;
	}
	if (!Check(!parsed.TryGetInt("invalid_int", invalid), "reject invalid integer")) {
		return 1;
	}
	if (!Check(!parsed.TryGetFloat("invalid_float", invalidFloat), "reject non-finite float")) {
		return 1;
	}
	if (!Check(!parsed.TryGetFloat("trailing_float", trailingFloat), "reject float with trailing data")) {
		return 1;
	}
	if (!Check(!parsed.TryGetBool("invalid_bool", invalidBool), "reject invalid boolean")) {
		return 1;
	}
	if (!Check(!parsed.GetString("broken-line"), "ignore malformed line")) {
		return 1;
	}

	std::error_code error;
	std::filesystem::remove(path, error);

	std::cout << "All tests passed.\n";
	return 0;
}
