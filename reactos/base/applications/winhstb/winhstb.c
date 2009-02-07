/*
 * PROJECT:         winhlp32.exe
 * FILE:            base\applications\winhlp32\winhstb\winhstb.c
 * PURPOSE:         Stub winhelp32
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "resource.h"

#define WINHLP  _T("winhlp32.exe")

int
WINAPI
_tWinMain(HINSTANCE hCurInst,
          HINSTANCE hPrevInst,
          LPTSTR lpsCmdLine,
          int nCmdShow)
{
    TCHAR szPath[MAX_PATH];

    if(GetWindowsDirectory(szPath, MAX_PATH))
    {
        PathAppend(szPath, WINHLP);
        ShellExecute(NULL,
                     NULL,
                     szPath,
                     lpsCmdLine,
                     NULL,
                     nCmdShow);
    }
    else
    {
        ShellExecute(NULL,
                     NULL,
                     WINHLP,
                     lpsCmdLine,
                     NULL,
                     nCmdShow);
    }

    return 0;
}
