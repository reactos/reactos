/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/stobject.cpp
 * PURPOSE:     COM registration services for STobject.dll
 * PROGRAMMERS: Robert Naumann
 *              David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SysTray, CSysTray)
END_OBJECT_MAP()

HINSTANCE  g_hInstance;
CComModule g_Module;

STDAPI_(BOOL)
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(g_hInstance);

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
