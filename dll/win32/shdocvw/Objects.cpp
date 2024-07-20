/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     shdocvw.dll objects
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "Objects.h"

void *operator new(size_t size)
{
    return ::LocalAlloc(LPTR, size);
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

EXTERN_C HRESULT
SHDOCVW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
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
