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

const int ObjectMapCount = _countof(ObjectMap);

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

STDAPI
DllRegisterServer(void)
{
    HRESULT hr;

    DbgPrint("DllRegisterServer should process %d classes...\n", ObjectMapCount);

    hr = g_Module.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;

    DbgPrint("DllUnregisterServer should process %d classes...\n", ObjectMapCount);

    hr = g_Module.DllUnregisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    DbgPrint("DllGetClassObject should process %d classes...\n", ObjectMapCount);

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
