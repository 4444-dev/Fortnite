#include "test_common.hpp"
#include "test_suites.hpp"

#include <workspace/util/config/kv_config.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace tests {

bool RunConfigTests() {
	const auto path =
		std::filesystem::temp_directory_path() /
		"luvkrimes-config-tests.ini";

	util::KeyValueConfig output;
	output.SetBool("enabled", true);
	output.SetInt("count", 42);
	output.SetFloat("scale", 1.25f);
	output.SetString("name", "test value");

	if (!Check(output.Save(path), "save config")) {
		return false;
	}

	util::KeyValueConfig input;
	if (!Check(input.Load(path), "load config")) {
		return false;
	}

	bool enabled = false;
	int count = 0;
	float scale = 0.0f;

	if (!Check(
		input.TryGetBool("enabled", enabled) && enabled,
		"bool roundtrip"
	)) {
		return false;
	}

	if (!Check(
		input.TryGetInt("count", count) && count == 42,
		"int roundtrip"
	)) {
		return false;
	}

	if (!Check(
		input.TryGetFloat("scale", scale) &&
			NearlyEqual(scale, 1.25, 0.0001),
		"float roundtrip"
	)) {
		return false;
	}

	const auto name = input.GetString("name");
	if (!Check(
		name && *name == "test value",
		"string roundtrip"
	)) {
		return false;
	}

	output.SetInt("count", 84);
	if (!Check(
		output.Save(path),
		"replace existing config"
	)) {
		return false;
	}

	util::KeyValueConfig replaced;
	int replacedCount = 0;

	if (!Check(
		replaced.Load(path) &&
			replaced.TryGetInt("count", replacedCount) &&
			replacedCount == 84,
		"atomic replacement roundtrip"
	)) {
		return false;
	}

	std::filesystem::path temporary = path;
	temporary += ".tmp";

	if (!Check(
		!std::filesystem::exists(temporary),
		"temporary file cleanup"
	)) {
		return false;
	}

	{
		std::ofstream manual(path, std::ios::trunc);
		manual
			<< "# comment\n"
			<< " valid = 7 \n"
			<< "flag=false\n"
			<< "duplicate=1\n"
			<< "duplicate=2\n"
			<< "broken-line\n"
			<< "invalid_int=nope\n"
			<< "invalid_float=nan\n"
			<< "trailing_float=1.25oops\n"
			<< "invalid_bool=yes\n";
	}

	util::KeyValueConfig parsed;
	if (!Check(
		parsed.Load(path),
		"load hand-written config"
	)) {
		return false;
	}

	int valid = 0;
	int invalid = 123;
	int duplicate = 0;
	bool flag = true;
	bool invalidBool = false;
	float invalidFloat = 0.0f;
	float trailingFloat = 0.0f;

	if (!Check(
		parsed.TryGetInt("valid", valid) && valid == 7,
		"trim whitespace"
	)) {
		return false;
	}

	if (!Check(
		parsed.TryGetBool("flag", flag) && !flag,
		"parse false"
	)) {
		return false;
	}

	if (!Check(
		!parsed.TryGetInt("invalid_int", invalid),
		"reject invalid integer"
	)) {
		return false;
	}

	if (!Check(
		parsed.TryGetInt("duplicate", duplicate) &&
			duplicate == 2,
		"last duplicate config value wins"
	)) {
		return false;
	}

	if (!Check(
		!parsed.TryGetFloat("invalid_float", invalidFloat),
		"reject non-finite float"
	)) {
		return false;
	}

	if (!Check(
		!parsed.TryGetFloat("trailing_float", trailingFloat),
		"reject float with trailing data"
	)) {
		return false;
	}

	if (!Check(
		!parsed.TryGetBool("invalid_bool", invalidBool),
		"reject invalid boolean"
	)) {
		return false;
	}

	if (!Check(
		!parsed.GetString("broken-line"),
		"ignore malformed line"
	)) {
		return false;
	}

	std::error_code error;
	std::filesystem::remove(path, error);

	return true;
}

} // namespace tests

int main() {
	if (!tests::RunLoaderTests()) {
		return 1;
	}

	if (!tests::RunMathTests()) {
		return 1;
	}

	if (!tests::RunConfigTests()) {
		return 1;
	}

	std::cout << "All regression tests passed.\n";
	return 0;
}
