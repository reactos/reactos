/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell WMI info functions
 * COPYRIGHT:  Copyright 2026 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#define COBJMACROS

#include "precomp.h"

#include <strsafe.h>
#include <initguid.h>
#include <objbase.h>
#include <wbemcli.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

UINT  VersionInfoArchitecture;
UINT  VersionInfoOsProductSuite;
UINT  VersionInfoOsType;
UINT  VersionInfoVersion;
WCHAR VersionInfoBuildNumber[MAX_PATH];
WCHAR VersionInfoServicePackMajorVersion[MAX_PATH];
WCHAR VersionInfoServicePackMinorVersion[MAX_PATH];

/* FUNCTIONS ******************************************************************/

static
HRESULT
QueryOperatingSystemInfo(
    _In_ IWbemServices *Services)
{
    IEnumWbemClassObject *Result = NULL;
    IWbemClassObject *Object = NULL;
    BSTR Wql = NULL;
    BSTR Query = NULL;
    BSTR Name;
    CIMTYPE Type;
    ULONG Count;
    VARIANT Value;
    HRESULT hr;

    Wql = SysAllocString(L"WQL");
    if (Wql == NULL)
        return E_OUTOFMEMORY;
    
    Query = SysAllocString(L"SELECT * FROM Win32_OperatingSystem");
    if (Query == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = IWbemServices_ExecQuery(Services, Wql, Query, 0, NULL, &Result);
    if (hr !=  S_OK)
    {
        DPRINT1("IWbemServices_ExecQuery failed %08x\n", hr);
        goto done;
    }

    hr = IEnumWbemClassObject_Next(Result, 10000, 1, &Object, &Count);
    if (hr != S_OK)
    {
        DPRINT1("IEnumWbemClassObject_Next failed %08x\n", hr);
        goto done;
    }

    hr = IWbemClassObject_BeginEnumeration(Object, 0);
    if (hr != S_OK)
    {
        DPRINT1("IWbemClassObject_BeginEnumeration returned %08x\n", hr);
        goto done;
    }

    while (IWbemClassObject_Next(Object, 0, &Name, &Value, &Type, NULL) == S_OK)
    {
        DPRINT1("Name: %S\n", Name);
        if (_wcsicmp(Name, L"BuildNumber") == 0)
        {
            DPRINT1("BuildNumber %S\n", V_BSTR(&Value));
            StringCbCopyW(VersionInfoBuildNumber, sizeof(VersionInfoBuildNumber), V_BSTR(&Value));
        }
        else if (_wcsicmp(Name, L"OSProductSuite") == 0)
        {
            DPRINT1("OSProductSuite 0x%x\n", V_UINT(&Value));
            VersionInfoOsProductSuite = V_UINT(&Value);
        }
        else if (_wcsicmp(Name, L"OSType") == 0)
        {
            DPRINT1("OSType %u\n", V_UINT(&Value));
            VersionInfoOsType = V_UINT(&Value);
        }
        else if (_wcsicmp(Name, L"ServicePackMajorVersion") == 0)
        {
            DPRINT1("ServicePackMajorVersion %S\n", V_BSTR(&Value));
            StringCbCopyW(VersionInfoServicePackMajorVersion, sizeof(VersionInfoServicePackMajorVersion), V_BSTR(&Value));
        }
        else if (_wcsicmp(Name, L"ServicePackMinorVersion") == 0)
        {
            DPRINT1("ServicePackMinorVersion %S\n", V_BSTR(&Value));
            StringCbCopyW(VersionInfoServicePackMinorVersion, sizeof(VersionInfoServicePackMinorVersion), V_BSTR(&Value));
        }
        else if (_wcsicmp(Name, L"Version") == 0)
        {
            DPRINT1("Version %S\n", V_BSTR(&Value));
            VersionInfoVersion = V_UINT(&Value);
        }

        SysFreeString(Name);
        VariantClear(&Value);
    }

    hr = IWbemClassObject_EndEnumeration(Object);
    if (hr != S_OK)
    {
        DPRINT1("got %08x\n", hr);
        goto done;
    }

done:
    if (Object)
        IWbemClassObject_Release(Object);

    if (Result)
        IEnumWbemClassObject_Release(Result);

    if (Query)
        SysFreeString(Query);

    if (Wql)
        SysFreeString(Wql);

    return hr;
}


static
HRESULT
QueryProcessorInfo(
    _In_ IWbemServices *Services)
{
    IEnumWbemClassObject *Result = NULL;
    IWbemClassObject *Object = NULL;
    BSTR Wql = NULL;
    BSTR Query = NULL;
    BSTR Name;
    CIMTYPE Type;
    ULONG Count;
    VARIANT Value;
    HRESULT hr;

    Wql = SysAllocString(L"WQL");
    if (Wql == NULL)
        return E_OUTOFMEMORY;
    
    Query = SysAllocString(L"SELECT * FROM Win32_Processor");
    if (Query == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = IWbemServices_ExecQuery(Services, Wql, Query, 0, NULL, &Result);
    if (hr !=  S_OK)
    {
        DPRINT1("IWbemServices_ExecQuery failed %08x\n", hr);
        goto done;
    }

    hr = IEnumWbemClassObject_Next(Result, 10000, 1, &Object, &Count);
    if (hr != S_OK)
    {
        DPRINT1("IEnumWbemClassObject_Next failed %08x\n", hr);
        goto done;
    }

    hr = IWbemClassObject_BeginEnumeration(Object, 0);
    if (hr != S_OK)
    {
        DPRINT1("IWbemClassObject_BeginEnumeration returned %08x\n", hr);
        goto done;
    }

    while (IWbemClassObject_Next(Object, 0, &Name, &Value, &Type, NULL) == S_OK)
    {
        DPRINT1("Name: %S\n", Name);
        if (_wcsicmp(Name, L"Architecture") == 0)
        {
            DPRINT1("Architecture %u\n", V_UINT(&Value));
        }

        SysFreeString(Name);
        VariantClear(&Value);
    }

    hr = IWbemClassObject_EndEnumeration(Object);
    if (hr != S_OK)
    {
        DPRINT1("got %08x\n", hr);
        goto done;
    }

done:
    if (Object)
        IWbemClassObject_Release(Object);

    if (Result)
        IEnumWbemClassObject_Release(Result);

    if (Query)
        SysFreeString(Query);

    if (Wql)
        SysFreeString(Wql);

    return hr;
}

HRESULT
GetWmiVersionInfo(VOID)
{
    BSTR Path = NULL;
    IWbemLocator *Locator = NULL;
    IWbemServices *Services = NULL;
    HRESULT hr;

    CoInitialize(NULL);

    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                         RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWbemLocator, (void **)&Locator);
    if (hr != S_OK)
    {
        DPRINT1("Can't create instance of WbemLocator\n");
        return hr;
    }

    Path = SysAllocString(L"ROOT\\CIMV2");
    if (Path == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = IWbemLocator_ConnectServer(Locator, Path, NULL, NULL, NULL, 0, NULL, NULL, &Services);
    if (hr != S_OK)
    {
        DPRINT1("Failed to get IWbemServices interface %08x\n", hr);
        goto done;
    }

    hr = CoSetProxyBlanket((IUnknown *)Services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (hr != S_OK)
    {
        DPRINT1("Failed to set proxy blanket %08x\n", hr);
        goto done;
    }

    hr = QueryOperatingSystemInfo(Services);
    hr = QueryProcessorInfo(Services);

done:
    if (Path)
        SysFreeString(Path);

    if (Services)
        IWbemServices_Release(Services);

    if (Locator)
        IWbemLocator_Release(Locator);

    CoUninitialize();

    return hr;
}
