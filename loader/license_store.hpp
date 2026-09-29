#pragma once

#include <string>

namespace loader::license_store {

bool Save(const std::string& license);
bool Load(std::string& license);
void Clear();

} // namespace loader::license_store
