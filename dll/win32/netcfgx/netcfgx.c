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

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

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


/* Append a REG_SZ to an existing REG_MULTI_SZ string in the registry.
 * If the value doesn't exist, create it.
 * Returns ERROR_SUCCESS if success. Othewise, returns an error code
 */
static
LONG
AppendStringToMultiSZ(
    IN HKEY hKey,
    IN PCWSTR ValueName,
    IN PCWSTR ValueToAppend)
{
    PWSTR Buffer = NULL;
    DWORD dwRegType;
    DWORD dwRequired, dwLength;
    DWORD dwTmp;
    LONG rc;

    rc = RegQueryValueExW(hKey,
                          ValueName,
                          NULL,
                          &dwRegType,
                          NULL,
                          &dwRequired);
    if (rc != ERROR_FILE_NOT_FOUND)
    {
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        if (dwRegType != REG_MULTI_SZ)
        {
            rc = ERROR_GEN_FAILURE;
            goto cleanup;
        }

        dwTmp = dwLength = dwRequired + wcslen(ValueToAppend) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
        Buffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
        if (!Buffer)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        rc = RegQueryValueExW(hKey,
                              ValueName,
                              NULL,
                              NULL,
                              (BYTE*)Buffer,
                              &dwTmp);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
    }
    else
    {
        dwRequired = sizeof(WCHAR);
        dwLength = wcslen(ValueToAppend) * sizeof(WCHAR) + 2 * sizeof(UNICODE_NULL);
        Buffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
        if (!Buffer)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    /* Append the value */
    wcscpy(&Buffer[dwRequired / sizeof(WCHAR) - 1], ValueToAppend);
    /* Terminate the REG_MULTI_SZ string */
    Buffer[dwLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

    rc = RegSetValueExW(hKey,
                        ValueName,
                        0,
                        REG_MULTI_SZ,
                        (const BYTE*)Buffer,
                        dwLength);

cleanup:
    HeapFree(GetProcessHeap(), 0, Buffer);
    return rc;
}


static
DWORD
InstallNetDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    LPCWSTR UuidString,
    DWORD Characteristics,
    LPCWSTR BusType)
{
    LPWSTR InstanceId = NULL;
    LPWSTR DeviceName = NULL;
    LPWSTR ExportName = NULL;
    LONG rc;
    HKEY hKey = NULL;
    HKEY hNetworkKey = NULL;
    HKEY hLinkageKey = NULL;
    HKEY hConnectionKey = NULL;
    DWORD dwShowIcon, dwLength, dwValue;
    WCHAR szBuffer[300];

    /* Get Instance ID */
    if (SetupDiGetDeviceInstanceIdW(DeviceInfoSet, DeviceInfoData, NULL, 0, &dwLength))
    {
        DPRINT("SetupDiGetDeviceInstanceIdW() returned TRUE. FALSE expected\n");
        rc = ERROR_GEN_FAILURE;
        goto cleanup;
    }

    InstanceId = HeapAlloc(GetProcessHeap(), 0, dwLength * sizeof(WCHAR));
    if (!InstanceId)
    {
        DPRINT("HeapAlloc() failed\n");
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (!SetupDiGetDeviceInstanceIdW(DeviceInfoSet, DeviceInfoData, InstanceId, dwLength, NULL))
    {
        rc = GetLastError();
        DPRINT("SetupDiGetDeviceInstanceIdW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Create device name */
    DeviceName = HeapAlloc(GetProcessHeap(), 0, (wcslen(L"\\Device\\") + wcslen(UuidString)) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!DeviceName)
    {
        DPRINT("HeapAlloc() failed\n");
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    wcscpy(DeviceName, L"\\Device\\");
    wcscat(DeviceName, UuidString);

    /* Create export name */
    ExportName = HeapAlloc(GetProcessHeap(), 0, (wcslen(L"\\Device\\Tcpip_") + wcslen(UuidString)) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!ExportName)
    {
        DPRINT("HeapAlloc() failed\n");
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    wcscpy(ExportName, L"\\Device\\Tcpip_");
    wcscat(ExportName, UuidString);

    /* Write Tcpip parameters in new service Key */
    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegCreateKeyExW(hKey, UuidString, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hNetworkKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    RegCloseKey(hKey);
    hKey = NULL;

    rc = RegCreateKeyExW(hNetworkKey, L"Parameters\\Tcpip", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    RegCloseKey(hNetworkKey);
    hNetworkKey = NULL;

    rc = RegSetValueExW(hKey, L"DefaultGateway", 0, REG_SZ, (const BYTE*)L"0.0.0.0", (wcslen(L"0.0.0.0") + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hKey, L"IPAddress", 0, REG_SZ, (const BYTE*)L"0.0.0.0", (wcslen(L"0.0.0.0") + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hKey, L"SubnetMask", 0, REG_SZ, (const BYTE*)L"0.0.0.0", (wcslen(L"0.0.0.0") + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    dwValue = 1;
    rc = RegSetValueExW(hKey, L"EnableDHCP", 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    RegCloseKey(hKey);
    hKey = NULL;

    /* Write 'Linkage' key in hardware key */
#if _WIN32_WINNT >= 0x502
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
#else
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS);
#endif
    if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
        hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
    {
        hKey = NULL;
        rc = GetLastError();
        DPRINT1("SetupDiCreateDevRegKeyW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hKey, L"NetCfgInstanceId", 0, REG_SZ, (const BYTE*)UuidString, (wcslen(UuidString) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hKey, L"Characteristics", 0, REG_DWORD, (const BYTE*)&Characteristics, sizeof(DWORD));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    if (BusType)
    {
        rc = RegSetValueExW(hKey, L"BusType", 0, REG_SZ, (const BYTE*)BusType, (wcslen(BusType) + 1) * sizeof(WCHAR));
        if (rc != ERROR_SUCCESS)
        {
            DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
            goto cleanup;
        }
    }

    rc = RegCreateKeyExW(hKey, L"Linkage", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hLinkageKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hLinkageKey, L"Export", 0, REG_SZ, (const BYTE*)DeviceName, (wcslen(DeviceName) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hLinkageKey, L"RootDevice", 0, REG_SZ, (const BYTE*)UuidString, (wcslen(UuidString) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hLinkageKey, L"UpperBind", 0, REG_SZ, (const BYTE*)L"Tcpip", (wcslen(L"Tcpip") + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    RegCloseKey(hKey);
    hKey = NULL;

    /* Write connection information in network subkey */
    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hNetworkKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegCreateKeyExW(hNetworkKey, UuidString, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegCreateKeyExW(hKey, L"Connection", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hConnectionKey, NULL);
    RegCloseKey(hKey);
    hKey = NULL;
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    if (!LoadStringW(netcfgx_hInstance, IDS_NET_CONNECT, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        wcscpy(szBuffer,L"Network connection");
    }

    rc = RegSetValueExW(hConnectionKey, L"Name", 0, REG_SZ, (const BYTE*)szBuffer, (wcslen(szBuffer) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegSetValueExW(hConnectionKey, L"PnpInstanceId", 0, REG_SZ, (const BYTE*)InstanceId, (wcslen(InstanceId) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    dwShowIcon = 1;
    rc = RegSetValueExW(hConnectionKey, L"ShowIcon", 0, REG_DWORD, (const BYTE*)&dwShowIcon, sizeof(dwShowIcon));
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Write linkage information in Tcpip service */
    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = AppendStringToMultiSZ(hKey, L"Bind", DeviceName);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("AppendStringToMultiSZ() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = AppendStringToMultiSZ(hKey, L"Export", ExportName);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("AppendStringToMultiSZ() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = AppendStringToMultiSZ(hKey, L"Route", UuidString);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("AppendStringToMultiSZ() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = ERROR_SUCCESS;

cleanup:
    HeapFree(GetProcessHeap(), 0, InstanceId);
    HeapFree(GetProcessHeap(), 0, DeviceName);
    HeapFree(GetProcessHeap(), 0, ExportName);
    if (hKey != NULL)
        RegCloseKey(hKey);
    if (hNetworkKey != NULL)
        RegCloseKey(hNetworkKey);
    if (hLinkageKey != NULL)
        RegCloseKey(hLinkageKey);
    if (hConnectionKey != NULL)
        RegCloseKey(hConnectionKey);
    return rc;
}

static
DWORD
InstallNetClient(VOID)
{
    DPRINT1("Installation of network clients is not yet supported\n");
    return ERROR_GEN_FAILURE;
}

static
DWORD
InstallNetService(VOID)
{
    DPRINT1("Installation of network services is not yet supported\n");
    return ERROR_GEN_FAILURE;
}

static
DWORD
InstallNetTransport(VOID)
{
    DPRINT1("Installation of network protocols is not yet supported\n");
    return ERROR_GEN_FAILURE;
}

DWORD
WINAPI
NetClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    SP_DRVINFO_DATA_W DriverInfoData;
    SP_DRVINFO_DETAIL_DATA_W DriverInfoDetail;
    WCHAR SectionName[LINE_LEN];
    HINF hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT InfContext;
    UINT ErrorLine;
    INT CharacteristicsInt;
    DWORD Characteristics;
    LPWSTR BusType = NULL;
    RPC_STATUS RpcStatus;
    UUID Uuid;
    LPWSTR UuidRpcString = NULL;
    LPWSTR UuidString = NULL;
    LONG rc;
    DWORD dwLength;

    if (InstallFunction != DIF_INSTALLDEVICE)
        return ERROR_DI_DO_DEFAULT;

    DPRINT("%lu %p %p\n", InstallFunction, DeviceInfoSet, DeviceInfoData);

    /* Get driver info details */
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA_W);
    if (!SetupDiGetSelectedDriverW(DeviceInfoSet, DeviceInfoData, &DriverInfoData))
    {
        rc = GetLastError();
        DPRINT("SetupDiGetSelectedDriverW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    DriverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA_W);
    if (!SetupDiGetDriverInfoDetailW(DeviceInfoSet, DeviceInfoData, &DriverInfoData, &DriverInfoDetail, sizeof(DriverInfoDetail), NULL)
     && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        rc = GetLastError();
        DPRINT("SetupDiGetDriverInfoDetailW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    hInf = SetupOpenInfFileW(DriverInfoDetail.InfFileName, NULL, INF_STYLE_WIN4, &ErrorLine);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        rc = GetLastError();
        DPRINT("SetupOpenInfFileW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    if (!SetupDiGetActualSectionToInstallW(hInf, DriverInfoDetail.SectionName, SectionName, LINE_LEN, NULL, NULL))
    {
        rc = GetLastError();
        DPRINT("SetupDiGetActualSectionToInstallW() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Get Characteristics and BusType (optional) from .inf file */
    if (!SetupFindFirstLineW(hInf, SectionName, L"Characteristics", &InfContext))
    {
        rc = GetLastError();
        DPRINT("Unable to find key %S in section %S of file %S (error 0x%lx)\n",
            L"Characteristics", SectionName, DriverInfoDetail.InfFileName, rc);
        goto cleanup;
    }

    if (!SetupGetIntField(&InfContext, 1, &CharacteristicsInt))
    {
        rc = GetLastError();
        DPRINT("SetupGetIntField() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    Characteristics = (DWORD)CharacteristicsInt;
    if (IsEqualIID(&DeviceInfoData->ClassGuid, &GUID_DEVCLASS_NET))
    {
        if (SetupFindFirstLineW(hInf, SectionName, L"BusType", &InfContext))
        {
            if (!SetupGetStringFieldW(&InfContext, 1, NULL, 0, &dwLength))
            {
                rc = GetLastError();
                DPRINT("SetupGetStringFieldW() failed with error 0x%lx\n", rc);
                goto cleanup;
            }

            BusType = HeapAlloc(GetProcessHeap(), 0, dwLength * sizeof(WCHAR));
            if (!BusType)
            {
                DPRINT("HeapAlloc() failed\n");
                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            if (!SetupGetStringFieldW(&InfContext, 1, BusType, dwLength, NULL))
            {
                rc = GetLastError();
                DPRINT("SetupGetStringFieldW() failed with error 0x%lx\n", rc);
                goto cleanup;
            }
        }
    }

    /* Create a new UUID */
    RpcStatus = UuidCreate(&Uuid);
    if (RpcStatus != RPC_S_OK && RpcStatus != RPC_S_UUID_LOCAL_ONLY)
    {
        DPRINT("UuidCreate() failed with RPC status 0x%lx\n", RpcStatus);
        rc = ERROR_GEN_FAILURE;
        goto cleanup;
    }

    RpcStatus = UuidToStringW(&Uuid, &UuidRpcString);
    if (RpcStatus != RPC_S_OK)
    {
        DPRINT("UuidToStringW() failed with RPC status 0x%lx\n", RpcStatus);
        rc = ERROR_GEN_FAILURE;
        goto cleanup;
    }

    /* Add curly braces around Uuid */
    UuidString = HeapAlloc(GetProcessHeap(), 0, (2 + wcslen(UuidRpcString)) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!UuidString)
    {
        DPRINT("HeapAlloc() failed\n");
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    wcscpy(UuidString, L"{");
    wcscat(UuidString, UuidRpcString);
    wcscat(UuidString, L"}");

    if (IsEqualIID(&DeviceInfoData->ClassGuid, &GUID_DEVCLASS_NET))
        rc = InstallNetDevice(DeviceInfoSet, DeviceInfoData, UuidString, Characteristics, BusType);
    else if (IsEqualIID(&DeviceInfoData->ClassGuid, &GUID_DEVCLASS_NETCLIENT))
        rc = InstallNetClient();
    else if (IsEqualIID(&DeviceInfoData->ClassGuid, &GUID_DEVCLASS_NETSERVICE))
        rc = InstallNetService();
    else if (IsEqualIID(&DeviceInfoData->ClassGuid, &GUID_DEVCLASS_NETTRANS))
        rc = InstallNetTransport();
    else
    {
        DPRINT("Invalid class guid\n");
        rc = ERROR_GEN_FAILURE;
    }

cleanup:
    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);
    if (UuidRpcString != NULL)
        RpcStringFreeW(&UuidRpcString);
    HeapFree(GetProcessHeap(), 0, BusType);
    HeapFree(GetProcessHeap(), 0, UuidString);

    if (rc == ERROR_SUCCESS)
        rc = ERROR_DI_DO_DEFAULT;
    DPRINT("Returning 0x%lx\n", rc);
    return rc;
}
