#pragma once
#include <Windows.h>

class PlayerCache;
class CameraCache;

namespace overlay {

	HWND hijack( );
	bool run( PlayerCache& Players, CameraCache& Camera );

}
