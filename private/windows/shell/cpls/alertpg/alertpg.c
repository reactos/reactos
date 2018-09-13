//
//  ALERT.C        Application installation wizard CPL
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ravir 05/01/95
//

#include "alertpg.h"
#include <cpl.h>


HINSTANCE hInst = NULL;


BOOL APIENTRY LibMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInst = hDll;
    }

    return TRUE;
}


LONG CALLBACK CPlApplet(HWND hwnd, UINT Msg, LPARAM lParam1, LPARAM lParam2 )
{
    UINT nStartPage;
    LPTSTR lpStartPage;

    switch (Msg)
    {
       case CPL_INIT:
           return TRUE;

       case CPL_GETCOUNT:
           return 1;

       case CPL_INQUIRE:
            #define lpCPlInfo ((LPCPLINFO)lParam2)
            lpCPlInfo->idIcon = IDI_ALERTICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;
            lpCPlInfo->lData  = 0;
            #undef lpCPlInfo
           break;

       case CPL_DBLCLK:
       case CPL_STARTWPARMS:
           return LoadComputerObjectAlertPage(hwnd);

       default:
           return FALSE;
    }

    return TRUE;

}  // CPlApplet




BOOL LoadComputerObjectAlertPage(HWND hwnd)
{
    PASLOADCOMPUTEROBJECTALERTPAGE pfunc = NULL;
    HINSTANCE hInst = NULL;

    hInst = LoadLibrary(TEXT("alertsys.dll"));

    if (hInst == NULL)
    {
#ifdef DEBUG
        ShellMessageBox(hInst, hwnd, TEXT("LoadLibrary"), NULL,
            MB_OK | MB_ICONEXCLAMATION,
            TEXT("Failed to load library alertsys.dll. (%d)"),
            GetLastError());
#endif
        return FALSE;
    }

    pfunc = (PASLOADCOMPUTEROBJECTALERTPAGE)GetProcAddress(
                    (HMODULE)hInst, "AsLoadComputerObjectAlertPage");

    if (pfunc != NULL)
    {
        pfunc(hwnd);
    }
    else
    {
#ifdef DEBUG
        ShellMessageBox(hInst, hwnd, TEXT("GetProcAddress"), NULL,
            MB_OK | MB_ICONEXCLAMATION,
            TEXT("Failed to get AsLoadComputerObjectAlertPage's address. (%d)"),
            GetLastError());
#endif
        return FALSE;
    }

    return TRUE;
}



