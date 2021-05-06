//#pragma comment( linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup" )

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "curl/curl.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <strsafe.h>

#include "modules/autoclicker/autoclicker.h"
#include "utils/encryption/xorencrypt.h"
#include "utils/misc/fonts.h"
#include "config/config.h"
#include "modules/hooks.h"
#include "utils/utils.h"
#include "auth/auth.h"

static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

bool CreateDeviceD3D( HWND hWnd );
void CleanupDeviceD3D( );
void CreateRenderTarget( );
void CleanupRenderTarget( );
LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

int main( int, char** ) {
	utils::show_console( );
	utils::get_max_perms( GetCurrentProcess( ) );
	//utils::clean_dps( );

	std::cout << _xor_( "Authenticating..." );

	//std::vector<char> encrypted = auth::encrypt( _xor_( "https://lithiumclient.wtf/api/check/" ) );
	//std::cout << auth::convert( _xor_( encrypted ) ) << std::endl << std::endl;
	//std::this_thread::sleep_for( std::chrono::milliseconds( 10000 ) );

	// Encrypted auth url

	const std::vector<char> code {
		38, 58, 58, 62, 61, 116, 97, 97, 34,
		39, 58, 38, 39, 59, 35, 45, 34, 39,
		43, 32, 58, 96, 57, 58, 40, 97, 47,
		62, 39, 97, 45, 38, 43, 45, 37, 97
	};

	std::string site_data {
		auth::download_site( ( auth::decrypt( code ) + utils::get_hwid( ) ).c_str( ) )
	};

	if ( site_data.find( _xor_( "hwidauthed" ) ) != std::string::npos ) {
		config::authenticated = true;

		std::cout << std::endl << _xor_( "Authenticated!" );

		std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

		utils::hide_console( );

		// imgui stuff starts here:

		WNDCLASSEX wc = { sizeof( WNDCLASSEX ), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle( NULL ), NULL, NULL, NULL, NULL, _T( _xor_( " " ) ), NULL };
		RegisterClassEx( &wc );
		config::hwnd = CreateWindow( wc.lpszClassName, _T( _xor_( " " ) ), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX, 100, 100, 500, 625, NULL, NULL, wc.hInstance, NULL );

		if ( !CreateDeviceD3D( config::hwnd ) ) {
			CleanupDeviceD3D( );
			UnregisterClass( wc.lpszClassName, wc.hInstance );
			return 1;
		}

		ShowWindow( config::hwnd, SW_SHOWDEFAULT );
		UpdateWindow( config::hwnd );

		IMGUI_CHECKVERSION( );
		ImGui::CreateContext( );
		ImGuiIO& io = ImGui::GetIO( ); ( void )io;

		io.IniFilename = NULL;

		ImGui::StyleColorsDark( );

		ImGui_ImplWin32_Init( config::hwnd );
		ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );

		ImVec4 clear_color = ImVec4( 0.45f, 0.55f, 0.60f, 1.00f );

		MSG msg;
		ZeroMemory( &msg, sizeof( msg ) );

		ImFont* verdana_font = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_font_compressed_data, verdana_font_compressed_size, 16.0f );
		ImFont* pollyroundedbold_font = io.Fonts->AddFontFromMemoryCompressedTTF( pollyroundedbold_font_compressed_data, pollyroundedbold_font_compressed_size, 22.0f );
		ImFont* pollyroundedbold_nextpage_font = io.Fonts->AddFontFromMemoryCompressedTTF( pollyroundedbold_font_compressed_data, pollyroundedbold_font_compressed_size, 45.0f );
		ImFont* icon_font = io.Fonts->AddFontFromMemoryCompressedTTF( icon_font_compressed_data, icon_font_compressed_size, 30.0f );

		// Run all threads here
		HANDLE hook_handle = CreateThread( 0, 0, ( LPTHREAD_START_ROUTINE )hook_thread, 0, 0, 0 ),
			autoclicker_handle = CreateThread( 0, 0, ( LPTHREAD_START_ROUTINE )autoclicker_thread, 0, 0, 0 );
		CloseHandle( hook_handle );
		CloseHandle( autoclicker_handle );

		while ( msg.message != WM_QUIT ) {
			if ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) ) {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
				continue;
			}

			ImGui_ImplDX11_NewFrame( );
			ImGui_ImplWin32_NewFrame( );
			ImGui::NewFrame( );

			// Style
			ImGuiStyle* style = &ImGui::GetStyle( );

			style->WindowBorderSize = 0.0f;
			style->WindowRounding = 0.0f;
			style->WindowTitleAlign = ImVec2( 0.5f, 0.5f );
			style->FrameRounding = 3.0f;
			style->GrabRounding = 3.0f;
			style->ChildBorderSize = 1.5f;
			style->ChildRounding = 3.0f;

			style->Colors[ ImGuiCol_WindowBg ] = ImColor( 20, 20, 20 );
			style->Colors[ ImGuiCol_FrameBg ] = ImColor( 30, 30, 30 );
			style->Colors[ ImGuiCol_TitleBgActive ] = ImColor( 20, 20, 20 );
			style->Colors[ ImGuiCol_FrameBgHovered ] = ImColor( 50, 50, 50 );
			style->Colors[ ImGuiCol_FrameBgActive ] = ImColor( 60, 60, 60 );
			// Progress Bar
			style->Colors[ ImGuiCol_PlotHistogram ] = ImColor( config::main_color );
			// Slider
			style->Colors[ ImGuiCol_SliderGrab ] = ImColor( config::main_color );
			style->Colors[ ImGuiCol_SliderGrabActive ] = ImColor( config::main_hovered_color );
			// Button
			style->Colors[ ImGuiCol_Button ] = ImColor( config::main_color );
			style->Colors[ ImGuiCol_ButtonHovered ] = ImColor( config::main_hovered_color );
			style->Colors[ ImGuiCol_ButtonActive ] = ImColor( config::main_color );
			// Check Box
			style->Colors[ ImGuiCol_CheckMark ] = ImColor( config::main_color );
			// Text
			style->Colors[ ImGuiCol_Text ] = ImColor( 255, 255, 255 );
			// Child
			style->Colors[ ImGuiCol_ChildBg ] = ImColor( 10, 10, 10 );

			// GUI
			ImGui::PushFont( verdana_font ); {
				ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
				ImGui::SetNextWindowSize( ImVec2( 500, 625 ) );
				ImGui::Begin( _xor_( " " ), NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

				// Tabs
				ImGui::PushFont( icon_font );
				ImGui::SetCursorPos( ImVec2( 85.0f, 30.0f ) );
				if ( config::selected_tab == 0 ) {
					ImGui::PushStyleColor( ImGuiCol_Text, config::main_color );
					ImGui::Text( _xor_( "a" ) );
					ImGui::PopStyleColor( );
				}
				else {
					ImGui::Text( _xor_( "a" ) );
				}
				if ( ImGui::IsItemClicked( 0 ) ) {
					config::selected_tab = 0;
				}
				ImGui::SetCursorPos( ImVec2( 225.0f, 30.0f ) );
				if ( config::selected_tab == 1 ) {
					ImGui::PushStyleColor( ImGuiCol_Text, config::main_color );
					ImGui::Text( _xor_( "b" ) );
					ImGui::PopStyleColor( );
				}
				else {
					ImGui::Text( _xor_( "b" ) );
				}
				if ( ImGui::IsItemClicked( 0 ) ) {
					config::selected_tab = 1;
				}
				ImGui::SetCursorPos( ImVec2( 365.0f, 30.0f ) );
				if ( config::selected_tab == 2 ) {
					ImGui::PushStyleColor( ImGuiCol_Text, config::main_color );
					ImGui::Text( _xor_( "d" ) );
					ImGui::PopStyleColor( );
				}
				else {
					ImGui::Text( _xor_( "d" ) );
				}
				if ( ImGui::IsItemClicked( 0 ) ) {
					config::selected_tab = 2;
				}
				ImGui::PopFont( );

				// Combat
				if ( config::selected_tab == 0 ) {
					// AutoClicker
					ImGui::SetCursorPos( ImVec2( 45.0f, 85.0f ) );
					ImGui::BeginChild( _xor_( "##autoclicker_child" ), ImVec2( 395, 180 ), true ); // Child Start
					ImGui::PushItemWidth( 310 );
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "AutoClicker" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Checkbox( _xor_(  "Enabled##autoclickerclicker_enabled_checkbox" ), &config::autoclicker_enabled );
					utils::hotkey_autoclicker( &config::autoclicker_keybind );
					ImGui::SetCursorPos( ImVec2( 10.0f, 70.0f ) );
					ImGui::SliderInt( _xor_( "CPS##clicker_cps_slider" ), &config::autoclicker_cps, config::autoclicker_min_cps, config::autoclicker_max_cps );
					ImGui::SetCursorPos( ImVec2( 10.0f, 95.0f ) );
					ImGui::Checkbox( _xor_( "Only in MC##only_in_minecraft_checkbox" ), &config::autoclicker_only_in_mc );
					//utils::tooltip( _xor_( "The clicker will only work in Minecraft if enabled." ) );
					ImGui::SetCursorPos( ImVec2( 10.0f, 120.0f ) );
					ImGui::Checkbox( _xor_( "Blatant Mode##blatant_mode_checkbox" ), &config::autoclicker_blatant_mode );
					utils::tooltip( _xor_( "Makes the CPS cap 200 instead of 20 but no randomization." ) );
					ImGui::SetCursorPos( ImVec2( 10.0f, 145.0f ) );
					ImGui::Checkbox( _xor_( "Click Sounds##click_sounds_checkbox" ), &config::autoclicker_click_sounds );
					ImGui::EndChild( ); // Child End

					if ( config::autoclicker_blatant_mode ) {
						config::autoclicker_max_cps = 200;
					}
					else {
						config::autoclicker_max_cps = 20;
					}

					if ( config::autoclicker_cps > config::autoclicker_max_cps ) {
						config::autoclicker_cps = config::autoclicker_max_cps;
					}
					else if ( config::autoclicker_cps < config::autoclicker_min_cps ) {
						config::autoclicker_cps = config::autoclicker_min_cps;
					}
					/*else if ( config::autoclicker_jitter_range > 10 ) {
						config::autoclicker_jitter_range = 10;
					}
					else if ( config::autoclicker_jitter_range < 0 ) {
						config::autoclicker_jitter_range = 0;
					}*/

					// Reach
					ImGui::SetCursorPos( ImVec2( 45.0f, 275.0f ) );
					ImGui::BeginChild( _xor_( "##reach_child" ), ImVec2( 395, 155 ), true );  // Child Start
					ImGui::PushItemWidth( 310 );
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "Reach" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Checkbox( _xor_( "Enabled##reach_enabled_checkbox" ), &config::reach_enabled );
					utils::hotkey_reach( &config::reach_keybind );
					ImGui::SetCursorPos( ImVec2( 10.0f, 70.0f ) );
					//ImGui::SliderFloat( _xor_( "Min##reach_min_slider" ), &config::reach_min_distance, 3.00f, 6.00f, _xor_( "%.2f" ) );
					ImGui::SliderFloat( _xor_( "Min##reach_min_slider" ), &config::PlayerReach, 3.00f, 6.00f, _xor_( "%.2f" ) );
					ImGui::SetCursorPos( ImVec2( 10.0f, 95.0f ) );
					ImGui::SliderFloat( _xor_( "Max##reach_max_slider" ), &config::reach_max_distance, 3.00f, 6.00f, _xor_( "%.2f" ) );
					ImGui::SetCursorPos( ImVec2( 10.0f, 120.0f ) );
					ImGui::Checkbox( _xor_( "Only while moving##only_while_moving_checkbox" ), &config::reach_only_while_moving );
					utils::tooltip( _xor_( "Your extra reach will only register while moving." ) );
					ImGui::EndChild( ); // Child End

					if ( config::reach_min_distance > 6.00f ) {
						config::reach_min_distance = 6.00f;
					}
					else if ( config::reach_min_distance < 3.00f ) {
						config::reach_min_distance = 3.00f;
					}
					else if ( config::reach_max_distance > 6.00f ) {
						config::reach_max_distance = 6.00f;
					}
					else if ( config::reach_max_distance < 3.00f ) {
						config::reach_max_distance = 3.00f;
					}
					else if ( config::reach_min_distance > config::reach_max_distance ) {
						config::reach_max_distance = config::reach_min_distance;
					}

					// Velocity
					ImGui::SetCursorPos( ImVec2( 45.0f, 440.0f ) );
					ImGui::BeginChild( _xor_( "##velocity_child" ), ImVec2( 395, 105 ), true ); // Child Start
					ImGui::PushItemWidth( 310 );
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "Velocity" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Checkbox( _xor_( "Enabled##velocity_enabled_checkbox" ), &config::velocity_enabled );
					utils::hotkey_velocity( &config::velocity_keybind );
					ImGui::SetCursorPos( ImVec2( 10.0f, 70.0f ) );
					ImGui::SliderInt( _xor_( "X & Y##velocity_percent_slider" ), &config::velocity_percent, 1.00f, 100.00f );
					ImGui::EndChild( ); // Child End

					config::velocitypercent = config::velocity_percent;
					config::velocityval = config::velocity_percent / 8000;

					if ( config::velocity_percent > 100.00f ) {
						config::velocity_percent = 100.00f;
					}
					else if ( config::velocity_percent < 1.00f ) {
						config::velocity_percent = 1.00f;
					}

					//ImGui::SetCursorPos( ImVec2( 450.0f, 515.0f ) );
					//ImGui::PushStyleColor( ImGuiCol_Text, config::main_color );
					//ImGui::PushFont( pollyroundedbold_nextpage_font );
					//ImGui::Text( _xor_( ">" ) );
					//ImGui::PopFont( );
					//ImGui::PopStyleColor( );
				}
				// Movement
				else if ( config::selected_tab == 1 ) {
					// Timer
					ImGui::SetCursorPos( ImVec2( 45.0f, 85.0f ) );
					ImGui::BeginChild( _xor_( "##timer_child" ), ImVec2( 395, 105 ), true ); // Child Start
					ImGui::PushItemWidth( 310 );
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "Timer" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Checkbox( _xor_(  "Enabled##timer_enabled_checkbox" ), &config::timer_enabled );
					utils::hotkey_timer( &config::timer_keybind );
					ImGui::SetCursorPos( ImVec2( 10.0f, 70.0f ) );
					ImGui::SliderFloat( _xor_( "Speed##timer_speed_slider" ), &config::timer_speed, 0.0f, 5.0f, _xor_( "%.1f" ) );
					ImGui::EndChild( ); // Child End

					if ( config::timer_speed > 5.0f ) {
						config::timer_speed = 5.0f;
					}
					else if ( config::timer_speed < 0.0f ) {
						config::timer_speed = 0.0f;
					}

					// BHop
					ImGui::SetCursorPos( ImVec2( 45.0f, 200.0f ) );
					ImGui::BeginChild( _xor_( "##bhop_child" ), ImVec2( 395, 75 ), true ); // Child Start
					ImGui::PushItemWidth( 310 );
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "BHop" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Checkbox( _xor_(  "Enabled##bhop_enabled_checkbox" ), &config::bhop_enabled );
					utils::hotkey_bhop( &config::bhop_keybind );
					ImGui::EndChild( ); // Child End
				}
				// Settings
				else if ( config::selected_tab == 2 ) {
					// Other
					ImGui::SetCursorPos( ImVec2( 45.0f, 85.0f ) );
					ImGui::BeginChild( _xor_( "##other_child" ), ImVec2( 395, 125 ), true ); // Child Start
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "Other" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Text( _xor_( "Hide Window" ) );
					utils::hotkey_hide_window( &config::settings_hide_window_keybind );
					ImGui::SetCursorPos( ImVec2( 10.0f, 65.0f ) );
					ImGui::Text( _xor_( "Click Sound Location:" ) );
					ImGui::SameLine( );
					ImGui::PushItemWidth( 220 );
					ImGui::InputText( _xor_( "##click_sound_location_textbox" ), config::settings_click_sound_location, IM_ARRAYSIZE( config::settings_click_sound_location ) );
					ImGui::SetCursorPos( ImVec2( 10.0f, 90.0f ) );
					ImGui::Checkbox( _xor_( "Tooltips##tooltps_checkbox" ), &config::settings_tooltips );
					ImGui::EndChild( ); // Child End

					// Destruct
					ImGui::SetCursorPos( ImVec2( 45.0f, 220.0f ) );
					ImGui::BeginChild( _xor_( "##destruct_child" ), ImVec2( 395, 75 ), true ); // Child Start
					ImGui::SetCursorPos( ImVec2( 10.0f, 5.0f ) );
					ImGui::PushFont( pollyroundedbold_font );
					ImGui::Text( _xor_( "Destruct" ) );
					ImGui::PopFont( );
					ImGui::SetCursorPos( ImVec2( 10.0f, 40.0f ) );
					ImGui::Checkbox( _xor_( "Delete File##delete_files_checkbox" ), &config::destruct_delete_file );
					utils::tooltip( _xor_( "Deletes the file on exit. (This is detected)" ) );
					ImGui::EndChild( ); // Child End
				}

				ImGui::SetCursorPos( ImVec2( 165.0f, 556.0f ) );
				ImGui::Text( _xor_( "Lithium Client | v1.0" ) );

				ImGui::End( );
			}
			ImGui::PopFont( );

			ImGui::Render( );
			g_pd3dDeviceContext->OMSetRenderTargets( 1, &g_mainRenderTargetView, NULL );
			g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView, ( float* )&clear_color );
			ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

			if ( g_pSwapChain->Present( 1, 0 ) == DXGI_STATUS_OCCLUDED ) {
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
			}
		}

		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );

		CleanupDeviceD3D( );
		DestroyWindow( config::hwnd );
		UnregisterClass( wc.lpszClassName, wc.hInstance );

		//    =====================================
		//    |             Destruct              |
		//    =====================================

		// Registry clean
		HKEY hKey = NULL;
		long resposta = RegOpenKeyEx( HKEY_CURRENT_USER, _T( _xor_( "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\\*" ) ), 0L, KEY_ALL_ACCESS, &hKey );
		if ( resposta == ERROR_SUCCESS ) {
			RegDeleteValue( hKey, _T( _xor_( "MRUListEx" ) ) );
			RegCloseKey( hKey );
		}
		resposta = RegOpenKeyEx( HKEY_CURRENT_USER, _T( _xor_( "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedPidlMRU" ) ), 0L, KEY_ALL_ACCESS, &hKey );
		if ( resposta == ERROR_SUCCESS )  {
			RegDeleteValue( hKey, _T( _xor_( "MRUListEx" ) ) );
			RegCloseKey( hKey );
		}
		resposta = RegOpenKeyEx( HKEY_CURRENT_USER, _T( _xor_( "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\\dll" ) ), 0L, KEY_ALL_ACCESS, &hKey );
		if ( resposta == ERROR_SUCCESS ) {
			RegDeleteValue( hKey, _T( _xor_( "MRUListEx" ) ) );
			RegCloseKey( hKey );
		}

		// String clean
		utils::terminate_service( _xor_( "DPS" ) );
		utils::start_service( _xor_( "DPS" ) );

		// Delete file
		if ( config::destruct_delete_file ) {
			utils::delete_exe( );
		}

		return 0;
	}
	else {
		config::authenticated = false;

		std::cout << std::endl << _xor_( "Not Authenticated! Your HWID has been copied to your clipboard." );
		utils::copy_to_clipboard( utils::get_hwid( ) );

		std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );

		return 0;
	}
}

bool CreateDeviceD3D( HWND hWnd ) {
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[ 2 ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if ( D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext ) != S_OK )
		return false;

	CreateRenderTarget( );
	return true;
}

void CleanupDeviceD3D( ) {
	CleanupRenderTarget( );
	if ( g_pSwapChain) { g_pSwapChain->Release( ); g_pSwapChain = NULL; }
	if ( g_pd3dDeviceContext ) { g_pd3dDeviceContext->Release( ); g_pd3dDeviceContext = NULL; }
	if ( g_pd3dDevice ) { g_pd3dDevice->Release( ); g_pd3dDevice = NULL; }
}

void CreateRenderTarget( ) {
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
	g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_mainRenderTargetView );
	pBackBuffer->Release( );
}

void CleanupRenderTarget( ) {
	if ( g_mainRenderTargetView ) { g_mainRenderTargetView->Release( ); g_mainRenderTargetView = NULL; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
		return true;

	switch ( msg ) {
	case WM_SIZE:
		if ( g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED ) {
			CleanupRenderTarget( );
			g_pSwapChain->ResizeBuffers( 0, ( UINT )LOWORD( lParam ), ( UINT )HIWORD( lParam ), DXGI_FORMAT_UNKNOWN, 0 );
			CreateRenderTarget( );
		}
		return 0;
	case WM_SYSCOMMAND:
		if ( ( wParam & 0xfff0 ) == SC_KEYMENU ) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
	}
	return DefWindowProc( hWnd, msg, wParam, lParam );
}