/*
 *  ReactOS Standard Dialog Application Template
 *
 *  memdlg.c
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
#include <tchar.h>
#include "trace.h"


extern HINSTANCE hInst;

#define ID_HELP   150
#define ID_TEXT   200

////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK DialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LPWORD lpwAlign(LPWORD lpIn)
{
    ULONG ul;

    ul = (ULONG)lpIn;
    ul += 3;
    ul >>= 2;
    ul <<= 2;
    return (LPWORD)ul;
}

////////////////////////////////////////////////////////////////////////////////
// Create an in memory dialog resource and display.
//  Note: this doesn't work
//
LRESULT CreateMemoryDialog(HINSTANCE hinst, HWND hwndOwner, LPSTR lpszMessage)
{
    HGLOBAL hgbl;
    LPDLGTEMPLATE lpdt;
    LPDLGITEMTEMPLATE lpdit;
    LPWORD lpw;
    LPWSTR lpwsz;
    LRESULT ret;
    int nchar;

    hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
    if (!hgbl)
        return -1;
 
    lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
 
    // Define a dialog box.
    lpdt->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION;
    lpdt->cdit = 3;  // number of controls
    lpdt->x  = 10; lpdt->y  = 10;
    lpdt->cx = 100; lpdt->cy = 100;
    lpw = (LPWORD)(lpdt + 1);
    *lpw++ = 0;   // no menu
    *lpw++ = 0;   // predefined dialog box class (by default)

    lpwsz = (LPWSTR)lpw;
    nchar = 1 + MultiByteToWideChar(CP_ACP, 0, "My Dialog", -1, lpwsz, 50);
    lpw += nchar;

    //-----------------------
    // Define an OK button.
    //-----------------------
    lpw = lpwAlign(lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->x  = 10; lpdit->y  = 70;
    lpdit->cx = 80; lpdit->cy = 20;
    lpdit->id = IDOK;  // OK button identifier
    lpdit->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;

    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0080;    // button class

    lpwsz = (LPWSTR)lpw;
    nchar = 1 + MultiByteToWideChar(CP_ACP, 0, "OK", -1, lpwsz, 50);
    lpw += nchar;
    lpw = lpwAlign(lpw); // align creation data on DWORD boundary
    *lpw++ = 0;           // no creation data

    //-----------------------
    // Define a Help button.
    //-----------------------
    lpw = lpwAlign(lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->x  = 55; lpdit->y  = 10;
    lpdit->cx = 40; lpdit->cy = 20;
    lpdit->id = ID_HELP;    // Help button identifier
    lpdit->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;

    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0080;                 // button class atom

    lpwsz = (LPWSTR)lpw;
    nchar = 1 + MultiByteToWideChar(CP_ACP, 0, "Help", -1, lpwsz, 50);
    lpw += nchar;
    lpw = lpwAlign(lpw); // align creation data on DWORD boundary
    *lpw++ = 0;           // no creation data

    //-----------------------
    // Define a static text control.
    //-----------------------
    lpw = lpwAlign(lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->x  = 10; lpdit->y  = 10;
    lpdit->cx = 40; lpdit->cy = 20;
    lpdit->id = ID_TEXT;  // text identifier
    lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;

    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0082;                         // static class

    for (lpwsz = (LPWSTR)lpw;    
        *lpwsz++ = (WCHAR)*lpszMessage++;
    );
    lpw = (LPWORD)lpwsz;
    lpw = lpwAlign(lpw);  // align creation data on DWORD boundary
    *lpw++ = 0;           // no creation data

    GlobalUnlock(hgbl); 
    ret = DialogBoxIndirect(hinst, (LPDLGTEMPLATE)hgbl, hwndOwner, (DLGPROC)DialogWndProc); 
    if (ret == 0) {
        TRACE(_T("DialogBoxIndirect() failed due to invalid handle to parent window: 0x%08X"), hwndOwner);
    } else if (ret == -1) {
        DWORD error = GetLastError();
        TRACE(_T("DialogBoxIndirect() failed, GetLastError returned 0x%08X"), error);
    }
    GlobalFree(hgbl); 
    return ret; 
}

////////////////////////////////////////////////////////////////////////////////
