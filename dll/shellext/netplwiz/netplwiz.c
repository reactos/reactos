/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Disconnect Network Drive
 * FILE:            dll/shellext/netplwiz/netplwiz.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Jared Smudde
 */

#include "netplwiz.h"

HRESULT WINAPI DllCanUnloadNow(VOID)
{
    return S_OK;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
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
