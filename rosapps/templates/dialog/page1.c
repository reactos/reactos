/*
 *  ReactOS Standard Dialog Application Template
 *
 *  page1.c
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


#define XBITMAP 80 
#define YBITMAP 20 
 
#define BUFFER_LEN MAX_PATH 
 
extern HINSTANCE hInst;

HBITMAP hbmpPicture; 
HBITMAP hbmpOld; 


////////////////////////////////////////////////////////////////////////////////

static void AddItem(HWND hListBox, LPCTSTR lpstr, HBITMAP hbmp) 
{ 
    int nItem = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)lpstr); 
    SendMessage(hListBox, LB_SETITEMDATA, nItem, (LPARAM)hbmp); 
} 

static TCHAR* items[] = {
    _T("services"),
    _T("event log"),
    _T("workstation"),
    _T("server")
};

static void InitListCtrl(HWND hDlg)
{
    TCHAR szBuffer[200];
    int i;

    HWND hListBox = GetDlgItem(hDlg, IDC_LIST1);

    _tcscpy(szBuffer, _T("foobar item"));
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)szBuffer);

    for (i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        _tcscpy(szBuffer, items[i]);
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)szBuffer);
    }

    SetFocus(hListBox); 
    SendMessage(hListBox, LB_SETCURSEL, 0, 0); 
}

static void OnDrawItem(HWND hWnd, LPARAM lParam)
{
//    int nItem; 
    TCHAR tchBuffer[BUFFER_LEN]; 
//    HBITMAP hbmp; 
    TEXTMETRIC tm; 
    int y; 
    HDC hdcMem; 
    LPDRAWITEMSTRUCT lpdis; 
    RECT rcBitmap; 

    lpdis = (LPDRAWITEMSTRUCT)lParam; 
    // If there are no list box items, skip this message. 
    if (lpdis->itemID != -1) { 
        // Draw the bitmap and text for the list box item. Draw a rectangle around the bitmap if it is selected. 
        switch (lpdis->itemAction) { 
        case ODA_SELECT: 
        case ODA_DRAWENTIRE: 
            // Display the bitmap associated with the item. 
            hbmpPicture = (HBITMAP)SendMessage(lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, (LPARAM)0); 
            hdcMem = CreateCompatibleDC(lpdis->hDC); 
            hbmpOld = SelectObject(hdcMem, hbmpPicture); 
            BitBlt(lpdis->hDC, 
                   lpdis->rcItem.left, lpdis->rcItem.top, 
                   lpdis->rcItem.right - lpdis->rcItem.left, 
                   lpdis->rcItem.bottom - lpdis->rcItem.top, 
                   hdcMem, 0, 0, SRCCOPY); 
            // Display the text associated with the item. 
            SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)tchBuffer); 
            GetTextMetrics(lpdis->hDC, &tm); 
            y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2; 
            TextOut(lpdis->hDC, XBITMAP + 6, y, tchBuffer, _tcslen(tchBuffer)); 
            SelectObject(hdcMem, hbmpOld); 
            DeleteDC(hdcMem); 
            // Is the item selected? 
            if (lpdis->itemState & ODS_SELECTED) { 
                // Set RECT coordinates to surround only the bitmap. 
                rcBitmap.left = lpdis->rcItem.left; 
                rcBitmap.top = lpdis->rcItem.top; 
                rcBitmap.right = lpdis->rcItem.left + XBITMAP; 
                rcBitmap.bottom = lpdis->rcItem.top + YBITMAP; 
                // Draw a rectangle around bitmap to indicate the selection. 
                DrawFocusRect(lpdis->hDC, &rcBitmap); 
            } 
            break; 
        case ODA_FOCUS: 
            // Do not process focus changes. The focus caret (outline rectangle) 
            // indicates the selection. The IDOK button indicates the final selection. 
            break; 
        } 
    } 
}


void OnSetFont(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	RECT rc;
	WINDOWPOS wp;

	GetWindowRect(hWnd, &rc);
	wp.hwnd = hWnd;
	wp.cx = rc.right - rc.left;
	wp.cy = rc.bottom - rc.top;
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessage(hWnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp);
}

void OnMeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    HFONT hFont; 
	LOGFONT lf;

    hFont = GetStockObject(SYSTEM_FONT); 
    GetObject(hFont, sizeof(LOGFONT), &lf);
	if (lf.lfHeight < 0)
		lpMeasureItemStruct->itemHeight = -lf.lfHeight; 
	else
		lpMeasureItemStruct->itemHeight = lf.lfHeight; 
}

LRESULT CALLBACK PageWndProc1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        InitListCtrl(hDlg);
        return TRUE;
    case WM_SETFONT: 
        OnSetFont(hDlg, wParam, lParam);
        return TRUE;
    case WM_MEASUREITEM: 
        OnMeasureItem((LPMEASUREITEMSTRUCT)lParam);
        return TRUE; 
    case WM_DRAWITEM: 
        OnDrawItem(hDlg, lParam);
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
