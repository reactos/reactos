/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell objects
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

void *operator new(size_t size)
{
    return ::LocalAlloc(LMEM_FIXED, size);
}

void operator delete(void *ptr)
{
    ::LocalFree(ptr);
}

void operator delete(void *ptr, size_t size)
{
    ::LocalFree(ptr);
}

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SH_FavBand, CFavBand)
    OBJECT_ENTRY(CLSID_ExplorerBand, CExplorerBand)
END_OBJECT_MAP()

class SHDOCVW_Module : public CComModule
{
public:
};

static SHDOCVW_Module gModule;

EXTERN_C VOID
SHDOCVW_Init(HINSTANCE hInstance)
{
    gModule.Init(ObjectMap, hInstance, NULL);
}

EXTERN_C HRESULT
SHDOCVW_DllCanUnloadNow(VOID)
{
    return gModule.DllCanUnloadNow();
}

EXTERN_C HRESULT CMruLongList_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3);
EXTERN_C HRESULT CMruPidlList_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3);
EXTERN_C HRESULT CMruClassFactory_CreateInstance(REFIID riid, void **ppv);

EXTERN_C HRESULT
SHDOCVW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hr = gModule.DllGetClassObject(rclsid, riid, ppv);
    if (SUCCEEDED(hr))
        return hr;

    if (IsEqualGUID(riid, IID_IClassFactory) || IsEqualGUID(riid, IID_IUnknown))
    {
        if (IsEqualGUID(rclsid, CLSID_MruLongList) ||
            IsEqualGUID(rclsid, CLSID_MruPidlList))
        {
            return CMruClassFactory_CreateInstance(riid, ppv);
        }
    }
    else if (IsEqualGUID(riid, IID_IMruDataList))
    {
        return CMruLongList_CreateInstance(0, ppv, 0);
    }
    else if (IsEqualGUID(riid, IID_IMruPidlList))
    {
        return CMruPidlList_CreateInstance(0, ppv, 0);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

EXTERN_C HRESULT
SHDOCVW_DllRegisterServer(VOID)
{
    return gModule.DllRegisterServer(FALSE);
}

EXTERN_C HRESULT
SHDOCVW_DllUnregisterServer(VOID)
{
    return gModule.DllUnregisterServer(FALSE);
}
