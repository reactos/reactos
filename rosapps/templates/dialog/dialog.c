/*
 *  ReactOS Standard Dialog Application Template
 *
 *  dialog.c
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


#define _USE_MSG_PUMP_

typedef struct tagDialogData {
    HWND hWnd; 
    LONG lData;
} DialogData; 

HINSTANCE hInst;
HWND      hTabWnd;
HWND      hPage1;
HWND      hPage2;
HWND      hPage3;

LRESULT CreateMemoryDialog(HINSTANCE, HWND hwndOwner, LPSTR lpszMessage);
LRESULT CALLBACK PageWndProc1(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PageWndProc2(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PageWndProc3(HWND, UINT, WPARAM, LPARAM);
 
////////////////////////////////////////////////////////////////////////////////

static BOOL OnCreate(HWND hWnd, LONG lData)
{
    TCHAR szTemp[256];
    TCITEM item;

    // Initialize the Windows Common Controls DLL
#ifdef __GNUC__
    InitCommonControls();
#else
    INITCOMMONCONTROLSEX ex = { sizeof(INITCOMMONCONTROLSEX), ICC_TAB_CLASSES };
    InitCommonControlsEx(&ex);
#endif

    // Create tab pages
    hTabWnd = GetDlgItem(hWnd, IDC_TAB);
    hPage1 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PAGE1), hWnd, (DLGPROC)PageWndProc1);
    hPage2 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PAGE2), hWnd, (DLGPROC)PageWndProc2);
    hPage3 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PAGE3), hWnd, (DLGPROC)PageWndProc3);

    // Insert tabs
    _tcscpy(szTemp, _T("Page One"));
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 0, &item);
    _tcscpy(szTemp, _T("Page Two"));
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 1, &item);
    _tcscpy(szTemp, _T("Page Three"));
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 2, &item);

    ShowWindow(hPage1, SW_SHOW);

    return TRUE;
}

void OnTabWndSelChange(void)
{
    switch (TabCtrl_GetCurSel(hTabWnd)) {
    case 0:
        ShowWindow(hPage1, SW_SHOW);
        ShowWindow(hPage2, SW_HIDE);
        ShowWindow(hPage3, SW_HIDE);
        BringWindowToTop(hPage1);
        SetFocus(hTabWnd);
        break;
    case 1:
        ShowWindow(hPage1, SW_HIDE);
        ShowWindow(hPage2, SW_SHOW);
        ShowWindow(hPage3, SW_HIDE);
        BringWindowToTop(hPage2);
        SetFocus(hTabWnd);
        break;
    case 2:
        ShowWindow(hPage1, SW_HIDE);
        ShowWindow(hPage2, SW_HIDE);
        ShowWindow(hPage3, SW_SHOW);
        BringWindowToTop(hPage3);
        SetFocus(hTabWnd);
        break;
    }
}

LRESULT CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int idctrl;
    LPNMHDR pnmh;
    LPCREATESTRUCT lpCS;
    LONG nThisApp = 0;
    DialogData* pData = (DialogData*)GetWindowLong(hDlg, DWL_USER);
    if (pData) nThisApp = pData->lData;

    switch (message) {

    case WM_CREATE:
        lpCS = (LPCREATESTRUCT)lParam;
        assert(lpCS);
        lpCS->style &= ~WS_POPUP;
        break;

    case WM_INITDIALOG:
        pData = (DialogData*)lParam;
        SetWindowLong(hDlg, DWL_USER, (LONG)pData);
        if (pData) nThisApp = pData->lData;
        return OnCreate(hDlg, nThisApp);

#ifdef _USE_MSG_PUMP_
    case WM_DESTROY:
        if (pData && (pData->hWnd == hDlg)) {
            pData->hWnd = 0;
            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            //MessageBox(NULL, _T("Good-bye"), _T("dialog"), MB_ICONEXCLAMATION);
            CreateMemoryDialog(hInst, GetDesktopWindow(), "DisplayMyMessage");
        case IDCANCEL:
            if (pData && (pData->hWnd == hDlg)) {
                DestroyWindow(pData->hWnd);
            }
            break;
        }
        break;
#else
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
#endif

    case WM_NOTIFY:
        idctrl = (int)wParam;
        pnmh = (LPNMHDR)lParam;
        if ((pnmh->hwndFrom == hTabWnd) &&
            (pnmh->idFrom == IDC_TAB) &&
            (pnmh->code == TCN_SELCHANGE))
        {
            OnTabWndSelChange();
        }
        break;
    }
    return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
#ifdef _USE_MSG_PUMP_
    MSG msg;
    HACCEL hAccel;
    DialogData instData = { NULL, 34 };

    hInst = hInstance;
    instData.hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_TABBED_DIALOG), NULL, (DLGPROC)DlgProc, (LPARAM)&instData);
    ShowWindow(instData.hWnd, SW_SHOW);
    hAccel = LoadAccelerators(hInst, (LPCTSTR)IDR_ACCELERATOR);
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
#else
    hInst = hInstance;
    DialogBox(hInst, (LPCTSTR)IDD_TABBED_DIALOG, NULL, (DLGPROC)DlgProc);
    //CreateMemoryDialog(hInst, GetDesktopWindow(), "CreateMemoryDialog");
#endif
	return 0;
}
