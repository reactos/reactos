/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "shellmenu.h"

DWORD WINAPI WinList_Init(void)
{
    /* do something here (perhaps we may want to add our own implementation fo win8) */
    return 0;
}

void *operator new (size_t, void *buf)
{
    return buf;
}

class CRShellModule : public CComModule
{
public:
};

CRShellModule                               gModule;
CAtlWinModule                               gWinModule;

HINSTANCE g_hRShell;

static LSTATUS inline _RegSetStringValueW(HKEY hKey, LPCWSTR lpValueName, LPCWSTR lpStringData)
{
    DWORD dwStringDataLen = lstrlenW(lpStringData);

    return RegSetValueExW(hKey, lpValueName, 0, REG_SZ, (BYTE *) lpStringData, 2 * (dwStringDataLen + 1));
}

static HRESULT RegisterComponent(REFGUID clsid, LPCWSTR szDisplayName)
{
    WCHAR szFilename[MAX_PATH];
    WCHAR szClsid[MAX_PATH];
    WCHAR szRoot[MAX_PATH];

    if (!StringFromGUID2(clsid, szClsid, _countof(szClsid)))
        return E_FAIL;

    if (!GetModuleFileNameW(g_hRShell, szFilename, _countof(szFilename)))
        return E_FAIL;

    HRESULT hr = StringCchPrintfW(szRoot, 0x104u, L"CLSID\\%s", szClsid);
    if (FAILED(hr))
        return hr;

    DWORD dwDisposition;
    HKEY hkRoot;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szRoot, 0, NULL, 0, KEY_WRITE, 0, &hkRoot, &dwDisposition) != 0)
        return E_FAIL;

    HKEY hkServer;

    _RegSetStringValueW(hkRoot, NULL, szDisplayName);

    if (RegCreateKeyExW(hkRoot, L"InprocServer32", 0, NULL, 0, KEY_SET_VALUE, 0, &hkServer, &dwDisposition) != 0)
    {
        RegCloseKey(hkRoot);
        return E_FAIL;
    }

    _RegSetStringValueW(hkServer, NULL, szFilename);
    _RegSetStringValueW(hkServer, L"ThreadingModel", L"Both");

    RegCloseKey(hkServer);
    RegCloseKey(hkRoot);
    return S_OK;
}

static HRESULT UnregisterComponent(REFGUID clsid)
{
    WCHAR szClsid[MAX_PATH];
    WCHAR szRoot[MAX_PATH];
    HKEY hkRoot;

    if (!StringFromGUID2(clsid, szClsid, _countof(szClsid)))
        return E_FAIL;

    HRESULT hr = StringCchPrintfW(szRoot, 0x104u, L"CLSID\\%s", szClsid);
    if (FAILED(hr))
        return hr;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szRoot, 0, KEY_WRITE, &hkRoot) != 0)
        return E_FAIL;

    RegDeleteKeyW(hkRoot, L"InprocServer32");
    RegCloseKey(hkRoot);

    RegDeleteKeyW(HKEY_CLASSES_ROOT, szRoot);

    return S_OK;
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hRShell = hInstance;

        /* HACK - the global constructors don't run, so I placement new them here */
        new (&gModule) CRShellModule;
        new (&gWinModule) CAtlWinModule;
        new (&_AtlBaseModule) CAtlBaseModule;
        new (&_AtlComModule) CAtlComModule;

        gModule.Init(NULL, hInstance, NULL);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
    }
    return TRUE;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    gModule.DllCanUnloadNow();
    return S_FALSE;
}

STDAPI
DllRegisterServer(void)
{
    RegisterComponent(CLSID_StartMenu, L"Shell Start Menu");
    RegisterComponent(CLSID_MenuDeskBar, L"Shell Menu Desk Bar");
    RegisterComponent(CLSID_MenuBand, L"Shell Menu Band");
    RegisterComponent(CLSID_MenuBandSite, L"Shell Menu Band Site");
    RegisterComponent(CLSID_MergedFolder, L"Merged Shell Folder");
    RegisterComponent(CLSID_RebarBandSite, L"Shell Rebar Band Site");
    return S_OK;
}

STDAPI
DllUnregisterServer(void)
{
    UnregisterComponent(CLSID_StartMenu);
    UnregisterComponent(CLSID_MenuDeskBar);
    UnregisterComponent(CLSID_MenuBand);
    UnregisterComponent(CLSID_MenuBandSite);
    UnregisterComponent(CLSID_MergedFolder);
    UnregisterComponent(CLSID_RebarBandSite);
    return S_OK;
}

class CRShellClassFactory :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IClassFactory
{
private:
    CLSID m_Clsid;

public:
    CRShellClassFactory() {}
    virtual ~CRShellClassFactory() {}

    HRESULT Initialize(REFGUID clsid)
    {
        m_Clsid = clsid;
        return S_OK;
    }

    /* IClassFactory */
    virtual HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
    {
        *ppvObject = NULL;

        if (IsEqualCLSID(m_Clsid, CLSID_StartMenu))
            return CStartMenu_Constructor(riid, ppvObject);
            
        if (IsEqualCLSID(m_Clsid, CLSID_MenuDeskBar))
            return CMenuDeskBar_Constructor(riid, ppvObject);

        if (IsEqualCLSID(m_Clsid, CLSID_MenuBand))
            return CMenuBand_Constructor(riid, ppvObject);

        if (IsEqualCLSID(m_Clsid, CLSID_MenuBandSite))
            return CMenuSite_Constructor(riid, ppvObject);

        if (IsEqualCLSID(m_Clsid, CLSID_MergedFolder))
            return CMergedFolder_Constructor(riid, ppvObject);

        if (IsEqualCLSID(m_Clsid, CLSID_RebarBandSite))
            return CBandSite_Constructor(riid, ppvObject);

        return E_NOINTERFACE;
    }

    virtual HRESULT WINAPI LockServer(BOOL fLock)
    {
        return E_NOTIMPL;
    }

    BEGIN_COM_MAP(CRShellClassFactory)
        COM_INTERFACE_ENTRY_IID(IID_IClassFactory, IClassFactory)
    END_COM_MAP()
};

STDAPI
DllGetClassObject(
REFCLSID rclsid,
REFIID riid,
LPVOID *ppv)
{
    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    return ShellObjectCreatorInit<CRShellClassFactory>(rclsid, riid, ppv);
}
