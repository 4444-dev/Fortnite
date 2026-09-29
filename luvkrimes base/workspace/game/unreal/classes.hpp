#pragma once
#include <cmath>

namespace Unreal {
	using ReadMemoryFn = bool( * )( void* Dst, uptr Src, u64 Size );
	inline ReadMemoryFn ReadMemory = nullptr;

	inline bool ValidAddress( uptr Address ) {
		return Address >= 0x10000 && Address < 0x00007FFFFFFFFFFFull;
	}
}

class UObject;

class UEngine;
class UGameViewportClient;
class UWorld;
class ULevel;
class UGameInstance;
class UPlayer;
class ULocalPlayer;
class USceneComponent;
class UPrimitiveComponent;
class UMeshComponent;
class USkinnedMeshComponent;
class USkeletalMeshComponent;
class USkinnedAsset;
class AActor;
class AController;
class APlayerCameraManager;
class APlayerController;
class APawn;
class ACharacter;
class AFGF_Character;
class APlayerState;
class AGameStateBase;
class AFortWeapon;
class AFortPawn;
class AFortPlayerPawn;
class AFortPlayerPawnAthena;
class AFortPlayerState;
class AFortPlayerStateAthena;
class UItemDefinitionBase;
class UFortItemDefinition;
class AFortPickup;
class ABuildingSMActor;
class ABuildingContainer;

class UObject {
public:
	uptr Address( ) const {
		return reinterpret_cast< uptr >( this );
	}

	bool IsValid( ) const {
		return Unreal::ValidAddress( Address( ) );
	}

	template<typename T>
	static T ReadAt( uptr Address ) {
		T Value {};
		if ( Unreal::ReadMemory )
			Unreal::ReadMemory( &Value, Address, sizeof( T ) );
		return Value;
	}

	template<typename T>
	T ReadField( u64 Offset ) const {
		return ReadAt< T >( Address( ) + Offset );
	}

	template<typename T>
	T* ReadPtr( u64 Offset ) const {
		return ReadAt< T* >( Address( ) + Offset );
	}

	template<typename T>
	TArray<T> ReadArray( u64 Offset ) const {
		return ReadAt< TArray<T> >( Address( ) + Offset );
	}

	template<typename T>
	T ReadElement( const TArray<T>& Array, i32 Index ) const {
		if ( !Array.IsValidIndex( Index ) )
			return T {};
		return ReadAt< T >( Array.GetAddress( ) + static_cast< uptr >( Index ) * sizeof( T ) );
	}

	template<typename T>
	static bool ReadRange( uptr Source, T* Out, i32 Count ) {
		if ( !Out || Count <= 0 || !Unreal::ReadMemory )
			return false;
		return Unreal::ReadMemory( Out, Source, static_cast< u64 >( Count ) * sizeof( T ) );
	}

	bool ReadBit( u64 Offset, u8 Bit ) const {
		return ( ReadField< u8 >( Offset ) & ( 1U << Bit ) ) != 0;
	}

	template<typename T, typename Callback>
	bool ForEach( const TArray<T>& Array, i32 MaximumCount, Callback&& Fn ) const {
		if ( !Array.IsValid( ) )
			return false;

		const i32 Count = Array.Num;
		if ( !Count )
			return true;
		if ( Count > MaximumCount )
			return false;

		constexpr i32 BatchCapacity = 128;
		T Values[ BatchCapacity ] {};

		for ( i32 First = 0; First < Count; ) {
			const i32 Remaining = Count - First;
			const i32 Number = Remaining < BatchCapacity ? Remaining : BatchCapacity;
			const uptr Address = Array.GetAddress( ) + static_cast< uptr >( First ) * sizeof( T );

			if ( ReadRange( Address, Values, Number ) ) {
				for ( i32 Index = 0; Index < Number; ++Index )
					Fn( Values[ Index ] );
			}
			else {
				for ( i32 Index = 0; Index < Number; ++Index )
					Fn( ReadAt< T >( Address + static_cast< uptr >( Index ) * sizeof( T ) ) );
			}

			First += Number;
		}

		return true;
	}
};

class UEngine : public UObject {
public:
	static UEngine* Get( uptr ImageBase ) {
		return ReadAt< UEngine* >( ImageBase + Unreal::Offsets::GEngine );
	}

	UGameViewportClient* GameViewport( ) const {
		return ReadPtr< UGameViewportClient >( Unreal::Offsets::UEngine::GameViewport );
	}
};

class UGameEngine : public UEngine {
};

class UGameViewportClient : public UObject {
public:
	UWorld* World( ) const {
		return ReadPtr< UWorld >( Unreal::Offsets::UGameViewportClient::World );
	}
};

class UWorld : public UObject {
public:
	ULevel* PersistentLevel( ) const {
		return ReadPtr< ULevel >( Unreal::Offsets::UWorld::PersistentLevel );
	}

	TArray<ULevel*> Levels( ) const {
		return ReadArray< ULevel* >( Unreal::Offsets::UWorld::Levels );
	}

	TArray<FWorldCachedViewInfo> CachedViewInfo( ) const {
		return ReadArray< FWorldCachedViewInfo >( Unreal::Offsets::UWorld::CachedViewInfo );
	}

	UGameInstance* OwningGameInstance( ) const {
		return ReadPtr< UGameInstance >( Unreal::Offsets::UWorld::OwningGameInstance );
	}

	float TimeSeconds( ) const {
		if ( Unreal::Offsets::UWorld::TimeSeconds == 0 )
			return 0.0f;
		return ReadField< float >( Unreal::Offsets::UWorld::TimeSeconds );
	}
};

class ULevel : public UObject {
public:
	TArray<AActor*> Actors( ) const {
		if ( Unreal::Offsets::ULevel::ActorCluster != 0 && Unreal::Offsets::ULevel::ActorClusterActors != 0 ) {
			UObject* Cluster = ReadPtr< UObject >( Unreal::Offsets::ULevel::ActorCluster );
			if ( Cluster && Cluster->IsValid( ) ) {
				return Cluster->ReadArray< AActor* >( Unreal::Offsets::ULevel::ActorClusterActors );
			}
		}
		return ReadArray< AActor* >( Unreal::Offsets::ULevel::Actors );
	}
};

class UGameInstance : public UObject {
public:
	TArray<ULocalPlayer*> LocalPlayers( ) const {
		return ReadArray< ULocalPlayer* >( Unreal::Offsets::UGameInstance::LocalPlayers );
	}
};

class UPlayer : public UObject {
public:
	APlayerController* PlayerController( ) const {
		return ReadPtr< APlayerController >( Unreal::Offsets::UPlayer::PlayerController );
	}
};

class ULocalPlayer : public UPlayer {
};

class USceneComponent : public UObject {
public:
	FVector RelativeLocation( ) const {
		return ReadField< FVector >( Unreal::Offsets::USceneComponent::RelativeLocation );
	}

	FTransform ComponentToWorld( ) const {
		return ReadField< FTransform >( Unreal::Offsets::USceneComponent::ComponentToWorld );
	}
};

class UPrimitiveComponent : public USceneComponent {
public:
	float LastRenderTime( ) const {
		if ( Unreal::Offsets::UPrimitiveComponent::LastRenderTime == 0 )
			return 0.0f;
		return ReadField< float >( Unreal::Offsets::UPrimitiveComponent::LastRenderTime );
	}

	bool IsRecentlyRendered( float WorldTime, float ThresholdSeconds = 0.06f ) const {
		if ( Unreal::Offsets::UPrimitiveComponent::LastRenderTime == 0 )
			return false;
		const float Last = LastRenderTime( );
		return Last > 0.0f && ( WorldTime - Last ) <= ThresholdSeconds;
	}
};

class UMeshComponent : public UPrimitiveComponent {
};

class USkinnedMeshComponent : public UMeshComponent {
};

class USkeletalMeshComponent : public USkinnedMeshComponent {
public:
	TArray<FTransform> BoneArray( ) const {
		return ReadArray< FTransform >( Unreal::Offsets::USkeletalMeshComponent::BoneArray );
	}

	TArray<FTransform> BoneArrayCache( ) const {
		return ReadArray< FTransform >( Unreal::Offsets::USkeletalMeshComponent::BoneArrayCache );
	}

	u8 CurrentReadComponentTransforms( ) const {
		return ReadField< u8 >( Unreal::Offsets::USkeletalMeshComponent::CurrentReadComponentTransforms );
	}

	bool ReadBoneLocations( const i32* Indices, i32 Count, FVector* Out ) const {
		if ( !IsValid( ) || !Indices || !Out || Count <= 0 )
			return false;

		const TArray<FTransform> BonesArray = ( CurrentReadComponentTransforms( ) & 1 ) ? BoneArrayCache( ) : BoneArray( );
		if ( !BonesArray.IsValid( ) )
			return false;

		i32 MaxIndex = 0;
		for ( i32 Index = 0; Index < Count; ++Index ) {
			if ( Indices [ Index ] > MaxIndex )
				MaxIndex = Indices [ Index ];
		}

		const i32 Needed = MaxIndex + 1;
		if ( Needed > BonesArray.Num || Needed > 128 )
			return false;

		FTransform Transforms [ 128 ];
		if ( !ReadRange( BonesArray.GetAddress( ), Transforms, Needed ) )
			return false;

		const FMatrix ComponentToWorld = this->ComponentToWorld( ).ToMatrixWithScale( );

		for ( i32 Index = 0; Index < Count; ++Index ) {
			const i32 BoneIndex = Indices [ Index ];
			if ( BoneIndex < 0 || BoneIndex >= Needed )
				return false;
			const FMatrix Matrix = Transforms [ BoneIndex ].ToMatrixWithScale( ) * ComponentToWorld;
			Out [ Index ] = { Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z };
		}

		return true;
	}
};

class AActor : public UObject {
public:
	USceneComponent* RootComponent( ) const {
		return ReadPtr< USceneComponent >( Unreal::Offsets::AActor::RootComponent );
	}

	FVector Location( ) const {
		USceneComponent* Root = RootComponent( );
		if ( !Root || !Root->IsValid( ) )
			return {};
		return Root->RelativeLocation( );
	}
};

class AController : public AActor {
};

class APlayerCameraManager : public AActor {
};

class APlayerController : public AController {
public:
	APawn* AcknowledgedPawn( ) const {
		return ReadPtr< APawn >( Unreal::Offsets::APlayerController::AcknowledgedPawn );
	}
};

class APawn : public AActor {
public:
	APlayerState* PlayerState( ) const {
		return ReadPtr< APlayerState >( Unreal::Offsets::APawn::PlayerState );
	}
};

class ACharacter : public APawn {
public:
	USkeletalMeshComponent* Mesh( ) const {
		return ReadPtr< USkeletalMeshComponent >( Unreal::Offsets::ACharacter::Mesh );
	}
};

class AFGF_Character : public ACharacter {
};

class AFortPawn : public AFGF_Character {
public:
	bool bIsDying( ) const {
		return ReadBit( Unreal::Offsets::AFortPawn::bIsDying, Unreal::Offsets::AFortPawn::bIsDyingBit );
	}

	bool bIsDBNO( ) const {
		return ReadBit( Unreal::Offsets::AFortPawn::bIsDBNO, Unreal::Offsets::AFortPawn::bIsDBNOBit );
	}

	AFortWeapon* CurrentWeapon( ) const {
		if ( Unreal::Offsets::AFortPawn::CurrentWeapon == 0 )
			return nullptr;
		return ReadPtr< AFortWeapon >( Unreal::Offsets::AFortPawn::CurrentWeapon );
	}
};

class AFortPlayerPawn : public AFortPawn {
};

class AFortPlayerPawnAthena : public AFortPlayerPawn {
public:
	float ReviveFromDBNOTime( ) const {
		return ReadField< float >( Unreal::Offsets::AFortPlayerPawnAthena::ReviveFromDBNOTime );
	}

	bool IsPlayerPawn( ) const {
		auto* Mesh = this->Mesh( );
		if ( !Mesh || !Mesh->IsValid( ) )
			return false;

		auto* State = this->PlayerState( );
		if ( !Unreal::ValidAddress( reinterpret_cast< uptr >( State ) ) )
			return false;

		return true;
	}
};

class APlayerState : public AActor {
public:
	bool bIsABot( ) const {
		return ReadBit( Unreal::Offsets::APlayerState::bIsABot, Unreal::Offsets::APlayerState::bIsABotBit );
	}

	APawn* PawnPrivate( ) const {
		return ReadPtr< APawn >( Unreal::Offsets::APlayerState::PawnPrivate );
	}
};

class AGameStateBase : public AActor {
public:
	TArray<APlayerState*> PlayerArray( ) const {
		return ReadArray< APlayerState* >( Unreal::Offsets::AGameStateBase::PlayerArray );
	}
};

class AFortWeapon : public AActor {
};

class AFortPlayerState : public APlayerState {
};

class AFortPlayerStateAthena : public AFortPlayerState {
public:
	u8 TeamIndex( ) const {
		return ReadField< u8 >( Unreal::Offsets::AFortPlayerStateAthena::TeamIndex );
	}
};

class UItemDefinitionBase : public UObject {
};

class UFortItemDefinition : public UItemDefinitionBase {
};

class AFortPickup : public AActor {
};

class ABuildingSMActor : public AActor {
};

class ABuildingContainer : public ABuildingSMActor {
};
