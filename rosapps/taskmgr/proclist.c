/*
 *  ReactOS Task Manager
 *
 *  proclist.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
	
#include "taskmgr.h"
#include "procpage.h"
#include "proclist.h"
#include "perfdata.h"


LRESULT CALLBACK	ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LONG				OldProcessListWndProc;


LRESULT CALLBACK ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HBRUSH	hbrBackground;
	RECT	rcItem;
	RECT	rcClip;
	HDC		hDC;
	int		DcSave;

	switch (message)
	{
	case WM_ERASEBKGND:

		//
		// The list control produces a nasty flicker
		// when the user is resizing the window because
		// it erases the background to white, then
		// paints the list items over it.
		//
		// We will clip the drawing so that it only
		// erases the parts of the list control that
		// show only the background.
		//

		//
		// Get the device context and save it's state
		// to be restored after we're done
		//
		hDC = (HDC) wParam;
		DcSave = SaveDC(hDC);

		//
		// Get the background brush
		//
		hbrBackground = (HBRUSH) GetClassLong(hWnd, GCL_HBRBACKGROUND);

		//
		// Calculate the clip rect by getting the RECT
		// of the first and last items and adding them up.
		//
		// We also have to get the item's icon RECT and
		// subtract it from our clip rect because we don't
		// use icons in this list control.
		//
		ListView_GetItemRect(hWnd, 0, &rcClip, LVIR_BOUNDS);
		ListView_GetItemRect(hWnd, ListView_GetItemCount(hWnd) - 1, &rcItem, LVIR_BOUNDS);
		rcClip.bottom = rcItem.bottom;
		ListView_GetItemRect(hWnd, 0, &rcItem, LVIR_ICON);
		rcClip.left = rcItem.right;

		//
		// Now exclude the clip rect
		//
		ExcludeClipRect(hDC, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

		//
		// Now erase the background
		//
		//
		// FIXME: Should I erase it myself or
		// pass down the updated HDC and let
		// the default handler do it?
		//
		GetClientRect(hWnd, &rcItem);
		FillRect(hDC, &rcItem, hbrBackground);

		//
		// Now restore the DC state that we
		// saved earlier
		//
		RestoreDC(hDC, DcSave);
		
		return TRUE;
	}

	//
	// We pass on all messages except WM_ERASEBKGND
	//
	return CallWindowProc((WNDPROC)OldProcessListWndProc, hWnd, message, wParam, lParam);
}
