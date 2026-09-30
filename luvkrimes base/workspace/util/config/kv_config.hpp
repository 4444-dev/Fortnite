#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace util {

class KeyValueConfig final {
public:
	bool Load(const std::filesystem::path& path);
	bool Save(const std::filesystem::path& path) const;

	void SetString(std::string key, std::string value);
	void SetBool(std::string key, bool value);
	void SetInt(std::string key, int value);
	void SetFloat(std::string key, float value);

	[[nodiscard]] std::optional<std::string> GetString(std::string_view key) const;
	[[nodiscard]] bool TryGetBool(std::string_view key, bool& value) const;
	[[nodiscard]] bool TryGetInt(std::string_view key, int& value) const;
	[[nodiscard]] bool TryGetFloat(std::string_view key, float& value) const;

private:
	std::map<std::string, std::string, std::less<>> m_Values;
};

} // namespace util
