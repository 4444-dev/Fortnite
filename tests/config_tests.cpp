#include <workspace/util/config/kv_config.hpp>

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

} // namespace

int main() {
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

	std::cout << "All config tests passed.\n";
	return 0;
}
