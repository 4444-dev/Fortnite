#pragma once

#include <string>
#include <string_view>

namespace loader::license_store {

bool Save(std::string_view productSlug, const std::string& license);
bool Load(std::string_view productSlug, std::string& license);
void Clear(std::string_view productSlug);

} // namespace loader::license_store
