/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/framedyn/main.c
 * PURPOSE:         framedyn entry point
 * PROGRAMMERS:     Pierre Schweitzer (pierre@reactos.org)
 *
 */

/* INCLUDES ******************************************************************/

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
