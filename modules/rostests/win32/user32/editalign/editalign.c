/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     LGPL-2.0+ (https://spdx.org/licenses/LGPL-2.0+)
 * PURPOSE:     Tests text alignment of EDIT control
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <dlgs.h>
#include "resource.h"

static HINSTANCE g_hInstance = NULL;
static HWND g_hMainWnd = NULL;

static void SetEditAlign(HWND hwndEdit, LONG_PTR add_style)
{
    LONG_PTR style = GetWindowLongPtr(hwndEdit, GWL_STYLE);
    style &= ~(ES_LEFT | ES_CENTER | ES_RIGHT);
    style |= add_style;
    SetWindowLongPtr(hwndEdit, GWL_STYLE, style);
    InvalidateRect(hwndEdit, NULL, TRUE);
}

static void SetMultiline(HWND hwnd, HWND hwndEdit, BOOL bMultiline)
{
    TCHAR text[1024];
    GetWindowText(hwndEdit, text, ARRAYSIZE(text));

    RECT rc;
    GetWindowRect(hwndEdit, &rc);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, sizeof(RECT) / sizeof(POINT));

    DWORD exstyle = (LONG)GetWindowLongPtr(hwndEdit, GWL_EXSTYLE);
    DWORD style = (LONG)GetWindowLongPtr(hwndEdit, GWL_STYLE);
    style &= ~ES_MULTILINE;
    if (bMultiline)
        style |= ES_MULTILINE;

    DestroyWindow(hwndEdit);
    CreateWindowEx(exstyle, TEXT("EDIT"), text, style,
                   rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                   hwnd, (HMENU)UlongToHandle(edt1), g_hInstance, NULL);

    HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendDlgItemMessage(hwnd, edt1, WM_SETFONT, (WPARAM)hFont, TRUE);
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            g_hMainWnd = hwnd;
            SetDlgItemText(hwnd, edt1, TEXT("Text"));
            CheckRadioButton(hwnd, rad1, rad3, rad1);
            return TRUE;
        }
        case WM_COMMAND:
        {
            HWND hwndEdit = GetDlgItem(hwnd, edt1);
            switch (LOWORD(wParam))
            {
            case IDCANCEL: // Cancel
                EndDialog(hwnd, LOWORD(wParam));
                break;
            case rad1: // Left
                if (HIWORD(wParam) == BN_CLICKED)
                    SetEditAlign(hwndEdit, ES_LEFT);
                break;
            case rad2: // Center
                if (HIWORD(wParam) == BN_CLICKED)
                    SetEditAlign(hwndEdit, ES_CENTER);
                break;
            case rad3: // Right
                if (HIWORD(wParam) == BN_CLICKED)
                    SetEditAlign(hwndEdit, ES_RIGHT);
                break;
            case chx1: // Multiline
                if (HIWORD(wParam) == BN_CLICKED)
                {
                    if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
                        SetMultiline(hwnd, hwndEdit, TRUE);
                    else
                        SetMultiline(hwnd, hwndEdit, FALSE);
                }
            }
            break;
        }
    }
    return 0;
}

INT WINAPI
wWinMain(HINSTANCE   hInstance,
         HINSTANCE   hPrevInstance,
         LPWSTR      lpCmdLine,
         INT         nCmdShow)
{
    g_hInstance = hInstance;
    InitCommonControls();
    DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
    return 0;
}
