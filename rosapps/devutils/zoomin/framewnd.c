/*
 *	ReactOS zoomin
 *
 *	framewnd.c
 *
 *	Copyright (C) 2002	Robert Dickenson <robd@reactos.org>
 *	Copyright (C) 2005	Martin Fuchs <martin-fuchs@gmx.net>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define WIN32_LEAN_AND_MEAN 	// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "main.h"
#include "framewnd.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static int s_factor = 2;	// zoom factor

static POINT s_srcPos = {0, 0}; // zoom factor


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//


////////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
//
//	PURPOSE:  Processes WM_COMMAND messages for the main frame window.
//
//

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
	// Parse the menu selections:
	case ID_EDIT_EXIT:
		DestroyWindow(hWnd);
		break;

	case ID_EDIT_COPY:
	case ID_EDIT_REFRESH:
	case ID_OPTIONS_REFRESH_RATE:
	case ID_HELP_ABOUT:
		// TODO:
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION: FrameWndProc(HWND, unsigned, WORD, LONG)
//
//	PURPOSE:  Processes messages for the main window.
//

LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		SetTimer(hWnd, 0, 200, NULL);	// refresh display all 200 ms
		break;

	case WM_COMMAND:
		if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdcMem;
		RECT rect;
		SIZE size;

		BeginPaint(hWnd, &ps);
		hdcMem = GetDC(GetDesktopWindow());

		GetClientRect(hWnd, &rect);
		size.cx = rect.right / s_factor;
		size.cy = rect.bottom / s_factor;

		StretchBlt(ps.hdc, 0, 0, size.cx*s_factor, size.cy*s_factor, hdcMem, s_srcPos.x, s_srcPos.y, size.cx, size.cy, SRCCOPY);

		ReleaseDC(GetDesktopWindow(), hdcMem);
		EndPaint(hWnd, &ps);
		break;}

	case WM_TIMER:
		if (GetCapture() == hWnd) {
			RECT rect;

			int width = GetSystemMetrics(SM_CXSCREEN);
			int height = GetSystemMetrics(SM_CYSCREEN);

			GetClientRect(hWnd, &rect);

			GetCursorPos(&s_srcPos);

			s_srcPos.x -= rect.right / s_factor / 2;
			s_srcPos.y -= rect.bottom / s_factor / 2;

			if (s_srcPos.x < 0)
				s_srcPos.x = 0;
			else if (s_srcPos.x+rect.right/s_factor > width)
				s_srcPos.x = width - rect.right/s_factor;

			if (s_srcPos.y < 0)
				s_srcPos.y = 0;
			else if (s_srcPos.y+rect.bottom/s_factor > height)
				s_srcPos.y = height - rect.bottom/s_factor;
		}

		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		break;

	case WM_DESTROY:
		KillTimer(hWnd, 0);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}
