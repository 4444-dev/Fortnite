#pragma once

#define sdk_offset inline u64
namespace Unreal {
	namespace Offsets {
		namespace core {
			sdk_offset GEngine = 0x1B2C7518;
			sdk_offset GameViewport = 0xB70;
			sdk_offset GameInstance = 0x238;
			sdk_offset GameState = 0x1C0;
			sdk_offset RootComponent = 0x1B0;
			sdk_offset ComponentToWorld = 0x1E0;
			sdk_offset BoneArray = 0x660;
			sdk_offset BoneArrayCache = 0x670;
			sdk_offset PlayerArray = 0x288;
			sdk_offset RelativeLocation = 0x140;
			sdk_offset LocationPointer = 0x168;
			sdk_offset RotationPointer = 0x178;
			sdk_offset Seconds = 0x7A8;
			sdk_offset FOV = 0x374;
			sdk_offset ServerWorldTime = 0x2A0;
			sdk_offset LastRenderTime = 0x290;
			sdk_offset PersistentLevel = 0x38;
			sdk_offset Levels = 0x1D8;
			sdk_offset Actors = 0x1A0;
			sdk_offset ReviveFromDBNOTime = 0x4A78;
			sdk_offset LifespanAfterDeath = 0x10A8;
			sdk_offset ServerCriticalHealth = 0x1BFC;
			sdk_offset CachedComponentSpaceTransforms = 0x9E8;
			sdk_offset CurrentReadComponentTransforms = 0x48;
			sdk_offset FNameToString = 0x4485C;
			sdk_offset StaticFindObject = 0x8141B8;
			sdk_offset StaticLoadObject = 0x5B6D05;
			sdk_offset ProcessEvent = 0xEEFC4;
			sdk_offset BoneMatrix = 0xF8C5796;
			sdk_offset GetBoneMatrix = 0xF8C5796;
			sdk_offset DrawTransition = 0x7672544;
			sdk_offset GetWorldCtxObj = 0xA68FDBE;
			sdk_offset AccumRotation = 0xFEE67BE;
			sdk_offset Rarity = 0x1873E5D8;
		}

		namespace player {
			sdk_offset Mesh = 0x2F0;
			sdk_offset LocalPlayers = 0x38;
			sdk_offset PlayerController = 0x30;
			sdk_offset LocalPawn = 0x318;
			sdk_offset PawnPrivate = 0x2E8;
			sdk_offset PlayerState = 0x290;
			sdk_offset TeamIndex = 0xF61;
			sdk_offset PlayerName = 0x9E8;
			sdk_offset KillScore = 0xF78;
			sdk_offset Platform = 0x400;
			sdk_offset bIsDying = 0x728;
			sdk_offset bIsDBNO = 0x881;
			sdk_offset bIsABot = 0x27A;
			sdk_offset bIsCrouched = 0x430;
			sdk_offset HabaneroComponent = 0x918;
			sdk_offset SkinnedAsset = 0x5F0;
			sdk_offset FinalRefBonePose = 0x2B0;
			sdk_offset ComponentSpaceTransforms = 0x660;
		}

		namespace weapon {
			sdk_offset CurrentWeapon = 0x9A0;
			sdk_offset WeaponData = 0x638;
			sdk_offset ItemName = 0x38;
			sdk_offset AmmoCount = 0x1100;
			sdk_offset bIsReloadingWeapon = 0x381;
			sdk_offset LastFireTime = 0xFFC;
			sdk_offset LastFireTimeVerified = 0x1004;
			sdk_offset LastDamagedTime = 0xDE8;
			sdk_offset ProjectileSpeed = 0x210C;
			sdk_offset ProjectileGravity = 0x2110;
			sdk_offset ComponentVelocity = 0x188;
		}

		namespace aim {
			sdk_offset TargetedFortPawn = 0x16C0;
			sdk_offset LocationUnderReticle = 0x2188;
			sdk_offset NetConnection = 0x4A8;
			sdk_offset RotationInput = 0x4B0;
			sdk_offset WeaponOffsetCorrection = 0x2340;
			sdk_offset WeaponRecoilOffset = 0x2328;
			sdk_offset PlayerAimOffset = 0x2310;
		}

		namespace loot {
			sdk_offset SpawnSourceOverride = 0xB78;
			sdk_offset SearchedFlags = 0xCE2;
			sdk_offset SearchText = 0xD38;
			sdk_offset ChosenRandomUpgrade = 0xBC4;
		}

		namespace pickup {
			sdk_offset SimulatingTooLongLength = 0x290;
			sdk_offset PrimaryPickupItemEntry = 0x368;
			sdk_offset ItemEntryItemDefinition = 0x10;
			sdk_offset ItemDefinitionName = 0x38;
			sdk_offset ItemDefinitionDataList = 0x68;
			sdk_offset ItemEntryItemDataList = 0x28;
			sdk_offset WeaponDisplayTier = 0x296;
			sdk_offset RarityStruct = 0x1873E5D8;
			sdk_offset PickupFlags = 0x28C;
			sdk_offset PickupExtendedFlags = 0x28D;
			sdk_offset PickupLocationData = 0x410;
		}

		sdk_offset GEngine = core::GEngine;

		namespace UEngine {
			sdk_offset GameViewport = core::GameViewport;
		}

		namespace UGameViewportClient {
			sdk_offset World = 0x78;
		}

		namespace UWorld {
			sdk_offset PersistentLevel = core::PersistentLevel;
			sdk_offset DefaultPhysicsVolume = 0x158;
			inline u64 CachedViewInfo = DefaultPhysicsVolume + 0x20;
			sdk_offset Levels = core::Levels;
			sdk_offset OwningGameInstance = core::GameInstance;
			sdk_offset GameState = core::GameState;
			inline u64 TimeSeconds = core::Seconds;
		}

		namespace ULevel {
			inline u64 Actors = core::Actors;
			inline u64 ActorCluster = 0x88;
			inline u64 ActorClusterActors = 0x28;
		}

		namespace UGameInstance {
			sdk_offset LocalPlayers = player::LocalPlayers;
		}

		namespace UPlayer {
			sdk_offset PlayerController = player::PlayerController;
		}

		namespace USceneComponent {
			sdk_offset RelativeLocation = core::RelativeLocation;
			sdk_offset ComponentToWorld = core::ComponentToWorld;
		}

		namespace UPrimitiveComponent {
			inline u64 LastRenderTime = core::LastRenderTime;
		}

		namespace USkeletalMeshComponent {
			inline u64 BoneArray = core::BoneArray;
			inline u64 BoneArrayCache = core::BoneArrayCache;
			inline u64 CurrentReadComponentTransforms = core::CurrentReadComponentTransforms;
		}

		namespace AActor {
			sdk_offset RootComponent = core::RootComponent;
		}

		namespace APlayerController {
			sdk_offset AcknowledgedPawn = player::LocalPawn;
		}

		namespace APawn {
			sdk_offset PlayerState = player::PlayerState;
		}

		namespace ACharacter {
			sdk_offset Mesh = player::Mesh;
		}

		namespace APlayerState {
			sdk_offset bIsABot = player::bIsABot;
			inline u8 bIsABotBit = 3;
			sdk_offset PawnPrivate = player::PawnPrivate;
			inline u64 PlayerNamePrivate = player::PlayerName;
			inline u64 Platform = player::Platform;
		}

		namespace AGameStateBase {
			sdk_offset PlayerArray = core::PlayerArray;
		}

		namespace AFortPawn {
			sdk_offset bIsDying = player::bIsDying;
			inline u8 bIsDyingBit = 5;
			sdk_offset bIsDBNO = player::bIsDBNO;
			inline u8 bIsDBNOBit = 0;
			inline u64 CurrentWeapon = weapon::CurrentWeapon;
		}

		namespace AFortPlayerPawnAthena {
			sdk_offset ReviveFromDBNOTime = core::ReviveFromDBNOTime;
		}

		namespace AFortPlayerStateAthena {
			sdk_offset TeamIndex = player::TeamIndex;
		}
	}
}
