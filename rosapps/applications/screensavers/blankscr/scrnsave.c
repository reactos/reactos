/*
 *  Copyright 2003 J Brown
 *  Copyright 2006 Eric Kohl
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <windows.h>
#include <scrnsave.h>
#include "resource.h"

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
    static HDC  hdc;
    static RECT rc;

    switch(message)
    {
        case WM_CREATE:
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_ERASEBKGND:
            hdc = GetDC(hwnd);
            GetClientRect (hwnd, &rc);
            FillRect (hdc, &rc, GetStockObject(BLACK_BRUSH));
            ReleaseDC(hwnd,hdc);
            break;
       case WM_PAINT:
            break;
       default:
            return DefScreenSaverProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}
