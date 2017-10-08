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

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefScreenSaverProc(hwnd, uMsg, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

// This function is only called one time before opening the configuration dialog.
// Use it to show a message that no configuration is necesssary and return FALSE to indicate that no configuration dialog shall be opened.
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    TCHAR szMessage[256];
    TCHAR szTitle[25];

    LoadString(hInst, IDS_TEXT, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    LoadString(hInst, IDS_DESCRIPTION, szTitle, sizeof(szTitle) / sizeof(TCHAR));

    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

    return FALSE;
}
