/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/msdvbnp.cpp
 * PURPOSE:         COM Initialization
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID KSCATEGORY_BDA_NETWORK_PROVIDER = {0x71985f4b, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
#endif

static INTERFACE_TABLE InterfaceTable[] =
{
    {&CLSID_DVBTNetworkProvider, CNetworkProvider_fnConstructor, L"ReactOS DVBT Network Provider"},
    {NULL, NULL, NULL}
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

#ifdef MSDVBNP_TRACE
            OutputDebugStringW(L"MSDVBNP::DllMain()\n");
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


    hr = StringFromCLSID(KSCATEGORY_BDA_NETWORK_PROVIDER, &pStr);
    if (FAILED(hr))
        return hr;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_SET_VALUE, &hClass) != ERROR_SUCCESS)
    {
        CoTaskMemFree(pStr);
        return E_FAIL;
    }

    RegDeleteKeyW(hClass, pStr);
    CoTaskMemFree(pStr);

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

VOID
RegisterBDAComponent(
    HKEY hFilter,
    LPCWSTR ComponentClsid,
    LPCWSTR ComponentName)
{
    HKEY hComp;

    // create network provider filter key
    if (RegCreateKeyExW(hFilter, ComponentClsid, 0, NULL, 0, KEY_WRITE, NULL, &hComp, NULL) == ERROR_SUCCESS)
    {
        // store class id
        RegSetValueExW(hComp, L"CLSID", 0, REG_SZ, (const BYTE*)ComponentClsid, (wcslen(ComponentClsid)+1) * sizeof(WCHAR));
        RegSetValueExW(hComp, L"FriendlyName", 0, REG_SZ, (const BYTE*)ComponentName, (wcslen(ComponentName)+1) * sizeof(WCHAR));
        RegCloseKey(hComp);
    }
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
    HKEY hClass, hKey, hSubKey, hProvider, hInstance, hFilter;
    static LPCWSTR ModuleName = L"msdvbnp.ax";
    static LPCWSTR ThreadingModel = L"Both";

    hr = StringFromCLSID(KSCATEGORY_BDA_NETWORK_PROVIDER, &pStr);
    if (FAILED(hr))
        return hr;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_WRITE, &hClass) != ERROR_SUCCESS)
    {
        CoTaskMemFree(pStr);
        return E_FAIL;
    }

    if (RegCreateKeyExW(hClass, pStr, 0, NULL, 0, KEY_WRITE, NULL, &hProvider, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hClass);
        CoTaskMemFree(pStr);
        return E_FAIL;
    }

    CoTaskMemFree(pStr);

    if (RegCreateKeyExW(hProvider, L"Instance", 0, NULL, 0, KEY_WRITE, NULL, &hInstance, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hClass);
        return E_FAIL;
    }
    RegCloseKey(hProvider);

    /* open active movie filter category key */
    if (RegCreateKeyExW(hClass, L"{da4e3da0-d07d-11d0-bd50-00a0c911ce86}\\Instance", 0, NULL, 0, KEY_WRITE, NULL, &hFilter, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hClass);
        RegCloseKey(hInstance);
        return E_FAIL;
    }

    RegisterBDAComponent(hFilter, L"{71985F4A-1CA1-11d3-9CC8-00C04F7971E0}", L"BDA Playback Filter");
    RegisterBDAComponent(hFilter, L"{71985F4B-1CA1-11D3-9CC8-00C04F7971E0}", L"BDA Network Providers");
    RegisterBDAComponent(hFilter, L"{71985F48-1CA1-11d3-9CC8-00C04F7971E0}", L"BDA Source Filter");
    RegisterBDAComponent(hFilter, L"{A2E3074F-6C3D-11D3-B653-00C04F79498E}", L"BDA Transport Information Renderers");
    RegisterBDAComponent(hFilter, L"{FD0A5AF4-B41D-11d2-9C95-00C04F7971E0}", L"BDA Receiver Component");
    RegCloseKey(hKey);

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

        if (RegCreateKeyExW(hInstance, InterfaceTable[Index].ProviderName, 0, 0, 0, KEY_WRITE, NULL, &hKey, 0) == ERROR_SUCCESS)
        {
            //FIXME filterdata
            RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (const BYTE*)InterfaceTable[Index].ProviderName, (wcslen(InterfaceTable[Index].ProviderName) + 1) * sizeof(WCHAR));
            RegSetValueExW(hKey, L"CLSID", 0, REG_SZ, (const BYTE*)pStr, (wcslen(pStr)+1) * sizeof(WCHAR));
            RegCloseKey(hKey);
        }




        CoTaskMemFree(pStr);
        Index++;
    }while(InterfaceTable[Index].lpfnCI != 0);

    RegCloseKey(hClass);
    RegCloseKey(hInstance);
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
