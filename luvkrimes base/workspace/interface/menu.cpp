#define IMGUI_DEFINE_MATH_OPERATORS

#include <workspace/interface/menu.hpp>
#include <workspace/interface/settings_store.hpp>
#include <thirdparty/imgui/imgui_internal.h>

#include <Windows.h>
#include <cctype>
#include <cstdio>

using menu::cfg;

namespace {

	const menu::Settings cfg_default {};

	ImFont*         font      = nullptr;
	constexpr float font_size = 12.0f;

	namespace col {
		const ImU32 black      = IM_COL32(   0,   0,   0, 255 );
		const ImU32 window     = IM_COL32(  17,  17,  17, 255 );
		const ImU32 edge       = IM_COL32(  48,  48,  48, 255 );
		const ImU32 group      = IM_COL32(  22,  22,  22, 255 );
		const ImU32 group_edge = IM_COL32(  40,  40,  40, 255 );
		const ImU32 ctrl_top   = IM_COL32(  44,  44,  44, 255 );
		const ImU32 ctrl_bot   = IM_COL32(  33,  33,  33, 255 );
		const ImU32 ctrl_hover = IM_COL32(  54,  54,  54, 255 );
		const ImU32 off_top    = IM_COL32(  72,  72,  72, 255 );
		const ImU32 off_bot    = IM_COL32(  50,  50,  50, 255 );
		const ImU32 off_hover  = IM_COL32(  90,  90,  90, 255 );
		const ImU32 text       = IM_COL32( 200, 200, 200, 255 );
		const ImU32 bright     = IM_COL32( 240, 240, 240, 255 );
		const ImU32 dim        = IM_COL32( 115, 115, 115, 255 );
	}

	constexpr float indent = 20.0f;

	int tab = 0;

	ImU32 accent( float shade = 1.0f ) {
		return ImGui::ColorConvertFloat4ToU32( ImVec4( cfg.accent[ 0 ] * shade, cfg.accent[ 1 ] * shade, cfg.accent[ 2 ] * shade, 1.0f ) );
	}

	ImVec2 text_size( const char* s ) {
		return font->CalcTextSizeA( font_size, FLT_MAX, 0.0f, s );
	}

	void text( ImDrawList* dl, ImVec2 p, ImU32 c, const char* s ) {
		dl->AddText( font, font_size, ImFloor( p ), c, s );
	}

	float text_y( float top, float h ) {
		return top + ImFloor( ( h - font_size ) * 0.5f ) - 1.0f;
	}

	void frame( ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 top, ImU32 bottom ) {
		dl->AddRectFilledMultiColor( a, b, top, top, bottom, bottom );
		dl->AddRect( a - ImVec2( 1, 1 ), b + ImVec2( 1, 1 ), col::black );
	}

	void begin_popup_style( ImVec2 padding ) {
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, padding );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
		ImGui::PushStyleColor( ImGuiCol_PopupBg, col::group );
		ImGui::PushStyleColor( ImGuiCol_Border, col::black );
	}

	void end_popup_style( ) {
		ImGui::PopStyleColor( 2 );
		ImGui::PopStyleVar( 2 );
	}

	void swatch( ImDrawList* dl, const ImRect& r, const float* c, bool hovered ) {
		const ImVec4 top( c[ 0 ], c[ 1 ], c[ 2 ], 1.0f );
		const ImVec4 bot( c[ 0 ] * 0.75f, c[ 1 ] * 0.75f, c[ 2 ] * 0.75f, 1.0f );
		frame( dl, r.Min, r.Max, ImGui::ColorConvertFloat4ToU32( top ), ImGui::ColorConvertFloat4ToU32( bot ) );
		if ( hovered )
			dl->AddRect( r.Min, r.Max, IM_COL32( 255, 255, 255, 60 ) );
	}

	void color_popup( const char* popup_id, float* c, ImVec2 anchor ) {
		ImGui::SetNextWindowPos( anchor, ImGuiCond_Appearing );
		begin_popup_style( ImVec2( 6, 6 ) );
		if ( ImGui::BeginPopup( popup_id, ImGuiWindowFlags_NoMove ) ) {
			ImGui::SetNextItemWidth( 170.0f );
			ImGui::ColorPicker4( "##picker", c,
				ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
				ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar );
			ImGui::EndPopup( );
		}
		end_popup_style( );
	}

	bool checkbox( const char* label, bool* v, float* color = nullptr ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		if ( win->SkipItems ) return false;

		const ImGuiID id = win->GetID( label );
		const float   w  = ImGui::GetContentRegionAvail( ).x;
		const float   h  = 18.0f;
		const ImVec2  p  = win->DC.CursorPos;
		const ImRect  bb( p, p + ImVec2( w, h ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		const ImRect sw( ImVec2( bb.Max.x - 18.0f, p.y + 5.0f ), ImVec2( bb.Max.x, p.y + 13.0f ) );
		const bool over_swatch = color && ( !v || sw.Contains( ImGui::GetIO( ).MousePos ) );

		bool hovered, held;
		const bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held );

		char popup_id[ 32 ];
		ImFormatString( popup_id, IM_ARRAYSIZE( popup_id ), "##clr_%08X", id );

		bool changed = false;
		if ( pressed ) {
			if ( over_swatch )
				ImGui::OpenPopup( popup_id );
			else if ( v ) {
				*v = !*v;
				changed = true;
			}
		}

		ImDrawList* dl = win->DrawList;
		const bool box_hover = hovered && !over_swatch;

		if ( v ) {
			const ImVec2 b0( p.x + 2.0f, p.y + 5.0f );
			const ImVec2 b1 = b0 + ImVec2( 8.0f, 8.0f );
			if ( *v ) frame( dl, b0, b1, accent( ), accent( 0.6f ) );
			else      frame( dl, b0, b1, box_hover ? col::off_hover : col::off_top, col::off_bot );
		}

		text( dl, ImVec2( p.x + indent, text_y( p.y, h ) ), box_hover ? col::bright : col::text, label );

		if ( color ) {
			swatch( dl, sw, color, hovered && over_swatch );
			color_popup( popup_id, color, ImVec2( sw.Min.x - 1.0f, sw.Max.y + 4.0f ) );
		}

		return changed;
	}

	bool list_item( const char* label, bool selected ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		const ImGuiID id = win->GetID( label );
		const float   w  = ImGui::GetContentRegionAvail( ).x;
		const ImVec2  p  = win->DC.CursorPos;
		const ImRect  bb( p, p + ImVec2( w, 18.0f ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		bool hovered, held;
		const bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held );

		ImDrawList* dl = win->DrawList;
		if ( hovered )
			dl->AddRectFilled( bb.Min, bb.Max, col::ctrl_hover );
		text( dl, ImVec2( p.x + 6.0f, text_y( p.y, 18.0f ) ), selected ? accent( ) : ( hovered ? col::bright : col::text ), label );
		return pressed;
	}

	bool combo( const char* label, int* v, const char* const* items, int count ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		if ( win->SkipItems ) return false;

		const ImGuiID id  = win->GetID( label );
		const bool    has_label = label[ 0 ] != '#';
		const float   lh  = has_label ? 16.0f : 0.0f;
		const float   w   = ImGui::GetContentRegionAvail( ).x;
		const ImVec2  p   = win->DC.CursorPos;
		const ImRect  bb( p, p + ImVec2( w, lh + 24.0f ) );
		const ImRect  box( ImVec2( p.x + indent, p.y + lh + 2.0f ), ImVec2( bb.Max.x, p.y + lh + 20.0f ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		char popup_id[ 64 ];
		ImFormatString( popup_id, IM_ARRAYSIZE( popup_id ), "##combo_%08X", id );

		bool hovered, held;
		if ( ImGui::ButtonBehavior( box, id, &hovered, &held ) && !ImGui::IsPopupOpen( popup_id ) )
			ImGui::OpenPopup( popup_id );
		const bool open = ImGui::IsPopupOpen( popup_id );

		ImDrawList* dl = win->DrawList;
		if ( has_label )
			text( dl, ImVec2( p.x + indent, p.y ), col::text, label );

		frame( dl, box.Min, box.Max, ( hovered || open ) ? col::ctrl_hover : col::ctrl_top, col::ctrl_bot );
		text( dl, ImVec2( box.Min.x + 6.0f, text_y( box.Min.y, box.GetHeight( ) ) ), col::text, items[ *v ] );

		const ImVec2 a( box.Max.x - 10.0f, box.GetCenter( ).y );
		if ( open ) dl->AddTriangleFilled( a + ImVec2( -3, 2 ), a + ImVec2( 3, 2 ), a + ImVec2( 0, -2 ), col::dim );
		else        dl->AddTriangleFilled( a + ImVec2( -3, -2 ), a + ImVec2( 3, -2 ), a + ImVec2( 0, 2 ), col::dim );

		bool changed = false;
		ImGui::SetNextWindowPos( ImVec2( box.Min.x, box.Max.y + 2.0f ) );
		ImGui::SetNextWindowSize( ImVec2( box.GetWidth( ), 0.0f ) );
		begin_popup_style( ImVec2( 1, 1 ) );
		if ( ImGui::BeginPopup( popup_id, ImGuiWindowFlags_NoMove ) ) {
			for ( int i = 0; i < count; ++i ) {
				ImGui::PushID( i );
				if ( list_item( items[ i ], *v == i ) ) {
					*v = i;
					changed = true;
					ImGui::CloseCurrentPopup( );
				}
				ImGui::PopID( );
			}
			ImGui::EndPopup( );
		}
		end_popup_style( );
		return changed;
	}

	bool slider( const char* label, float* v, float v_min, float v_max, const char* fmt ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		if ( win->SkipItems ) return false;

		const ImGuiID id = win->GetID( label );
		const float   w  = ImGui::GetContentRegionAvail( ).x;
		const ImVec2  p  = win->DC.CursorPos;
		const ImRect  bb( p, p + ImVec2( w, 38.0f ) );
		const ImRect  bar( ImVec2( p.x + indent, p.y + 19.0f ), ImVec2( bb.Max.x, p.y + 25.0f ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		bool hovered, held;
		const ImRect hit( bar.Min - ImVec2( 0, 4 ), bar.Max + ImVec2( 0, 4 ) );
		ImGui::ButtonBehavior( hit, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick );

		bool changed = false;
		if ( held ) {
			const float t = ImSaturate( ( ImGui::GetIO( ).MousePos.x - bar.Min.x ) / bar.GetWidth( ) );
			const float nv = v_min + t * ( v_max - v_min );
			if ( nv != *v ) {
				*v = nv;
				changed = true;
			}
		}

		ImDrawList* dl = win->DrawList;
		text( dl, ImVec2( p.x + indent, p.y ), col::text, label );

		const float t = ImSaturate( ( *v - v_min ) / ( v_max - v_min ) );
		const float fill_x = ImFloor( bar.Min.x + bar.GetWidth( ) * t );
		frame( dl, bar.Min, bar.Max, ( hovered || held ) ? col::ctrl_hover : col::ctrl_top, col::ctrl_bot );
		if ( fill_x > bar.Min.x )
			dl->AddRectFilledMultiColor( bar.Min, ImVec2( fill_x, bar.Max.y ), accent( ), accent( ), accent( 0.6f ), accent( 0.6f ) );

		char buf[ 32 ];
		ImFormatString( buf, IM_ARRAYSIZE( buf ), fmt, *v );
		const ImVec2 vs = text_size( buf );
		const float vx = ImClamp( fill_x - vs.x * 0.5f, bar.Min.x, bar.Max.x - vs.x );
		dl->AddText( font, font_size, ImFloor( ImVec2( vx + 1, bar.Max.y + 2 ) ), col::black, buf );
		dl->AddText( font, font_size, ImFloor( ImVec2( vx, bar.Max.y + 1 ) ), col::text, buf );

		return changed;
	}

	void key_name( int key, char* out, size_t size ) {
		const char* name = "none";
		switch ( key ) {
		case ImGuiKey_None:        break;
		case ImGuiKey_MouseRight:  name = "mouse2"; break;
		case ImGuiKey_MouseMiddle: name = "mouse3"; break;
		case ImGuiKey_MouseX1:     name = "mouse4"; break;
		case ImGuiKey_MouseX2:     name = "mouse5"; break;
		default:                   name = ImGui::GetKeyName( static_cast< ImGuiKey >( key ) ); break;
		}
		size_t i = 0;
		for ( ; name[ i ] && i + 1 < size; ++i )
			out[ i ] = static_cast< char >( std::tolower( static_cast< unsigned char >( name[ i ] ) ) );
		out[ i ] = '\0';
	}

	bool keybind( const char* label, int* key ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		if ( win->SkipItems ) return false;

		static ImGuiID waiting_id    = 0;
		static int     waiting_frame = 0;

		const ImGuiID id = win->GetID( label );
		const float   w  = ImGui::GetContentRegionAvail( ).x;
		const float   h  = 18.0f;
		const ImVec2  p  = win->DC.CursorPos;
		const ImRect  bb( p, p + ImVec2( w, h ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		bool hovered, held;
		if ( ImGui::ButtonBehavior( bb, id, &hovered, &held ) ) {
			waiting_id    = id;
			waiting_frame = ImGui::GetFrameCount( );
		}

		bool changed = false;
		if ( waiting_id == id && ImGui::GetFrameCount( ) > waiting_frame ) {
			if ( ImGui::IsKeyPressed( ImGuiKey_Escape, false ) || ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
				waiting_id = 0;
			else {
				for ( int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k ) {
					if ( k == ImGuiKey_MouseLeft || k == ImGuiKey_MouseWheelX || k == ImGuiKey_MouseWheelY )
						continue;
					if ( k >= ImGuiKey_ReservedForModCtrl && k <= ImGuiKey_ReservedForModSuper )
						continue;
					if ( ImGui::IsKeyPressed( static_cast< ImGuiKey >( k ), false ) ) {
						*key = k;
						waiting_id = 0;
						changed = true;
						break;
					}
				}
			}
		}
		const bool waiting = waiting_id == id;

		ImDrawList* dl = win->DrawList;
		text( dl, ImVec2( p.x + indent, text_y( p.y, h ) ), hovered ? col::bright : col::text, label );

		char name[ 32 ];
		key_name( *key, name, sizeof( name ) );
		char buf[ 40 ];
		ImFormatString( buf, IM_ARRAYSIZE( buf ), "[%s]", waiting ? "..." : name );
		const ImVec2 bs = text_size( buf );
		text( dl, ImVec2( bb.Max.x - bs.x, text_y( p.y, h ) ), waiting ? accent( ) : col::dim, buf );

		return changed;
	}

	bool button( const char* label ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		if ( win->SkipItems ) return false;

		const ImGuiID id = win->GetID( label );
		const float   w  = ImGui::GetContentRegionAvail( ).x;
		const ImVec2  p  = win->DC.CursorPos;
		const ImRect  bb( p, p + ImVec2( w, 26.0f ) );
		const ImRect  box( ImVec2( p.x + indent, p.y + 3.0f ), ImVec2( bb.Max.x, p.y + 23.0f ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		bool hovered, held;
		const bool pressed = ImGui::ButtonBehavior( box, id, &hovered, &held );

		ImDrawList* dl = win->DrawList;
		frame( dl, box.Min, box.Max, held ? col::ctrl_bot : ( hovered ? col::ctrl_hover : col::ctrl_top ), held ? col::ctrl_top : col::ctrl_bot );
		const ImVec2 ls = text_size( label );
		text( dl, ImVec2( box.GetCenter( ).x - ls.x * 0.5f, text_y( box.Min.y, box.GetHeight( ) ) ), hovered ? col::bright : col::text, label );
		return pressed;
	}

	void info( const char* label, const char* value ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		if ( win->SkipItems ) return;

		const float  w = ImGui::GetContentRegionAvail( ).x;
		const ImVec2 p = win->DC.CursorPos;
		const ImRect bb( p, p + ImVec2( w, 18.0f ) );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, 0 ) ) return;

		ImDrawList* dl = win->DrawList;
		text( dl, ImVec2( p.x + indent, text_y( p.y, 18.0f ) ), col::text, label );
		const ImVec2 vs = text_size( value );
		text( dl, ImVec2( bb.Max.x - vs.x, text_y( p.y, 18.0f ) ), col::dim, value );
	}

	void begin_group( const char* title, ImVec2 size ) {
		const ImVec2 p = ImGui::GetCursorScreenPos( );
		ImDrawList* dl = ImGui::GetWindowDrawList( );

		const ImVec2 a( p.x, p.y + 6.0f );
		const ImVec2 b = p + size;
		dl->AddRectFilled( a, b, col::group );
		dl->AddRect( a, b, col::black );
		dl->AddRect( a + ImVec2( 1, 1 ), b - ImVec2( 1, 1 ), col::group_edge );

		const ImVec2 ts = text_size( title );
		dl->AddRectFilled( ImVec2( a.x + 8.0f, a.y - 1.0f ), ImVec2( a.x + 14.0f + ts.x, a.y + 2.0f ), col::group );
		text( dl, ImVec2( a.x + 11.0f, a.y - ImFloor( font_size * 0.5f ) - 1.0f ), col::text, title );

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 14, 20 ) );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 2 ) );
		ImGui::BeginChild( title, size, ImGuiChildFlags_AlwaysUseWindowPadding,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground );
	}

	void end_group( ) {
		ImGui::EndChild( );
		ImGui::PopStyleVar( 2 );
	}

	void page_visuals( ImVec2 avail ) {
		const float col_w = ImFloor( ( avail.x - 12.0f ) * 0.5f );
		static const char* box_styles[ ] = { "full", "corner" };
		static const char* origins[ ]    = { "bottom", "center", "top" };

		begin_group( "players", ImVec2( col_w, avail.y ) );
		checkbox( "bounding box", &cfg.box, cfg.box_color );
		combo( "##box_style", &cfg.box_style, box_styles, IM_ARRAYSIZE( box_styles ) );
		checkbox( "box fill", &cfg.box_fill );
		checkbox( "skeleton", &cfg.skeleton, cfg.skeleton_color );
		checkbox( "distance", &cfg.distance );
		end_group( );

		ImGui::SameLine( 0, 12.0f );

		begin_group( "snaplines", ImVec2( avail.x - col_w - 12.0f, avail.y ) );
		checkbox( "enabled", &cfg.snaplines, cfg.snapline_color );
		combo( "origin", &cfg.snapline_origin, origins, IM_ARRAYSIZE( origins ) );
		end_group( );
	}

	void page_config( ImVec2 avail, const menu::RuntimeStatus& status ) {
		const float col_w = ImFloor( ( avail.x - 12.0f ) * 0.5f );

		begin_group( "menu", ImVec2( col_w, avail.y ) );
		keybind( "menu key", &cfg.menu_key );
		checkbox( "accent color", nullptr, cfg.accent );
		ImGui::Dummy( ImVec2( 0, 6 ) );
		if ( button( "save settings" ) )
			( void )app_settings::Save( cfg );
		if ( button( "load settings" ) )
			( void )app_settings::Load( cfg );
		if ( button( "restore defaults" ) )
			cfg = cfg_default;
		end_group( );

		ImGui::SameLine( 0, 12.0f );

		begin_group( "info", ImVec2( avail.x - col_w - 12.0f, avail.y ) );
		info( "build", "1.0" );
		info( "game", "fortnite" );
		info( "world", status.world_valid ? "ok" : "invalid" );
		info( "camera", status.camera_valid ? "ok" : "invalid" );

		char actors[ 16 ] {};
		std::snprintf( actors, sizeof( actors ), "%d", status.actor_count );
		info( "actors", actors );

		char players[ 16 ] {};
		std::snprintf( players, sizeof( players ), "%d", status.player_count );
		info( "players", players );

		char fps[ 16 ] {};
		std::snprintf( fps, sizeof( fps ), "%.0f", status.fps );
		info( "fps", fps );

		char frameMs[ 24 ] {};
		std::snprintf( frameMs, sizeof( frameMs ), "%.2f ms", status.frame_ms );
		info( "frame", frameMs );

		char engineMs[ 24 ] {};
		std::snprintf( engineMs, sizeof( engineMs ), "%.3f ms", status.engine_ms );
		info( "engine", engineMs );

		char actorsMs[ 24 ] {};
		std::snprintf( actorsMs, sizeof( actorsMs ), "%.3f ms", status.actors_ms );
		info( "actor scan", actorsMs );

		char playersMs[ 24 ] {};
		std::snprintf( playersMs, sizeof( playersMs ), "%.3f ms", status.players_ms );
		info( "player cache", playersMs );

		char dpiScale[ 24 ] {};
		std::snprintf( dpiScale, sizeof( dpiScale ), "%.2fx", status.dpi_scale );
		info( "dpi", dpiScale );
		end_group( );
	}

	bool tab_button( const char* label, bool active, ImVec2 size ) {
		ImGuiWindow* win = ImGui::GetCurrentWindow( );
		const ImGuiID id = win->GetID( label );
		const ImVec2  p  = win->DC.CursorPos;
		const ImRect  bb( p, p + size );
		ImGui::ItemSize( bb );
		if ( !ImGui::ItemAdd( bb, id ) ) return false;

		bool hovered, held;
		const bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held );

		ImDrawList* dl = win->DrawList;
		if ( active ) frame( dl, bb.Min, bb.Max, IM_COL32( 38, 38, 38, 255 ), IM_COL32( 28, 28, 28, 255 ) );
		else          frame( dl, bb.Min, bb.Max, hovered ? IM_COL32( 32, 32, 32, 255 ) : IM_COL32( 28, 28, 28, 255 ), IM_COL32( 20, 20, 20, 255 ) );

		const ImVec2 ls = text_size( label );
		text( dl, ImVec2( bb.GetCenter( ).x - ls.x * 0.5f, text_y( bb.Min.y, size.y ) ),
			active ? accent( ) : ( hovered ? col::text : col::dim ), label );
		return pressed;
	}

	void apply_style( ) {
		ImGuiStyle& s = ImGui::GetStyle( );
		s.WindowRounding    = 0.0f;
		s.ChildRounding     = 0.0f;
		s.FrameRounding     = 0.0f;
		s.PopupRounding     = 0.0f;
		s.GrabRounding      = 0.0f;
		s.ScrollbarRounding = 0.0f;
		s.WindowPadding     = ImVec2( 0, 0 );
		s.WindowBorderSize  = 0.0f;
		s.ChildBorderSize   = 0.0f;
		s.PopupBorderSize   = 1.0f;
		s.FrameBorderSize   = 1.0f;

		ImVec4* c = s.Colors;
		c[ ImGuiCol_Text ]           = ImColor( col::text );
		c[ ImGuiCol_WindowBg ]       = ImColor( col::window );
		c[ ImGuiCol_ChildBg ]        = ImVec4( 0, 0, 0, 0 );
		c[ ImGuiCol_PopupBg ]        = ImColor( col::group );
		c[ ImGuiCol_Border ]         = ImColor( col::black );
		c[ ImGuiCol_FrameBg ]        = ImColor( col::ctrl_bot );
		c[ ImGuiCol_FrameBgHovered ] = ImColor( col::ctrl_top );
		c[ ImGuiCol_FrameBgActive ]  = ImColor( col::ctrl_hover );
		c[ ImGuiCol_NavCursor ]      = ImVec4( 0, 0, 0, 0 );
	}

}

void menu::setup( ) {
	ImGuiIO& io = ImGui::GetIO( );
	font = io.Fonts->AddFontFromFileTTF( "C:\\Windows\\Fonts\\verdana.ttf", font_size );
	if ( !font )
		font = io.Fonts->AddFontDefault( );
	apply_style( );
}

void menu::render( const RuntimeStatus& status ) {
	const ImVec2 size( 560.0f, 380.0f );

	ImGui::SetNextWindowPos( ImVec2( 60.0f, 60.0f ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( size );
	ImGui::PushFont( font, font_size );
	ImGui::Begin( "##nexus", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse );
	{
		ImDrawList*  dl = ImGui::GetWindowDrawList( );
		const ImVec2 wp = ImGui::GetWindowPos( );

		ImGui::PushClipRect( wp, wp + size, false );
		dl->AddRect( wp, wp + size, col::black );
		dl->AddRect( wp + ImVec2( 1, 1 ), wp + size - ImVec2( 1, 1 ), col::edge );

		static const char* months[ ] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
		SYSTEMTIME st {};
		GetLocalTime( &st );
		char date[ 32 ];
		std::snprintf( date, sizeof( date ), " | %s %d %d", months[ ( st.wMonth - 1 ) % 12 ], st.wDay, st.wYear );

		const ImVec2 ms = text_size( "Nexus" );
		text( dl, wp + ImVec2( 9, 6 ), accent( ), "Nexus" );
		text( dl, wp + ImVec2( 9 + ms.x, 6 ), col::dim, date );

		dl->AddRectFilledMultiColor( wp + ImVec2( 2, 24 ), wp + ImVec2( size.x - 2, 26 ), accent( ), accent( 0.45f ), accent( 0.45f ), accent( ) );
		ImGui::PopClipRect( );

		static const char* tabs[ ] = { "visuals", "config" };
		const float tabs_x = 10.0f, tabs_y = 36.0f, tab_h = 24.0f;
		const float tab_w = ImFloor( ( size.x - tabs_x * 2.0f - ( IM_ARRAYSIZE( tabs ) - 1 ) * 3.0f ) / IM_ARRAYSIZE( tabs ) );
		ImGui::SetCursorPos( ImVec2( tabs_x, tabs_y ) );
		for ( int i = 0; i < IM_ARRAYSIZE( tabs ); ++i ) {
			if ( i ) ImGui::SameLine( 0, 3.0f );
			if ( tab_button( tabs[ i ], tab == i, ImVec2( tab_w, tab_h ) ) )
				tab = i;
		}

		const float content_y = tabs_y + tab_h + 10.0f;
		const ImVec2 avail( size.x - tabs_x * 2.0f, size.y - content_y - 10.0f );
		ImGui::SetCursorPos( ImVec2( tabs_x, content_y ) );
		ImGui::BeginGroup( );
		if ( tab == 0 ) page_visuals( avail );
		else            page_config ( avail, status );
		ImGui::EndGroup( );
	}
	ImGui::End( );
	ImGui::PopFont( );
}
