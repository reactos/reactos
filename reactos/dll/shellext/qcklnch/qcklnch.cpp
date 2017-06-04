/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/qcklnch.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "precomp.h"

#include <atlwin.h>

WINE_DEFAULT_DEBUG_CHANNEL(qcklnch);

BEGIN_OBJECT_MAP(ObjectMap)    
    OBJECT_ENTRY(CLSID_QuickLaunchBand, CQuickLaunchBand)
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
    HRESULT hr = g_Module.DllRegisterServer(FALSE);

    if (FAILED(hr)) 
        return hr;
    else
        return RegisterComCat();
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
