#include <includes.hpp>
#include <workspace/util/logger/logger.hpp>

#include <chrono>
#include <thread>

static PlayerCache g_Players;
static CameraCache g_Camera;

namespace {

	u32 FindPidByName( const wchar_t* Name ) {
		HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
		if ( snap == INVALID_HANDLE_VALUE )
			return 0;

		PROCESSENTRY32W e {};
		e.dwSize = sizeof( e );

		u32 pid = 0;
		if ( Process32FirstW( snap, &e ) ) {
			do {
				if ( _wcsicmp( e.szExeFile, Name ) == 0 ) {
					pid = e.th32ProcessID;
					break;
				}
			} while ( Process32NextW( snap, &e ) );
		}

		CloseHandle( snap );
		return pid;
	}

	u32 WaitForProcess( const wchar_t* Name, const char* PrettyName ) {
		if ( const auto pid = FindPidByName( Name ) ) {
			logger::Log( "%s already running (pid %u)", PrettyName, pid );
			return pid;
		}

		logger::Log( "waiting for %s...", PrettyName );

		u32 pid = 0;
		while ( !( pid = FindPidByName( Name ) ) )
			std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

		logger::Log( "%s launched (pid %u)", PrettyName, pid );
		return pid;
	}

	void PressKeyToExit( ) {
		logger::Log( "press any key to exit..." );
		( void ) getchar( );
	}

}

i32 main( i32, char** ) {
	logger::Init( );
	SetConsoleTitleW( L"luvkrimes base" );

	constexpr const wchar_t* ProcessName = L"FortniteClient-Win64-Shipping.exe";

	WaitForProcess( ProcessName, "fortnite" );

	logger::Log( "attaching driver" );

	uptr ImageBase = 0;
	if ( !AttachDriver( ProcessName, &ImageBase ) ) {
		logger::Log( "attach failed (win err %lu)", GetLastError( ) );
		logger::Log( "check that the luvkrimes driver is loaded and mapped" );
		PressKeyToExit( );
		return 1;
	}

	logger::Log( "attached, image 0x%llx", static_cast< unsigned long long >( ImageBase ) );

	g_Players.SetImageBase( ImageBase );
	g_Players.Start( );
	logger::Log( "cache threads running" );

	logger::Log( "starting overlay" );
	const bool ok = overlay::run( g_Players, g_Camera );

	g_Players.Stop( );

	if ( !ok ) {
		logger::Log( "overlay exited with error" );
		PressKeyToExit( );
		return 1;
	}

	logger::Log( "clean exit" );
	return 0;
}
