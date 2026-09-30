#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace loader::license_store {

inline constexpr std::size_t kMaxLicenseLength = 4096;

bool Save(std::string_view productSlug, const std::string& license);
bool Load(std::string_view productSlug, std::string& license);
void Clear(std::string_view productSlug);

} // namespace loader::license_store
