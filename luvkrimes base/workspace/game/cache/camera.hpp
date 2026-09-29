#pragma once

struct CachedCamera {
	FVector Location;
	FRotator Rotation;
	FVector AxisRight;
	FVector AxisUp;
	FVector AxisForward;
	float FOV;
	FWorldCachedViewInfo ViewInfo;
	bool Valid;
};

class CameraCache {
public:
	const CachedCamera& Camera( ) const;
	FVector Location( ) const;
	FRotator Rotation( ) const;
	float FOV( ) const;
	bool Valid( ) const;

	bool Update( UWorld* World );
	bool WorldToScreen( const FVector& World, double Width, double Height, FVector2D& Screen ) const;

private:
	CachedCamera m_Camera {};
	i32 m_UnstableFrames {};
};
