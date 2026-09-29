#pragma once
#include <thirdparty/imgui/imgui.h>

namespace menu {

	struct Settings {
		bool  box                 = true;
		int   box_style           = 0;
		bool  box_fill            = false;
		float box_color[ 4 ]      = { 1.0f, 0.27f, 0.27f, 1.0f };
		bool  skeleton            = true;
		float skeleton_color[ 4 ] = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool  distance            = true;
		bool  snaplines           = false;
		int   snapline_origin     = 0;
		float snapline_color[ 4 ] = { 1.0f, 1.0f, 1.0f, 0.8f };

		int   menu_key            = ImGuiKey_Insert;
		float accent[ 4 ]         = { 150 / 255.0f, 90 / 255.0f, 220 / 255.0f, 1.0f };
	};

	inline Settings cfg {};

	void setup( );
	void render( );

}
