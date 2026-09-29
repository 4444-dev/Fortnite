#pragma once

#include <string_view>

namespace loader::keyauth_config {

// These values identify the KeyAuth application; they are not administrative
// secrets. KeyAuth 1.3 intentionally uses the 5-argument constructor below.
inline constexpr std::string_view Name = "Timocod18ytb's Application";
inline constexpr std::string_view OwnerId = "ZOhORJsXc1";
inline constexpr std::string_view Version = "1.0";
inline constexpr std::string_view Url = "https://keyauth.win/api/1.3/";
inline constexpr std::string_view Path = "";

static_assert(OwnerId.size() == 10, "KeyAuth owner ID must be 10 characters");

} // namespace loader::keyauth_config
