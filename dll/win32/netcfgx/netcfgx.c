/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/win32/netcfgx/netcfgx.c
 * PURPOSE:         Network devices installer
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

#include <olectl.h>


HINSTANCE netcfgx_hInstance;
const GUID CLSID_TcpipConfigNotifyObject      = {0xA907657F, 0x6FDF, 0x11D0, {0x8E, 0xFB, 0x00, 0xC0, 0x4F, 0xD9, 0x12, 0xB2}};

static INTERFACE_TABLE InterfaceTable[] =
{
    {
        &CLSID_CNetCfg,
        INetCfg_Constructor
    },
    {
        &CLSID_TcpipConfigNotifyObject,
        TcpipConfigNotify_Constructor
    },
    {
        NULL,
        NULL
    }
};

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            netcfgx_hInstance = hinstDLL;
            DisableThreadLibraryCalls(netcfgx_hInstance);
            InitCommonControls();
            break;

        default:
            break;
    }

    return TRUE;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_FALSE;
}

STDAPI
DllRegisterServer(void)
{
    HKEY hKey, hSubKey;
    LPOLESTR pStr;
    WCHAR szName[MAX_PATH] = L"CLSID\\";

    if (FAILED(StringFromCLSID(&CLSID_CNetCfg, &pStr)))
        return SELFREG_E_CLASS;

    wcscpy(&szName[6], pStr);
    CoTaskMemFree(pStr);

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szName, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    if (RegCreateKeyExW(hKey, L"InProcServer32", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        if (!GetModuleFileNameW(netcfgx_hInstance, szName, sizeof(szName)/sizeof(WCHAR)))
        {
            RegCloseKey(hSubKey);
            RegCloseKey(hKey);
            return SELFREG_E_CLASS;
        }
        szName[(sizeof(szName)/sizeof(WCHAR))-1] = L'\0';
        RegSetValueW(hSubKey, NULL, REG_SZ, szName, (wcslen(szName)+1) * sizeof(WCHAR));
        RegSetValueExW(hSubKey, L"ThreadingModel", 0, REG_SZ, (LPBYTE)L"Both", 10);
        RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);
    return S_OK;
}

STDAPI
DllUnregisterServer(void)
{
    //FIXME
    // implement unregistering services
    //
    return S_OK;
}

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv)
{
    UINT i;
    HRESULT hres = E_OUTOFMEMORY;
    IClassFactory * pcf = NULL;

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    for (i = 0; InterfaceTable[i].riid; i++)
    {
        if (IsEqualIID(InterfaceTable[i].riid, rclsid))
        {
            pcf = IClassFactory_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
            break;
        }
    }

    if (!pcf)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    hres = IClassFactory_QueryInterface(pcf, riid, ppv);
    IClassFactory_Release(pcf);

    return hres;
}
