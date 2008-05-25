/*
 *
 * PROJECT:         ReactOS WDM Audio driver mapper
 * FILE:            dll/win32/wdmaud.drv/wdmaud.c
 * PURPOSE:         wdmaud.drv
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *
 * UPDATE HISTORY:
 *      25/05/2008  Created
 */

#include <stdarg.h>

#include <windows.h>
#include <mmsystem.h>

DWORD APIENTRY
mxdMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
auxMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
wodMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
widMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
modMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    return MMSYSERR_NOTSUPPORTED;
}

LRESULT
DriverProc(DWORD dwDriverID,
           HDRVR hDriver,
           UINT uiMessage,
           LPARAM lParam1,
           LPARAM lParam2)
{
    return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

