#include <includes.hpp>
#include <workspace/game/cache/cache.hpp>
#include <workspace/util/logger/logger.hpp>

#include <chrono>
#include <exception>

using namespace std::chrono_literals;

namespace {
	enum class EngineStage : u8 {
		Ok,
		NoImageBase,
		EngineInvalid,
		ViewportInvalid,
		WorldInvalid,
		GameInstanceInvalid,
		LocalPlayersInvalid,
		LocalPlayerInvalid,
		PlayerControllerInvalid,
	};

	void LogEngineStage( EngineStage Stage, uptr ImageBase = 0, uptr Engine = 0, uptr Viewport = 0, uptr World = 0 ) {
		static EngineStage LastStage = EngineStage::Ok;
		static u32 RepeatCount = 0;
		static auto LastLogTime = std::chrono::steady_clock::time_point {};
		const auto Now = std::chrono::steady_clock::now();

		if ( Stage == LastStage ) {
			++RepeatCount;
			if ( RepeatCount % 500 != 0 )
				return;
		}
		else {
			if ( LastLogTime.time_since_epoch().count() != 0 &&
				( Now - LastLogTime ) < std::chrono::milliseconds( 150 ) ) {
				LastStage = Stage;
				return;
			}
			LastStage = Stage;
			RepeatCount = 0;
		}

		LastLogTime = Now;

		switch ( Stage ) {
		case EngineStage::Ok:
			logger::Log( "[engine] chain ok img=0x%llx gengine_off=0x%llx world=0x%llx",
				static_cast< unsigned long long >( ImageBase ),
				static_cast< unsigned long long >( Unreal::Offsets::GEngine ),
				static_cast< unsigned long long >( World ) );
			break;
		case EngineStage::NoImageBase:
			logger::Log( "[engine] waiting image base" );
			break;
		case EngineStage::EngineInvalid:
			logger::Log( "[engine] invalid UEngine img=0x%llx gengine_off=0x%llx ptr=0x%llx",
				static_cast< unsigned long long >( ImageBase ),
				static_cast< unsigned long long >( Unreal::Offsets::GEngine ),
				static_cast< unsigned long long >( Engine ) );
			break;
		case EngineStage::ViewportInvalid:
			logger::Log( "[engine] invalid GameViewport engine=0x%llx viewport_off=0x%llx ptr=0x%llx",
				static_cast< unsigned long long >( Engine ),
				static_cast< unsigned long long >( Unreal::Offsets::UEngine::GameViewport ),
				static_cast< unsigned long long >( Viewport ) );
			break;
		case EngineStage::WorldInvalid:
			logger::Log( "[engine] invalid UWorld viewport=0x%llx world_off=0x%llx ptr=0x%llx",
				static_cast< unsigned long long >( Viewport ),
				static_cast< unsigned long long >( Unreal::Offsets::UGameViewportClient::World ),
				static_cast< unsigned long long >( World ) );
			break;
		case EngineStage::GameInstanceInvalid:
			logger::Log( "[engine] invalid GameInstance world=0x%llx gi_off=0x%llx",
				static_cast< unsigned long long >( World ),
				static_cast< unsigned long long >( Unreal::Offsets::UWorld::OwningGameInstance ) );
			break;
		case EngineStage::LocalPlayersInvalid:
			logger::Log( "[engine] invalid LocalPlayers gi_off=0x%llx",
				static_cast< unsigned long long >( Unreal::Offsets::UGameInstance::LocalPlayers ) );
			break;
		case EngineStage::LocalPlayerInvalid:
			logger::Log( "[engine] invalid LocalPlayer index 0" );
			break;
		case EngineStage::PlayerControllerInvalid:
			logger::Log( "[engine] invalid PlayerController localplayer_off=0x%llx",
				static_cast< unsigned long long >( Unreal::Offsets::UPlayer::PlayerController ) );
			break;
		}
	}

	bool ReadRaw( uptr Source, void* Dest, u64 Size ) {
		return Unreal::ReadMemory && Dest && Size > 0 && Unreal::ReadMemory( Dest, Source, Size );
	}

	bool ReadPtr( uptr Source, uptr& Out ) {
		Out = 0;
		return ReadRaw( Source, &Out, sizeof( Out ) );
	}

	bool TryResolvePawnFromPlayerState( APlayerState* PlayerState, AFortPlayerPawnAthena*& OutPawn, bool CommitOffset = true ) {
		OutPawn = nullptr;
		if ( !PlayerState || !PlayerState->IsValid( ) )
			return false;

		const uptr PlayerStatePtr = reinterpret_cast< uptr >( PlayerState );
		const u64 CandidateOffsets [ ] = {
			Unreal::Offsets::APlayerState::PawnPrivate,
			0x2E8,
			0x2F0,
			0x2F8,
			0x300,
			0x308,
			0x310,
			0x2D8,
		};

		for ( const u64 Offset : CandidateOffsets ) {
			if ( Offset == 0 )
				continue;

			uptr PawnPtr = 0;
			if ( !ReadPtr( PlayerStatePtr + Offset, PawnPtr ) || !Unreal::ValidAddress( PawnPtr ) )
				continue;

			auto* Pawn = reinterpret_cast< AFortPlayerPawnAthena* >( PawnPtr );
			if ( !Pawn || !Pawn->IsValid( ) )
				continue;

			APlayerState* BackState = Pawn->PlayerState( );
			if ( BackState != PlayerState )
				continue;

			auto* Mesh = Pawn->Mesh( );
			if ( !Mesh || !Mesh->IsValid( ) )
				continue;

			OutPawn = Pawn;
			if ( CommitOffset && Unreal::Offsets::APlayerState::PawnPrivate != Offset ) {
				Unreal::Offsets::APlayerState::PawnPrivate = Offset;
				logger::Log( "[engine] resolved playerstate->pawn off=0x%llx",
					static_cast< unsigned long long >( Offset ) );
			}
			return true;
		}

		return false;
	}

	bool TryResolveGameStatePtr( UWorld* World, AGameStateBase*& OutGameState, bool CommitOffset = true ) {
		OutGameState = nullptr;
		if ( !World || !World->IsValid( ) )
			return false;

		const uptr WorldPtr = reinterpret_cast< uptr >( World );
		const u64 CandidateOffsets [ ] = {
			Unreal::Offsets::UWorld::GameState,
			0x1C0,
			0x1B8,
			0x1C8,
			0x1D0,
		};

		for ( const u64 Offset : CandidateOffsets ) {
			if ( Offset == 0 )
				continue;

			uptr GameStatePtr = 0;
			if ( !ReadPtr( WorldPtr + Offset, GameStatePtr ) || !Unreal::ValidAddress( GameStatePtr ) )
				continue;

			auto* GameState = reinterpret_cast< AGameStateBase* >( GameStatePtr );
			if ( !GameState || !GameState->IsValid( ) )
				continue;

			const auto PlayerStates = GameState->PlayerArray( );
			if ( PlayerStates.Num < 0 || PlayerStates.Num > 256 || PlayerStates.Max < PlayerStates.Num || PlayerStates.Max > 512 )
				continue;
			if ( PlayerStates.Num > 0 && !Unreal::ValidAddress( PlayerStates.GetAddress( ) ) )
				continue;

			OutGameState = GameState;
			if ( CommitOffset && Unreal::Offsets::UWorld::GameState != Offset ) {
				Unreal::Offsets::UWorld::GameState = Offset;
				logger::Log( "[engine] resolved world->gamestate off=0x%llx",
					static_cast< unsigned long long >( Offset ) );
			}
			return true;
		}

		return false;
	}

	bool TryResolveViewportWorldPtr( uptr ViewportPtr, uptr& OutWorld, bool CommitOffset = true, u64* OutResolvedOffset = nullptr ) {
		OutWorld = 0;
		if ( OutResolvedOffset )
			*OutResolvedOffset = 0;
		if ( !Unreal::ValidAddress( ViewportPtr ) )
			return false;

		const u64 CandidateOffsets [ ] = {
			Unreal::Offsets::UGameViewportClient::World,
			0x78,
			0x80,
			0x88,
			0x70,
		};

		for ( const u64 Offset : CandidateOffsets ) {
			if ( Offset == 0 )
				continue;

			uptr WorldPtr = 0;
			if ( !ReadPtr( ViewportPtr + Offset, WorldPtr ) || !Unreal::ValidAddress( WorldPtr ) )
				continue;

			auto* World = reinterpret_cast< UWorld* >( WorldPtr );
			if ( !World || !World->IsValid( ) )
				continue;

			uptr GameInstancePtr = 0;
			if ( !ReadPtr( WorldPtr + Unreal::Offsets::UWorld::OwningGameInstance, GameInstancePtr ) || !Unreal::ValidAddress( GameInstancePtr ) )
				continue;
			auto* GameInstance = reinterpret_cast< UGameInstance* >( GameInstancePtr );
			if ( !GameInstance || !GameInstance->IsValid( ) )
				continue;

			OutWorld = WorldPtr;
			if ( OutResolvedOffset )
				*OutResolvedOffset = Offset;
			if ( CommitOffset && Unreal::Offsets::UGameViewportClient::World != Offset ) {
				Unreal::Offsets::UGameViewportClient::World = Offset;
				logger::Log( "[engine] resolved viewport->world off=0x%llx",
					static_cast< unsigned long long >( Offset ) );
			}
			return true;
		}

		return false;
	}

	bool TryResolveLocalPlayerControllerPtr( uptr LocalPlayerPtr, uptr& OutPlayerController, bool CommitOffset = true, u64* OutResolvedOffset = nullptr ) {
		OutPlayerController = 0;
		if ( OutResolvedOffset )
			*OutResolvedOffset = 0;
		if ( !Unreal::ValidAddress( LocalPlayerPtr ) )
			return false;

		const u64 CandidateOffsets [ ] = {
			Unreal::Offsets::UPlayer::PlayerController,
			0x30,
			0x38,
			0x40,
		};

		for ( const u64 Offset : CandidateOffsets ) {
			if ( Offset == 0 )
				continue;

			uptr PlayerControllerPtr = 0;
			if ( !ReadPtr( LocalPlayerPtr + Offset, PlayerControllerPtr ) || !Unreal::ValidAddress( PlayerControllerPtr ) )
				continue;

			auto* PlayerController = reinterpret_cast< APlayerController* >( PlayerControllerPtr );
			if ( !PlayerController || !PlayerController->IsValid( ) )
				continue;

			OutPlayerController = PlayerControllerPtr;
			if ( OutResolvedOffset )
				*OutResolvedOffset = Offset;
			if ( CommitOffset && Unreal::Offsets::UPlayer::PlayerController != Offset ) {
				Unreal::Offsets::UPlayer::PlayerController = Offset;
				logger::Log( "[engine] resolved localplayer->pc off=0x%llx",
					static_cast< unsigned long long >( Offset ) );
			}
			return true;
		}

		return false;
	}

	bool LooksLikeEngineChain( uptr EnginePtr ) {
		if ( !Unreal::ValidAddress( EnginePtr ) )
			return false;

		uptr Viewport = 0;
		if ( !ReadPtr( EnginePtr + Unreal::Offsets::UEngine::GameViewport, Viewport ) || !Unreal::ValidAddress( Viewport ) )
			return false;

		uptr World = 0;
		if ( !TryResolveViewportWorldPtr( Viewport, World, false ) )
			return false;

		uptr GameInstance = 0;
		if ( !ReadPtr( World + Unreal::Offsets::UWorld::OwningGameInstance, GameInstance ) || !Unreal::ValidAddress( GameInstance ) )
			return false;

		auto* GameInstanceObj = reinterpret_cast< UGameInstance* >( GameInstance );
		if ( !GameInstanceObj || !GameInstanceObj->IsValid( ) )
			return false;

		const auto LocalPlayers = GameInstanceObj->LocalPlayers( );
		if ( !LocalPlayers.IsValidIndex( 0 ) )
			return false;

		ULocalPlayer* LocalPlayer = GameInstanceObj->ReadElement( LocalPlayers, 0 );
		if ( !LocalPlayer || !LocalPlayer->IsValid( ) )
			return false;

		uptr PlayerController = 0;
		if ( !TryResolveLocalPlayerControllerPtr( reinterpret_cast< uptr >( LocalPlayer ), PlayerController, false ) )
			return false;

		return true;
	}

	bool TryResolveGEngineOffset( uptr ImageBase, u64& OutOffset ) {
		if ( !Unreal::ReadMemory || !ImageBase )
			return false;

		IMAGE_DOS_HEADER Dos {};
		if ( !ReadRaw( ImageBase, &Dos, sizeof( Dos ) ) ) {
			logger::Log( "[engine] gengine fallback failed: cannot read DOS header at 0x%llx",
				static_cast< unsigned long long >( ImageBase ) );
			return false;
		}
		if ( Dos.e_magic != IMAGE_DOS_SIGNATURE ) {
			logger::Log( "[engine] gengine fallback failed: bad DOS magic=0x%x at 0x%llx",
				static_cast< unsigned >( Dos.e_magic ),
				static_cast< unsigned long long >( ImageBase ) );
			return false;
		}

		IMAGE_NT_HEADERS64 Nt {};
		const uptr NtAddress = ImageBase + static_cast< uptr >( Dos.e_lfanew );
		if ( !ReadRaw( NtAddress, &Nt, sizeof( Nt ) ) || Nt.Signature != IMAGE_NT_SIGNATURE ) {
			logger::Log( "[engine] gengine fallback failed: invalid NT header" );
			return false;
		}

		const u16 SectionCount = Nt.FileHeader.NumberOfSections;
		if ( SectionCount == 0 || SectionCount > 96 ) {
			logger::Log( "[engine] gengine fallback failed: bad section count=%u", static_cast< unsigned >( SectionCount ) );
			return false;
		}

		const uptr SectionTableAddress = NtAddress + sizeof( u32 ) + sizeof( IMAGE_FILE_HEADER ) + Nt.FileHeader.SizeOfOptionalHeader;
		std::vector<IMAGE_SECTION_HEADER> Sections( SectionCount );
		if ( !ReadRaw( SectionTableAddress, Sections.data(), static_cast< u64 >( sizeof( IMAGE_SECTION_HEADER ) ) * SectionCount ) ) {
			logger::Log( "[engine] gengine fallback failed: cannot read section table" );
			return false;
		}

		constexpr u64 BlockSize = 0x2000;
		logger::Log( "[engine] gengine fallback scanning data sections" );

		for ( const auto& Section : Sections ) {
			const bool Readable = ( Section.Characteristics & IMAGE_SCN_MEM_READ ) != 0;
			const bool Executable = ( Section.Characteristics & IMAGE_SCN_MEM_EXECUTE ) != 0;
			if ( !Readable || Executable )
				continue;

			u64 SectionSize = static_cast< u64 >( Section.Misc.VirtualSize );
			if ( SectionSize == 0 )
				SectionSize = static_cast< u64 >( Section.SizeOfRawData );
			if ( SectionSize < sizeof( uptr ) )
				continue;

			if ( SectionSize > 0x08000000ull )
				continue;

			char SectionName[ 9 ] {};
			std::memcpy( SectionName, Section.Name, 8 );

			const u64 SectionOffset = static_cast< u64 >( Section.VirtualAddress );
			logger::Log( "[engine] scanning section %s off=0x%llx size=0x%llx",
				SectionName,
				static_cast< unsigned long long >( SectionOffset ),
				static_cast< unsigned long long >( SectionSize ) );

			for ( u64 Relative = 0; Relative < SectionSize; Relative += BlockSize ) {
				const u64 Remaining = SectionSize - Relative;
				const u64 ToRead = Remaining < BlockSize ? Remaining : BlockSize;

				u8 Block[ BlockSize ] {};
				if ( !ReadRaw( ImageBase + SectionOffset + Relative, Block, ToRead ) )
					continue;

				for ( u64 Index = 0; Index + sizeof( uptr ) <= ToRead; Index += sizeof( uptr ) ) {
					uptr Candidate = 0;
					std::memcpy( &Candidate, Block + Index, sizeof( Candidate ) );
					if ( !Unreal::ValidAddress( Candidate ) )
						continue;

					if ( LooksLikeEngineChain( Candidate ) ) {
						OutOffset = SectionOffset + Relative + Index;
						logger::Log( "[engine] gengine fallback hit off=0x%llx ptr=0x%llx",
							static_cast< unsigned long long >( OutOffset ),
							static_cast< unsigned long long >( Candidate ) );
						return true;
					}
				}
			}
		}

		logger::Log( "[engine] gengine fallback failed: no valid chain found" );
		return false;
	}

	bool TryDecryptWorldAddress( uptr ImageBase, uptr& OutWorld ) {
		OutWorld = 0;
		if ( !ImageBase )
			return false;

		constexpr u64 EncWorldOffset = 0x1B2C5BA0ull;
		constexpr u64 DecryptMul = 0x6501B96661E130DDull;
		constexpr u64 DecryptAdd = 0x79D95BD19230E74Dull;

		static u32 DecryptFailCount = 0;

		u64 EncodedWorld = 0;
		if ( !ReadRaw( ImageBase + EncWorldOffset, &EncodedWorld, sizeof( EncodedWorld ) ) ) {
			if ( ( DecryptFailCount++ % 1500 ) == 0 ) {
				logger::Log( "[engine] decrypt UWorld read failed addr=0x%llx",
					static_cast< unsigned long long >( ImageBase + EncWorldOffset ) );
			}
			return false;
		}

		const uptr DecryptedWorld = static_cast< uptr >( DecryptMul * EncodedWorld + DecryptAdd );
		if ( !Unreal::ValidAddress( DecryptedWorld ) ) {
			if ( ( DecryptFailCount++ % 1500 ) == 0 ) {
				logger::Log( "[engine] decrypt UWorld invalid enc=0x%llx dec=0x%llx",
					static_cast< unsigned long long >( EncodedWorld ),
					static_cast< unsigned long long >( DecryptedWorld ) );
			}
			return false;
		}

		DecryptFailCount = 0;
		OutWorld = DecryptedWorld;
		return true;
	}

}


void PlayerCache::SetImageBase( uptr ImageBase ) { m_ImageBase.store( ImageBase, std::memory_order_release ); }
uptr PlayerCache::ImageBase( ) const { return m_ImageBase.load( std::memory_order_acquire ); }
UWorld* PlayerCache::World( ) const { return m_World.load( std::memory_order_acquire ); }
u8 PlayerCache::LocalTeamIndex( ) const { return m_LocalTeamIndex.load( std::memory_order_relaxed ); }

std::vector<CachedPlayer> PlayerCache::Snapshot( ) const {
	std::lock_guard<std::mutex> lk( m_PlayerMutex );
	return m_Players;
}

std::size_t PlayerCache::Count( ) const {
	std::lock_guard<std::mutex> lk( m_PlayerMutex );
	return m_Players.size( );
}

void PlayerCache::SetCameraLocation( const FVector& CameraLocation ) {
	m_CamX.store( CameraLocation.X, std::memory_order_relaxed );
	m_CamY.store( CameraLocation.Y, std::memory_order_relaxed );
	m_CamZ.store( CameraLocation.Z, std::memory_order_relaxed );
}

void PlayerCache::Start( ) {
	if ( m_Running.exchange( true ) )
		return;
	m_EngineThread = std::thread( [ this ] { EngineLoop( ); } );
	m_ActorsThread = std::thread( [ this ] { ActorsLoop( ); } );
	m_PlayersThread = std::thread( [ this ] { PlayersLoop( ); } );
}

void PlayerCache::Stop( ) {
	if ( !m_Running.exchange( false ) )
		return;
	if ( m_EngineThread.joinable( ) ) m_EngineThread.join( );
	if ( m_ActorsThread.joinable( ) ) m_ActorsThread.join( );
	if ( m_PlayersThread.joinable( ) ) m_PlayersThread.join( );

	std::lock_guard<std::mutex> a( m_ActorMutex );
	std::lock_guard<std::mutex> p( m_PlayerMutex );
	m_Actors.clear( );
	m_Players.clear( );
}

void PlayerCache::EngineLoop( ) {
	while ( m_Running.load( std::memory_order_acquire ) ) {
		try { ResolveEngineSnapshot( ); }
		catch ( ... ) {}
		std::this_thread::sleep_for( 2ms );
	}
}

void PlayerCache::ActorsLoop( ) {
	while ( m_Running.load( std::memory_order_acquire ) ) {
		try { ScanActors( ); }
		catch ( ... ) {}
		std::this_thread::sleep_for( 100ms );
	}
}

void PlayerCache::PlayersLoop( ) {
	while ( m_Running.load( std::memory_order_acquire ) ) {
		try { ResolvePlayers( ); }
		catch ( ... ) {}
		std::this_thread::sleep_for( 4ms );
	}
}

void PlayerCache::ResolveEngineSnapshot( ) {
	static auto LastHealthyChain = std::chrono::steady_clock::time_point {};
	static u32 ConsecutiveEngineInvalid = 0;

	const uptr ImageBase = m_ImageBase.load( std::memory_order_acquire );
	if ( !ImageBase ) {
		LogEngineStage( EngineStage::NoImageBase );
		return;
	}

	UEngine* Engine = UEngine::Get( ImageBase );
	UGameViewportClient* Viewport = nullptr;
	UWorld* World = nullptr;
	bool UsedDecryptedWorld = false;
	u64 ResolvedWorldOffset = 0;
	u64 ResolvedPlayerControllerOffset = 0;

	const auto Now = std::chrono::steady_clock::now();
	if ( Engine && Engine->IsValid( ) )
		ConsecutiveEngineInvalid = 0;
	else
		++ConsecutiveEngineInvalid;

	if ( !Engine || !Engine->IsValid( ) ) {
		static auto LastGEngineFallbackAttempt = std::chrono::steady_clock::time_point {};
		static u64 PendingGEngineOffset = 0;
		static u32 PendingGEngineHits = 0;
		const bool RecentlyHealthy =
			LastHealthyChain.time_since_epoch().count() != 0 &&
			( Now - LastHealthyChain ) < std::chrono::seconds( 15 );
		const bool ProlongedEngineFailure = ConsecutiveEngineInvalid >= 2000;
		if ( ProlongedEngineFailure && !RecentlyHealthy &&
			( LastGEngineFallbackAttempt.time_since_epoch().count() == 0 || ( Now - LastGEngineFallbackAttempt ) >= std::chrono::seconds( 10 ) ) ) {
			LastGEngineFallbackAttempt = Now;
			u64 ResolvedOffset = 0;
			if ( TryResolveGEngineOffset( ImageBase, ResolvedOffset ) ) {
				if ( ResolvedOffset == Unreal::Offsets::GEngine ) {
					PendingGEngineOffset = 0;
					PendingGEngineHits = 0;
				}
				else if ( PendingGEngineOffset != ResolvedOffset ) {
					PendingGEngineOffset = ResolvedOffset;
					PendingGEngineHits = 1;
					logger::Log( "[engine] gengine candidate off=0x%llx (1/3)",
						static_cast< unsigned long long >( ResolvedOffset ) );
				}
				else {
					++PendingGEngineHits;
					logger::Log( "[engine] gengine candidate off=0x%llx (%u/3)",
						static_cast< unsigned long long >( ResolvedOffset ),
						static_cast< unsigned >( PendingGEngineHits ) );
				}

				if ( PendingGEngineOffset == ResolvedOffset && PendingGEngineHits >= 3 ) {
					Unreal::Offsets::GEngine = ResolvedOffset;
					PendingGEngineOffset = 0;
					PendingGEngineHits = 0;
					logger::Log( "[engine] using resolved gengine_off=0x%llx",
						static_cast< unsigned long long >( Unreal::Offsets::GEngine ) );
				}

				Engine = UEngine::Get( ImageBase );
			}
		}

		if ( !Engine || !Engine->IsValid( ) ) {
			uptr DecryptedWorldAddress = 0;
			if ( TryDecryptWorldAddress( ImageBase, DecryptedWorldAddress ) ) {
				World = reinterpret_cast< UWorld* >( DecryptedWorldAddress );
				if ( World && World->IsValid( ) ) {
					UsedDecryptedWorld = true;
					logger::Log( "[engine] using decrypted UWorld ptr=0x%llx",
						static_cast< unsigned long long >( DecryptedWorldAddress ) );
				}
			}

			if ( !UsedDecryptedWorld ) {
				static u32 BootstrapFailCount = 0;
				if ( ( BootstrapFailCount++ % 1500 ) == 0 ) {
					logger::Log( "[engine] bootstrap hard-fail: cannot resolve GEngine and decrypt UWorld (driver read path or offsets invalid)" );
				}
				LogEngineStage( EngineStage::EngineInvalid, ImageBase, reinterpret_cast< uptr >( Engine ) );
				return;
			}
		}
	}

	if ( !UsedDecryptedWorld ) {
		Viewport = Engine->GameViewport( );
		if ( !Viewport || !Viewport->IsValid( ) ) {
			LogEngineStage( EngineStage::ViewportInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ) );
			return;
		}

		uptr WorldPtr = 0;
		if ( !TryResolveViewportWorldPtr( reinterpret_cast< uptr >( Viewport ), WorldPtr, false, &ResolvedWorldOffset ) ) {
			LogEngineStage( EngineStage::WorldInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), 0 );
			return;
		}

		World = reinterpret_cast< UWorld* >( WorldPtr );
		if ( !World || !World->IsValid( ) ) {
			LogEngineStage( EngineStage::WorldInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
			return;
		}
	}

	UGameInstance* GameInstance = World->OwningGameInstance( );
	if ( !GameInstance || !GameInstance->IsValid( ) ) {
		LogEngineStage( EngineStage::GameInstanceInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
		return;
	}

	const auto LocalPlayers = GameInstance->LocalPlayers( );
	if ( !LocalPlayers.IsValidIndex( 0 ) ) {
		LogEngineStage( EngineStage::LocalPlayersInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
		return;
	}

	ULocalPlayer* LocalPlayer = GameInstance->ReadElement( LocalPlayers, 0 );
	if ( !LocalPlayer || !LocalPlayer->IsValid( ) ) {
		LogEngineStage( EngineStage::LocalPlayerInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
		return;
	}

	uptr PlayerControllerPtr = 0;
	if ( !TryResolveLocalPlayerControllerPtr( reinterpret_cast< uptr >( LocalPlayer ), PlayerControllerPtr, false, &ResolvedPlayerControllerOffset ) ) {
		LogEngineStage( EngineStage::PlayerControllerInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
		return;
	}

	APlayerController* PlayerController = reinterpret_cast< APlayerController* >( PlayerControllerPtr );
	if ( !PlayerController || !PlayerController->IsValid( ) ) {
		LogEngineStage( EngineStage::PlayerControllerInvalid, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
		return;
	}

	if ( !UsedDecryptedWorld && ResolvedWorldOffset != 0 && Unreal::Offsets::UGameViewportClient::World != ResolvedWorldOffset ) {
		Unreal::Offsets::UGameViewportClient::World = ResolvedWorldOffset;
		logger::Log( "[engine] resolved viewport->world off=0x%llx",
			static_cast< unsigned long long >( ResolvedWorldOffset ) );
	}

	if ( ResolvedPlayerControllerOffset != 0 && Unreal::Offsets::UPlayer::PlayerController != ResolvedPlayerControllerOffset ) {
		Unreal::Offsets::UPlayer::PlayerController = ResolvedPlayerControllerOffset;
		logger::Log( "[engine] resolved localplayer->pc off=0x%llx",
			static_cast< unsigned long long >( ResolvedPlayerControllerOffset ) );
	}

	AFortPlayerPawnAthena* LocalPawn =
		static_cast< AFortPlayerPawnAthena* >( PlayerController->AcknowledgedPawn( ) );
	if ( LocalPawn && !LocalPawn->IsValid( ) )
		LocalPawn = nullptr;

	u8 LocalTeam = 0;
	AFortPlayerStateAthena* LocalState = nullptr;
	if ( LocalPawn ) {
		LocalState = static_cast< AFortPlayerStateAthena* >( LocalPawn->PlayerState( ) );
		if ( LocalState && LocalState->IsValid( ) )
			LocalTeam = LocalState->TeamIndex( );
		else
			LocalState = nullptr;
	}

	m_World.store( World, std::memory_order_release );
	m_GameInstance.store( GameInstance, std::memory_order_release );
	m_PlayerController.store( PlayerController, std::memory_order_release );
	m_LocalPawn.store( LocalPawn, std::memory_order_release );
	m_LocalPlayerState.store( LocalState, std::memory_order_release );
	m_LocalTeamIndex.store( LocalTeam, std::memory_order_relaxed );
	m_WorldTimeSeconds.store( World->TimeSeconds( ), std::memory_order_relaxed );
	LastHealthyChain = std::chrono::steady_clock::now();
	ConsecutiveEngineInvalid = 0;
	LogEngineStage( EngineStage::Ok, ImageBase, reinterpret_cast< uptr >( Engine ), reinterpret_cast< uptr >( Viewport ), reinterpret_cast< uptr >( World ) );
}

void PlayerCache::ScanActors( ) {
	UWorld* World = m_World.load( std::memory_order_acquire );
	if ( !World || !World->IsValid( ) )
		return;

	std::vector<AFortPlayerPawnAthena*> found;
	found.reserve( 64 );

	AGameStateBase* GameState = nullptr;
	if ( TryResolveGameStatePtr( World, GameState ) ) {
		const auto PlayerStates = GameState->PlayerArray( );
		if ( PlayerStates.IsValid( ) && PlayerStates.Num > 0 && PlayerStates.Num <= 256 ) {
			GameState->ForEach( PlayerStates, 256, [ & ]( APlayerState* PlayerState ) {
				if ( !PlayerState || !PlayerState->IsValid( ) )
					return;

				AFortPlayerPawnAthena* Pawn = nullptr;
				if ( !TryResolvePawnFromPlayerState( PlayerState, Pawn ) || !Pawn )
					return;

				if ( !Pawn->IsPlayerPawn( ) )
					return;
				for ( auto* Existing : found ) {
					if ( Existing == Pawn )
						return;
				}

				found.push_back( Pawn );
			} );
		}
	}

	if ( !found.empty( ) ) {
		std::lock_guard<std::mutex> lk( m_ActorMutex );
		m_Actors = std::move( found );
		return;
	}

	const auto Levels = World->Levels( );
	if ( !Levels.IsValid( ) || Levels.Num > MaxLevels )
		return;

	World->ForEach( Levels, MaxLevels, [ & ]( ULevel* Level ) {
		if ( !Level || !Level->IsValid( ) )
			return;
		const auto Actors = Level->Actors( );
		if ( !Actors.IsValid( ) || Actors.Num > MaxActorsPerLevel )
			return;
		Level->ForEach( Actors, MaxActorsPerLevel, [ & ]( AActor* Actor ) {
			if ( !Actor || !Actor->IsValid( ) )
				return;
			auto* Pawn = static_cast< AFortPlayerPawnAthena* >( Actor );
			if ( Pawn->IsPlayerPawn( ) )
			{
				APlayerState* PlayerState = Pawn->PlayerState( );
				if ( !PlayerState || !PlayerState->IsValid( ) )
					return;

				AFortPlayerPawnAthena* ResolvedPawn = nullptr;
				if ( !TryResolvePawnFromPlayerState( PlayerState, ResolvedPawn, false ) )
					return;
				if ( ResolvedPawn != Pawn )
					return;

				found.push_back( Pawn );
			}
		} );
	} );

	if ( found.size( ) > 128 ) {
		static u32 ExcessActorScanCount = 0;
		if ( ( ExcessActorScanCount++ % 1500 ) == 0 ) {
			logger::Log( "[engine] actor scan rejected: implausible pawn count=%u",
				static_cast< unsigned >( found.size( ) ) );
		}
		return;
	}

	std::lock_guard<std::mutex> lk( m_ActorMutex );
	m_Actors = std::move( found );
}

void PlayerCache::ResolvePlayers( ) {
	std::vector<AFortPlayerPawnAthena*> actors;
	{
		std::lock_guard<std::mutex> lk( m_ActorMutex );
		actors = m_Actors;
	}

	if ( actors.empty( ) ) {
		std::lock_guard<std::mutex> lk( m_PlayerMutex );
		m_Players.clear( );
		return;
	}

	FVector cam {};
	cam.X = m_CamX.load( std::memory_order_relaxed );
	cam.Y = m_CamY.load( std::memory_order_relaxed );
	cam.Z = m_CamZ.load( std::memory_order_relaxed );

	AFortPlayerPawnAthena* localPawn = m_LocalPawn.load( std::memory_order_acquire );
	AFortPlayerStateAthena* localPlayerState = m_LocalPlayerState.load( std::memory_order_acquire );

	std::vector<CachedPlayer> resolved;
	resolved.reserve( actors.size( ) );
	bool anyLocal = false;

	for ( auto* pawn : actors ) {
		if ( !pawn )
			continue;
		CachedPlayer p = ResolveOne( pawn, cam );
		if ( pawn == localPawn || ( localPlayerState && p.PlayerState == localPlayerState ) ) {
			p.bIsLocal = true;
			anyLocal = true;
		}
		resolved.push_back( std::move( p ) );
	}

	// Lobby fallback: if only one pawn is tracked and local identity is unresolved, treat it as local.
	if ( !anyLocal && resolved.size( ) == 1 )
		resolved [ 0 ].bIsLocal = true;

	std::lock_guard<std::mutex> lk( m_PlayerMutex );
	m_Players = std::move( resolved );
}

CachedPlayer PlayerCache::ResolveOne( AFortPlayerPawnAthena* Pawn, const FVector& CameraLoc ) const {
	CachedPlayer p {};
	p.Pawn = Pawn;
	p.Mesh = Pawn->Mesh( );
	p.PlayerState = static_cast< AFortPlayerStateAthena* >( Pawn->PlayerState( ) );
	p.Location = Pawn->Location( );
	p.bIsDying = Pawn->bIsDying( );
	p.bIsDBNO = Pawn->bIsDBNO( );
	p.bHasWeapon = Pawn->CurrentWeapon( ) != nullptr;

	if ( p.PlayerState && p.PlayerState->IsValid( ) ) {
		p.TeamIndex = p.PlayerState->TeamIndex( );
		p.bIsBot = p.PlayerState->bIsABot( );
	}

	if ( CameraLoc.IsValid( ) && p.Location.IsValid( ) )
		p.Distance = static_cast< float >( CameraLoc.DistanceTo( p.Location ) );

	if ( p.Mesh && p.Mesh->IsValid( ) ) {
		FVector bones [ 16 ] {};
		if ( p.Mesh->ReadBoneLocations( g_ESPBones, g_ESPBoneCount, bones ) ) {
			for ( i32 i = 0; i < g_ESPBoneCount; ++i )
				p.Bones [ i ] = bones [ i ];
			p.BonesValid = true;
		}
		const float wt = m_WorldTimeSeconds.load( std::memory_order_relaxed );
		if ( wt > 0.0f )
			p.bIsVisible = p.Mesh->IsRecentlyRendered( wt );
	}

	return p;
}
