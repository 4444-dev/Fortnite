#include <includes.hpp>
#include <workspace/game/features/player/player.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

	struct BBox {
		float x { 0.0f };
		float y { 0.0f };
		float w { 0.0f };
		float h { 0.0f };
		bool  valid { false };
	};

	BBox GetBodyBBox(
		const FVector2D& head,
		const FVector2D& pelvis,
		const FVector2D& leftFoot,
		const FVector2D& rightFoot
	) {
		BBox box {};

		const float headX = static_cast<float>( head.X );
		const float headY = static_cast<float>( head.Y );
		const float pelvisX = static_cast<float>( pelvis.X );
		const float pelvisY = static_cast<float>( pelvis.Y );
		const float leftFootX = static_cast<float>( leftFoot.X );
		const float leftFootY = static_cast<float>( leftFoot.Y );
		const float rightFootX = static_cast<float>( rightFoot.X );
		const float rightFootY = static_cast<float>( rightFoot.Y );

		const float feetY = std::max( leftFootY, rightFootY );
		const float bodyHeight = feetY - headY;
		if ( !std::isfinite( bodyHeight ) || bodyHeight < 8.0f )
			return box;

		// Use the torso/feet axis instead of arm/hand extrema. This keeps
		// the box stable when a character stretches an arm toward the camera.
		const float centerX =
			( headX * 0.20f ) +
			( pelvisX * 0.55f ) +
			( ( leftFootX + rightFootX ) * 0.5f * 0.25f );

		const float width = bodyHeight * 0.46f;
		const float topPad = bodyHeight * 0.08f;
		const float bottomPad = bodyHeight * 0.03f;

		box.x = centerX - width * 0.5f;
		box.y = headY - topPad;
		box.w = width;
		box.h = bodyHeight + topPad + bottomPad;
		box.valid =
			std::isfinite( box.x ) &&
			std::isfinite( box.y ) &&
			std::isfinite( box.w ) &&
			std::isfinite( box.h ) &&
			box.w > 2.0f &&
			box.h > 8.0f;
		return box;
	}

	bool SegmentLooksSane(
		const FVector& worldA,
		const FVector& worldB,
		const FVector2D& screenA,
		const FVector2D& screenB,
		float boxHeight
	) {
		const double worldLength = ( worldA - worldB ).Size( );
		if ( !std::isfinite( worldLength ) || worldLength <= 0.1 || worldLength > 95.0 )
			return false;

		const double dx = screenA.X - screenB.X;
		const double dy = screenA.Y - screenB.Y;
		const double screenLength = std::sqrt( dx * dx + dy * dy );
		if ( !std::isfinite( screenLength ) )
			return false;

		// Individual human body segments should never span most of the
		// full body box. Reject projection glitches before drawing.
		return screenLength <= static_cast<double>( boxHeight ) * 0.62;
	}

	void DrawBox( ImDrawList* draw, float x, float y, float w, float h,
	              ImU32 color, float thickness, bool outline ) {
		if ( outline ) {
			const float outline_thick = thickness + 2.0f;
			const ImU32 black = IM_COL32( 0, 0, 0, 255 );
			draw->AddLine( ImVec2( x,     y     ), ImVec2( x + w, y     ), black, outline_thick );
			draw->AddLine( ImVec2( x + w, y     ), ImVec2( x + w, y + h ), black, outline_thick );
			draw->AddLine( ImVec2( x + w, y + h ), ImVec2( x,     y + h ), black, outline_thick );
			draw->AddLine( ImVec2( x,     y + h ), ImVec2( x,     y     ), black, outline_thick );
		}
		draw->AddLine( ImVec2( x,     y     ), ImVec2( x + w, y     ), color, thickness );
		draw->AddLine( ImVec2( x + w, y     ), ImVec2( x + w, y + h ), color, thickness );
		draw->AddLine( ImVec2( x + w, y + h ), ImVec2( x,     y + h ), color, thickness );
		draw->AddLine( ImVec2( x,     y + h ), ImVec2( x,     y     ), color, thickness );
	}

	void DrawCornerBox( ImDrawList* draw, float x, float y, float w, float h,
	                    ImU32 color, float thickness, bool outline ) {
		const float cw = w / 3.0f;
		const float ch = h / 3.0f;

		if ( outline ) {
			const ImU32 black = IM_COL32( 0, 0, 0, 255 );
			const float ot    = thickness + 2.0f;
			draw->AddLine( ImVec2( x,          y          ), ImVec2( x,          y + ch     ), black, ot );
			draw->AddLine( ImVec2( x,          y          ), ImVec2( x + cw,     y          ), black, ot );
			draw->AddLine( ImVec2( x + w - cw, y          ), ImVec2( x + w,      y          ), black, ot );
			draw->AddLine( ImVec2( x + w,      y          ), ImVec2( x + w,      y + ch     ), black, ot );
			draw->AddLine( ImVec2( x,          y + h - ch ), ImVec2( x,          y + h      ), black, ot );
			draw->AddLine( ImVec2( x,          y + h      ), ImVec2( x + cw,     y + h      ), black, ot );
			draw->AddLine( ImVec2( x + w - cw, y + h      ), ImVec2( x + w,      y + h      ), black, ot );
			draw->AddLine( ImVec2( x + w,      y + h - ch ), ImVec2( x + w,      y + h      ), black, ot );
		}

		draw->AddLine( ImVec2( x,          y          ), ImVec2( x,          y + ch     ), color, thickness );
		draw->AddLine( ImVec2( x,          y          ), ImVec2( x + cw,     y          ), color, thickness );
		draw->AddLine( ImVec2( x + w - cw, y          ), ImVec2( x + w,      y          ), color, thickness );
		draw->AddLine( ImVec2( x + w,      y          ), ImVec2( x + w,      y + ch     ), color, thickness );
		draw->AddLine( ImVec2( x,          y + h - ch ), ImVec2( x,          y + h      ), color, thickness );
		draw->AddLine( ImVec2( x,          y + h      ), ImVec2( x + cw,     y + h      ), color, thickness );
		draw->AddLine( ImVec2( x + w - cw, y + h      ), ImVec2( x + w,      y + h      ), color, thickness );
		draw->AddLine( ImVec2( x + w,      y + h - ch ), ImVec2( x + w,      y + h      ), color, thickness );
	}

	void DrawTextOutlined( ImDrawList* draw, ImFont* font, float size,
	                       ImVec2 pos, ImU32 color, const char* text ) {
		if ( !text || !*text )
			return;
		const ImU32 shadow = IM_COL32( 0, 0, 0, 200 );
		draw->AddText( font, size, ImVec2( pos.x - 1.0f, pos.y - 1.0f ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x + 1.0f, pos.y - 1.0f ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x - 1.0f, pos.y + 1.0f ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x + 1.0f, pos.y + 1.0f ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x - 1.0f, pos.y        ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x + 1.0f, pos.y        ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x,        pos.y - 1.0f ), shadow, text );
		draw->AddText( font, size, ImVec2( pos.x,        pos.y + 1.0f ), shadow, text );
		draw->AddText( font, size, pos, color, text );
	}

}

namespace player {

	void Draw(
		const PlayerCache& Players,
		const CameraCache& Camera,
		ImDrawList* DL,
		float Width,
		float Height,
		const Config& Cfg
	) {
		if ( !DL || !Camera.Valid( ) )
			return;

		const auto snapshot  = Players.Snapshot( );
		const u8   localTeam = Players.LocalTeamIndex( );

		const ImU32 box_color = ImGui::ColorConvertFloat4ToU32( ImVec4(
			Cfg.BoxColor [ 0 ], Cfg.BoxColor [ 1 ],
			Cfg.BoxColor [ 2 ], Cfg.BoxColor [ 3 ] ) );

		const ImU32 box_fill = IM_COL32(
			static_cast< int >( Cfg.BoxColor [ 0 ] * 255.0f ),
			static_cast< int >( Cfg.BoxColor [ 1 ] * 255.0f ),
			static_cast< int >( Cfg.BoxColor [ 2 ] * 255.0f ),
			50 );

		const ImU32 skel_color = ImGui::ColorConvertFloat4ToU32( ImVec4(
			Cfg.SkeletonColor [ 0 ], Cfg.SkeletonColor [ 1 ],
			Cfg.SkeletonColor [ 2 ], Cfg.SkeletonColor [ 3 ] ) );

		const ImU32 snap_color = ImGui::ColorConvertFloat4ToU32( ImVec4(
			Cfg.SnaplineColor [ 0 ], Cfg.SnaplineColor [ 1 ],
			Cfg.SnaplineColor [ 2 ], Cfg.SnaplineColor [ 3 ] ) );

		const ImVec2 snap_origin(
			Width * 0.5f,
			Cfg.SnaplineOrigin == 2 ? 0.0f : ( Cfg.SnaplineOrigin == 1 ? Height * 0.5f : Height ) );

		static constexpr int kSkeleton[][ 2 ] = {
			{  0,  1 }, {  1,  2 }, {  2,  3 },
			{  2,  4 }, {  4,  5 }, {  5,  6 },
			{  2,  7 }, {  7,  8 }, {  8,  9 },
			{  3, 10 }, { 10, 11 }, { 11, 12 },
			{  3, 13 }, { 13, 14 }, { 14, 15 },
		};

		for ( const auto& Player : snapshot ) {
			if ( Player.bIsLocal || Player.bIsDying )
				continue;

			const bool teammate =
				Player.TeamIndex != 0 &&
				Player.TeamIndex == localTeam;
			if ( teammate )
				continue;

			bool sane = false;
			FVector2D screen_bones [ 16 ] {};
			bool      bone_valid  [ 16 ] {};

			if ( Player.BonesValid ) {
				sane = true;
				double maxOffset = 0.0;
				for ( i32 i = 0; i < g_ESPBoneCount; ++i ) {
					const FVector& b = Player.Bones [ i ];
					if ( !std::isfinite( b.X ) || !std::isfinite( b.Y ) || !std::isfinite( b.Z ) ) {
						sane = false;
						break;
					}
					const double off = ( b - Player.Bones [ 3 ] ).Size( );
					if ( off > maxOffset )
						maxOffset = off;
				}
				if ( sane && maxOffset <= 400.0 ) {
					for ( i32 i = 0; i < g_ESPBoneCount; ++i ) {
						FVector2D s {};
						if ( Camera.WorldToScreen( Player.Bones [ i ], Width, Height, s ) ) {
							screen_bones [ i ] = s;
							bone_valid   [ i ] = true;
						}
					}
				} else {
					sane = false;
				}
			}

			BBox bbox {};
			const bool hasCoreBones =
				sane &&
				bone_valid [ 0 ] &&  // head
				bone_valid [ 3 ] &&  // pelvis
				bone_valid [ 12 ] && // left foot
				bone_valid [ 15 ];   // right foot

			if ( hasCoreBones ) {
				bbox = GetBodyBBox(
					screen_bones [ 0 ],
					screen_bones [ 3 ],
					screen_bones [ 12 ],
					screen_bones [ 15 ]
				);
			}

			const bool hasBoneBox = bbox.valid;

			// Lobby fallback: when skeletal data is not ready, build a simple box from actor location.
			if ( !bbox.valid ) {
				if ( !Player.Location.IsValid( ) )
					continue;

				FVector2D feet {};
				FVector2D head {};
				if ( !Camera.WorldToScreen( Player.Location, Width, Height, feet ) )
					continue;
				if ( !Camera.WorldToScreen( Player.Location + FVector { 0.0, 0.0, 80.0 }, Width, Height, head ) )
					continue;

				const float h = std::fabs( static_cast< float >( feet.Y - head.Y ) );
				if ( h < 4.0f )
					continue;
				const float w = h * 0.5f;

				bbox.x = static_cast< float >( head.X ) - ( w * 0.5f );
				bbox.y = static_cast< float >( head.Y );
				bbox.w = w;
				bbox.h = h;
				bbox.valid = true;
			}


			if ( Cfg.Snaplines ) {
				const ImVec2 target(
					bbox.x + bbox.w * 0.5f,
					Cfg.SnaplineOrigin == 2 ? bbox.y : bbox.y + bbox.h );
				DL->AddLine( snap_origin, target, IM_COL32( 0, 0, 0, 160 ), 2.5f );
				DL->AddLine( snap_origin, target, snap_color, 1.0f );
			}

			if ( Cfg.BoxFill && Cfg.BoxStyle != 0 ) {
				DL->AddRectFilled(
					ImVec2( bbox.x, bbox.y ),
					ImVec2( bbox.x + bbox.w, bbox.y + bbox.h ),
					box_fill );
			}

			switch ( Cfg.BoxStyle ) {
			case 1: DrawBox      ( DL, bbox.x, bbox.y, bbox.w, bbox.h, box_color, 1.0f, true ); break;
			case 2: DrawCornerBox( DL, bbox.x, bbox.y, bbox.w, bbox.h, box_color, 1.0f, true ); break;
			default: break;
			}

			if ( Cfg.Skeleton && hasBoneBox ) {
				const ImU32 shadow = IM_COL32( 0, 0, 0, 200 );
				for ( const auto& pair : kSkeleton ) {
					const int a = pair [ 0 ];
					const int b = pair [ 1 ];
					if ( !bone_valid [ a ] || !bone_valid [ b ] )
						continue;
					if ( !SegmentLooksSane(
						Player.Bones [ a ],
						Player.Bones [ b ],
						screen_bones [ a ],
						screen_bones [ b ],
						bbox.h ) )
						continue;
					const ImVec2 pa( static_cast< float >( screen_bones [ a ].X ),
					                 static_cast< float >( screen_bones [ a ].Y ) );
					const ImVec2 pb( static_cast< float >( screen_bones [ b ].X ),
					                 static_cast< float >( screen_bones [ b ].Y ) );
					DL->AddLine( pa, pb, shadow,     2.5f );
					DL->AddLine( pa, pb, skel_color, 1.0f );
				}
			}

			if ( Cfg.Distance ) {
				ImFont* font = Cfg.EspFont ? Cfg.EspFont : ImGui::GetFont( );
				const float fsize = font->LegacySize > 0.0f ? font->LegacySize : ImGui::GetFontSize( );
				const ImU32 white = IM_COL32( 255, 255, 255, 255 );
				const float cx    = bbox.x + bbox.w * 0.5f;

				char buf [ 32 ] {};
				std::snprintf( buf, sizeof( buf ), "%dm",
				               static_cast< int >( Player.Distance ) );
				const ImVec2 sz = font->CalcTextSizeA( fsize, FLT_MAX, 0.0f, buf );
				const ImVec2 pos( cx - sz.x * 0.5f, bbox.y - ( fsize + 2.0f ) );
				DrawTextOutlined( DL, font, fsize, pos, white, buf );
			}
		}
	}

}
