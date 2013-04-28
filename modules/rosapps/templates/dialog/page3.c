/*
 *  ReactOS Standard Dialog Application Template
 *
 *  page3.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <assert.h>
#include "resource.h"
#include "trace.h"


extern HINSTANCE hInst;


////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK PageWndProc3(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            break;
        }
        break;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

