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

	struct RuntimeStatus {
		bool world_valid = false;
		bool camera_valid = false;
		int actor_count = 0;
		int player_count = 0;
		float fps = 0.0f;
		float frame_ms = 0.0f;
		float engine_ms = 0.0f;
		float actors_ms = 0.0f;
		float players_ms = 0.0f;
		float dpi_scale = 1.0f;
	};

	inline Settings cfg {};

	void setup( );
	void render( const RuntimeStatus& status );

}
