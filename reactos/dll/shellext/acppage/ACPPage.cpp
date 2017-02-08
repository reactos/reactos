/*
 * Copyright 2015 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

#include <shellutils.h>

HMODULE g_hModule = NULL;
LONG g_ModuleRefCnt = 0;

class CLayerUIPropPageModule : public CComModule
{
public:
    void Term()
    {
        CComModule::Term();
    }
};

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CLayerUIPropPage, CLayerUIPropPage)
END_OBJECT_MAP()

CLayerUIPropPageModule gModule;

EXTERN_C
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        g_hModule = hInstance;
        gModule.Init(ObjectMap, hInstance, NULL);
        break;
    }

    return(TRUE);
}

STDAPI DllCanUnloadNow()
{
    if (g_ModuleRefCnt)
        return S_FALSE;
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    HRESULT hr;

    hr = gModule.DllRegisterServer(FALSE);
    if (FAILED(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_ACPPAGE, TRUE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr;

    hr = gModule.DllUnregisterServer(FALSE);
    if (FAILED(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_ACPPAGE, FALSE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

struct CCoInit
{
    CCoInit() { hres = CoInitialize(NULL); }
    ~CCoInit() { if (SUCCEEDED(hres)) { CoUninitialize(); } }
    HRESULT hres;
};

EXTERN_C
BOOL WINAPI GetExeFromLnk(PCWSTR pszLnk, PWSTR pszExe, size_t cchSize)
{
    CCoInit init;
    if (FAILED(init.hres))
        return FALSE;

    CComPtr<IShellLinkW> spShellLink;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLinkW, &spShellLink))))
        return FALSE;

    CComPtr<IPersistFile> spPersistFile;
    if (FAILED(spShellLink->QueryInterface(IID_PPV_ARG(IPersistFile, &spPersistFile))))
        return FALSE;

    if (FAILED(spPersistFile->Load(pszLnk, STGM_READ)) || FAILED(spShellLink->Resolve(NULL, SLR_NO_UI | SLR_NOUPDATE | SLR_NOSEARCH)))
        return FALSE;

    return SUCCEEDED(spShellLink->GetPath(pszExe, cchSize, NULL, SLGP_RAWPATH));
}
