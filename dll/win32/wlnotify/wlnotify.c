/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Winlogon notifications
 * PROGRAMMER:  Eric Kohl
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlnotify);


BOOL
WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            break;
    }

    return TRUE;
}

/* EOF */
