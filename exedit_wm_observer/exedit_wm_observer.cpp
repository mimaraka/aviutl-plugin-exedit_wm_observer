//----------------------------------------------------------------------------------
//		イージング ドラッグ&ドロップ
//		ソースファイル
//		Visual C++ 2022
//----------------------------------------------------------------------------------

#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <aviutl_plugin_sdk/filter.h>
#include <aulslib/exedit.h>
#include <auluilib/aului.hpp>


#define EWO_FILTER_NAME		"拡張編集ウィンドウメッセージ監視"
#define FILTER_INFO			EWO_FILTER_NAME "v1.0"
#define DEFAULT_WIDTH		450
#define DEFAULT_HEIGHT		300
#define EWO_COMMAND_HEIGHT	18
#define EWO_INPUT_HEIGHT	24
#define EWO_BUTTON_WIDTH	100
#define EWO_CM_FILTER		0x4000
#define POPUP(content)		::MessageBox(hwnd, content, "Debug Message", MB_OK)



WNDPROC wndproc_obj_orig;
WNDPROC wndproc_exedit_orig;
HWND hwnd_observer;
HWND hwnd_list_exedit, hwnd_list_obj;
HWND g_hwnd_exedit;
HWND g_hwnd_obj;
std::vector<int> g_filter_list;



//---------------------------------------------------------------------
//		ウィンドウプロシージャ(オブジェクト設定ウィンドウ)
//---------------------------------------------------------------------
LRESULT CALLBACK wndproc_obj(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	char result[1024] = "";

	if (sizeof(g_filter_list))
		for (int m : g_filter_list)
			if (msg == m)
				return ::CallWindowProc(wndproc_obj_orig, hwnd, msg, wparam, lparam);

	::sprintf_s(result, 1024, "Message: 0x%04x, wParam:%12d, lParam:%12d", (int)msg, (int)wparam, (int)lparam);
	::SendMessage(hwnd_list_obj, LB_ADDSTRING, 0, (LPARAM)result);
	::SendMessage(hwnd_list_obj, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);

	return ::CallWindowProc(wndproc_obj_orig, hwnd, msg, wparam, lparam);
}



//---------------------------------------------------------------------
//		ウィンドウプロシージャ(拡張編集ウィンドウ)
//---------------------------------------------------------------------
LRESULT CALLBACK wndproc_exedit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	char result[1024] = "";

	if (sizeof(g_filter_list))
		for (int m : g_filter_list)
			if (msg == m)
				return ::CallWindowProc(wndproc_exedit_orig, hwnd, msg, wparam, lparam);

	::sprintf_s(result, 1024, "Message: 0x%04x, wParam:%12d, lParam:%12d", (int)msg, (int)wparam, (int)lparam);
	::SendMessage(hwnd_list_exedit, LB_ADDSTRING, 0, (LPARAM)result);
	::SendMessage(hwnd_list_exedit, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);

	return ::CallWindowProc(wndproc_exedit_orig, hwnd, msg, wparam, lparam);
}



std::vector<int> split(const std::string& s)
{
	std::vector<int> elems;
	std::string item;
	for (char ch : s) {
		if (ch == ',') {
			if (!item.empty()) {
				elems.emplace_back(std::strtol(item.c_str(), NULL, 16));
				item.clear();
			}
		}
		else item += ch;
	}
	if (!item.empty())
		elems.emplace_back(std::strtol(item.c_str(), NULL, 16));

	return elems;
}



//---------------------------------------------------------------------
//		ウィンドウプロシージャ
//---------------------------------------------------------------------
BOOL filter_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, void* editp, FILTER* fp)
{
	static HWND edit_filter;
	static HWND button;
	RECT rect_wnd;
	static RECT rect_button;

	rect_button = {
		0, 0, 0, 0
	};

	::GetClientRect(hwnd, &rect_wnd);

	switch (msg) {
	case WM_FILTER_INIT:
	{
		hwnd_observer = hwnd;
		g_hwnd_exedit = auls::Exedit_GetWindow(fp);
		g_hwnd_obj = auls::ObjDlg_GetWindow(g_hwnd_exedit);

		wndproc_exedit_orig = (WNDPROC)::GetWindowLong(g_hwnd_exedit, GWL_WNDPROC);
		::SetWindowLong(g_hwnd_exedit, GWL_WNDPROC, (LONG)wndproc_exedit);

		wndproc_obj_orig = (WNDPROC)::GetWindowLong(g_hwnd_obj, GWL_WNDPROC);
		::SetWindowLong(g_hwnd_obj, GWL_WNDPROC, (LONG)wndproc_obj);

		hwnd_list_exedit = ::CreateWindowEx(
			NULL,
			"LISTBOX",
			NULL,
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
			0,
			EWO_INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - EWO_INPUT_HEIGHT,
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
			EWO_INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - EWO_INPUT_HEIGHT,
			hwnd,
			(HMENU)2,
			fp->dll_hinst, NULL
		);

		HFONT font = ::CreateFont(
			EWO_COMMAND_HEIGHT, 0,
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
			rect_wnd.right - EWO_BUTTON_WIDTH,
			EWO_INPUT_HEIGHT,
			hwnd,
			NULL,
			fp->dll_hinst,
			NULL
		);

		button = ::CreateWindowEx(
			NULL,
			"BUTTON",
			"Filter",
			WS_CHILD | WS_VISIBLE,
			rect_wnd.right - EWO_BUTTON_WIDTH,
			0,
			EWO_BUTTON_WIDTH,
			EWO_INPUT_HEIGHT,
			hwnd,
			(HMENU)EWO_CM_FILTER,
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
			EWO_INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - EWO_INPUT_HEIGHT,
			TRUE);
		::MoveWindow(hwnd_list_obj,
			rect_wnd.right / 2,
			EWO_INPUT_HEIGHT,
			rect_wnd.right / 2,
			rect_wnd.bottom - EWO_INPUT_HEIGHT,
			TRUE);
		::MoveWindow(edit_filter,
			0,
			0,
			rect_wnd.right - EWO_BUTTON_WIDTH,
			EWO_INPUT_HEIGHT,
			TRUE);
		::MoveWindow(button,
			rect_wnd.right - EWO_BUTTON_WIDTH,
			0,
			EWO_BUTTON_WIDTH,
			EWO_INPUT_HEIGHT,
			TRUE);
		::SendMessage(hwnd_list_exedit, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);
		::SendMessage(hwnd_list_obj, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);
		return 0;

	case WM_COMMAND:
		switch (wparam) {
		case EWO_CM_FILTER:
		{
			std::string str;
			char filter[1024];
			::GetWindowText(edit_filter, filter, 1024);
			str = filter;
			try {
				g_filter_list = split(str);
			}
			catch (std::invalid_argument&) {
				::MessageBox(hwnd, "Invalid argument", "Error", MB_ICONERROR);
			}

			return 0;
		}
		}
		return 0;

	}
	return 0;
}



//---------------------------------------------------------------------
//		FILTER構造体を定義
//---------------------------------------------------------------------
FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE |
	FILTER_FLAG_WINDOW_SIZE |
	FILTER_FLAG_WINDOW_THICKFRAME |
	FILTER_FLAG_DISP_FILTER |
	FILTER_FLAG_EX_INFORMATION,
	DEFAULT_WIDTH,
	DEFAULT_HEIGHT,
	EWO_FILTER_NAME,
	NULL,NULL,NULL,
	NULL,NULL,
	NULL,NULL,NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	filter_wndproc,
	NULL,NULL,
	NULL,
	NULL,
	FILTER_INFO,
	NULL,NULL,
	NULL,NULL,NULL,NULL,
	NULL,
};



//---------------------------------------------------------------------
//		FILTER構造体のポインタを取得
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport)* __stdcall GetFilterTable(void)
{
	return &filter;
}