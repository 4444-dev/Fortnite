#include <includes.hpp>
#include <workspace/interface/interface.hpp>

#include <d3d11.h>
#include <dxgi.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>
#include <workspace/interface/menu.hpp>
#include <Uxtheme.h>
#include <dwmapi.h>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dwmapi.lib" )

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey( WPARAM wParam, LPARAM lParam );

namespace overlay {

	struct resources_t {
		HWND hwnd;
		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		IDXGISwapChain* swap_chain;
		ID3D11RenderTargetView* render_target_view;
	};

	resources_t resources = { nullptr, nullptr, nullptr, nullptr, nullptr };

	ImFont* g_EspFont = nullptr;

	LRESULT __stdcall WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam ) {
		if ( ImGui_ImplWin32_WndProcHandler( hwnd, message, wparam, lparam ) )
			return true;
		if ( message == WM_DESTROY ) {
			PostQuitMessage( 0 );
			return 0;
		}
		return DefWindowProcW( hwnd, message, wparam, lparam );
	}

	void set_click_through( HWND hwnd, bool click_through ) {
		LONG_PTR ex_style = GetWindowLongPtrW( hwnd, GWL_EXSTYLE );
		if ( click_through )
			ex_style |= WS_EX_TRANSPARENT;
		else
			ex_style &= ~WS_EX_TRANSPARENT;
		SetWindowLongPtrW( hwnd, GWL_EXSTYLE, ex_style );
	}

	HWND create_fallback_window( ) {
		WNDCLASSEXW wc { };
		wc.cbSize = sizeof( wc );
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.hInstance = GetModuleHandleW( nullptr );
		wc.hCursor = LoadCursorW( nullptr, IDC_ARROW );
		wc.hIcon = LoadIconW( nullptr, IDI_APPLICATION );
		wc.lpszClassName = L"LuvkrimesOverlay";

		if ( !RegisterClassExW( &wc ) && GetLastError( ) != ERROR_CLASS_ALREADY_EXISTS )
			return nullptr;

		const i32 screen_width = GetSystemMetrics( SM_CXSCREEN );
		const i32 screen_height = GetSystemMetrics( SM_CYSCREEN );

		HWND hwnd = CreateWindowExW(
			WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
			wc.lpszClassName,
			L"luvkrimes base",
			WS_POPUP,
			0, 0, screen_width, screen_height,
			nullptr, nullptr, wc.hInstance, nullptr );
		if ( !hwnd )
			return nullptr;

		MARGINS window_margin { -1 };
		DwmExtendFrameIntoClientArea( hwnd, &window_margin );
		SetLayeredWindowAttributes( hwnd, 0, 255, LWA_ALPHA );

		ShowWindow( hwnd, SW_SHOW );
		UpdateWindow( hwnd );
		return hwnd;
	}

	RECT get_client_area_and_size( HWND hwnd ) {
		RECT rect {};
		if ( GetClientRect( hwnd, &rect ) ) {
			POINT top_left = { rect.left, rect.top };
			POINT bottom_right = { rect.right, rect.bottom };

			ClientToScreen( hwnd, &top_left );
			ClientToScreen( hwnd, &bottom_right );

			rect.left = top_left.x;
			rect.top = top_left.y;
			rect.right = bottom_right.x;
			rect.bottom = bottom_right.y;
		}
		else {
			rect = { 0, 0, 0, 0 };
		}

		return rect;
	}

	bool setup_d3d( HWND hwnd ) {
		DXGI_SWAP_CHAIN_DESC desc { };
		ZeroMemory( &desc, sizeof( desc ) );
		desc.BufferCount = 2;
		desc.BufferDesc.Width = 0;
		desc.BufferDesc.Height = 0;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = hwnd;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		const D3D_FEATURE_LEVEL levels [ ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
		D3D_FEATURE_LEVEL selected_level = { };

		if ( FAILED( D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
			levels, _countof( levels ), D3D11_SDK_VERSION,
			&desc,
			&resources.swap_chain,
			&resources.device,
			&selected_level,
			&resources.device_context
		) ) ) {
			return false;
		}

		ID3D11Texture2D* back_buffer = nullptr;
		if ( FAILED( resources.swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) ) ) || !back_buffer )
			return false;

		if ( FAILED( resources.device->CreateRenderTargetView( back_buffer, nullptr, &resources.render_target_view ) ) ) {
			back_buffer->Release( );
			return false;
		}
		back_buffer->Release( );

		ImGui::CreateContext( );

		ImGuiIO& io = ImGui::GetIO( );
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;
		ImGui::StyleColorsDark( );

		menu::setup( );

		g_EspFont = io.Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\verdanab.ttf", 14.0f, nullptr,
			io.Fonts->GetGlyphRangesDefault( ) );
		if ( !g_EspFont ) {
			g_EspFont = io.Fonts->AddFontFromFileTTF(
				"C:\\Windows\\Fonts\\seguisb.ttf", 14.0f, nullptr,
				io.Fonts->GetGlyphRangesDefault( ) );
		}
		if ( !g_EspFont )
			g_EspFont = io.Fonts->Fonts.back( );

		ImGui_ImplWin32_Init( hwnd );
		ImGui_ImplDX11_Init( resources.device, resources.device_context );

		resources.hwnd = hwnd;
		return true;
	}

	void shutdown_d3d( ) {
		if ( resources.render_target_view ) {
			resources.render_target_view->Release( );
			resources.render_target_view = nullptr;
		}
		if ( resources.swap_chain ) {
			resources.swap_chain->Release( );
			resources.swap_chain = nullptr;
		}
		if ( resources.device_context ) {
			resources.device_context->Release( );
			resources.device_context = nullptr;
		}
		if ( resources.device ) {
			resources.device->Release( );
			resources.device = nullptr;
		}
		resources.hwnd = nullptr;
	}

	int vk_from_key( int key ) {
		switch ( key ) {
		case ImGuiKey_MouseRight:  return VK_RBUTTON;
		case ImGuiKey_MouseMiddle: return VK_MBUTTON;
		case ImGuiKey_MouseX1:     return VK_XBUTTON1;
		case ImGuiKey_MouseX2:     return VK_XBUTTON2;
		default: break;
		}
		for ( int vk = 8; vk < 256; ++vk )
			if ( ImGui_ImplWin32_KeyEventToImGuiKey( vk, 0 ) == key )
				return vk;
		return VK_INSERT;
	}

	void feed_keyboard( ) {
		static bool was_down[ 256 ] {};
		ImGuiIO& io = ImGui::GetIO( );

		BYTE state[ 256 ] {};
		for ( int vk = 0; vk < 256; ++vk )
			state[ vk ] = ( GetAsyncKeyState( vk ) & 0x8000 ) ? 0x80 : 0;
		if ( GetKeyState( VK_CAPITAL ) & 1 )
			state[ VK_CAPITAL ] |= 1;

		io.AddKeyEvent( ImGuiMod_Ctrl,  state[ VK_CONTROL ] != 0 );
		io.AddKeyEvent( ImGuiMod_Shift, state[ VK_SHIFT ] != 0 );
		io.AddKeyEvent( ImGuiMod_Alt,   state[ VK_MENU ] != 0 );

		for ( int vk = 8; vk < 256; ++vk ) {
			if ( vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU )
				continue;
			const bool down = state[ vk ] != 0;
			if ( down == was_down[ vk ] )
				continue;
			was_down[ vk ] = down;

			const ImGuiKey key = ImGui_ImplWin32_KeyEventToImGuiKey( vk, 0 );
			if ( key != ImGuiKey_None )
				io.AddKeyEvent( key, down );

			if ( down ) {
				wchar_t chars[ 4 ] {};
				const int n = ToUnicode( vk, MapVirtualKeyW( vk, MAPVK_VK_TO_VSC ), state, chars, 4, 0 );
				for ( int i = 0; i < n; ++i )
					if ( chars[ i ] >= 32 )
						io.AddInputCharacterUTF16( chars[ i ] );
			}
		}
	}

	void loop( PlayerCache& Players, CameraCache& Camera ) {
		constexpr float clear_color [ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
		MSG msg { };
		bool menu_open = true;
		bool toggle_was_down = false;

		while ( true ) {
			while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			if ( msg.message == WM_QUIT )
				break;

			const bool toggle_down = ( GetAsyncKeyState( vk_from_key( menu::cfg.menu_key ) ) & 0x8000 ) != 0;
			if ( toggle_down && !toggle_was_down ) {
				menu_open = !menu_open;
				set_click_through( resources.hwnd, !menu_open );
				if ( !menu_open )
					ImGui::GetIO( ).AddFocusEvent( false );
			}
			toggle_was_down = toggle_down;

			if ( Players.ImageBase( ) ) {
				Camera.Update( Players.World( ) );
				if ( Camera.Valid( ) )
					Players.SetCameraLocation( Camera.Location( ) );
			}

			ImGui_ImplWin32_NewFrame( );
			ImGui_ImplDX11_NewFrame( );

			POINT cursor_pos { };
			if ( GetCursorPos( &cursor_pos ) && resources.hwnd )
				ScreenToClient( resources.hwnd, &cursor_pos );
			auto& io = ImGui::GetIO( );
			io.AddMousePosEvent( static_cast< float >( cursor_pos.x ), static_cast< float >( cursor_pos.y ) );
			io.AddMouseButtonEvent( 0, ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) != 0 );
			io.AddMouseButtonEvent( 1, ( GetAsyncKeyState( VK_RBUTTON ) & 0x8000 ) != 0 );
			io.AddMouseButtonEvent( 2, ( GetAsyncKeyState( VK_MBUTTON ) & 0x8000 ) != 0 );
			io.AddMouseButtonEvent( 3, ( GetAsyncKeyState( VK_XBUTTON1 ) & 0x8000 ) != 0 );
			io.AddMouseButtonEvent( 4, ( GetAsyncKeyState( VK_XBUTTON2 ) & 0x8000 ) != 0 );
			if ( menu_open )
				feed_keyboard( );

			ImGui::NewFrame( );

			const auto Display = ImGui::GetIO( ).DisplaySize;
			const menu::Settings& s = menu::cfg;

			{
				const player::Config pcfg {
					s.box ? s.box_style + 1 : 0,
					s.box_fill,
					{ s.box_color[ 0 ], s.box_color[ 1 ], s.box_color[ 2 ], s.box_color[ 3 ] },
					s.distance,
					s.skeleton,
					{ s.skeleton_color[ 0 ], s.skeleton_color[ 1 ], s.skeleton_color[ 2 ], s.skeleton_color[ 3 ] },
					s.snaplines,
					s.snapline_origin,
					{ s.snapline_color[ 0 ], s.snapline_color[ 1 ], s.snapline_color[ 2 ], s.snapline_color[ 3 ] },
					g_EspFont,
				};
				player::Draw(
					Players,
					Camera,
					ImGui::GetBackgroundDrawList( ),
					Display.x,
					Display.y,
					pcfg );
			}

			if ( menu_open ) {
				const auto players = Players.Snapshot( );
				const menu::RuntimeStatus status {
					Players.World( ) != nullptr,
					Camera.Valid( ),
					static_cast< int >( players.size( ) )
				};
				menu::render( status );
			}

			ImGui::Render( );

			resources.device_context->OMSetRenderTargets( 1, &resources.render_target_view, nullptr );
			resources.device_context->ClearRenderTargetView( resources.render_target_view, clear_color );
			ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
			resources.swap_chain->Present( 0, 0 );
		}

		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );
	}

	bool run( PlayerCache& Players, CameraCache& Camera ) {
		resources.hwnd = create_fallback_window( );
		if ( !resources.hwnd )
			return false;

		if ( !setup_d3d( resources.hwnd ) ) {
			DestroyWindow( resources.hwnd );
			resources.hwnd = nullptr;
			shutdown_d3d( );
			return false;
		}

		loop( Players, Camera );

		HWND window = resources.hwnd;
		shutdown_d3d( );

		if ( window && IsWindow( window ) )
			DestroyWindow( window );

		return true;
	}

}
