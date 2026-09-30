#include <workspace/util/config/kv_config.hpp>

#include <cstdint>
using uptr = uintptr_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i32 = int32_t;
#include <workspace/game/unreal/structures.hpp>
#include "../loader/product_registry.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

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
	ok &= Check(loader::FindProduct(loader::ProductId::Fortnite) == &loader::Fortnite, "find Fortnite product");
	ok &= Check(loader::FindProduct(loader::ProductId::ApexLegends) == &loader::ApexLegends, "find Apex product");
	ok &= Check(loader::Fortnite.Configured, "Fortnite configured");
	ok &= Check(!loader::ApexLegends.Configured, "Apex remains disabled until configured");
	ok &= Check(loader::Fortnite.Slug != loader::ApexLegends.Slug, "product slugs isolated");
	ok &= Check(loader::Fortnite.TargetEnvironmentVariable != loader::ApexLegends.TargetEnvironmentVariable,
		"product launch targets isolated");
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

	FMatrix behind = identity;
	behind.M44 = -1.0;
	ok &= Check(!behind.WorldToScreen({0.0, 0.0, 0.0}, 1920.0, 1080.0, screen), "reject point behind camera");

	FTransform transform;
	transform.Translation = {10.0, 20.0, 30.0};
	transform.Scale3D = {2.0, 3.0, 4.0};
	const FVector transformed = transform.TransformPosition({1.0, 1.0, 1.0});
	ok &= Check(NearlyEqual(transformed.X, 12.0) && NearlyEqual(transformed.Y, 23.0) &&
		NearlyEqual(transformed.Z, 34.0), "transform scale and translation");

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
			<< "invalid_int=nope\n";
	}

	util::KeyValueConfig parsed;
	if (!Check(parsed.Load(path), "load hand-written config")) {
		return 1;
	}

	int valid = 0;
	int invalid = 123;
	bool flag = true;

	if (!Check(parsed.TryGetInt("valid", valid) && valid == 7, "trim whitespace")) {
		return 1;
	}
	if (!Check(parsed.TryGetBool("flag", flag) && !flag, "parse false")) {
		return 1;
	}
	if (!Check(!parsed.TryGetInt("invalid_int", invalid), "reject invalid integer")) {
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
