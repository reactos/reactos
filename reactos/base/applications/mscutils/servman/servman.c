/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/servman.c
 * PURPOSE:     Program HQ
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

#include <winnls.h>

HINSTANCE hInstance;
HANDLE ProcessHeap;
HWND g_hProgDlg;

int WINAPI
wWinMain(HINSTANCE hThisInstance,
          HINSTANCE hPrevInstance,
          LPWSTR lpCmdLine,
          int nCmdShow)
{
    LPWSTR lpAppName;
    HWND hMainWnd;
    MSG Msg;
    int Ret = 1;
    INITCOMMONCONTROLSEX icex;
    
    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
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
            while (GetMessageW( &Msg, NULL, 0, 0 ) )
            {
                //if ( !hProgDlg || !IsWindow(hProgDlg) || !IsDialogMessage(hProgDlg, &Msg) )
                //if (!IsDialogMessage(g_hProgDlg, &Msg))
                {
                    TranslateMessage(&Msg);
                    DispatchMessageW(&Msg);
                }
            }

            Ret = 0;
        }

        UninitMainWindowImpl();
    }

    LocalFree((HLOCAL)lpAppName);

    return Ret;
}
