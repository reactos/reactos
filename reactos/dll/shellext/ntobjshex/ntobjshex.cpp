/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\stobject\stobject.cpp
 * PURPOSE:     COM registration services for STobject.dll
 * PROGRAMMERS: Robert Naumann
 David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#include <atlwin.h>

WINE_DEFAULT_DEBUG_CHANNEL(ntobjshex);

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_NtObjectFolder, CNtObjectFolder)
END_OBJECT_MAP()

HINSTANCE  g_hInstance;
CComModule g_Module;

void *operator new (size_t, void *buf)
{
    return buf;
}

STDAPI_(BOOL)
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(g_hInstance);

        /* HACK - the global constructors don't run, so I placement new them here */
        new (&g_Module) CComModule;
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
    return g_Module.DllRegisterServer(FALSE);
}

STDAPI
DllUnregisterServer(void)
{
    return g_Module.DllUnregisterServer(FALSE);
}

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return g_Module.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI
DllCanUnloadNow(void)
{
    return g_Module.DllCanUnloadNow();
}
