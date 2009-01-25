/*
 *
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            dll/cpl/telephon/telephon.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *    10-19-2007  Created
 */

#include <windows.h>
#include <cpl.h>

#include "resource.h"

typedef LONG (CALLBACK* LPINTERNALCONFIG)(HWND, UINT, LPARAM, LPARAM);

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    LPINTERNALCONFIG lpInternalConfig;
    HINSTANCE hTapi32;
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
        {
            hTapi32 = LoadLibraryW(L"tapi32.dll");
            if (!hTapi32) return FALSE;

            lpInternalConfig = (LPINTERNALCONFIG) GetProcAddress(hTapi32, "internalConfig");
            if (!lpInternalConfig)
            {
                FreeLibrary(hTapi32);
                return FALSE;
            }

            lpInternalConfig(hwndCPl, 0, 0, 0);
            FreeLibrary(hTapi32);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    return TRUE;
}
