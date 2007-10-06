/*
 *  ReactOS
 *  Copyright (C) 2007 ReactOS Team
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
/*
 *
 * PROJECT:         			input.dll
 * FILE:            			dll/win32/input/inputlangprop.c
 * PURPOSE:         			input.dll
 * PROGRAMMER:      		Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

#include "resource.h"
#include "input.h"

static
VOID
SelectKeyboardLayout(HWND hWnd)
{
	TCHAR Layout[256];
	
	SendMessage(hWnd,
			    CB_SELECTSTRING,
				(WPARAM) -1,
				(LPARAM)Layout);
}

INT_PTR CALLBACK
InputLangPropDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
			CreateKeyboardLayoutList(GetDlgItem(hDlg, IDC_KEYBOARD_LAYOUT_IME_COMBO));
			SelectKeyboardLayout(GetDlgItem(hDlg, IDC_KEYBOARD_LAYOUT_IME_COMBO));
        }
        case WM_COMMAND:
        {
		    switch (LOWORD(wParam))
			{
				case IDOK:
				
				break;
				case IDCANCEL:
					EndDialog(hDlg,LOWORD(wParam));
				break;
			}
        }
        break;
    }

    return FALSE;
}

/* EOF */
