/*
 * PROJECT:     ReactOS Network Event Handler
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/netevent/netevent.c
 * PURPOSE:     Dummy main file
 * COPYRIGHT:   Eric Kohl
 */

#include <windows.h>

BOOL
WINAPI
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
