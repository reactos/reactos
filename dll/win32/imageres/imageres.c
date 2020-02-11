/*
 * PROJECT:     imageres
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Image Resource
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define COBJMACROS
#define WIN32_NO_STATUS

#include <windows.h>
#include <wine/debug.h>
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(imageres);

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, lpvReserved);
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
