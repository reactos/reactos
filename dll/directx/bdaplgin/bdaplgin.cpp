/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/bdaplgin.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

const GUID CBDADeviceControl_GUID = {STATIC_KSMETHODSETID_BdaChangeSync};
const GUID CBDAPinControl_GUID = {0x0DED49D5, 0xA8B7, 0x4d5d, {0x97, 0xA1, 0x12, 0xB0, 0xC1, 0x95, 0x87, 0x4D}};

static INTERFACE_TABLE InterfaceTable[] =
{
    {&CBDADeviceControl_GUID, CBDADeviceControl_fnConstructor},
    {&CBDAPinControl_GUID, CBDAPinControl_fnConstructor},
    {NULL, NULL}
};

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            CoInitialize(NULL);

#ifdef BDAPLGIN_TRACE
            OutputDebugStringW(L"BDAPLGIN::DllMain()\n");
#endif

            DisableThreadLibraryCalls(hInstDLL);
            break;
    default:
        break;
    }

    return TRUE;
}


extern "C"
KSDDKAPI
HRESULT
WINAPI
DllUnregisterServer(void)
{
    ULONG Index = 0;
    LPOLESTR pStr;
    HRESULT hr = S_OK;
    HKEY hClass;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_SET_VALUE, &hClass) != ERROR_SUCCESS)
        return E_FAIL;

    do
    {
        hr = StringFromCLSID(*InterfaceTable[Index].riid, &pStr);
        if (FAILED(hr))
            break;

        RegDeleteKeyW(hClass, pStr);
        CoTaskMemFree(pStr);
        Index++;
    }while(InterfaceTable[Index].lpfnCI != 0);

    RegCloseKey(hClass);
    return hr;
}

extern "C"
KSDDKAPI
HRESULT
WINAPI
DllRegisterServer(void)
{
    ULONG Index = 0;
    LPOLESTR pStr;
    HRESULT hr = S_OK;
    HKEY hClass, hKey, hSubKey;
    static LPCWSTR ModuleName = L"bdaplgin.ax";
    static LPCWSTR ThreadingModel = L"Both";

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_WRITE, &hClass) != ERROR_SUCCESS)
        return E_FAIL;

    do
    {
        hr = StringFromCLSID(*InterfaceTable[Index].riid, &pStr);
        if (FAILED(hr))
            break;

        if (RegCreateKeyExW(hClass, pStr, 0, 0, 0, KEY_WRITE, NULL, &hKey, 0) == ERROR_SUCCESS)
        {
            if (RegCreateKeyExW(hKey, L"InprocServer32", 0, 0, 0, KEY_WRITE, NULL, &hSubKey, 0) == ERROR_SUCCESS)
            {
                RegSetValueExW(hSubKey, 0, 0, REG_SZ, (const BYTE*)ModuleName, (wcslen(ModuleName) + 1) * sizeof(WCHAR));
                RegSetValueExW(hSubKey, L"ThreadingModel", 0, REG_SZ, (const BYTE*)ThreadingModel, (wcslen(ThreadingModel) + 1) * sizeof(WCHAR));
                RegCloseKey(hSubKey);
            }
            RegCloseKey(hKey);
        }

        CoTaskMemFree(pStr);
        Index++;
    }while(InterfaceTable[Index].lpfnCI != 0);

    RegCloseKey(hClass);
    return hr;
}

KSDDKAPI
HRESULT
WINAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv)
{
    UINT i;
    HRESULT hres = E_OUTOFMEMORY;
    IClassFactory * pcf = NULL;

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    for (i = 0; InterfaceTable[i].riid; i++)
    {
        if (IsEqualIID(*InterfaceTable[i].riid, rclsid))
        {
            pcf = CClassFactory_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
            break;
        }
    }

    if (!pcf)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    hres = pcf->QueryInterface(riid, ppv);
    pcf->Release();

    return hres;
}

KSDDKAPI
HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_OK;
}
