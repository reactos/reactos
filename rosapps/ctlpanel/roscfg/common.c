/*
 *  ReactOS control
 *
 *  common.c
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <tchar.h>
#include <windowsx.h>
#include "main.h"
#include "system.h"
#include "assert.h"
#include "trace.h"


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

extern HMODULE hModule;

HWND hApplyButton;

int  nMinimumWidth;              // Minimum width of the dialog (OnSize()'s cx)
int  nMinimumHeight;             // Minimum height of the dialog (OnSize()'s cy)
int  nOldWidth;                  // Holds the previous client area width
int  nOldHeight;                 // Holds the previous client area height

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

typedef struct CPlAppletDlgPagesCfg {
//    int nTabCtrlId;
    int nDlgTitleId;
    int nFirstPageId;
    int nLastPageId;
    DLGPROC pDlgProc;
} CPlAppletDlgPagesCfg;

//typedef BOOL(CALLBACK *DLGPROC)(HWND,UINT,WPARAM,LPARAM);

#define DLGPROC_RESULT BOOL
//#define DLGPROC_RESULT LRESULT 

DLGPROC_RESULT CALLBACK SystemAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK KeyboardAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK MouseAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK UsersAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK DisplayAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK FoldersAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK RegionalAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK DateTimeAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK IrDaAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK AccessibleAppletDlgProc(HWND, UINT, WPARAM, LPARAM);
DLGPROC_RESULT CALLBACK PhoneModemAppletDlgProc(HWND, UINT, WPARAM, LPARAM);

CPlAppletDlgPagesCfg CPlAppletDlgPagesData[] = {
    { IDC_CPL_SYSTEM_DLG, IDD_CPL_SYSTEM_GENERAL_PAGE, IDD_CPL_SYSTEM_ADVANCED_PAGE, SystemAppletDlgProc },
    { IDC_CPL_IRDA_DLG, IDD_CPL_IRDA_FILE_TRANSFER_PAGE, IDD_CPL_IRDA_HARDWARE_PAGE, IrDaAppletDlgProc },
    { IDC_CPL_MOUSE_DLG, IDD_CPL_MOUSE_BUTTONS_PAGE, IDD_CPL_MOUSE_HARDWARE_PAGE, MouseAppletDlgProc },
    { IDC_CPL_KEYBOARD_DLG, IDD_CPL_KEYBOARD_SPEED_PAGE, IDD_CPL_KEYBOARD_HARDWARE_PAGE, KeyboardAppletDlgProc },
    { IDC_CPL_REGIONAL_DLG, IDD_CPL_REGIONAL_GENERAL_PAGE, IDD_CPL_REGIONAL_INPUT_LOCALES_PAGE, RegionalAppletDlgProc },
    { IDC_CPL_USERS_DLG, IDD_CPL_USERS_USERS_PAGE, IDD_CPL_USERS_ADVANCED_PAGE, UsersAppletDlgProc },
    { IDC_CPL_DATETIME_DLG, IDD_CPL_DATETIME_DATETIME_PAGE, IDD_CPL_DATETIME_TIMEZONE_PAGE, DateTimeAppletDlgProc },
    { IDC_CPL_FOLDERS_DLG, IDD_CPL_FOLDERS_GENERAL_PAGE, IDD_CPL_FOLDERS_OFFLINE_FILES_PAGE, FoldersAppletDlgProc },
    { IDC_CPL_DISPLAY_DLG, IDD_CPL_DISPLAY_BACKGROUND_PAGE, IDD_CPL_DISPLAY_SETTINGS_PAGE, DisplayAppletDlgProc },
    { IDC_CPL_ACCESSIBLE_DLG, IDD_CPL_ACCESSIBLE_KEYBOARD_PAGE, IDD_CPL_ACCESSIBLE_MOUSE_PAGE, AccessibleAppletDlgProc },
    { IDC_CPL_PHONE_MODEM_DLG, IDD_CPL_PHONE_MODEM_DIALING_PAGE, IDD_CPL_PHONE_MODEM_ADVANCED_PAGE, PhoneModemAppletDlgProc },
};

#define BORDER_X  10
#define BORDER_Y  10
#define BORDER_TAB_X  2
#define BORDER_TAB_Y  2
#define BUTTON_BAR_Y  70

static void MoveButton(HWND hDlg, int btnId, int pos)
{
    HWND hBtnWnd;
    RECT rc, rcBtn;

    hBtnWnd = GetDlgItem(hDlg, btnId);
    GetClientRect(hDlg, &rc);
    GetWindowRect(hBtnWnd, &rcBtn);
    SetWindowPos(hBtnWnd, NULL, rc.right - pos * (BORDER_X + (rcBtn.right - rcBtn.left)),
                                rc.bottom - (BORDER_Y + (rcBtn.bottom - rcBtn.top)),
                                0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
}

static BOOL CreateTabPages(HWND hDlg, CPlAppletDlgPagesCfg* pPagesData)
{
    TCHAR  szPageTitle[MAX_LOADSTRING];
    RECT   rc;
    TCITEM item;
    int    i, count;
    int    nWidth = 0;
    int    nHeight = 0;
    int    nTabWndHeight = 0;
    HWND   hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);
    
    //GetWindowRect(hTabWnd, &rc);
    GetClientRect(hTabWnd, &rc);
    nWidth = rc.right - rc.left;
    nHeight = rc.bottom - rc.top;
    //MapWindowPoints(hTabWnd, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)));

    for (i = pPagesData->nFirstPageId; i <= pPagesData->nLastPageId; i++) {
        HWND hPage;
        //TRACE(_T("Calling CreateDialogParam(%08X, %u, %08X, %08X)\n"), hModule, i, hDlg, pPagesData->pDlgProc);
        hPage = CreateDialogParam(hModule, MAKEINTRESOURCE(i), hDlg, pPagesData->pDlgProc, i);
        LoadString(hModule, i, szPageTitle, MAX_LOADSTRING);
        memset(&item, 0, sizeof(TCITEM));
        item.mask = TCIF_TEXT | TCIF_PARAM;
        item.pszText = szPageTitle;
        item.lParam = (LPARAM)hPage;
        if (TabCtrl_InsertItem(hTabWnd, i - pPagesData->nFirstPageId, &item) == -1) {
            assert(0);
        }
        //if (i == pPagesData->nFirstPageId) ShowWindow(hPage, SW_SHOW);
        GetWindowRect(hPage, &rc);
        nWidth = max(nWidth, rc.right - rc.left);
        nHeight = max(nHeight, rc.bottom - rc.top);
    }

    if (TabCtrl_GetItemRect(hTabWnd, 0, &rc)) {
        nTabWndHeight = rc.bottom - rc.top;
        assert(nTabWndHeight);
    }

    // size the Tab control window to contain the largest property page
    SetWindowPos(hTabWnd, NULL, BORDER_X, BORDER_Y, nWidth+2, nHeight+nTabWndHeight+4, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    InvalidateRect(hTabWnd, NULL, TRUE);
    GetWindowRect(hTabWnd, &rc);

    // set all the property pages in the Tab control to the size of the largest page
    count = TabCtrl_GetItemCount(hTabWnd);
    for (i = 0; i < count; i++) {
        item.mask = TCIF_PARAM;
        item.lParam = 0;
        if (TabCtrl_GetItem(hTabWnd, i, &item)) {
            if (item.lParam) {
                //GetWindowRect((HWND)item.lParam, &rc);
                //SetWindowPos((HWND)item.lParam, NULL, 0, 0, nWidth-10, nHeight-10, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
                SetWindowPos((HWND)item.lParam, NULL, rc.left-2, rc.top-2, nWidth, nHeight, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
                InvalidateRect((HWND)item.lParam, NULL, TRUE);

                // show the first property page while we have a handle on it
                if (i == 0) ShowWindow((HWND)item.lParam, SW_SHOW);
            }
        }
    }

    // resize the dialog box to contain the Tab control with borders
    if (nWidth && nHeight) {
        nWidth += 2 * (BORDER_X + BORDER_TAB_X);
        nHeight += 3 * (BORDER_Y + BORDER_TAB_Y) + (BUTTON_BAR_Y + nTabWndHeight);
        MoveWindow(hDlg, 0, 0, nWidth, nHeight, TRUE);
        MoveButton(hDlg, IDCANCEL, 1);
        MoveButton(hDlg, IDOK, 2);
        MoveButton(hDlg, ID_APPLY, 3);
    }
    return 0;
}

static BOOL OnCreate(HWND hDlg, LONG lData)
{
    TCHAR buffer[50];
    RECT rc;
    HWND hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);

    hApplyButton = GetDlgItem(hDlg, ID_APPLY);

    //SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_TASKMANAGER)));

    // Get the minimum window sizes
    GetWindowRect(hDlg, &rc);
    nMinimumWidth = (rc.right - rc.left);
    nMinimumHeight = (rc.bottom - rc.top);

    // Create tab pages
    if (lData >= 0 && lData < GetCountProc()) {
        CreateTabPages(hDlg, &CPlAppletDlgPagesData[lData]);
    }
    LoadString(hModule, CPlAppletDlgPagesData[lData].nDlgTitleId, buffer, sizeof(buffer)/sizeof(TCHAR));
    SetWindowText(hDlg, buffer);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 0);
    return TRUE;
}

static LRESULT OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam, CPlAppletDlgPagesCfg* pPagesData)
{
//    int idctrl = (int)wParam;
    HWND hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);
    LPNMHDR pnmh = (LPNMHDR)lParam;
    if ((pnmh->hwndFrom == hTabWnd) && (pnmh->code == TCN_SELCHANGE)) {
//        (pnmh->idFrom == IDD_CPL_TABBED_DIALOG) &&
        TCITEM item;
        int i;
        int nActiveTabPage = TabCtrl_GetCurSel(hTabWnd);
        int nPageCount = TabCtrl_GetItemCount(hTabWnd);
        for (i = 0; i < nPageCount; i++) {
            item.mask = TCIF_PARAM;
            if (TabCtrl_GetItem(hTabWnd, i, &item)) {
                if ((HWND)item.lParam) {
                    if (i == nActiveTabPage) {
                        ShowWindow((HWND)item.lParam, SW_SHOW);
                        //BringWindowToTop((HWND)item.lParam);
                        //TabCtrl_SetCurFocus(hTabWnd, i);
                    } else {
                        ShowWindow((HWND)item.lParam, SW_HIDE);
                    }
                }
            }
        }
    }
    return 0L;
}

typedef struct tagCPlAppletInstanceData {
    HWND hWnd;
    LONG lData;
} CPlAppletInstanceData;

LRESULT CALLBACK CPlAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CPlAppletInstanceData* pData = (CPlAppletInstanceData*)GetWindowLong(hDlg, GWL_USERDATA);
    LONG nThisApp = 0;
    if (pData) nThisApp = pData->lData;

    switch (message) {
    case WM_INITDIALOG:
        pData = (CPlAppletInstanceData*)lParam;
        SetWindowLong(hDlg, GWL_USERDATA, (LONG)pData);
        nThisApp = pData->lData;
        return OnCreate(hDlg, nThisApp);

    case WM_DESTROY:
        if (pData && (pData->hWnd == hDlg)) {
            DestroyWindow(pData->hWnd);
            pData->hWnd = 0;
        }
        break;

    case WM_COMMAND:
        // Handle the button clicks
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (pData && (pData->hWnd == hDlg)) {
                DestroyWindow(pData->hWnd);
                pData->hWnd = 0;
            }
            break;
        }
        break;

    case WM_NOTIFY:
        return OnNotify(hDlg, wParam, lParam, &CPlAppletDlgPagesData[nThisApp]);
    }
    return 0;
}
