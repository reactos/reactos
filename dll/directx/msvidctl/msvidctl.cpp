/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS DVB
 * FILE:            dll/directx/msvidctl/msvidctl.cpp
 * PURPOSE:         ReactOS DVB Initialization
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

static INTERFACE_TABLE InterfaceTable[] =
{
    {&CLSID_SystemTuningSpaces, CTuningSpaceContainer_fnConstructor},
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

#ifdef MSDVBNP_TRACE
            OutputDebugStringW(L"MSVIDCTL::DllMain()\n");
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
    static LPCWSTR ModuleName = L"msvidctl.ax";
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
