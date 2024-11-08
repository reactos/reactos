/*
 * PROJECT:     ReactOS CabView Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     DLL entry point
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "cabview.h"

#include <initguid.h>
DEFINE_GUID(CLSID_CabFolder, 0x0CD7A5C0,0x9F37,0x11CE,0xAE,0x65,0x08,0x00,0x2B,0x2E,0x12,0x62);

CComModule g_Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CabFolder, CCabFolder)
END_OBJECT_MAP()

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            g_Module.Init(ObjectMap, hInstance, NULL);
            break;
    }

    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return g_Module.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return g_Module.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    HRESULT hr;

    hr = g_Module.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = g_Module.UpdateRegistryFromResource(IDR_FOLDER, TRUE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr1 = g_Module.DllUnregisterServer(FALSE);
    HRESULT hr2 = g_Module.UpdateRegistryFromResource(IDR_FOLDER, FALSE, NULL);
    if (FAILED_UNEXPECTEDLY(hr1))
        return hr1;
    if (FAILED_UNEXPECTEDLY(hr2))
        return hr2;
    return S_OK;
}
