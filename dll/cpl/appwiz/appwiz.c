/*
 * PROJECT:                 ReactOS Software Control Panel
 * FILE:                    dll/cpl/appwiz/appwiz.c
 * PURPOSE:                 ReactOS Software Control Panel
 * PROGRAMMERS:             Gero Kuehn (reactos.filter@gkware.com)
 *                          Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "appwiz.h"

#include <shellapi.h>
#include <cpl.h>
#include <wine/unicode.h>

HINSTANCE hApplet = NULL;

static LONG start_params(const WCHAR *params, HWND hwnd_parent)
{
    if (!params)
        return FALSE;

    if (!strcmpW(params, L"install_gecko"))
    {
        install_addon(ADDON_GECKO, hwnd_parent);
        return TRUE;
    }

    if (!strcmpW(params, L"install_mono"))
    {
        install_addon(ADDON_MONO, hwnd_parent);
        return TRUE;
    }

    WARN("unknown param %s\n", debugstr_w(params));
    return FALSE;
}

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

        case CPL_STARTWPARMSW:
            return start_params((const WCHAR *)lParam2, hwndCPl);

        case CPL_INQUIRE:
            CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->lData = 0;
            CPlInfo->idIcon = IDI_CPLSYSTEM;
            CPlInfo->idName = IDS_CPLSYSTEMNAME;
            CPlInfo->idInfo = IDS_CPLSYSTEMDESCRIPTION;
            break;

        case CPL_DBLCLK:
            ShellExecuteW(NULL,
                          NULL,
                          L"rapps.exe",
                          L"/appwiz",
                          NULL,
                          SW_SHOWNORMAL);
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
