#pragma once
#include <thirdparty/imgui/imgui.h>

class PlayerCache;
class CameraCache;

namespace player {

	struct Config {
		int      BoxStyle;
		bool     BoxFill;
		float    BoxColor[ 4 ];
		bool     Distance;
		bool     Skeleton;
		float    SkeletonColor[ 4 ];
		bool     Snaplines;
		int      SnaplineOrigin;
		float    SnaplineColor[ 4 ];
		ImFont*  EspFont;
	};

	void Draw(
		const PlayerCache& Players,
		const CameraCache& Camera,
		ImDrawList* DrawList,
		float Width,
		float Height,
		const Config& Cfg
	);

}
