#pragma once
#include <cmath>

constexpr double PI = 3.14159265358979323846;
constexpr double SMALL_NUMBER = 1e-8;
constexpr double KINDA_SMALL_NUMBER = 1e-4;

template<typename T>
struct TArray {
	T* Data;
	i32 Num;
	i32 Max;

	TArray( ) : Data( nullptr ), Num( 0 ), Max( 0 ) { }
	TArray( T* InData, i32 InNum, i32 InMax ) : Data( InData ), Num( InNum ), Max( InMax ) { }

	bool IsValid( ) const {
		return Data && Num >= 0 && Max >= Num;
	}

	bool IsValidIndex( i32 Index ) const {
		return Index >= 0 && Index < Num && Index < Max;
	}

	i32 Size( ) const { return Num; }
	i32 MaxSize( ) const { return Max; }
	uptr GetAddress( ) const { return reinterpret_cast< uptr >( Data ); }
};

static_assert( sizeof( TArray<u8> ) == 0x10, "TArray must be 16 bytes" );

template<typename KeyType, typename ValueType>
struct TPair {
	KeyType Key;
	ValueType Value;
};

struct FBitArray {
	u32* Data;
	i32 NumBits;
	i32 MaxBits;
};

template<typename T>
struct TSparseArray {
	TArray<T> Data;
	FBitArray AllocationFlags;
	i32 FirstFreeIndex;
	i32 NumFreeIndices;
};

template<typename T>
struct TSet {
	TSparseArray<T> Elements;
	u8 HashPad [ 0x10 ];
};

template<typename KeyType, typename ValueType>
struct TMap {
	TSet< TPair<KeyType, ValueType> > Pairs;
};

struct FName {
	u32 ComparisonIndex;
	u32 Number;

	FName( ) : ComparisonIndex( 0 ), Number( 0 ) { }
	FName( u32 Index, u32 InNumber = 0 ) : ComparisonIndex( Index ), Number( InNumber ) { }

	bool operator==( const FName& Other ) const {
		return ComparisonIndex == Other.ComparisonIndex && Number == Other.Number;
	}

	bool operator!=( const FName& Other ) const {
		return !( *this == Other );
	}

	bool IsNone( ) const {
		return ComparisonIndex == 0 && Number == 0;
	}
};

static_assert( sizeof( FName ) == 0x8, "FName must be 8 bytes" );

struct FNameEntryHeader {
	u16 bIsWide : 1;
	u16 Len : 15;
};

struct FString : TArray<wchar_t> {
	FString( ) { }

	FString( const wchar_t* Other ) {
		this->Max = this->Num = Other && *Other ? static_cast< i32 >( wcslen( Other ) ) + 1 : 0;
		this->Data = const_cast< wchar_t* >( Other );
	}

	operator bool( ) const { return Data != nullptr && Num > 0; }

	wchar_t* CStr( ) { return Data; }
	const wchar_t* CStr( ) const { return Data; }
};

static_assert( sizeof( FString ) == 0x10, "FString must be 16 bytes" );

struct FText {
	uptr TextData;
	uptr SharedRefs;
	u32 Flags;
	u8 Pad_14 [ 0x4 ];
};

static_assert( sizeof( FText ) == 0x18, "FText must be 24 bytes" );

struct FTextData {
	u8 Pad_0 [ 0x20 ];
	wchar_t* Name;
	i32 Length;
};

struct FGameplayTag {
	FName TagName;
};

struct FGameplayTagContainer {
	TArray<FGameplayTag> GameplayTags;
	TArray<FGameplayTag> ParentTags;
};

struct FGuid {
	i32 A;
	i32 B;
	i32 C;
	i32 D;

	FGuid( ) : A( 0 ), B( 0 ), C( 0 ), D( 0 ) { }

	bool operator==( const FGuid& Other ) const {
		return A == Other.A && B == Other.B && C == Other.C && D == Other.D;
	}

	bool IsValid( ) const {
		return A | B | C | D;
	}
};

static_assert( sizeof( FGuid ) == 0x10, "FGuid must be 16 bytes" );

struct FColor {
	u8 B, G, R, A;

	FColor( ) : B( 0 ), G( 0 ), R( 0 ), A( 0 ) { }
	FColor( u8 InR, u8 InG, u8 InB, u8 InA = 255 ) : B( InB ), G( InG ), R( InR ), A( InA ) { }
};

struct FLinearColor {
	float R, G, B, A;

	FLinearColor( ) : R( 0.f ), G( 0.f ), B( 0.f ), A( 0.f ) { }
	FLinearColor( float InR, float InG, float InB, float InA = 1.f ) : R( InR ), G( InG ), B( InB ), A( InA ) { }

	FLinearColor WithAlpha( float Alpha ) const {
		return FLinearColor( R, G, B, Alpha );
	}
};

struct FIntPoint {
	i32 X, Y;
};

struct FIntVector {
	i32 X, Y, Z;
};

struct FVector2D {
	double X, Y;

	FVector2D( ) : X( 0.0 ), Y( 0.0 ) { }
	FVector2D( double InX, double InY ) : X( InX ), Y( InY ) { }

	FVector2D operator+( const FVector2D& Other ) const { return { X + Other.X, Y + Other.Y }; }
	FVector2D operator-( const FVector2D& Other ) const { return { X - Other.X, Y - Other.Y }; }
	FVector2D operator*( double Scale ) const { return { X * Scale, Y * Scale }; }
	FVector2D operator/( double Scale ) const { return { X / Scale, Y / Scale }; }

	FVector2D& operator+=( const FVector2D& Other ) { X += Other.X; Y += Other.Y; return *this; }
	FVector2D& operator-=( const FVector2D& Other ) { X -= Other.X; Y -= Other.Y; return *this; }
	FVector2D& operator*=( double Scale ) { X *= Scale; Y *= Scale; return *this; }
	FVector2D& operator/=( double Scale ) { X /= Scale; Y /= Scale; return *this; }

	bool operator==( const FVector2D& Other ) const { return X == Other.X && Y == Other.Y; }
	bool operator!=( const FVector2D& Other ) const { return !( *this == Other ); }
	operator bool( ) const { return X != 0.0 || Y != 0.0; }

	double Dot( const FVector2D& Other ) const { return X * Other.X + Y * Other.Y; }
	double Size( ) const { return std::sqrt( X * X + Y * Y ); }
	double SizeSquared( ) const { return X * X + Y * Y; }
	double DistanceTo( const FVector2D& Other ) const {
		const double DX = Other.X - X;
		const double DY = Other.Y - Y;
		return std::sqrt( DX * DX + DY * DY );
	}
	bool IsValid( ) const { return X != 0.0 || Y != 0.0; }
};

static_assert( sizeof( FVector2D ) == 0x10, "FVector2D must be 16 bytes" );

struct FVector {
	double X, Y, Z;

	FVector( ) : X( 0.0 ), Y( 0.0 ), Z( 0.0 ) { }
	FVector( double InX, double InY, double InZ ) : X( InX ), Y( InY ), Z( InZ ) { }

	FVector operator+( const FVector& Other ) const { return { X + Other.X, Y + Other.Y, Z + Other.Z }; }
	FVector operator-( const FVector& Other ) const { return { X - Other.X, Y - Other.Y, Z - Other.Z }; }
	FVector operator*( const FVector& Other ) const { return { X * Other.X, Y * Other.Y, Z * Other.Z }; }
	FVector operator*( double Scale ) const { return { X * Scale, Y * Scale, Z * Scale }; }
	FVector operator/( double Scale ) const { return { X / Scale, Y / Scale, Z / Scale }; }

	FVector& operator+=( const FVector& Other ) { X += Other.X; Y += Other.Y; Z += Other.Z; return *this; }
	FVector& operator-=( const FVector& Other ) { X -= Other.X; Y -= Other.Y; Z -= Other.Z; return *this; }
	FVector& operator*=( const FVector& Other ) { X *= Other.X; Y *= Other.Y; Z *= Other.Z; return *this; }
	FVector& operator*=( double Scale ) { X *= Scale; Y *= Scale; Z *= Scale; return *this; }
	FVector& operator/=( double Scale ) { X /= Scale; Y /= Scale; Z /= Scale; return *this; }

	bool operator==( const FVector& Other ) const { return X == Other.X && Y == Other.Y && Z == Other.Z; }
	bool operator!=( const FVector& Other ) const { return !( *this == Other ); }
	operator bool( ) const { return X != 0.0 || Y != 0.0 || Z != 0.0; }

	double Dot( const FVector& Other ) const { return X * Other.X + Y * Other.Y + Z * Other.Z; }

	FVector Cross( const FVector& Other ) const {
		return {
			Y * Other.Z - Z * Other.Y,
			Z * Other.X - X * Other.Z,
			X * Other.Y - Y * Other.X
		};
	}

	double Size( ) const { return std::sqrt( X * X + Y * Y + Z * Z ); }
	double SizeSquared( ) const { return X * X + Y * Y + Z * Z; }
	double Size2D( ) const { return std::sqrt( X * X + Y * Y ); }

	double DistanceTo( const FVector& Other ) const {
		return ( Other - *this ).Size( ) / 100.0;
	}

	double DistanceToSquared( const FVector& Other ) const {
		return ( Other - *this ).SizeSquared( );
	}

	FVector GetSafeNormal( double Tolerance = SMALL_NUMBER ) const {
		const double SquareSum = SizeSquared( );
		if ( SquareSum > Tolerance ) {
			const double Scale = 1.0 / std::sqrt( SquareSum );
			return { X * Scale, Y * Scale, Z * Scale };
		}
		return {};
	}

	FVector GetUnsafeNormal( ) const {
		const double Scale = 1.0 / Size( );
		return { X * Scale, Y * Scale, Z * Scale };
	}

	bool IsNearlyZero( double Tolerance = KINDA_SMALL_NUMBER ) const {
		return std::fabs( X ) <= Tolerance && std::fabs( Y ) <= Tolerance && std::fabs( Z ) <= Tolerance;
	}

	bool IsZero( ) const { return X == 0.0 && Y == 0.0 && Z == 0.0; }
	bool IsValid( ) const { return X != 0.0 || Y != 0.0 || Z != 0.0; }

	FVector Lerp( const FVector& Other, double Alpha ) const {
		return *this * ( 1.0 - Alpha ) + Other * Alpha;
	}
};

static_assert( sizeof( FVector ) == 0x18, "FVector must be 24 bytes (LWC)" );

struct FVector4 {
	double X, Y, Z, W;

	FVector4( ) : X( 0.0 ), Y( 0.0 ), Z( 0.0 ), W( 1.0 ) { }
	FVector4( double InX, double InY, double InZ, double InW ) : X( InX ), Y( InY ), Z( InZ ), W( InW ) { }
};

static_assert( sizeof( FVector4 ) == 0x20, "FVector4 must be 32 bytes" );

struct FPlane {
	double X, Y, Z, W;
};

static_assert( sizeof( FPlane ) == 0x20, "FPlane must be 32 bytes" );

struct FRotator {
	double Pitch, Yaw, Roll;

	FRotator( ) : Pitch( 0.0 ), Yaw( 0.0 ), Roll( 0.0 ) { }
	FRotator( double InPitch, double InYaw, double InRoll ) : Pitch( InPitch ), Yaw( InYaw ), Roll( InRoll ) { }

	FRotator operator+( const FRotator& Other ) const { return { Pitch + Other.Pitch, Yaw + Other.Yaw, Roll + Other.Roll }; }
	FRotator operator-( const FRotator& Other ) const { return { Pitch - Other.Pitch, Yaw - Other.Yaw, Roll - Other.Roll }; }
	FRotator operator*( double Scale ) const { return { Pitch * Scale, Yaw * Scale, Roll * Scale }; }

	FRotator& operator+=( const FRotator& Other ) { Pitch += Other.Pitch; Yaw += Other.Yaw; Roll += Other.Roll; return *this; }
	FRotator& operator-=( const FRotator& Other ) { Pitch -= Other.Pitch; Yaw -= Other.Yaw; Roll -= Other.Roll; return *this; }

	bool operator==( const FRotator& Other ) const {
		return Pitch == Other.Pitch && Yaw == Other.Yaw && Roll == Other.Roll;
	}

	bool operator!=( const FRotator& Other ) const { return !( *this == Other ); }

	static double ClampAxis( double Angle ) {
		Angle = std::fmod( Angle, 360.0 );
		if ( Angle < 0.0 )
			Angle += 360.0;
		return Angle;
	}

	static double NormalizeAxis( double Angle ) {
		Angle = ClampAxis( Angle );
		if ( Angle > 180.0 )
			Angle -= 360.0;
		return Angle;
	}

	void Normalize( ) {
		Pitch = NormalizeAxis( Pitch );
		Yaw = NormalizeAxis( Yaw );
		Roll = NormalizeAxis( Roll );
	}

	FRotator GetNormalized( ) const {
		FRotator Result = *this;
		Result.Normalize( );
		return Result;
	}

	FVector Vector( ) const {
		const double PitchRad = Pitch * ( PI / 180.0 );
		const double YawRad = Yaw * ( PI / 180.0 );
		const double CP = std::cos( PitchRad );
		const double SP = std::sin( PitchRad );
		const double CY = std::cos( YawRad );
		const double SY = std::sin( YawRad );
		return { CP * CY, CP * SY, SP };
	}

	FVector GetForwardVector( ) const { return Vector( ); }

	bool IsValid( ) const { return Pitch != 0.0 || Yaw != 0.0 || Roll != 0.0; }
};

static_assert( sizeof( FRotator ) == 0x18, "FRotator must be 24 bytes (LWC)" );

struct alignas( 16 ) FQuat {
	double X, Y, Z, W;

	FQuat( ) : X( 0.0 ), Y( 0.0 ), Z( 0.0 ), W( 1.0 ) { }
	FQuat( double InX, double InY, double InZ, double InW ) : X( InX ), Y( InY ), Z( InZ ), W( InW ) { }

	FVector RotateVector( const FVector& V ) const {
		const FVector Q( X, Y, Z );
		const FVector T = Q.Cross( V ) * 2.0;
		return V + ( T * W ) + Q.Cross( T );
	}
};

static_assert( sizeof( FQuat ) == 0x20, "FQuat must be 32 bytes" );

struct FBox {
	FVector Min;
	FVector Max;
	u8 IsValid;
	u8 Pad_31 [ 0x7 ];
};

struct FBoxSphereBounds {
	FVector Origin;
	FVector BoxExtent;
	double SphereRadius;
};

static_assert( sizeof( FBoxSphereBounds ) == 0x38, "FBoxSphereBounds must be 56 bytes" );

struct FMatrix {
	union {
		struct {
			double M11, M12, M13, M14;
			double M21, M22, M23, M24;
			double M31, M32, M33, M34;
			double M41, M42, M43, M44;
		};
		double M [ 4 ][ 4 ];
		struct {
			FPlane XPlane, YPlane, ZPlane, WPlane;
		};
	};

	FMatrix( ) {
		M11 = 0; M12 = 0; M13 = 0; M14 = 0;
		M21 = 0; M22 = 0; M23 = 0; M24 = 0;
		M31 = 0; M32 = 0; M33 = 0; M34 = 0;
		M41 = 0; M42 = 0; M43 = 0; M44 = 0;
	}

	FVector TransformPosition( const FVector& V ) const {
		return {
			V.X * M11 + V.Y * M21 + V.Z * M31 + M41,
			V.X * M12 + V.Y * M22 + V.Z * M32 + M42,
			V.X * M13 + V.Y * M23 + V.Z * M33 + M43
		};
	}

	FVector4 TransformPosition4( const FVector& V ) const {
		return {
			V.X * M11 + V.Y * M21 + V.Z * M31 + M41,
			V.X * M12 + V.Y * M22 + V.Z * M32 + M42,
			V.X * M13 + V.Y * M23 + V.Z * M33 + M43,
			V.X * M14 + V.Y * M24 + V.Z * M34 + M44
		};
	}

	FMatrix operator*( const FMatrix& Other ) const {
		FMatrix Result;
		for ( i32 Row = 0; Row < 4; ++Row ) {
			for ( i32 Col = 0; Col < 4; ++Col ) {
				Result.M [ Row ][ Col ] =
					M [ Row ][ 0 ] * Other.M [ 0 ][ Col ] +
					M [ Row ][ 1 ] * Other.M [ 1 ][ Col ] +
					M [ Row ][ 2 ] * Other.M [ 2 ][ Col ] +
					M [ Row ][ 3 ] * Other.M [ 3 ][ Col ];
			}
		}
		return Result;
	}

	static FMatrix MakeFromRotator( const FRotator& Rotation ) {
		FMatrix Matrix;

		const double Pitch = Rotation.Pitch * ( PI / 180.0 );
		const double Yaw = Rotation.Yaw * ( PI / 180.0 );
		const double Roll = Rotation.Roll * ( PI / 180.0 );

		const double SP = std::sin( Pitch ), CP = std::cos( Pitch );
		const double SY = std::sin( Yaw ), CY = std::cos( Yaw );
		const double SR = std::sin( Roll ), CR = std::cos( Roll );

		Matrix.XPlane.X = CP * CY;
		Matrix.XPlane.Y = CP * SY;
		Matrix.XPlane.Z = SP;

		Matrix.YPlane.X = SR * SP * CY - CR * SY;
		Matrix.YPlane.Y = SR * SP * SY + CR * CY;
		Matrix.YPlane.Z = -SR * CP;

		Matrix.ZPlane.X = -( CR * SP * CY + SR * SY );
		Matrix.ZPlane.Y = CY * SR - CR * SP * SY;
		Matrix.ZPlane.Z = CR * CP;

		Matrix.WPlane.W = 1.0;
		return Matrix;
	}

	bool WorldToScreen( const FVector& World, double ScreenWidth, double ScreenHeight, FVector2D& Screen ) const {
		if ( ScreenWidth <= 0.0 || ScreenHeight <= 0.0 )
			return false;

		const FVector4 Clip = TransformPosition4( World );
		if ( !std::isfinite( Clip.W ) || Clip.W <= 0.001 )
			return false;

		const double RecipW = 1.0 / Clip.W;
		Screen.X = ( Clip.X * RecipW + 1.0 ) * 0.5 * ScreenWidth;
		Screen.Y = ( 1.0 - Clip.Y * RecipW ) * 0.5 * ScreenHeight;
		return std::isfinite( Screen.X ) && std::isfinite( Screen.Y );
	}
};

static_assert( sizeof( FMatrix ) == 0x80, "FMatrix must be 128 bytes" );

struct alignas( 16 ) FTransform {
	FQuat Rotation;
	FVector Translation;
	u8 Pad_38 [ 0x8 ];
	FVector Scale3D;
	u8 Pad_58 [ 0x8 ];

	FTransform( )
		: Rotation( ), Translation( ), Pad_38 { }, Scale3D( 1.0, 1.0, 1.0 ), Pad_58 { } {
	}

	FVector TransformPosition( const FVector& V ) const {
		const FVector Scaled { V.X * Scale3D.X, V.Y * Scale3D.Y, V.Z * Scale3D.Z };
		return Rotation.RotateVector( Scaled ) + Translation;
	}

	FMatrix ToMatrixWithScale( ) const {
		FMatrix Matrix;

		const double X2 = Rotation.X + Rotation.X;
		const double Y2 = Rotation.Y + Rotation.Y;
		const double Z2 = Rotation.Z + Rotation.Z;

		const double XX2 = Rotation.X * X2;
		const double YY2 = Rotation.Y * Y2;
		const double ZZ2 = Rotation.Z * Z2;
		const double YZ2 = Rotation.Y * Z2;
		const double WX2 = Rotation.W * X2;
		const double XY2 = Rotation.X * Y2;
		const double WZ2 = Rotation.W * Z2;
		const double XZ2 = Rotation.X * Z2;
		const double WY2 = Rotation.W * Y2;

		Matrix.XPlane.X = ( 1.0 - ( YY2 + ZZ2 ) ) * Scale3D.X;
		Matrix.XPlane.Y = ( XY2 + WZ2 ) * Scale3D.X;
		Matrix.XPlane.Z = ( XZ2 - WY2 ) * Scale3D.X;

		Matrix.YPlane.X = ( XY2 - WZ2 ) * Scale3D.Y;
		Matrix.YPlane.Y = ( 1.0 - ( XX2 + ZZ2 ) ) * Scale3D.Y;
		Matrix.YPlane.Z = ( YZ2 + WX2 ) * Scale3D.Y;

		Matrix.ZPlane.X = ( XZ2 + WY2 ) * Scale3D.Z;
		Matrix.ZPlane.Y = ( YZ2 - WX2 ) * Scale3D.Z;
		Matrix.ZPlane.Z = ( 1.0 - ( XX2 + YY2 ) ) * Scale3D.Z;

		Matrix.WPlane.X = Translation.X;
		Matrix.WPlane.Y = Translation.Y;
		Matrix.WPlane.Z = Translation.Z;
		Matrix.WPlane.W = 1.0;
		return Matrix;
	}
};

static_assert( sizeof( FTransform ) == 0x60, "FTransform must be 96 bytes" );

inline FVector operator*( const FTransform& Transform, const FVector& V ) {
	return Transform.TransformPosition( V );
}

struct FWeakObjectPtr {
	i32 ObjectIndex;
	i32 ObjectSerialNumber;
};

template<typename T>
struct TWeakObjectPtr : FWeakObjectPtr { };

struct FSoftObjectPath {
	FName AssetPathName;
	FString SubPathString;
};

template<typename T>
struct TSoftObjectPtr {
	FWeakObjectPtr WeakPtr;
	FSoftObjectPath ObjectPath;
};

struct FTopLevelAssetPath {
	FName PackageName;
	FName AssetName;
};

struct FMinimalViewInfo {
	FVector Location;
	FRotator Rotation;
	float FOV;
	float DesiredFOV;
	float OrthoWidth;
	float OrthoNearClipPlane;
	float OrthoFarClipPlane;
	float PerspectiveNearClipPlane;
	float AspectRatio;
};

struct FCameraCacheEntry {
	float Timestamp;
	u8 Pad_4 [ 0xC ];
	FMinimalViewInfo POV;
};

struct FViewMatrices {
	FMatrix ProjectionMatrix;
	FMatrix ProjectionNoAAMatrix;
	FMatrix InvProjectionMatrix;
	FMatrix ViewMatrix;
	FMatrix InvViewMatrix;
	FMatrix ViewProjectionMatrix;
};

static_assert( sizeof( FViewMatrices ) == 0x300, "FViewMatrices must be 768 bytes" );

struct FWorldCachedViewInfo {
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	FMatrix ViewProjectionMatrix;
	FMatrix ViewToWorld;

	bool WorldToScreen( const FVector& World, double ScreenWidth, double ScreenHeight, FVector2D& Screen ) const {
		return ViewProjectionMatrix.WorldToScreen( World, ScreenWidth, ScreenHeight, Screen );
	}
};

static_assert( sizeof( FWorldCachedViewInfo ) == 0x200, "FWorldCachedViewInfo must be 512 bytes" );

struct FSceneViewStateReference {
	uptr VTable;
	uptr Reference;
	u8 Pad_10 [ 0x28 ];
};

static_assert( sizeof( FSceneViewStateReference ) == 0x38, "FSceneViewStateReference must be 56 bytes" );

struct FHitResult {
	i32 FaceIndex;
	float Time;
	float Distance;
	FVector Location;
	FVector ImpactPoint;
	FVector Normal;
	FVector ImpactNormal;
	FVector TraceStart;
	FVector TraceEnd;
	float PenetrationDepth;
	i32 Item;
	u8 ElementIndex;
	u8 bBlockingHit : 1;
	u8 bStartPenetrating : 1;
};

struct FFortItemEntry {
	u8 Pad_0 [ 0x10 ];
	uptr ItemDefinition;
};

struct FFortPickupLocationData {
	uptr PickupTarget;
	uptr CombineTarget;
	uptr ItemOwner;
	FVector LootInitialPosition;
	FVector LootFinalPosition;
	float FlyTime;
	u8 Pad_4C [ 0x4 ];
	FVector StartDirection;
	FVector FinalTossRestLocation;
};

struct FRankedProgressReplicatedData {
	FString RankType;
	i32 Rank;
};
