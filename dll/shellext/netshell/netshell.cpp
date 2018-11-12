/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Networking Configuration
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

HMODULE g_hModule = NULL;

HINSTANCE netshell_hInstance;

class CNetshellModule : public CComModule
{
public:
};

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ConnectionFolder, CNetworkConnections)
    OBJECT_ENTRY(CLSID_ConnectionManager, CNetConnectionManager)
    OBJECT_ENTRY(CLSID_LanConnectionUi, CNetConnectionPropertyUi)
    OBJECT_ENTRY(CLSID_ConnectionTray, CLanStatus)
END_OBJECT_MAP()

CNetshellModule gModule;

HPROPSHEETPAGE
InitializePropertySheetPage(LPWSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle)
{
    PROPSHEETPAGEW ppage;

    memset(&ppage, 0x0, sizeof(PROPSHEETPAGEW));
    ppage.dwSize = sizeof(PROPSHEETPAGEW);
    ppage.dwFlags = PSP_DEFAULT;
    ppage.pszTemplate = resname;
    ppage.pfnDlgProc = dlgproc;
    ppage.lParam = lParam;
    ppage.hInstance = netshell_hInstance;
    if (szTitle)
    {
        ppage.dwFlags |= PSP_USETITLE;
        ppage.pszTitle = szTitle;
    }
    return CreatePropertySheetPageW(&ppage);
}

extern "C"
{

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            netshell_hInstance = hinstDLL;
            DisableThreadLibraryCalls(netshell_hInstance);
            gModule.Init(ObjectMap, netshell_hInstance, NULL);
            break;
    default:
        break;
    }

    return TRUE;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_FALSE;
}

STDAPI
DllRegisterServer(void)
{
    HRESULT hr;

    hr = gModule.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_NETSHELL, TRUE, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;

    hr = gModule.DllUnregisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_NETSHELL, FALSE, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI
DllGetClassObject(
  REFCLSID rclsid,
  REFIID riid,
  LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

VOID
WINAPI
NcFreeNetconProperties(NETCON_PROPERTIES *pProps)
{
    CoTaskMemFree(pProps->pszwName);
    CoTaskMemFree(pProps->pszwDeviceName);
    CoTaskMemFree(pProps);
}

BOOL
WINAPI
NcIsValidConnectionName(_In_ PCWSTR pszwName)
{
    if (!pszwName)
        return FALSE;

    BOOL nonSpace = FALSE;
    while (*pszwName)
    {
        switch(*(pszwName++))
        {
        case L'\\':
        case L'/':
        case L':':
        case L'*':
        case L'\t':
        case L'?':
        case L'<':
        case L'>':
        case L'|':
        case L'\"':
            return FALSE;
        case L' ':
            break;
        default:
            nonSpace = TRUE;
            break;
        }
    }
    return nonSpace;
}

} // extern "C"
