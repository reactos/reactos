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

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <scrnsave.h>
#include <stdlib.h>
#include <tchar.h>

#include "resource.h"

#define RANDOM( min, max ) ((rand() % (int)(((max)+1) - (min))) + (min))

#define APPNAME               _T("Logon")
#define APP_TIMER             1
#define APP_TIMER_INTERVAL    2000

HBITMAP
GetScreenSaverBitmap(VOID)
{
    OSVERSIONINFOEX osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx ((OSVERSIONINFO *) &osvi);

    switch(osvi.wProductType)
    {
        case VER_NT_WORKSTATION:
            return LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_WORKSTATION), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            break;
        default:
            return LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SERVER), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            break;
    }
}

LRESULT
CALLBACK
ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static RECT rect;
    static HBITMAP bitmap;

    switch (message)
    {
        case WM_CREATE:
        {
            bitmap = GetScreenSaverBitmap ();
            if (bitmap == NULL)
            {
                MessageBox(hWnd,
                           _T("Fatal Error: Could not load bitmap"),
                           _T("Error"),
                           MB_OK | MB_ICONEXCLAMATION);
            }

            SetTimer(hWnd,
                     APP_TIMER,
                     APP_TIMER_INTERVAL,
                     NULL);

             break;
        }
        case WM_PAINT:
        {
             BITMAP bm; /* Bitmap structure as seen in bmWidth & bmHeight */
             PAINTSTRUCT ps;
             HDC hdc;
             HDC hdcMem;
             HBITMAP hbmOld;

             // Obtain window coordinates.
             GetClientRect (hWnd, &rect);

             hdc = BeginPaint(hWnd, &ps);
             hdcMem = CreateCompatibleDC(hdc);
             hbmOld = SelectObject(hdcMem, bitmap);

             GetObject(bitmap, sizeof(bm), &bm);

             if (rect.right < bm.bmWidth ||
                 rect.bottom < bm.bmHeight)
             {
                StretchBlt(
                    hdc,
                    RANDOM (0, rect.right - (bm.bmWidth /5)),
                    RANDOM (0, rect.bottom - (bm.bmHeight /5)),
                    bm.bmWidth /5,
                    bm.bmHeight /5,
                    hdcMem,
                    0,
                    0,
                    bm.bmWidth,
                    bm.bmHeight,
                    SRCCOPY);
             }
             else
             {
                 BitBlt(
                     hdc,
                     RANDOM (0, rect.right - bm.bmWidth),
                     RANDOM (0, rect.bottom - bm.bmHeight),
                     bm.bmWidth,
                     bm.bmHeight,
                     hdcMem,
                     0,
                     0,
                     SRCCOPY);
             }

             SelectObject(hdcMem, hbmOld);
             DeleteDC(hdcMem);

             EndPaint(hWnd, &ps);
             break;
        }
        case WM_TIMER:
        {
          InvalidateRect(hWnd, NULL, 1);
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
            // Pass Windows Messages to the default screensaver window procedure
            return DefScreenSaverProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

BOOL
WINAPI
ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

// This function is only called one time before opening the configuration dialog.
// Use it to show a message that no configuration is necessary and return FALSE to indicate that no configuration dialog shall be opened.
BOOL
WINAPI
RegisterDialogClasses(HANDLE hInst)
{
    TCHAR szMessage[256];
    TCHAR szTitle[25];

    LoadString(hInst, IDS_TEXT, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    LoadString(hInst, IDS_DESCRIPTION, szTitle, sizeof(szTitle) / sizeof(TCHAR));

    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

    return FALSE;
}
