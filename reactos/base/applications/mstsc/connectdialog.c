/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Connection settings dialog
   Copyright (C) Ged Murphy 2007

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <windows.h>
#include <commctrl.h>
#include "resource.h"

typedef struct _INFO
{
    HWND hSelf;
    HWND hTab;
    HWND hGeneralPage;
    HWND hDisplayPage;
    HBITMAP hHeader;
    BITMAP headerbitmap;
} INFO, *PINFO;

HINSTANCE hInst;

void OnTabWndSelChange(PINFO pInfo)
{
    switch (TabCtrl_GetCurSel(pInfo->hTab))
    {
        case 0: //General
            ShowWindow(pInfo->hGeneralPage, SW_SHOW);
            ShowWindow(pInfo->hDisplayPage, SW_HIDE);
            BringWindowToTop(pInfo->hGeneralPage);
            break;
        case 1: //Display
            ShowWindow(pInfo->hGeneralPage, SW_HIDE);
            ShowWindow(pInfo->hDisplayPage, SW_SHOW);
            BringWindowToTop(pInfo->hDisplayPage);
            break;
    }
}


INT_PTR CALLBACK
GeneralDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    static HICON hLogon, hConn;
    HWND hParent = GetParent(hDlg);
    PINFO pInfo = (PINFO)GetWindowLongPtr(hParent,
                                          GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            {
                SetWindowPos(hDlg,
                             NULL, 
                             15, 
                             122, 
                             0, 
                             0, 
                             SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

                hLogon = LoadImage(hInst,
                                   MAKEINTRESOURCE(IDI_LOGON),
                                   IMAGE_ICON,
                                   32,
                                   32,
                                   LR_DEFAULTCOLOR);
                if (hLogon)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_LOGONICON,
                                       STM_SETICON,
                                       (WPARAM)hLogon,
                                       0);
                }

                hConn = LoadImage(hInst,
                                  MAKEINTRESOURCE(IDI_CONN),
                                  IMAGE_ICON,
                                  32,
                                  32,
                                  LR_DEFAULTCOLOR);
                if (hConn)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_CONNICON,
                                       STM_SETICON,
                                       (WPARAM)hConn,
                                       0);
                }

                return TRUE;
            }

            case WM_CLOSE:
            {
                if (hLogon)
                    DestroyIcon(hLogon);

                if (hConn)
                    DestroyIcon(hConn);

                break;
            }
    }

    return 0;
}


INT_PTR CALLBACK
DisplayDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    static HICON hRemote, hColor;
    static HBITMAP hSpectrum;
    static BITMAP bitmap;
    PINFO pInfo = (PINFO)GetWindowLongPtr(GetParent(hDlg),
                                          GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg,
                         NULL,
                         15,
                         122,
                         0,
                         0,
                         SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

                hRemote = LoadImage(hInst,
                                    MAKEINTRESOURCE(IDI_REMOTE),
                                    IMAGE_ICON,
                                    32,
                                    32,
                                    LR_DEFAULTCOLOR);
                if (hRemote)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_REMICON,
                                       STM_SETICON,
                                       (WPARAM)hRemote,
                                       0);
                }

                hColor = LoadImage(hInst,
                                   MAKEINTRESOURCE(IDI_COLORS),
                                   IMAGE_ICON,
                                   32,
                                   32,
                                   LR_DEFAULTCOLOR);
                if (hColor)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_COLORSICON,
                                       STM_SETICON,
                                       (WPARAM)hColor,
                                       0);
                }

                hSpectrum = LoadImage(hInst,
                                      MAKEINTRESOURCE(IDB_SPECT),
                                      IMAGE_BITMAP,
                                      0,
                                      0,
                                      LR_DEFAULTCOLOR);
                if (hSpectrum)
                {
                    GetObject(hSpectrum, sizeof(BITMAP), &bitmap);
                }

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_COLORIMAGE)
            {
                HDC hdcMem;
                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, hSpectrum);
                    StretchBlt(lpDrawItem->hDC,
                               lpDrawItem->rcItem.left,
                               lpDrawItem->rcItem.top,
                               lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                               lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                               hdcMem,
                               0,
                               0,
                               bitmap.bmWidth,
                               bitmap.bmHeight,
                               SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
            break;
        }

            case WM_CLOSE:
            {
                if (hRemote)
                    DestroyIcon(hRemote);

                if (hColor)
                    DestroyIcon(hColor);

                if (hSpectrum)
                    DeleteObject(hSpectrum);

                break;
            }

        break;
    }
    return 0;
}



static BOOL
OnCreate(HWND hwnd)
{
    PINFO pInfo;
    TCITEM item;
    BOOL bRet = FALSE;

    pInfo = HeapAlloc(GetProcessHeap(),
                      0,
                      sizeof(INFO));
    if (pInfo)
    {
        SetWindowLongPtr(hwnd,
                         GWLP_USERDATA,
                         (LONG_PTR)pInfo);

        pInfo->hHeader = LoadImage(hInst,
                                   MAKEINTRESOURCE(IDB_HEADER),
                                   IMAGE_BITMAP,
                                   0,
                                   0,
                                   LR_DEFAULTCOLOR);
        if (pInfo->hHeader)
        {
            GetObject(pInfo->hHeader, sizeof(BITMAP), &pInfo->headerbitmap);
        }

        pInfo->hTab = GetDlgItem(hwnd, IDC_TAB);
        if (pInfo->hTab)
        {
            pInfo->hGeneralPage = CreateDialog(hInst,
                                               MAKEINTRESOURCE(IDD_GENERAL),
                                               hwnd,
                                               (DLGPROC)GeneralDlgProc);
            if (pInfo->hGeneralPage)
            {
                char str[256];
                LoadString(hInst, IDS_TAB_GENERAL, str, 256);
                ZeroMemory(&item, sizeof(TCITEM));
                item.mask = TCIF_TEXT;
                item.pszText = str;
                item.cchTextMax = 256;
                (void)TabCtrl_InsertItem(pInfo->hTab, 0, &item);
            }

            pInfo->hDisplayPage = CreateDialog(hInst,
                                               MAKEINTRESOURCE(IDD_DISPLAY),
                                               hwnd,
                                               (DLGPROC)DisplayDlgProc);
            if (pInfo->hDisplayPage)
            {
                char str[256];
                LoadString(hInst, IDS_TAB_DISPLAY, str, 256);
                ZeroMemory(&item, sizeof(TCITEM));
                item.mask = TCIF_TEXT;
                item.pszText = str;
                item.cchTextMax = 256;
                (void)TabCtrl_InsertItem(pInfo->hTab, 1, &item);
            }

            OnTabWndSelChange(pInfo);

            bRet = TRUE;
        }
    }

    return bRet;
}


static BOOL CALLBACK
DlgProc(HWND hDlg,
        UINT Message,
        WPARAM wParam,
        LPARAM lParam)
{
    PINFO pInfo;

    /* Get the window context */
    pInfo = (PINFO)GetWindowLongPtr(hDlg,
                                    GWLP_USERDATA);
    if (pInfo == NULL && Message != WM_INITDIALOG)
    {
        goto HandleDefaultMessage;
    }

    switch(Message)
    {
        case WM_INITDIALOG:
            OnCreate(hDlg);
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                if (pInfo)
                {
                    HeapFree(GetProcessHeap(),
                             0,
                             pInfo);
                }

                EndDialog(hDlg, LOWORD(wParam));
            }

            switch(LOWORD(wParam))
            {

                break;
            }

            break;
        }

        case WM_NOTIFY:
        {
            INT idctrl;
            LPNMHDR pnmh;
            idctrl = (int)wParam;
            pnmh = (LPNMHDR)lParam;
            if (//(pnmh->hwndFrom == pInfo->hSelf) &&
                (pnmh->idFrom == IDC_TAB) &&
                (pnmh->code == TCN_SELCHANGE))
            {
                OnTabWndSelChange(pInfo);
            }

            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;

            hdc = BeginPaint(hDlg, &ps);
            if (hdc != NULL)
            {
                HDC hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem)
                {
                    SelectObject(hdcMem, pInfo->hHeader);
                    BitBlt(hdc,
                           0,
                           0,
                           pInfo->headerbitmap.bmWidth,
                           pInfo->headerbitmap.bmHeight,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);
                    DeleteDC(hdcMem);
                }

                EndPaint(hDlg, &ps);
            }

            break;
        }

        case WM_CLOSE:
        {
            if (pInfo)
                HeapFree(GetProcessHeap(),
                         0,
                         pInfo);

            EndDialog(hDlg, 0);
        }
        break;

HandleDefaultMessage:
        default:
            return FALSE;
    }

    return FALSE;
}

BOOL
OpenRDPConnectDialog(HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX iccx;

    hInst = hInstance;

    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&iccx);

    return (DialogBox(hInst,
                      MAKEINTRESOURCE(IDD_CONNECTDIALOG),
                      NULL,
                      (DLGPROC)DlgProc) == IDOK);
}
