#pragma once
#include <cstdio>

namespace logger {

	void Init( );

	void LineRaw( const char* text );

	template <typename... Args>
	void Log( const char* fmt, Args... args ) {
		char buf[ 1024 ];
		std::snprintf( buf, sizeof( buf ), fmt, args... );
		LineRaw( buf );
	}

}
