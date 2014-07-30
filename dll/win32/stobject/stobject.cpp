/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\stobject\stobject.cpp
 * PURPOSE:     COM registration services for STobject.dll
 * PROGRAMMERS: Robert Naumann
 David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#include <olectl.h>
#include <atlwin.h>

WINE_DEFAULT_DEBUG_CHANNEL(stobject);

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SysTray, CSysTray)
END_OBJECT_MAP()

class CShellTrayModule : public CComModule
{
public:
};

HINSTANCE             g_hInstance;
CShellTrayModule      g_Module;
SysTrayIconHandlers_t g_IconHandlers [] = {
        { Volume_Init, Volume_Shutdown, Volume_Update, Volume_Message }
};
const int             g_NumIcons = _countof(g_IconHandlers);

void *operator new (size_t, void *buf)
{
    return buf;
}

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(g_hInstance);

        /* HACK - the global constructors don't run, so I placement new them here */
        new (&g_Module) CShellTrayModule;
        new (&_AtlWinModule) CAtlWinModule;
        new (&_AtlBaseModule) CAtlBaseModule;
        new (&_AtlComModule) CAtlComModule;

        g_Module.Init(ObjectMap, g_hInstance, NULL);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        g_hInstance = NULL;
        g_Module.Term();
    }
    return TRUE;
}

static
HRESULT
RegisterShellServiceObject(REFGUID guidClass, LPCWSTR lpName, BOOL bRegister)
{
    const LPCWSTR strRegistryLocation = L"Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad";

    HRESULT hr = E_FAIL;

    OLECHAR strGuid[128];

    HKEY hKey = 0;

    if (!StringFromGUID2(guidClass, strGuid, _countof(strGuid)))
    {
        DbgPrint("StringFromGUID2 failed\n");
        goto cleanup;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, strRegistryLocation, 0, KEY_WRITE, &hKey))
    {
        DbgPrint("RegOpenKeyExW failed\n");
        goto cleanup;
    }

    if (bRegister)
    {
        LONG cbGuid = (lstrlenW(strGuid) + 1) * 2;
        if (RegSetValueExW(hKey, lpName, 0, REG_SZ, (const BYTE *) strGuid, cbGuid))
        {
            DbgPrint("RegSetValueExW failed\n");
            goto cleanup;
        }
    }
    else
    {
        if (RegDeleteValueW(hKey, lpName))
        {
            DbgPrint("RegDeleteValueW failed\n");
            goto cleanup;
        }
    }

    hr = S_OK;

cleanup:
    if (hKey)
        RegCloseKey(hKey);

    return hr;
}

STDAPI
DllRegisterServer(void)
{
    HRESULT hr;

    hr = g_Module.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = RegisterShellServiceObject(CLSID_SysTray, L"SysTray", TRUE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;

    hr = RegisterShellServiceObject(CLSID_SysTray, L"SysTray", FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = g_Module.DllUnregisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    
    hr = g_Module.DllGetClassObject(rclsid, riid, ppv);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return g_Module.DllCanUnloadNow();
}
