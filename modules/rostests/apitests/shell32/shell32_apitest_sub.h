#pragma once

#include <shlwapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <strsafe.h>

#define MAIN_CLASSNAME   L"SHChangeNotify main window"
#define SUB_CLASSNAME    L"SHChangeNotify sub-window"

#define WM_SHELL_NOTIFY     (WM_USER + 100)

static inline HWND DoWaitForWindow(LPCWSTR clsname, LPCWSTR text, BOOL bClosing, BOOL bForce)
{
    HWND hwnd = NULL;
    for (INT i = 0; i < 50; ++i)
    {
        hwnd = FindWindowW(clsname, text);
        if (bClosing)
        {
            if (!hwnd)
                break;

            if (bForce)
                PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else
        {
            if (hwnd)
                break;
        }

        Sleep(1);
    }
    return hwnd;
}
