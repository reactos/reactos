/*
 * ReactOS Management Console
 * Copyright (C) 2006 - 2007 Thomas Weidenmueller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"


HINSTANCE hAppInstance;
HANDLE hAppHeap;
HWND hwndMainConsole;
HWND hwndMDIClient;


int WINAPI
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpCmdLine,
          int nCmdShow)
{
    MSG Msg;

    hAppInstance = hInstance; // GetModuleHandle(NULL);
    hAppHeap = GetProcessHeap();

    InitCommonControls();

    if (!RegisterMMCWndClasses())
    {
        /* FIXME - Display error */
        return 1;
    }

    hwndMainConsole = CreateConsoleWindow(NULL /*argc > 1 ? argv[1] : NULL*/, nCmdShow);
    if (hwndMainConsole != NULL)
    {
        while (GetMessage(&Msg, NULL, 0, 0))
        {
            if (!TranslateMDISysAccel(hwndMDIClient, &Msg))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    UnregisterMMCWndClasses();

    return 0;
}
