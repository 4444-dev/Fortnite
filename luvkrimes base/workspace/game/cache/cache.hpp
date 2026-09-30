#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

inline constexpr i32 g_ESPBones[] = {
	static_cast<i32>(EFortSkeleton::Head),
	static_cast<i32>(EFortSkeleton::Neck_01),
	static_cast<i32>(EFortSkeleton::Spine_05),
	static_cast<i32>(EFortSkeleton::Pelvis),
	static_cast<i32>(EFortSkeleton::Upperarm_L),
	static_cast<i32>(EFortSkeleton::Lowerarm_L),
	static_cast<i32>(EFortSkeleton::Hand_L),
	static_cast<i32>(EFortSkeleton::Upperarm_R),
	static_cast<i32>(EFortSkeleton::Lowerarm_R),
	static_cast<i32>(EFortSkeleton::Hand_R),
	static_cast<i32>(EFortSkeleton::Thigh_L),
	static_cast<i32>(EFortSkeleton::Calf_L),
	static_cast<i32>(EFortSkeleton::Foot_L),
	static_cast<i32>(EFortSkeleton::Thigh_R),
	static_cast<i32>(EFortSkeleton::Calf_R),
	static_cast<i32>(EFortSkeleton::Foot_R),
};

inline constexpr i32 g_ESPBoneCount =
	static_cast<i32>(sizeof(g_ESPBones) / sizeof(g_ESPBones[0]));

struct CachedPlayer {
	AFortPlayerPawnAthena* Pawn{};
	USkeletalMeshComponent* Mesh{};
	AFortPlayerStateAthena* PlayerState{};
	FVector Location{};
	float Distance{};
	u8 TeamIndex{};
	bool bIsLocal{};
	bool bIsDying{};
	bool bIsDBNO{};
	bool bIsBot{};
	bool bHasWeapon{};
	bool bIsVisible{};

	std::array<FVector, 16> Bones{};
	bool BonesValid{};
};

class PlayerSnapshotView final {
public:
	using Container = std::vector<CachedPlayer>;
	using const_iterator = Container::const_iterator;

	PlayerSnapshotView(
		const Container& players,
		std::shared_mutex& mutex
	)
		: m_Lock(mutex),
		  m_Players(players) {
	}

	PlayerSnapshotView(const PlayerSnapshotView&) = delete;
	PlayerSnapshotView& operator=(const PlayerSnapshotView&) = delete;
	PlayerSnapshotView(PlayerSnapshotView&&) noexcept = default;
	PlayerSnapshotView& operator=(PlayerSnapshotView&&) noexcept = delete;

	[[nodiscard]] const_iterator begin() const noexcept {
		return m_Players.begin();
	}

	[[nodiscard]] const_iterator end() const noexcept {
		return m_Players.end();
	}

	[[nodiscard]] std::size_t size() const noexcept {
		return m_Players.size();
	}

	[[nodiscard]] bool empty() const noexcept {
		return m_Players.empty();
	}

private:
	std::shared_lock<std::shared_mutex> m_Lock;
	const Container& m_Players;
};

struct PlayerCacheStats {
	double EngineMs{};
	double ActorsMs{};
	double PlayersMs{};
	std::size_t ActorCount{};
	std::size_t PlayerCount{};
};

class PlayerCache {
public:
	static constexpr i32 MaxLevels = 2048;
	static constexpr i32 MaxActorsPerLevel = 262144;

	PlayerCache() = default;
	~PlayerCache() { Stop(); }

	PlayerCache(const PlayerCache&) = delete;
	PlayerCache& operator=(const PlayerCache&) = delete;

	void SetImageBase(uptr ImageBase);
	void Start();
	void Stop();

	[[nodiscard]] uptr ImageBase() const;
	[[nodiscard]] UWorld* World() const;
	[[nodiscard]] u8 LocalTeamIndex() const;

	void SetCameraLocation(const FVector& CameraLocation);

	[[nodiscard]] PlayerSnapshotView Snapshot() const;
	[[nodiscard]] PlayerCacheStats Stats() const;

private:
	void EngineLoop();
	void ActorsLoop();
	void PlayersLoop();

	void ResolveEngineSnapshot();
	void ScanActors();
	void ResolvePlayers();

	void PublishPlayers(std::vector<CachedPlayer>&& Players);
	CachedPlayer ResolveOne(
		AFortPlayerPawnAthena* Pawn,
		const FVector& CameraLoc
	) const;

	std::atomic<uptr> m_ImageBase{0};
	std::atomic<UWorld*> m_World{nullptr};
	std::atomic<UGameInstance*> m_GameInstance{nullptr};
	std::atomic<APlayerController*> m_PlayerController{nullptr};
	std::atomic<AFortPlayerPawnAthena*> m_LocalPawn{nullptr};
	std::atomic<AFortPlayerStateAthena*> m_LocalPlayerState{nullptr};
	std::atomic<u8> m_LocalTeamIndex{0};
	std::atomic<float> m_WorldTimeSeconds{0.0f};

	std::atomic<double> m_CamX{0.0};
	std::atomic<double> m_CamY{0.0};
	std::atomic<double> m_CamZ{0.0};

	mutable std::mutex m_ActorMutex;
	std::vector<AFortPlayerPawnAthena*> m_Actors;

	mutable std::array<std::shared_mutex, 2> m_PlayerBufferMutexes;
	std::array<std::vector<CachedPlayer>, 2> m_PlayerBuffers;
	std::atomic<u8> m_ActivePlayerBuffer{0};

	std::atomic<u64> m_EngineMicros{0};
	std::atomic<u64> m_ActorsMicros{0};
	std::atomic<u64> m_PlayersMicros{0};
	std::atomic<u32> m_ActorCount{0};
	std::atomic<u32> m_PlayerCount{0};

	std::thread m_EngineThread;
	std::thread m_ActorsThread;
	std::thread m_PlayersThread;
	std::atomic<bool> m_Running{false};
};
