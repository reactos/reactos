/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/lrgcell.c
 * PURPOSE:     large cell window implementation
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static
HFONT
SetLrgFont(PMAP infoPtr)
{
    LOGFONTW lf;
    HFONT hFont = NULL;
    HDC hdc;
    HWND hCombo;
    LPWSTR lpFontName;
    INT Len;

    hCombo = GetDlgItem(infoPtr->hParent,
                        IDC_FONTCOMBO);

    Len = GetWindowTextLengthW(hCombo);

    if (Len != 0)
    {
        lpFontName = HeapAlloc(GetProcessHeap(),
                               0,
                               (Len + 1) * sizeof(WCHAR));

        if (lpFontName)
        {
            SendMessageW(hCombo,
                         WM_GETTEXT,
                         Len + 1,
                         (LPARAM)lpFontName);

            ZeroMemory(&lf,
                       sizeof(lf));

            hdc = GetDC(infoPtr->hLrgWnd);
            lf.lfHeight = GetDeviceCaps(hdc,
                                        LOGPIXELSY) / 2;
            ReleaseDC(infoPtr->hLrgWnd,
                      hdc);

            lf.lfCharSet =  DEFAULT_CHARSET;
            wcsncpy(lf.lfFaceName,
                    lpFontName,
                    sizeof(lf.lfFaceName) / sizeof(lf.lfFaceName[0]));

            hFont = CreateFontIndirectW(&lf);

            HeapFree(GetProcessHeap(),
                     0,
                     lpFontName);
        }
    }

    return hFont;
}


LRESULT CALLBACK
LrgCellWndProc(HWND hwnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PMAP infoPtr;
    LRESULT Ret = 0;
    static INT cxClient, cyClient;
    static RECT rc;
    static HFONT hFont = NULL;

    infoPtr = (PMAP)GetWindowLongPtrW(hwnd,
                                     GWLP_USERDATA);

    if (infoPtr == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
        case WM_CREATE:
        {
            infoPtr = (PMAP)(((LPCREATESTRUCTW)lParam)->lpCreateParams);

            SetWindowLongPtrW(hwnd,
                              GWLP_USERDATA,
                              (LONG_PTR)infoPtr);

            hFont = SetLrgFont(infoPtr);

            break;
        }

        case WM_SIZE:
        {
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);

            rc.left = 0;
            rc.top = 0;
            rc.right = cxClient;
            rc.bottom = cyClient;

            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            HFONT hOldFont;

            hdc = BeginPaint(hwnd,
                             &ps);

            Rectangle(hdc,
                      0,
                      0,
                      cxClient,
                      cyClient);

            hOldFont = SelectObject(hdc, hFont);

            DrawTextW(hdc,
                      &infoPtr->pActiveCell->ch,
                      1,
                      &rc,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);

            EndPaint(hwnd,
                     &ps);

            break;
        }

        case WM_DESTROY:
        {
            DeleteObject(hFont);

            break;
        }

        default:
        {
HandleDefaultMessage:
            Ret = DefWindowProcW(hwnd,
                                 uMsg,
                                 wParam,
                                 lParam);
            break;
        }
    }

    return Ret;
}
