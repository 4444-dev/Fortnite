#pragma once

#include "product_registry.hpp"

#include <string>

namespace loader {

[[nodiscard]] bool LaunchConfiguredTarget(
	const ProductDefinition& product,
	std::string& message
);

} // namespace loader
