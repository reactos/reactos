/*
 *  Copyright 2008 Marc Piulachs (marc.piulachs@codexchange.net)
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <windows.h>
#include <scrnsave.h>
#include <tchar.h>
#include "resource.h"

#define RANDOM_COLOR (rand () % 256)

#define APPNAME              _T("Circles")
#define APP_TIMER            1
#define APP_TIMER_INTERVAL   100
#define MAX_CIRCLES          50

int circlesCount;
int width, x, y;

LRESULT WINAPI ScreenSaverProc (HWND hwnd, UINT iMsg, WPARAM wparam, LPARAM lparam)
{
    HDC hdc;
    RECT rect;
    HBRUSH hbrush, hbrushOld;

    switch (iMsg)
    {
        case WM_CREATE:
            SetTimer (hwnd, APP_TIMER, APP_TIMER_INTERVAL, NULL);
            break;

        case WM_DESTROY:
            KillTimer (hwnd, APP_TIMER);
            PostQuitMessage (0);
            return 0;

        case WM_TIMER:
            hdc = GetDC (hwnd);
            GetClientRect (hwnd, &rect);
            hbrush = CreateSolidBrush (RGB (RANDOM_COLOR, RANDOM_COLOR, RANDOM_COLOR));
            hbrushOld = SelectObject (hdc, hbrush);

            x = rand () % rect.right;
            y = rand () % rect.bottom;

            /* the circle will be 10% of total screen */
            width = rect.right / 10;
            if (rect.bottom / 10 < width)
                width = rect.bottom / 10;

            /* Draw circle on screen */
            Ellipse (
                hdc,
                x,
                y,
                x + width,
                y + width);

            //Track the number of painted circles on scren
            circlesCount++;
            if (circlesCount == MAX_CIRCLES)
            {
                InvalidateRect (hwnd, NULL, TRUE);
                circlesCount = 0;
            }

            SelectObject (hdc, hbrushOld);
            DeleteObject (hbrush);
            ReleaseDC (hwnd, hdc);

            return 0;
    }

    return DefScreenSaverProc (hwnd, iMsg, wparam, lparam);
}


BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

// This function is only called one time before opening the configuration dialog.
// Use it to show a message that no configuration is necesssary and return FALSE to indicate that no configuration dialog shall be opened.
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    TCHAR szMessage[256];
    TCHAR szTitle[25];

    LoadString(hInst, IDS_TEXT, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    LoadString(hInst, IDS_DESCRIPTION, szTitle, sizeof(szTitle) / sizeof(TCHAR));

    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

    return FALSE;
}
