/*
	vfdguitip.c

	Virtual Floppy Drive for Windows
	Driver control library
	tooltip information GUI utility functions

	Copyright (c) 2003-2005 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdlib.h"
#ifndef __REACTOS__
#include "vfdmsg.h"
#else
#include "vfdmsg_lib.h"
#endif

//
//	tooltip window class name
//
#define VFD_INFOTIP_WNDCLASS	"VfdInfoTip"

//
//	the window procedure
//
static LRESULT CALLBACK ToolTipProc(
	HWND			hWnd,
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		//	Store Font handle
#ifndef __REACTOS__
		SetWindowLong(hWnd, GWL_USERDATA,
			(LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);
#else
		SetWindowLongPtr(hWnd, GWLP_USERDATA,
			(LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
#endif
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC hDC = BeginPaint(hWnd, &paint);

			if (hDC) {
				char text[MAX_PATH];
				int len;
				RECT rc;


#ifndef __REACTOS__
				SelectObject(hDC, (HFONT)GetWindowLong(hWnd, GWL_USERDATA));
#else
				SelectObject(hDC, (HFONT)GetWindowLongPtr(hWnd, GWLP_USERDATA));
#endif

				SetTextColor(hDC, GetSysColor(COLOR_INFOTEXT));
				SetBkMode(hDC, TRANSPARENT);

				len = GetWindowText(hWnd, text, sizeof(text));

				rc.top = 8;
				rc.left = 8;
				rc.right = paint.rcPaint.right;
				rc.bottom = paint.rcPaint.bottom;

				DrawText(hDC, text, len, &rc, DT_LEFT | DT_TOP);

				EndPaint(hWnd, &paint);
			}
		}
		return 0;

	case WM_KILLFOCUS:
		if (!(GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
			//	Stick tool tip - Closed on kill focus
			DestroyWindow(hWnd);
		}
		return 0;

	case WM_SETCURSOR:
		if (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) {
			//	Non-stick tool tip - Closed when cursor leaves
			TRACKMOUSEEVENT track;

			track.cbSize	= sizeof(track);
			track.dwFlags	= TME_LEAVE;
			track.hwndTrack	= hWnd;
			track.dwHoverTime = 0;

			TrackMouseEvent(&track);
		}
		return 0;

	case WM_MOUSELEAVE:
		//	Non-stick tool tip - Closed when cursor leaves
		DestroyWindow(hWnd);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		//	Both stick and non-stick tool tip
		//	Closed when clicked
		SetCapture(hWnd);
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		//	Both stick and non-stick tool tip
		//	Closed when clicked
		if (GetCapture() == hWnd) {
			DestroyWindow(hWnd);
		}
		return 0;

	case WM_DESTROY:
		//	delete font
#ifndef __REACTOS__
		DeleteObject((HFONT)GetWindowLong(hWnd, GWL_USERDATA));
#else
		DeleteObject((HFONT)GetWindowLongPtr(hWnd, GWLP_USERDATA));
#endif
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//
//	Create and show tooltip window
//
void WINAPI VfdToolTip(
	HWND			hParent,
	PCSTR			sText,
	int				pos_x,
	int				pos_y,
	BOOL			stick)
{
#ifndef __REACTOS__
	HWND			hWnd;
#endif
	WNDCLASS		wc = {0};
	LOGFONT			lf;
	HFONT			font;
	HDC				dc;
	int				len;
	SIZE			sz;
	RECT			rc;
	int				scr_x;
	int				scr_y;

	//
	//	Register Window Class
	//

	wc.lpfnWndProc		= ToolTipProc;
	wc.hInstance		= g_hDllModule;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)(COLOR_INFOBK + 1);
	wc.lpszClassName	= VFD_INFOTIP_WNDCLASS;

	RegisterClass(&wc);

	//
	//	Create Tool Tip Font (== Icon title font)
	//

	SystemParametersInfo(
		SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);

	font = CreateFontIndirect(&lf);

	//
	//	Calculate Tool Tip Window size
	//

	dc = GetDC(hParent);

	SelectObject(dc, font);

	len = strlen(sText);

	GetTextExtentPoint32(dc, sText, len, &sz);

	rc.left		= 0;
	rc.top		= 0;
	rc.right	= sz.cx;
	rc.bottom	= sz.cy;

	DrawText(dc, sText, len, &rc, DT_CALCRECT | DT_LEFT | DT_TOP);

	ReleaseDC(hParent, dc);

	sz.cx = rc.right - rc.left + 16;
	sz.cy = rc.bottom - rc.top + 16;

	//
	//	Decide the window position
	//
	if (pos_x == -1 || pos_y == -1) {
		//
		//	Use current cursor position
		//
		POINT	pt;

		GetCursorPos(&pt);

		pos_x = pt.x - (sz.cx / 2);
		pos_y = pt.y - (sz.cy / 2);
	}
	else {
		pos_x = pos_x - (sz.cx / 2);
	}

	//
	//	make sure the tip window fits in visible area
	//
	scr_x = GetSystemMetrics(SM_CXSCREEN);
	scr_y = GetSystemMetrics(SM_CYSCREEN);

	if (pos_x < 0) {
		pos_x = 0;
	}
	if (pos_x + sz.cx > scr_x) {
		pos_x = scr_x - sz.cx;
	}
	if (pos_y < 0) {
		pos_y = 0;
	}
	if (pos_y + sz.cy > scr_y) {
		pos_y = scr_y - sz.cy;
	}

	//
	//	Create the tool tip window
	//
#ifndef __REACTOS__
	hWnd = CreateWindowEx(
#else
	CreateWindowEx(
#endif
		stick ? 0 : WS_EX_TOPMOST,
		VFD_INFOTIP_WNDCLASS,
		sText,
		WS_BORDER | WS_POPUP | WS_VISIBLE,
		pos_x, pos_y,
		sz.cx, sz.cy,
		hParent,
		NULL,
		NULL,
		(PVOID)font);

	//
	//	Give focus if it is not a stick tool-tip
	//
	if (!stick) {
		SetFocus(hParent);
	}
}

//
//	Show an image information tooltip
//
void WINAPI VfdImageTip(
	HWND			hParent,
	ULONG			nDevice)
{
	HANDLE			hDevice;
	PSTR			info_str	= NULL;
	PSTR			type_str	= NULL;
	PSTR			prot_str	= NULL;
	PCSTR			media_str	= NULL;
	CHAR			path[MAX_PATH];
	CHAR			desc[MAX_PATH];
	VFD_DISKTYPE	disk_type;
	VFD_MEDIA		media_type;
	VFD_FLAGS		media_flags;
	VFD_FILETYPE	file_type;
	ULONG			image_size;
	DWORD			file_attr;
	ULONG			ret;

	hDevice = VfdOpenDevice(nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		VfdToolTip(hParent,
			SystemMessage(GetLastError()), -1, -1, FALSE);
		return;
	}

	ret = VfdGetImageInfo(
		hDevice,
		path,
		&disk_type,
		&media_type,
		&media_flags,
		&file_type,
		&image_size);

	CloseHandle(hDevice);

	if (ret != ERROR_SUCCESS) {
		VfdToolTip(hParent, SystemMessage(ret), -1, -1, FALSE);
		return;
	}

	if (path[0]) {
		file_attr = GetFileAttributes(path);
	}
	else {
		if (disk_type != VFD_DISKTYPE_FILE) {
			strcpy(path, "<RAM>");
		}
		file_attr = 0;
	}

	VfdMakeFileDesc(desc, sizeof(desc),
		file_type, image_size, file_attr);

	if (disk_type == VFD_DISKTYPE_FILE) {
		type_str = "FILE";
	}
	else {
		type_str = "RAM";
	}

	media_str = VfdMediaTypeName(media_type);

	if (media_flags & VFD_FLAG_WRITE_PROTECTED) {
		prot_str = ModuleMessage(MSG_WRITE_PROTECTED);
	}
	else {
		prot_str = ModuleMessage(MSG_WRITE_ALLOWED);
	}

	info_str = ModuleMessage(
		MSG_IMAGE_INFOTIP,
		path,
		desc,
		type_str ? type_str : "",
		media_str ? media_str : "",
		prot_str ? prot_str : "");

	if (info_str) {
		VfdToolTip(hParent, info_str, -1, -1, FALSE);
		LocalFree(info_str);
	}

	if (prot_str) {
		LocalFree(prot_str);
	}
}
