/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/wlnotify.c
 * PURPOSE:     Winlogon notifications
 * PROGRAMMER:  Eric Kohl
 */

#include "precomp.h"

#define _NDEBUG
#include <debug.h>


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
