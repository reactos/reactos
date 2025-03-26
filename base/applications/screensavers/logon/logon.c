/*
 *  Copyright 2003 J Brown
 *  Copyright 2006 Eric Kohl
 *  Copyright 2007 Marc Piulachs (marc.piulachs@codexchange.net)
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <scrnsave.h>

#include "resource.h"

#define RANDOM(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))

#define APP_TIMER             1
#define APP_TIMER_INTERVAL    10000

static
HBITMAP
GetScreenSaverBitmap(VOID)
{
    OSVERSIONINFOEX osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx((POSVERSIONINFO)&osvi);

    return LoadImageW(GetModuleHandle(NULL),
                      osvi.wProductType == VER_NT_WORKSTATION ?
                          MAKEINTRESOURCEW(IDB_WORKSTATION) : MAKEINTRESOURCEW(IDB_SERVER),
                      IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
}

LRESULT
CALLBACK
ScreenSaverProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP bitmap;

    switch (uMsg)
    {
        case WM_CREATE:
        {
            bitmap = GetScreenSaverBitmap();
            if (bitmap == NULL)
            {
                /* Extremely unlikely, message not localized. */
                MessageBoxW(hWnd,
                            L"Fatal Error: Could not load bitmap",
                            L"Error",
                            MB_OK | MB_ICONEXCLAMATION);
            }

            SetTimer(hWnd, APP_TIMER, APP_TIMER_INTERVAL, NULL);
            break;
        }
        case WM_PAINT:
        {
            BITMAP bm;
            PAINTSTRUCT ps;
            HDC hdc;
            HDC hdcMem;
            HBITMAP hbmOld;
            RECT rect;

            hdc = BeginPaint(hWnd, &ps);
            hdcMem = CreateCompatibleDC(hdc);
            hbmOld = SelectObject(hdcMem, bitmap);
            GetObjectW(bitmap, sizeof(bm), &bm);

            GetClientRect(hWnd, &rect);
            if (rect.right < bm.bmWidth || rect.bottom < bm.bmHeight)
            {
                StretchBlt(hdc, RANDOM(0, rect.right - (bm.bmWidth / 5)),
                           RANDOM(0, rect.bottom - (bm.bmHeight / 5)),
                           bm.bmWidth / 5, bm.bmHeight / 5, hdcMem, 0, 0,
                           bm.bmWidth, bm.bmHeight, SRCCOPY);
            }
            else
            {
                BitBlt(hdc, RANDOM(0, rect.right - bm.bmWidth),
                    RANDOM(0, rect.bottom - bm.bmHeight),
                    bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
            }

            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_TIMER:
        {
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case WM_DESTROY:
        {
            KillTimer(hWnd, APP_TIMER);
            DeleteObject(bitmap);
            PostQuitMessage(0);
            break;
        }
        default:
        {
            /* Pass window messages to the default screensaver window procedure */
            return DefScreenSaverProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

BOOL
WINAPI
ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

/* This function is only called once before opening the configuration dialog.
 * Use it to show a message that no configuration is necessary and return FALSE to indicate that no configuration dialog shall be opened.
 */
BOOL
WINAPI
RegisterDialogClasses(HANDLE hInst)
{
    WCHAR szMessage[256];
    WCHAR szTitle[25];

    LoadStringW(hInst, IDS_TEXT, szMessage, _countof(szMessage));
    LoadStringW(hInst, IDS_DESCRIPTION, szTitle, _countof(szTitle));

    MessageBoxW(NULL, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

    return FALSE;
}
