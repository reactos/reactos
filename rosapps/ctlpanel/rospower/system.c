/*
 *  ReactOS control
 *
 *  system.c
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
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include <windowsx.h>
#include "main.h"
#include "system.h"

#include "assert.h"

/*
LRESULT CALLBACK SystemGeneralPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemNetworkPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemHardwarePageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemUsersPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemAdvancedPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}
 */
////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

extern HMODULE hModule;

//HWND hSystemGeneralPage;
//HWND hSystemNetworkPage;
//HWND hSystemHardwarePage;
//HWND hSystemUsersPage;
//HWND hSystemAdvancedPage;

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

#define DLGPROC_RESULT BOOL
//#define DLGPROC_RESULT LRESULT 

LRESULT CALLBACK CPlPowerAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

CPlAppletDlgPagesCfg CPlAppletDlgPagesData[] = {
    { IDS_CPL_POWER_DIALOG, IDD_CPL_POWER_SCHEMES_PAGE, IDD_CPL_POWER_HARDWARE_PAGE, CPlPowerAppletDlgProc },
};

//static BOOL CreateTabPages(HWND hDlg, int nTabCtrlId, int nFirstPageId, int nLastPageId, DLGPROC pDlgProc)
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
    
    if (TabCtrl_GetItemRect(hTabWnd, 0, &rc)) {
        nTabWndHeight = rc.bottom - rc.top;
    }

    //GetWindowRect(hTabWnd, &rc);
    GetClientRect(hTabWnd, &rc);
    nWidth = rc.right - rc.left;
    nHeight = rc.bottom - rc.top;

    MapWindowPoints(hTabWnd, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)));

    for (i = pPagesData->nFirstPageId; i <= pPagesData->nLastPageId; i++) {
        HWND hPage;
//        hPage = CreateDialog(hModule, MAKEINTRESOURCE(i), hDlg, pPagesData->pDlgProc);
        hPage = CreateDialogParam(hModule, MAKEINTRESOURCE(i), hDlg, pPagesData->pDlgProc, i);
#ifdef __GNUC__
        SetParent(hPage, hDlg);
#endif
        LoadString(hModule, i, szPageTitle, MAX_LOADSTRING);
        memset(&item, 0, sizeof(TCITEM));
        item.mask = TCIF_TEXT | TCIF_PARAM;
        item.pszText = szPageTitle;
        item.lParam = (LPARAM)hPage;
        if (TabCtrl_InsertItem(hTabWnd, i - pPagesData->nFirstPageId, &item) == -1) {
            assert(0);
        }
        //if (i == pPagesData->nFirstPageId) ShowWindow(hPage, SW_SHOW);
        //GetClientRect(hPage, &rc);
        GetWindowRect(hPage, &rc);
        nWidth = max(nWidth, rc.right - rc.left);
        nHeight = max(nHeight, rc.bottom - rc.top);
    }
    SetWindowPos(hTabWnd, NULL, 0, 0, nWidth + rc.left, nHeight/* + rc.top*/, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
    //SetWindowPos(hTabWnd, NULL, rc.left, rc.top, nWidth, nHeight, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    InvalidateRect(hTabWnd, NULL, TRUE);

    count = TabCtrl_GetItemCount(hTabWnd);
    for (i = 0; i < count; i++) {
        item.mask = TCIF_PARAM;
        item.lParam = 0;
        if (TabCtrl_GetItem(hTabWnd, i, &item)) {
            if (item.lParam) {
                //GetWindowRect((HWND)item.lParam, &rc);
                //SetWindowPos((HWND)item.lParam, NULL, 0, 0, nWidth-10, nHeight-10, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);

                GetWindowRect((HWND)item.lParam, &rc);
                SetWindowPos((HWND)item.lParam, NULL, rc.left, rc.top, nWidth, nHeight, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
//                SetWindowPos((HWND)item.lParam, NULL, rc.left, rc.top + nTabWndHeight, nWidth-10, nHeight-10, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
                
                //SetWindowPos((HWND)item.lParam, NULL, rc.left, rc.top, nWidth-10, nHeight-10, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
                InvalidateRect((HWND)item.lParam, NULL, TRUE);
                if (i == 0) ShowWindow((HWND)item.lParam, SW_SHOW);
            }
        }
    }
#define BORDER_X  15
#define BORDER_Y  15
#define BUTTON_BAR_Y  70
    if (nWidth && nHeight) {
        nWidth += (BORDER_X * 3);
        nHeight += ((BORDER_Y * 4) + BUTTON_BAR_Y);
        MoveWindow(hDlg, 0, 0, nWidth, nHeight, TRUE);
//        MoveWindow(hTabWnd, 0, 0, nWidth, nHeight, TRUE);
    }
    return 0;
}

static BOOL OnCreate(HWND hDlg, LONG lData)
{
    TCHAR buffer[50];
    RECT rc;
    HWND hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);

    //SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_TASKMANAGER)));

    // Get the minimum window sizes
    GetWindowRect(hDlg, &rc);
    nMinimumWidth = (rc.right - rc.left);
    nMinimumHeight = (rc.bottom - rc.top);

    // Create tab pages
    if (lData >= 0 && lData < GetCountProc()) {
        //HWND hTabWnd = GetDlgItem(hDlg, pPagesData->nTabCtrlId);
        CreateTabPages(hDlg, &CPlAppletDlgPagesData[lData]);
    }
    LoadString(hModule, CPlAppletDlgPagesData[lData].nDlgTitleId, buffer, sizeof(buffer)/sizeof(TCHAR));
    SetWindowText(hDlg, buffer);

    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 0);

    return TRUE;
}

// OnSize()
// This function handles all the sizing events for the application
// It re-sizes every window, and child window that needs re-sizing
static void OnSize(HWND hDlg, int cx, int cy, CPlAppletDlgPagesCfg* pPagesData)
{
    int    nXDifference;
    int    nYDifference;
    RECT   rc;
    TCITEM item;
    int    i, nPageCount;
    int nTabWndHeight = 0;
    HWND hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);

    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    nPageCount = TabCtrl_GetItemCount(hTabWnd);
//    for (i = pPagesData->nFirstPageId; i <= pPagesData->nLastPageId; i++) {
    if (TabCtrl_GetItemRect(hTabWnd, 0, &rc)) {
        nTabWndHeight = rc.bottom - rc.top;
    }
    for (i = 0; i < nPageCount; i++) {
        // Resize the pages
         item.mask = TCIF_PARAM;
         if (TabCtrl_GetItem(hTabWnd, i, &item)) {
            HWND hPage;
            hPage = (HWND)item.lParam;
            if (hPage) {
                int width, height;

                //GetWindowRect(hPage, &rc);
                GetWindowRect(hTabWnd, &rc);
                nTabWndHeight += 3;
                //InflateRect(&rc, -2, -2);

                width = rc.right - rc.left;
                height = rc.bottom - rc.top;

                height -= nTabWndHeight;
                height -= 2;
                width -= 3;

                GetWindowRect(hDlg, &rc);
                rc.top += nTabWndHeight;
                rc.left += 1;
                //rc.top += 3;

                SetWindowPos(hPage, NULL, rc.left, rc.top, width, height, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
            }
        }
    }
}

static LRESULT OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam, CPlAppletDlgPagesCfg* pPagesData)
{
//    int idctrl = (int)wParam;
    HWND hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);
    LPNMHDR pnmh = (LPNMHDR)lParam;
    if ((pnmh->hwndFrom == hTabWnd) && (pnmh->code == TCN_SELCHANGE)) {
//        (pnmh->idFrom == IDD_CPL_POWER_DIALOG) &&
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
//    assert(0);
    return 0L;
}

typedef struct tagCPlAppletInstanceData {
    HWND hWnd;
    LONG lData;
} CPlAppletInstanceData;

LRESULT CALLBACK CPlAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CPlAppletInstanceData* pData = (CPlAppletInstanceData*)GetWindowLong(hDlg, DWL_USER);
    LONG nThisApp = 0;
    if (pData) nThisApp = pData->lData;

    switch (message) {
    case WM_INITDIALOG:
//        nThisApp = (LONG)lParam;
        pData = (CPlAppletInstanceData*)lParam;
        SetWindowLong(hDlg, DWL_USER, (LONG)pData);
        nThisApp = pData->lData;
        if (OnCreate(hDlg, nThisApp)) {
            OnSize(hDlg, 0, 0, &CPlAppletDlgPagesData[nThisApp]);
            return TRUE;
        }
        return FALSE;

    case WM_DESTROY:
        if (pData && (pData->hWnd == hDlg)) {
            DestroyWindow(pData->hWnd);
            pData->hWnd = 0;
        }
        break;

    case WM_COMMAND:
        // Handle the button clicks
        switch (LOWORD(wParam)) {
//        case IDC_ENDTASK:
//            break;
//        }
        case IDOK:
        case IDCANCEL:
            if (pData && (pData->hWnd == hDlg)) {
                DestroyWindow(pData->hWnd);
                pData->hWnd = 0;
            }
            break;
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        OnSize(hDlg, LOWORD(lParam), HIWORD(lParam), &CPlAppletDlgPagesData[nThisApp]);
        break;

    case WM_NOTIFY:
        return OnNotify(hDlg, wParam, lParam, &CPlAppletDlgPagesData[nThisApp]);
    }
    return 0;
}

