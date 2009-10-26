/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/dsound.c
 * PURPOSE:         Handles DSound initialization
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"


HINSTANCE dsound_hInstance;

HRESULT
WINAPI
DllCanUnloadNow()
{
    return S_FALSE;
}

HRESULT
WINAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv)
{
    UNIMPLEMENTED
    return CLASS_E_CLASSNOTAVAILABLE;
}

BOOL
WINAPI
DllMain(
    HINSTANCE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            dsound_hInstance = hInstDLL;
            DisableThreadLibraryCalls(dsound_hInstance);
            break;
    default:
        break;
    }

    return TRUE;
}

