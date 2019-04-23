/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implements the Connect/Disconnect Network places dialogs
 * COPYRIGHT:   Copyright 2018 Jared Smudde (computerwhiz02@hotmail.com)
 */

#include "netplwiz.h"

HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    return S_OK;
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    return E_NOINTERFACE;
}

HRESULT WINAPI
DllRegisterServer(VOID)
{
    return S_OK;
}

HRESULT WINAPI
DllUnregisterServer(VOID)
{
    return S_OK;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    INITCOMMONCONTROLSEX iccx;
    hInstance = hinstDLL;
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
            iccx.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&iccx);
            DisableThreadLibraryCalls(hInstance);
            break;
    }

    return TRUE;
}
