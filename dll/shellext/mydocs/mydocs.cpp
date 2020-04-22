/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     MyDocs implementation
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(mydocs);

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MyDocsDropHandler, CMyDocsDropHandler)
END_OBJECT_MAP()

CComModule gModule;
LONG g_ModuleRefCnt = 0;

STDAPI DllCanUnloadNow(void)
{
    if (g_ModuleRefCnt)
        return S_FALSE;
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("CLSID:%s,IID:%s\n", wine_dbgstr_guid(&rclsid), wine_dbgstr_guid(&riid));

    HRESULT hr = gModule.DllGetClassObject(rclsid, riid, ppv);

    TRACE("-- pointer to class factory: %p\n", *ppv);

    return hr;
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr = gModule.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = gModule.DllUnregisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hInstance, dwReason, fImpLoad);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        gModule.Init(ObjectMap, hInstance, NULL);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
    }
    return TRUE;
}
