//----------------------------------------------------------------------------------
//		拡張編集ウィンドウメッセージ監視
//		ソースファイル
//		Visual C++ 2022
//----------------------------------------------------------------------------------

#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

#include <mkaul/include/mkaul.hpp>
#include <aviutl.hpp>
#include <exedit.hpp>

#pragma comment(lib, "C:/aviutl_libs/mkaul/mkaul.lib")


#define FILTER_NAME				"拡張編集ウィンドウメッセージ監視"
#define FILTER_VERSION			"v1.0.1"
#define FILTER_DEVELOPER		"mimaraka"
#define COMMAND_HEIGHT			18
#define INPUT_HEIGHT			24
#define BUTTON_WIDTH			100
#define COMMAND_FILTER			0x4000
#define POPSTR(str)				::MessageBox(NULL, str, FILTER_NAME, MB_ICONINFORMATION)
#define POPNUM(n)				::MessageBox(NULL, std::to_string(n).c_str(), FILTER_NAME, MB_ICONINFORMATION)



BOOL(*wndproc_exedit_orig)(HWND, UINT, WPARAM, LPARAM, AviUtl::EditHandle*, AviUtl::FilterPlugin*);
HWND hwnd_observer;
HWND hwnd_list_exedit, hwnd_list_obj;
std::vector<int> g_filter_list;
mkaul::exedit::Internal g_exedit_internal;



//---------------------------------------------------------------------
//		ウィンドウプロシージャ(オブジェクト設定ダイアログ)
//---------------------------------------------------------------------
LRESULT CALLBACK wndproc_objdialog_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	char result[1024] = "";

	if (sizeof(g_filter_list)) {
		for (int m : g_filter_list) {
			if (message == m) {
				return (g_exedit_internal.get_wndproc_objdialog())(hwnd, message, wparam, lparam);
			}
		}
	}

	::sprintf_s(result, 1024, "Message: 0x%04x, wParam:%12d, lParam:%12d", (int)message, (int)wparam, (int)lparam);
	::SendMessage(hwnd_list_obj, LB_ADDSTRING, 0, (LPARAM)result);
	::SendMessage(hwnd_list_obj, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);

	return (g_exedit_internal.get_wndproc_objdialog())(hwnd, message, wparam, lparam);
}



//---------------------------------------------------------------------
//		ウィンドウプロシージャ(タイムラインウィンドウ)
//---------------------------------------------------------------------
BOOL wndproc_timeline_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	char result[1024] = "";

	if (sizeof(g_filter_list)) {
		for (int m : g_filter_list) {
			if (message == m)
				return wndproc_exedit_orig(hwnd, message, wparam, lparam, editp, fp);
		}
	}

	::sprintf_s(
		result,
		1024,
		"Message: 0x%04x, wParam:%12d, lParam:%12d",
		(int)message,
		(int)wparam,
		(int)lparam
	);
	::SendMessage(hwnd_list_exedit, LB_ADDSTRING, 0, (LPARAM)result);
	::SendMessage(hwnd_list_exedit, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);

	return wndproc_exedit_orig(hwnd, message, wparam, lparam, editp, fp);
}



//---------------------------------------------------------------------
//		ウィンドウプロシージャ
//---------------------------------------------------------------------
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	static HWND edit_filter;
	static HWND button;
	RECT rect_wnd;

	::GetClientRect(hwnd, &rect_wnd);

	switch (message) {
	case AviUtl::FilterPlugin::WindowMessage::Init:
	{
		hwnd_observer = hwnd;

		hwnd_list_exedit = ::CreateWindowEx(
			NULL,
			"LISTBOX",
			NULL,
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
			0,
			INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - INPUT_HEIGHT,
			hwnd,
			(HMENU)1,
			fp->dll_hinst, NULL
		);

		hwnd_list_obj = ::CreateWindowEx(
			NULL,
			"LISTBOX",
			NULL,
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
			rect_wnd.right / 2,
			INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - INPUT_HEIGHT,
			hwnd,
			(HMENU)2,
			fp->dll_hinst, NULL
		);

		HFONT font = ::CreateFont(
			COMMAND_HEIGHT, 0,
			0, 0,
			FW_REGULAR,
			FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			NULL,
			"Consolas"
		);

		edit_filter = ::CreateWindowEx(
			NULL,
			"EDIT",
			NULL,
			WS_CHILD | WS_VISIBLE,
			0,
			0,
			rect_wnd.right - BUTTON_WIDTH,
			INPUT_HEIGHT,
			hwnd,
			NULL,
			fp->dll_hinst,
			NULL
		);

		button = ::CreateWindowEx(
			NULL,
			"BUTTON",
			"Hide",
			WS_CHILD | WS_VISIBLE,
			rect_wnd.right - BUTTON_WIDTH,
			0,
			BUTTON_WIDTH,
			INPUT_HEIGHT,
			hwnd,
			(HMENU)COMMAND_FILTER,
			fp->dll_hinst,
			NULL
		);

		::SendMessage(hwnd_list_exedit, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
		::SendMessage(hwnd_list_obj, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
		::SendMessage(edit_filter, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));

		return 0;
	}

	case WM_SIZE:
		::MoveWindow(hwnd_list_exedit,
			0,
			INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - INPUT_HEIGHT,
			TRUE);
		::MoveWindow(hwnd_list_obj,
			rect_wnd.right / 2,
			INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - INPUT_HEIGHT,
			TRUE);
		::MoveWindow(edit_filter,
			0,
			0,
			rect_wnd.right - BUTTON_WIDTH,
			INPUT_HEIGHT,
			TRUE);
		::MoveWindow(button,
			rect_wnd.right - BUTTON_WIDTH,
			0,
			BUTTON_WIDTH,
			INPUT_HEIGHT,
			TRUE);
		::SendMessage(hwnd_list_exedit, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);
		::SendMessage(hwnd_list_obj, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);
		return 0;

	case WM_COMMAND:
		switch (wparam) {
		// FILTERボタン押下時
		case COMMAND_FILTER:
		{
			char filter[1024];
			::GetWindowText(edit_filter, filter, 1024);

			std::string str = filter;
			
			auto vec_str = mkaul::split(str, ',');

			for (auto s : vec_str) {
				try {
					g_filter_list.emplace_back(std::strtol(s.c_str(), NULL, 16));
				}
				catch (std::invalid_argument&) {
					continue;
				}
			}
			return 0;
		}
		}
		return 0;
	}
	return 0;
}



BOOL func_init(AviUtl::FilterPlugin* fp)
{
	if (!g_exedit_internal.init(fp)) {
		::MessageBox(NULL, "拡張編集(ver.0.92)が見つからないか、バージョンが異なります。", FILTER_NAME, MB_ICONERROR);
		return FALSE;
	}

	// フック処理(タイムラインウィンドウ)
	wndproc_exedit_orig = g_exedit_internal.fp()->func_WndProc;
	g_exedit_internal.fp()->func_WndProc = wndproc_timeline_hooked;
	
	// フック処理(オブジェクト設定ダイアログ)
	mkaul::replace_var(g_exedit_internal.base_address() + 0x2e804, &wndproc_objdialog_hooked);

	return TRUE;
}



//---------------------------------------------------------------------
//		FILTER構造体のポインタを取得
//---------------------------------------------------------------------
auto __stdcall GetFilterTable()
{
	using Flag = AviUtl::FilterPluginDLL::Flag;
	static AviUtl::FilterPluginDLL filter = {
		.flag = Flag::AlwaysActive |
				Flag::WindowSize |
				Flag::WindowThickFrame |
				Flag::DispFilter |
				Flag::ExInformation,
		.x = 450,
		.y = 300,
		.name = FILTER_NAME,
		.func_init = func_init,
		.func_WndProc = func_WndProc,
		.information = FILTER_NAME " " FILTER_VERSION " by " FILTER_DEVELOPER
	};

	return &filter;
}