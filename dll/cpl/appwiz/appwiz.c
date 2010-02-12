/*
 * PROJECT:                 ReactOS Software Control Panel
 * FILE:                    dll/cpl/appwiz/appwiz.c
 * PURPOSE:                 ReactOS Software Control Panel
 * PROGRAMMERS:             Gero Kuehn (reactos.filter@gkware.com)
 *                          Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "appwiz.h"

HINSTANCE hApplet = NULL;

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    CPLINFO *CPlInfo;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->lData = 0;
            CPlInfo->idIcon = IDI_CPLSYSTEM;
            CPlInfo->idName = IDS_CPLSYSTEMNAME;
            CPlInfo->idInfo = IDS_CPLSYSTEMDESCRIPTION;
            break;

        case CPL_DBLCLK:
            ShellExecute(NULL,
                         NULL,
                         _T("rapps.exe"),
                         NULL,
                         NULL,
                         1);
            break;
    }

    return FALSE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            CoInitialize(NULL);
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
