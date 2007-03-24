/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/servman.c
 * PURPOSE:     Program HQ
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

HINSTANCE hInstance;
HANDLE ProcessHeap;

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
                //if(! IsDialogMessage(hProgDlg, &Msg) )
                //{
                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                //}
            }

            Ret = 0;
        }

        UninitMainWindowImpl();
    }

    LocalFree((HLOCAL)lpAppName);

    return Ret;
}



