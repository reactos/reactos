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

int
_tmain(IN int argc,
       IN const TCHAR *argv[])
{
    HWND hMainConsole;
    MSG Msg;
    BOOL bRet;

    hAppInstance = GetModuleHandle(NULL);
    hAppHeap = GetProcessHeap();

    InitCommonControls();

    if (!RegisterMMCWndClasses())
    {
        /* FIXME - Display error */
        return 1;
    }

    hMainConsole = CreateConsoleWindow(argc > 1 ? argv[1] : NULL);
    if (hMainConsole != NULL)
    {
        for (;;)
        {
            bRet = GetMessage(&Msg,
                              NULL,
                              0,
                              0);
            if (bRet != 0 && bRet != -1)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
            else if (bRet == 0)
                break;
        }
    }

    UnregisterMMCWndClasses();
    return 0;
}
