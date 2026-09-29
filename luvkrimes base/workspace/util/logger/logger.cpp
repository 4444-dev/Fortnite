#include "logger.hpp"

#include <Windows.h>
#include <cstdio>

namespace logger {

	void Init( ) {
		const HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( h == INVALID_HANDLE_VALUE )
			return;
		DWORD mode = 0;
		if ( !GetConsoleMode( h, &mode ) )
			return;
		SetConsoleMode( h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
	}

	void LineRaw( const char* text ) {
		SYSTEMTIME t;
		GetLocalTime( &t );
		std::printf( "[%02d:%02d:%02d] %s\n", t.wHour, t.wMinute, t.wSecond, text );
	}

}
