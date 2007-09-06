/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/devmgmt/devmgmt.c
 * PURPOSE:     Program HQ
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

HINSTANCE hInstance;
HANDLE ProcessHeap;
HANDLE hMutex;

int WINAPI
_tWinMain(HINSTANCE hThisInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpCmdLine,
          int nCmdShow)
{
    LPTSTR lpAppName;
    HWND hMainWnd;
    MSG Msg;
    int Ret = 1;
    INITCOMMONCONTROLSEX icex;

    hMutex = CreateMutex(NULL, TRUE, _T("devmgmt_mutex"));
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex)
        {
            CloseHandle(hMutex);
        }
        return 0;
    }



    hInstance = hThisInstance;
    ProcessHeap = GetProcessHeap();

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    if (!AllocAndLoadString(&lpAppName,
                            hInstance,
                            IDS_APPNAME))
    {
        return 1;
    }

    if (InitMainWindowImpl())
    {
        hMainWnd = CreateMainWindow(lpAppName,
                                    nCmdShow);
        if (hMainWnd != NULL)
        {
            /* pump the message queue */
            while( GetMessage( &Msg, NULL, 0, 0 ) )
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);

            }

            Ret = 0;
        }

        UninitMainWindowImpl();
    }

    LocalFree((HLOCAL)lpAppName);
    CloseHandle(hMutex);
    return Ret;
}



