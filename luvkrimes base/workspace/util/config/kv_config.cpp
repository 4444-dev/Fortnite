#include <workspace/util/config/kv_config.hpp>

#include <charconv>
#include <cmath>
#include <limits>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif
#include <fstream>

namespace util {
namespace {

std::string Trim(std::string value) {
	const auto first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return {};
	}

	const auto last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

} // namespace

bool KeyValueConfig::Load(const std::filesystem::path& path) {
	std::ifstream input(path);
	if (!input) {
		return false;
	}

	std::map<std::string, std::string, std::less<>> parsed;
	std::string line;

	while (std::getline(input, line)) {
		line = Trim(std::move(line));
		if (line.empty() || line.front() == '#') {
			continue;
		}

		const auto separator = line.find('=');
		if (separator == std::string::npos) {
			continue;
		}

		std::string key = Trim(line.substr(0, separator));
		std::string value = Trim(line.substr(separator + 1));
		if (!key.empty()) {
			parsed[std::move(key)] = std::move(value);
		}
	}

	if (input.bad()) {
		return false;
	}

	m_Values = std::move(parsed);
	return true;
}

bool KeyValueConfig::Save(const std::filesystem::path& path) const {
	std::error_code error;
	const auto parent = path.parent_path();
	if (!parent.empty()) {
		std::filesystem::create_directories(parent, error);
		if (error) {
			return false;
		}
	}

	std::filesystem::path temporary = path;
	temporary += ".tmp";

	{
		std::ofstream output(temporary, std::ios::trunc);
		if (!output) {
			return false;
		}

		output << "# Nexus settings\n";
		for (const auto& [key, value] : m_Values) {
			output << key << '=' << value << '\n';
		}

		output.flush();
		if (!output.good()) {
			output.close();
			std::filesystem::remove(temporary, error);
			return false;
		}
	}

#if defined(_WIN32)
	if (!MoveFileExW(
		temporary.c_str(),
		path.c_str(),
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
	)) {
		std::filesystem::remove(temporary, error);
		return false;
	}
#else
	std::filesystem::rename(temporary, path, error);
	if (error) {
		std::filesystem::remove(temporary, error);
		return false;
	}
#endif

	return true;
}

void KeyValueConfig::SetString(std::string key, std::string value) {
	m_Values[std::move(key)] = std::move(value);
}

void KeyValueConfig::SetBool(std::string key, bool value) {
	SetString(std::move(key), value ? "true" : "false");
}

void KeyValueConfig::SetInt(std::string key, int value) {
	SetString(std::move(key), std::to_string(value));
}

void KeyValueConfig::SetFloat(std::string key, float value) {
	if (!std::isfinite(value)) {
		value = 0.0f;
	}

	char buffer[64]{};
	const auto result = std::to_chars(
		buffer,
		buffer + sizeof(buffer),
		value,
		std::chars_format::general,
		std::numeric_limits<float>::max_digits10
	);

	if (result.ec != std::errc{}) {
		SetString(std::move(key), "0");
		return;
	}

	SetString(
		std::move(key),
		std::string(buffer, result.ptr)
	);
}

std::optional<std::string> KeyValueConfig::GetString(std::string_view key) const {
	const auto it = m_Values.find(key);
	if (it == m_Values.end()) {
		return std::nullopt;
	}
	return it->second;
}

bool KeyValueConfig::TryGetBool(std::string_view key, bool& value) const {
	const auto raw = GetString(key);
	if (!raw) {
		return false;
	}

	if (*raw == "true" || *raw == "1") {
		value = true;
		return true;
	}
	if (*raw == "false" || *raw == "0") {
		value = false;
		return true;
	}
	return false;
}

bool KeyValueConfig::TryGetInt(std::string_view key, int& value) const {
	const auto raw = GetString(key);
	if (!raw) {
		return false;
	}

	int parsed = 0;
	const char* begin = raw->data();
	const char* end = begin + raw->size();
	const auto result = std::from_chars(begin, end, parsed);
	if (result.ec != std::errc{} || result.ptr != end) {
		return false;
	}

	value = parsed;
	return true;
}

bool KeyValueConfig::TryGetFloat(std::string_view key, float& value) const {
	const auto raw = GetString(key);
	if (!raw || raw->empty()) {
		return false;
	}

	float parsed = 0.0f;
	const char* begin = raw->data();
	const char* end = begin + raw->size();
	const auto result = std::from_chars(
		begin,
		end,
		parsed,
		std::chars_format::general
	);

	if (
		result.ec != std::errc{} ||
		result.ptr != end ||
		!std::isfinite(parsed)
	) {
		return false;
	}

	value = parsed;
	return true;
}

} // namespace util
