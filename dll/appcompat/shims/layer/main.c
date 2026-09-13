/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shim entrypoint
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen <mark.jansen@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <strsafe.h>
#include <shimlib.h>

/* Forward to the generic implementation */
PHOOKAPI WINAPI GetHookAPIs(IN LPCSTR szCommandLine, IN LPCWSTR wszShimName, OUT PDWORD pdwHookCount)
{
    return ShimLib_GetHookAPIs(szCommandLine, wszShimName, pdwHookCount);
}

/* Forward to the generic implementation */
BOOL WINAPI NotifyShims(DWORD fdwReason, PVOID ptr)
{
    return ShimLib_NotifyShims(fdwReason, ptr);
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        ShimLib_Init(hInstance);
        break;
    case DLL_PROCESS_DETACH:
        ShimLib_Deinit();
        break;
    }
    return TRUE;
}
