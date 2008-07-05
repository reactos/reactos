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

extern void ExitInstance();

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static int s_factor = 2;	// zoom factor

static POINT s_srcPos = {0, 0};			// source rectangle position
static RECT s_lastSrc = {-1,-1,-1,-1};	// last cursor position

BOOL s_dragging = FALSE;


 // zoom range

#define MIN_ZOOM	1
#define MAX_ZOOM	16


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: SetZoom()
//
// PURPOSE:  Change zoom level
//

static void SetZoom(HWND hWnd, int factor)
{
	TCHAR buffer[MAX_LOADSTRING];

	if (factor>=MIN_ZOOM && factor<=MAX_ZOOM) {
		s_factor = factor;

		SetScrollPos(hWnd, SB_VERT, s_factor, TRUE);

		wsprintf(buffer, TEXT("%s  %dx"), szTitle, s_factor);
		SetWindowText(hWnd, buffer);
	}
}


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

	case ID_REFRESH:
		InvalidateRect(hWnd, NULL, FALSE);
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
		SetScrollRange(hWnd, SB_VERT, 1, MAX_ZOOM, FALSE);
		SetZoom(hWnd, s_factor);
		break;

	case WM_COMMAND:
		if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdcMem;
		RECT clnt;
		SIZE size;

		BeginPaint(hWnd, &ps);
		hdcMem = GetDC(GetDesktopWindow());

		GetClientRect(hWnd, &clnt);
		size.cx = (clnt.right + s_factor-1) / s_factor;
		size.cy = (clnt.bottom + s_factor-1) / s_factor;

		StretchBlt(ps.hdc, 0, 0, size.cx*s_factor, size.cy*s_factor,
					hdcMem, s_srcPos.x, s_srcPos.y, size.cx, size.cy, SRCCOPY);

		ReleaseDC(GetDesktopWindow(), hdcMem);
		EndPaint(hWnd, &ps);
		break;}

	case WM_TIMER:
		if (s_dragging && GetCapture()==hWnd) {
			RECT clnt, rect;

			int width = GetSystemMetrics(SM_CXSCREEN);
			int height = GetSystemMetrics(SM_CYSCREEN);

			GetCursorPos(&s_srcPos);

			GetClientRect(hWnd, &clnt);

			s_srcPos.x -= clnt.right / s_factor / 2;
			s_srcPos.y -= clnt.bottom / s_factor / 2;

			if (s_srcPos.x < 0)
				s_srcPos.x = 0;
			else if (s_srcPos.x+clnt.right/s_factor > width)
				s_srcPos.x = width - clnt.right/s_factor;

			if (s_srcPos.y < 0)
				s_srcPos.y = 0;
			else if (s_srcPos.y+clnt.bottom/s_factor > height)
				s_srcPos.y = height - clnt.bottom/s_factor;

			if (memcmp(&rect, &s_lastSrc, sizeof(RECT))) {
				HDC hdc = GetDC(0);

				if (s_lastSrc.bottom != -1)
					DrawFocusRect(hdc, &s_lastSrc);

				rect.left = s_srcPos.x - 1;
				rect.top = s_srcPos.y - 1;
				rect.right = rect.left + clnt.right/s_factor + 2;
				rect.bottom = rect.top + clnt.bottom/s_factor + 2;
				DrawFocusRect(hdc, &rect);

				ReleaseDC(0, hdc);

				s_lastSrc = rect;
			}
		}

		InvalidateRect(hWnd, NULL, FALSE);
		UpdateWindow(hWnd);
		break;

	case WM_LBUTTONDOWN:
		s_dragging = TRUE;
		SetCapture(hWnd);
		break;

	case WM_LBUTTONUP:
	case WM_CANCELMODE:
		if (s_dragging) {
			HDC hdc = GetDC(0);
			DrawFocusRect(hdc, &s_lastSrc);
			ReleaseDC(0, hdc);

			s_lastSrc.bottom = -1;

			s_dragging = FALSE;
			ReleaseCapture();
		}
		break;

	case WM_VSCROLL:
		switch(wParam) {
		  case SB_LINEUP:
		  case SB_PAGEUP:
			SetZoom(hWnd, s_factor-1);
			break;

		  case SB_LINEDOWN:
		  case SB_PAGEDOWN:
			SetZoom(hWnd, s_factor+1);
			break;
		}
		break;

	case WM_DESTROY:
		KillTimer(hWnd, 0);
		PostQuitMessage(0);
		ExitInstance();
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}
