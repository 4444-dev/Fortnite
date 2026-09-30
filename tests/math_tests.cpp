#include "test_common.hpp"
#include "test_suites.hpp"

#include <cstdint>
#include <limits>

using uptr = uintptr_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i32 = int32_t;

#include <workspace/game/unreal/structures.hpp>

namespace tests {

bool RunMathTests() {
	bool ok = true;

	FMatrix identity;
	for (int i = 0; i < 4; ++i) {
		identity.M[i][i] = 1.0;
	}

	FVector2D screen{};
	ok &= Check(
		identity.WorldToScreen(
			{0.0, 0.0, 0.0},
			1920.0,
			1080.0,
			screen
		) &&
			NearlyEqual(screen.X, 960.0) &&
			NearlyEqual(screen.Y, 540.0),
		"identity projection center"
	);

	ok &= Check(
		identity.WorldToScreen(
			{1.0, 1.0, 0.0},
			1920.0,
			1080.0,
			screen
		) &&
			NearlyEqual(screen.X, 1920.0) &&
			NearlyEqual(screen.Y, 0.0),
		"identity projection upper-right"
	);

	ok &= Check(
		!identity.WorldToScreen(
			{0.0, 0.0, 0.0},
			0.0,
			1080.0,
			screen
		),
		"reject zero-width viewport"
	);

	ok &= Check(
		!identity.WorldToScreen(
			{0.0, 0.0, 0.0},
			1920.0,
			0.0,
			screen
		),
		"reject zero-height viewport"
	);

	FMatrix nonFinite = identity;
	nonFinite.M41 = std::numeric_limits<double>::infinity();
	ok &= Check(
		!nonFinite.WorldToScreen(
			{0.0, 0.0, 0.0},
			1920.0,
			1080.0,
			screen
		),
		"reject infinite projected coordinate"
	);

	nonFinite = identity;
	nonFinite.M44 = std::numeric_limits<double>::quiet_NaN();
	ok &= Check(
		!nonFinite.WorldToScreen(
			{0.0, 0.0, 0.0},
			1920.0,
			1080.0,
			screen
		),
		"reject non-finite projection W"
	);

	FMatrix invalidW = identity;
	invalidW.M14 = std::numeric_limits<double>::quiet_NaN();
	ok &= Check(
		!invalidW.WorldToScreen(
			{1.0, 0.0, 0.0},
			1920.0,
			1080.0,
			screen
		),
		"reject non-finite clip W"
	);

	FMatrix invalidX = identity;
	invalidX.M11 = std::numeric_limits<double>::infinity();
	ok &= Check(
		!invalidX.WorldToScreen(
			{1.0, 0.0, 0.0},
			1920.0,
			1080.0,
			screen
		),
		"reject non-finite projected coordinate"
	);

	FMatrix behind = identity;
	behind.M44 = -1.0;
	ok &= Check(
		!behind.WorldToScreen(
			{0.0, 0.0, 0.0},
			1920.0,
			1080.0,
			screen
		),
		"reject point behind camera"
	);

	FTransform transform;
	transform.Translation = {10.0, 20.0, 30.0};
	transform.Scale3D = {2.0, 3.0, 4.0};

	const FVector transformed =
		transform.TransformPosition({1.0, 1.0, 1.0});

	ok &= Check(
		NearlyEqual(transformed.X, 12.0) &&
			NearlyEqual(transformed.Y, 23.0) &&
			NearlyEqual(transformed.Z, 34.0),
		"transform scale and translation"
	);

	const double halfSqrtTwo = std::sqrt(0.5);

	FTransform rotated;
	rotated.Rotation =
		FQuat(0.0, 0.0, halfSqrtTwo, halfSqrtTwo);

	const FVector rotatedVector =
		rotated.TransformPosition({1.0, 0.0, 0.0});

	ok &= Check(
		NearlyEqual(rotatedVector.X, 0.0, 1e-8) &&
			NearlyEqual(rotatedVector.Y, 1.0, 1e-8) &&
			NearlyEqual(rotatedVector.Z, 0.0, 1e-8),
		"quaternion rotates 90 degrees around Z"
	);

	const FMatrix rotatedMatrix =
		rotated.ToMatrixWithScale();
	const FVector matrixRotated =
		rotatedMatrix.TransformPosition({1.0, 0.0, 0.0});

	ok &= Check(
		NearlyEqual(matrixRotated.X, rotatedVector.X, 1e-8) &&
			NearlyEqual(matrixRotated.Y, rotatedVector.Y, 1e-8) &&
			NearlyEqual(matrixRotated.Z, rotatedVector.Z, 1e-8),
		"rotation matrix agrees with quaternion transform"
	);

	const FMatrix transformMatrix =
		transform.ToMatrixWithScale();
	const FVector matrixTransformed =
		transformMatrix.TransformPosition({1.0, 1.0, 1.0});

	ok &= Check(
		NearlyEqual(matrixTransformed.X, transformed.X) &&
			NearlyEqual(matrixTransformed.Y, transformed.Y) &&
			NearlyEqual(matrixTransformed.Z, transformed.Z),
		"transform matrix agrees with direct transform"
	);

	return ok;
}

} // namespace tests
