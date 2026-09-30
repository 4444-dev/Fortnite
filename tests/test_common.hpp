#pragma once

#include <cmath>
#include <iostream>

namespace tests {

inline bool Check(bool condition, const char* message) {
	if (!condition) {
		std::cerr << "FAILED: " << message << '\n';
		return false;
	}

	return true;
}

inline bool NearlyEqual(
	double a,
	double b,
	double epsilon = 1e-9
) {
	return std::fabs(a - b) <= epsilon;
}

} // namespace tests
