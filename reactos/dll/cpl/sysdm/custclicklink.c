/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/custclicklink.c
 * PURPOSE:     Subclass static control to link to apps
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

#define LINK_COLOR RGB(0,0,128)
#define MAX_PARAMS 256

typedef struct _LINKCTL
{
    HWND hSelf;
    WNDPROC OldProc;
    TCHAR szApp[MAX_PATH];
    TCHAR szParams[MAX_PARAMS];
    HFONT hFont;
    BOOL bClicked;
} LINKCTL, *PLINKCTL;



static VOID
DoLButtonUp(PLINKCTL pLink, LPARAM lParam)
{
    ReleaseCapture();
    if (pLink->bClicked)
    {
        POINT pt;
        RECT rc;

        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        ClientToScreen(pLink->hSelf, &pt);
        GetWindowRect(pLink->hSelf, &rc);
        if (PtInRect(&rc, pt))
        {
            ShellExecute(NULL,
                        _T("open"),
                        pLink->szApp,
                        pLink->szParams,
                        NULL,
                        SW_SHOWNORMAL);
        }

        pLink->bClicked = FALSE;
    }
}


static VOID
DoPaint(PLINKCTL pLink, HDC hdc)
{
    TCHAR szText[MAX_PATH];
    DWORD WinStyle, DrawStyle;
    RECT rc;
    HANDLE hOld;

    WinStyle = GetWindowLongPtr(pLink->hSelf, GWL_STYLE);
    DrawStyle = DT_SINGLELINE;

    if (WinStyle & SS_CENTER)
        DrawStyle |= DT_CENTER;
    if (WinStyle & SS_RIGHT)
        DrawStyle |= DT_RIGHT;
    if (WinStyle & SS_CENTERIMAGE)
        DrawStyle |= DT_VCENTER;

    SetTextColor(hdc, LINK_COLOR);
    SetBkMode(hdc, TRANSPARENT);
    hOld = SelectObject(hdc, pLink->hFont);
    SetBkColor(hdc, GetSysColor(COLOR_3DFACE));

    GetClientRect(pLink->hSelf, &rc);

    GetWindowText(pLink->hSelf, szText, sizeof(szText));
    DrawText(hdc, szText, -1, &rc, DrawStyle);

}


static LRESULT CALLBACK
LinkCtlWndProc(HWND hwnd,
               UINT msg,
               WPARAM wParam,
               LPARAM lParam)
{
    PLINKCTL pLink = (PLINKCTL)GetWindowLongPtr(hwnd, GWL_USERDATA);
    WNDPROC oldproc = pLink->OldProc;

    switch(msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(hwnd, &ps);
            DoPaint(pLink, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SETCURSOR:
        {
            HCURSOR hCur = LoadCursor(NULL, IDC_HAND);
            SetCursor(hCur);
            return TRUE;
        }

        case WM_SETFONT:
        {
            LOGFONT LogFont;
            HFONT hOldFont;
            
            hOldFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            GetObject(hOldFont, sizeof(LogFont), &LogFont);
            LogFont.lfUnderline = TRUE;
            if (pLink->hFont) DeleteObject(hwnd);
            pLink->hFont = CreateFontIndirect(&LogFont);

            CallWindowProc(pLink->OldProc, hwnd, msg, wParam, lParam);

            if (LOWORD(lParam))
            {
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
            }
            return 0;
        }

        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_LBUTTONDOWN:
        {
            SetFocus(hwnd);
            SetCapture(hwnd);
            pLink->bClicked = TRUE;
        }
        break;

        case WM_LBUTTONUP:
        {
            DoLButtonUp(pLink, lParam);
        }
        break;

        case WM_NCDESTROY:
        {
            HeapFree(GetProcessHeap(),
                     0,
                     pLink);
        }
        break;
    }

    return CallWindowProc(oldproc,
                          hwnd,
                          msg,
                          wParam,
                          lParam);
}


BOOL
TextToLink(HWND hwnd,
           LPTSTR lpApp,
           LPTSTR lpParams)
{
    PLINKCTL pLink;
    HFONT hFont;

    /* error checking */
    if (lstrlen(lpApp) >= (MAX_PATH - 1) ||
        lstrlen(lpParams) >= (MAX_PARAMS -1))
    {
        return FALSE;
    }

    pLink = (PLINKCTL)HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(LINKCTL));
    if (pLink == NULL)
        return FALSE;

    pLink->hSelf = hwnd;
    lstrcpyn(pLink->szApp, lpApp, MAX_PATH);
    lstrcpyn(pLink->szParams, lpParams, MAX_PARAMS);
    pLink->bClicked = FALSE;

    hFont=(HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    

    pLink->OldProc = (WNDPROC)SetWindowLongPtr(hwnd,
                                               GWL_WNDPROC,
                                               (LONG_PTR)LinkCtlWndProc);
    SetWindowLongPtr(hwnd,
                     GWL_USERDATA,
                     (LONG_PTR)pLink);

    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0);

    return TRUE;
}
