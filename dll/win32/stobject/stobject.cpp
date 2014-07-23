/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\stobject\stobject.c
 * PURPOSE:     Systray shell service object
 * PROGRAMMERS: Copyright 2014 Robert Naumann
 */

#include "precomp.h"

#include <olectl.h>

WINE_DEFAULT_DEBUG_CHANNEL(stobject);

const GUID CLSID_SysTray = { 0x35CEC8A3, 0x2BE6, 0x11D2, { 0x87, 0x73, 0x92, 0xE2, 0x20, 0x52, 0x41, 0x53 } };

class CShellTrayModule : public CComModule
{
public:
};

class CSysTray :
    public CComCoClass<CSysTray, &CLSID_SysTray>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IOleCommandTarget
{
    // TODO: keep icon handlers here

public:
    CSysTray() {}
    virtual ~CSysTray() {}

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) 
    {
        UNIMPLEMENTED;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {
        if (!IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
            return E_FAIL;

        switch (nCmdID)
        {
        case OLECMDID_NEW: // init
            DbgPrint("CSysTray Init TODO: Initialize tray icon handlers.\n");
            break;
        case OLECMDID_SAVE: // shutdown
            DbgPrint("CSysTray Shutdown TODO: Shutdown.\n");
            break;
        }
        return S_OK;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_SYSTRAY)
    DECLARE_NOT_AGGREGATABLE(CSysTray)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSysTray)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    END_COM_MAP()
};


BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SysTray, CSysTray)
END_OBJECT_MAP()

CShellTrayModule gModule;

HINSTANCE g_hInstance;

HRESULT RegisterShellServiceObject(REFGUID guidClass, LPCWSTR lpName, BOOL bRegister)
{
    const LPCWSTR strRegistryLocation = L"Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad";

    OLECHAR strGuid[128]; // shouldn't need so much!
    LSTATUS ret = 0;
    HKEY hKey = 0;
    if (StringFromGUID2(guidClass, strGuid, 128))
    {
        if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, strRegistryLocation, 0, KEY_WRITE, &hKey))
        {
            if (bRegister)
            {
                LONG cbGuid = (lstrlenW(strGuid) + 1) * 2;
                ret = RegSetValueExW(hKey, lpName, 0, REG_SZ, (const BYTE *) strGuid, cbGuid);
            }
            else
            {
                ret = RegDeleteValueW(hKey, lpName);
            }
        }
    }
    if (hKey)
        RegCloseKey(hKey);
    return /*HRESULT_FROM_NT*/(ret); // regsvr32 considers anything != S_OK to be an error

}

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
        /* HACK - the global constructors don't run, so I placement new them here */
        new (&gModule) CShellTrayModule;
        new (&_AtlWinModule) CAtlWinModule;
        new (&_AtlBaseModule) CAtlBaseModule;
        new (&_AtlComModule) CAtlComModule;

        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(g_hInstance);

        gModule.Init(ObjectMap, g_hInstance, NULL);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        g_hInstance = NULL;
        gModule.Term();
    }
    return TRUE;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return gModule.DllCanUnloadNow();
}

STDAPI
DllRegisterServer(void)
{
    HRESULT hr = gModule.DllRegisterServer(FALSE);
    if (FAILED(hr))
        return hr;

    return RegisterShellServiceObject(CLSID_SysTray, L"SysTray", TRUE);
}

STDAPI
DllUnregisterServer(void)
{
    RegisterShellServiceObject(CLSID_SysTray, L"SysTray", FALSE);

    return gModule.DllUnregisterServer(FALSE);
}

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}
