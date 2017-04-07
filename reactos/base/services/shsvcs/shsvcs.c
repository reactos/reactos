/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Shell
 * FILE:             base/services/shsvcs/shsvcs.c
 * PURPOSE:          ReactOS Shell Services
 * PROGRAMMER:       Giannis Adamopoulos
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shsvcs);

/* FUNCTIONS *****************************************************************/

HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    return S_OK;
}

HRESULT WINAPI DllRegisterServer()
{
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer()
{
    return S_OK;
}

HRESULT WINAPI DllCanUnloadNow()
{
    return S_OK;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
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
