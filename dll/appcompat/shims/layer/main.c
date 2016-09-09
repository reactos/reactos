/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/layer/main.c
 * PURPOSE:         Shim entrypoint
 * PROGRAMMER:      Mark Jansen
 */

#include <windows.h>
#include <strsafe.h>
#include <shimlib.h>

/* Forward to the generic implementation */
PHOOKAPI WINAPI GetHookAPIs(IN LPCSTR szCommandLine, IN LPCWSTR wszShimName, OUT PDWORD pdwHookCount)
{
    return ShimLib_GetHookAPIs(szCommandLine, wszShimName, pdwHookCount);
}

/* PLDR_DATA_TABLE_ENTRY */
BOOL WINAPI NotifyShims(DWORD fdwReason, PVOID P)
{
    return TRUE;
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
