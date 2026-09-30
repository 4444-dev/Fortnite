#pragma once

#include <array>
#include <string_view>

namespace loader {

enum class ProductId {
	Fortnite,
	ApexLegends
};

struct ProductDefinition {
	ProductId Id;
	std::string_view Slug;
	std::string_view DisplayName;
	std::string_view Subtitle;

	std::string_view KeyAuthName;
	std::string_view KeyAuthOwnerId;
	std::string_view KeyAuthVersion;
	std::string_view KeyAuthUrl;
	std::string_view KeyAuthPath;

	std::wstring_view TargetEnvironmentVariable;
	bool Configured;
};

inline constexpr ProductDefinition Fortnite{
	ProductId::Fortnite,
	"fortnite",
	"FORTNITE",
	"Fortnite project",
	"Timocod18ytb's Application",
	"ZOhORJsXc1",
	"1.0",
	"https://keyauth.win/api/1.3/",
	"",
	L"LUVKRIMES_TARGET_FORTNITE",
	true
};

// Apex is deliberately a separate KeyAuth application. Leave it unavailable
// until its own KeyAuth application identifiers are supplied. Never reuse the
// Fortnite owner/application configuration here if separate key pools are
// required.
inline constexpr ProductDefinition ApexLegends{
	ProductId::ApexLegends,
	"apex",
	"APEX LEGENDS",
	"Apex Legends project",
	"",
	"",
	"1.0",
	"https://keyauth.win/api/1.3/",
	"",
	L"LUVKRIMES_TARGET_APEX",
	false
};

inline constexpr std::array Products{
	Fortnite,
	ApexLegends
};

static_assert(
	Fortnite.KeyAuthOwnerId.size() == 10,
	"Fortnite KeyAuth owner ID must be 10 characters"
);

[[nodiscard]] inline constexpr const ProductDefinition* FindProduct(
	ProductId id
) noexcept {
	for (const auto& product : Products) {
		if (product.Id == id) {
			return &product;
		}
	}
	return nullptr;
}

} // namespace loader
