#pragma once

#include <array>
#include <cstddef>
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
	std::wstring_view PackagedTargetRelativePath;
	bool Configured;
};

[[nodiscard]] inline constexpr bool IsValidProductSlug(
	std::string_view slug
) noexcept {
	if (slug.empty() || slug.size() > 64) {
		return false;
	}

	for (const char ch : slug) {
		const bool valid =
			(ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9') ||
			ch == '-' ||
			ch == '_';

		if (!valid) {
			return false;
		}
	}

	return true;
}

inline constexpr std::array<ProductDefinition, 2> Products{{
	{
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
		L"projects\\fortnite\\Luvkrimes-Fortnite.exe",
		true
	},
	{
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
		L"",
		false
	}
}};

inline constexpr const ProductDefinition& Fortnite = Products[0];
inline constexpr const ProductDefinition& ApexLegends = Products[1];

[[nodiscard]] inline constexpr bool RegistryIsValid() noexcept {
	for (std::size_t i = 0; i < Products.size(); ++i) {
		const auto& product = Products[i];

		if (
			!IsValidProductSlug(product.Slug) ||
			product.DisplayName.empty() ||
			product.TargetEnvironmentVariable.empty()
		) {
			return false;
		}

		if (
			product.Configured &&
			(
				product.KeyAuthName.empty() ||
				product.KeyAuthOwnerId.empty() ||
				product.KeyAuthVersion.empty() ||
				product.KeyAuthUrl.empty() ||
				!product.KeyAuthUrl.starts_with("https://") ||
				product.PackagedTargetRelativePath.empty()
			)
		) {
			return false;
		}

		for (std::size_t j = i + 1; j < Products.size(); ++j) {
			if (
				product.Id == Products[j].Id ||
				product.Slug == Products[j].Slug ||
				product.TargetEnvironmentVariable ==
					Products[j].TargetEnvironmentVariable ||
				(
					!product.PackagedTargetRelativePath.empty() &&
					product.PackagedTargetRelativePath ==
						Products[j].PackagedTargetRelativePath
				)
			) {
				return false;
			}

			if (
				product.Configured &&
				Products[j].Configured &&
				product.KeyAuthName == Products[j].KeyAuthName &&
				product.KeyAuthOwnerId == Products[j].KeyAuthOwnerId
			) {
				return false;
			}
		}
	}

	return true;
}

static_assert(
	RegistryIsValid(),
	"Product registry contains an invalid or duplicate product definition"
);

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
