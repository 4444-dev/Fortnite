#pragma once
#include <array>
#include <atomic>
#include <mutex>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

inline constexpr i32 g_ESPBones[ ] = {
	static_cast< i32 >( EFortSkeleton::Head ),
	static_cast< i32 >( EFortSkeleton::Neck_01 ),
	static_cast< i32 >( EFortSkeleton::Spine_05 ),
	static_cast< i32 >( EFortSkeleton::Pelvis ),
	static_cast< i32 >( EFortSkeleton::Upperarm_L ),
	static_cast< i32 >( EFortSkeleton::Lowerarm_L ),
	static_cast< i32 >( EFortSkeleton::Hand_L ),
	static_cast< i32 >( EFortSkeleton::Upperarm_R ),
	static_cast< i32 >( EFortSkeleton::Lowerarm_R ),
	static_cast< i32 >( EFortSkeleton::Hand_R ),
	static_cast< i32 >( EFortSkeleton::Thigh_L ),
	static_cast< i32 >( EFortSkeleton::Calf_L ),
	static_cast< i32 >( EFortSkeleton::Foot_L ),
	static_cast< i32 >( EFortSkeleton::Thigh_R ),
	static_cast< i32 >( EFortSkeleton::Calf_R ),
	static_cast< i32 >( EFortSkeleton::Foot_R ),
};
inline constexpr i32 g_ESPBoneCount = static_cast< i32 >( sizeof( g_ESPBones ) / sizeof( g_ESPBones [ 0 ] ) );

struct CachedPlayer {
	AFortPlayerPawnAthena* Pawn {};
	USkeletalMeshComponent* Mesh {};
	AFortPlayerStateAthena* PlayerState {};
	FVector Location {};
	float Distance {};
	u8 TeamIndex {};
	bool bIsLocal {};
	bool bIsDying {};
	bool bIsDBNO {};
	bool bIsBot {};
	bool bHasWeapon {};
	bool bIsVisible {};

	std::array<FVector, 16> Bones {};
	bool BonesValid {};
};

class PlayerCache {
public:
	static constexpr i32 MaxLevels = 2048;
	static constexpr i32 MaxActorsPerLevel = 262144;

	PlayerCache( ) = default;
	~PlayerCache( ) { Stop( ); }

	PlayerCache( const PlayerCache& ) = delete;
	PlayerCache& operator=( const PlayerCache& ) = delete;

	void SetImageBase( uptr ImageBase );
	void Start( );
	void Stop( );

	uptr ImageBase( ) const;
	UWorld* World( ) const;
	u8 LocalTeamIndex( ) const;

	void SetCameraLocation( const FVector& CameraLocation );

	std::vector<CachedPlayer> Snapshot( ) const;
	std::size_t Count( ) const;

private:
	void EngineLoop( );
	void ActorsLoop( );
	void PlayersLoop( );

	void ResolveEngineSnapshot( );
	void ScanActors( );
	void ResolvePlayers( );

	CachedPlayer ResolveOne( AFortPlayerPawnAthena* Pawn, const FVector& CameraLoc ) const;

	std::atomic<uptr> m_ImageBase { 0 };
	std::atomic<UWorld*> m_World { nullptr };
	std::atomic<UGameInstance*> m_GameInstance { nullptr };
	std::atomic<APlayerController*> m_PlayerController { nullptr };
	std::atomic<AFortPlayerPawnAthena*> m_LocalPawn { nullptr };
	std::atomic<AFortPlayerStateAthena*> m_LocalPlayerState { nullptr };
	std::atomic<u8> m_LocalTeamIndex { 0 };
	std::atomic<float> m_WorldTimeSeconds { 0.0f };

	std::atomic<double> m_CamX { 0.0 };
	std::atomic<double> m_CamY { 0.0 };
	std::atomic<double> m_CamZ { 0.0 };

	mutable std::mutex m_ActorMutex;
	std::vector<AFortPlayerPawnAthena*> m_Actors;

	mutable std::mutex m_PlayerMutex;
	std::vector<CachedPlayer> m_Players;

	std::thread m_EngineThread;
	std::thread m_ActorsThread;
	std::thread m_PlayersThread;
	std::atomic<bool> m_Running { false };
};
