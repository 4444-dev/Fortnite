#include <includes.hpp>
#include <workspace/game/cache/camera.hpp>
#include <workspace/util/logger/logger.hpp>

#include <cmath>

namespace {
	bool TryBuildCameraFromView( const FWorldCachedViewInfo& View, CachedCamera& OutCamera ) {
		CachedCamera Next {};
		const auto& ViewToWorld = View.ViewToWorld;
		const auto& Projection = View.ProjectionMatrix;

		Next.ViewInfo = View;
		Next.AxisRight = { ViewToWorld.M [ 0 ][ 0 ], ViewToWorld.M [ 0 ][ 1 ], ViewToWorld.M [ 0 ][ 2 ] };
		Next.AxisUp = { ViewToWorld.M [ 1 ][ 0 ], ViewToWorld.M [ 1 ][ 1 ], ViewToWorld.M [ 1 ][ 2 ] };
		Next.AxisForward = { ViewToWorld.M [ 2 ][ 0 ], ViewToWorld.M [ 2 ][ 1 ], ViewToWorld.M [ 2 ][ 2 ] };
		Next.Location = { ViewToWorld.M [ 3 ][ 0 ], ViewToWorld.M [ 3 ][ 1 ], ViewToWorld.M [ 3 ][ 2 ] };

		double PitchValue = ViewToWorld.M [ 2 ][ 2 ];
		if ( PitchValue > 1.0 )
			PitchValue = 1.0;
		if ( PitchValue < -1.0 )
			PitchValue = -1.0;

		Next.Rotation.Pitch = std::asin( PitchValue ) * 180.0 / PI;
		Next.Rotation.Yaw = std::atan2( ViewToWorld.M [ 2 ][ 1 ], ViewToWorld.M [ 2 ][ 0 ] ) * 180.0 / PI;
		Next.Rotation.Roll = 0.0;

		const auto Scale = Projection.M [ 0 ][ 0 ];
		Next.FOV = static_cast< float >( 2.0 * std::atan( 1.0 / Scale ) * 180.0 / PI );

		const auto finiteVector = []( const FVector& V ) {
			return std::isfinite( V.X ) && std::isfinite( V.Y ) && std::isfinite( V.Z );
		};
		if ( !finiteVector( Next.AxisRight ) || !finiteVector( Next.AxisUp ) || !finiteVector( Next.AxisForward ) )
			return false;

		const double RightLenSq = Next.AxisRight.SizeSquared( );
		const double UpLenSq = Next.AxisUp.SizeSquared( );
		const double ForwardLenSq = Next.AxisForward.SizeSquared( );
		if ( RightLenSq < 0.81 || RightLenSq > 1.21 || UpLenSq < 0.81 || UpLenSq > 1.21 || ForwardLenSq < 0.81 || ForwardLenSq > 1.21 )
			return false;
		if ( std::fabs( Next.AxisRight.Dot( Next.AxisUp ) ) > 0.15 ||
			std::fabs( Next.AxisRight.Dot( Next.AxisForward ) ) > 0.15 ||
			std::fabs( Next.AxisUp.Dot( Next.AxisForward ) ) > 0.15 )
			return false;

		for ( i32 Row = 0; Row < 4; ++Row ) {
			for ( i32 Col = 0; Col < 4; ++Col ) {
				if ( !std::isfinite( View.ViewProjectionMatrix.M [ Row ][ Col ] ) )
					return false;
			}
		}

		const double LocationSquared = Next.Location.SizeSquared( );
		if ( !std::isfinite( Scale ) || Scale <= 0.0 || Scale > 1000.0 )
			return false;
		if ( !std::isfinite( LocationSquared ) || LocationSquared <= 0.0 || LocationSquared > 1.0e20 )
			return false;
		if ( !std::isfinite( Next.FOV ) || Next.FOV < 10.0f || Next.FOV > 170.0f )
			return false;

		Next.Valid = true;
		OutCamera = Next;
		return true;
	}
}

const CachedCamera& CameraCache::Camera( ) const {
	return m_Camera;
}

FVector CameraCache::Location( ) const {
	return m_Camera.Location;
}

FRotator CameraCache::Rotation( ) const {
	return m_Camera.Rotation;
}

float CameraCache::FOV( ) const {
	return m_Camera.FOV;
}

bool CameraCache::Valid( ) const {
	return m_Camera.Valid;
}

bool CameraCache::Update( UWorld* World ) {
	CachedCamera Next {};
	bool ok = false;

	if ( !World || !World->IsValid( ) )
		return false;

	const u64 CandidateOffsets [ ] = {
		Unreal::Offsets::UWorld::CachedViewInfo,
		0x178,
		0x180,
		0x170,
		0x188,
	};

	for ( const u64 Offset : CandidateOffsets ) {
		if ( Offset == 0 )
			continue;

		const auto Views = World->ReadArray< FWorldCachedViewInfo >( Offset );
		if ( !Views.IsValidIndex( 0 ) || Views.Num > 8 )
			continue;

		const auto View = World->ReadElement( Views, 0 );
		if ( !TryBuildCameraFromView( View, Next ) )
			continue;

		ok = true;
		if ( Unreal::Offsets::UWorld::CachedViewInfo != Offset ) {
			Unreal::Offsets::UWorld::CachedViewInfo = Offset;
			logger::Log( "[camera] resolved UWorld::CachedViewInfo off=0x%llx",
				static_cast< unsigned long long >( Offset ) );
		}
		break;
	}

	if ( ok ) {
		m_Camera = Next;
		m_UnstableFrames = 0;
		return true;
	}

	if ( m_Camera.Valid && m_UnstableFrames < 3 ) {
		++m_UnstableFrames;
		return true;
	}

	m_Camera = {};
	return false;
}

bool CameraCache::WorldToScreen( const FVector& World, double Width, double Height, FVector2D& Screen ) const {
	if ( !m_Camera.Valid )
		return false;
	return m_Camera.ViewInfo.WorldToScreen( World, Width, Height, Screen );
}
