/*
 * PROJECT:    ReactOS IF Monitor DLL
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    IP context functions
 * COPYRIGHT:  Copyright 2025 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#include <guiddef.h>
#include <devguid.h>

#define NDEBUG
#include <debug.h>

#include "guid.h"
#include "resource.h"

#define DISPLAY_ADRESSES   0x1
#define DISPLAY_DNS        0x2

static FN_HANDLE_CMD IpSetAddress;
static FN_HANDLE_CMD IpShowAddresses;
static FN_HANDLE_CMD IpShowConfig;
static FN_HANDLE_CMD IpShowDns;


static
CMD_ENTRY
IpSetCommands[] = 
{
    {L"address", IpSetAddress, IDS_HLP_IP_SET_ADDRESS, IDS_HLP_IP_SET_ADDRESS_EX, 0}
};


static
CMD_ENTRY
IpShowCommands[] = 
{
    {L"addresses", IpShowAddresses, IDS_HLP_ADDRESSES, IDS_HLP_ADDRESSES_EX, 0},
    {L"config", IpShowConfig, IDS_HLP_CONFIG, IDS_HLP_CONFIG_EX, 0},
    {L"dns", IpShowDns, IDS_HLP_DNS, IDS_HLP_DNS_EX, 0}
};


static
CMD_GROUP_ENTRY
IpGroups[] = 
{
    {L"set", IDS_HLP_IP_SET, sizeof(IpSetCommands) / sizeof(CMD_ENTRY), 0, IpSetCommands, NULL},
    {L"show", IDS_HLP_IP_SHOW, sizeof(IpShowCommands) / sizeof(CMD_ENTRY), 0, IpShowCommands, NULL},
};


static
DWORD
WINAPI
IpSetAddress(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    TAG_TYPE pttTags[] = {{L"name", NS_REQ_ZERO, FALSE},
                          {L"source", NS_REQ_ZERO, FALSE},
                          {L"addr", NS_REQ_ZERO, FALSE},
                          {L"mask", NS_REQ_ZERO, FALSE},
                          {L"gateway", NS_REQ_ZERO, FALSE},
                          {L"gwmetric", NS_REQ_ZERO, FALSE}};
    TOKEN_VALUE ptvSource[] = {{L"static", 1},
                               {L"dhcp", 2}};
    GUID InterfaceGUID;
    PDWORD pdwTagType = NULL;
    DWORD i, dwSource = 0;
    BOOL bHaveName = FALSE, bHaveSource = FALSE, bHaveAddress = FALSE,
         bHaveMask = FALSE, bHaveGateway = FALSE, bHaveMetric = FALSE;
    IN_ADDR Address, Mask, Gateway;
    DWORD dwMetric;
    PCWSTR Term;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT1("IpSetAddress()\n");

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           (dwArgCount - dwCurrentIndex) * sizeof(DWORD));
    if (pdwTagType == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwError = MatchTagsInCmdLine(hDllInstance,
                                 argv,
                                 dwCurrentIndex,
                                 dwArgCount,
                                 pttTags,
                                 ARRAYSIZE(pttTags),
                                 pdwTagType);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("MatchTagsInCmdLine() failed (Error %lu)\n", dwError);
        HeapFree(GetProcessHeap(), 0, pdwTagType);
        return dwError;
    }

    for (i = 0; i < (dwArgCount - dwCurrentIndex); i++)
    {
        DPRINT1("Tag %lu: %lu\n", i, pdwTagType[i]);

        switch (pdwTagType[i])
        {
            case 0: /* name */
                DPRINT1("Tag: name (%S)\n", argv[i + dwCurrentIndex]);
                dwError = NhGetGuidFromInterfaceName(argv[i + dwCurrentIndex],
                                                     &InterfaceGUID,
                                                     0, 0);
                if (dwError != ERROR_SUCCESS)
                {
                    DPRINT1("NhGetGuidFromInterfaceName() failed (Error %lu)\n", dwError);
                    PrintMessageFromModule(hDllInstance,
                                           IDS_ERROR_INVALID_INTERFACE,
                                           argv[i + dwCurrentIndex]);
                    dwError = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
                DPRINT1("Interface: {%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                        InterfaceGUID.Data1, InterfaceGUID.Data2, InterfaceGUID.Data3, InterfaceGUID.Data4[0], InterfaceGUID.Data4[1],
                        InterfaceGUID.Data4[2], InterfaceGUID.Data4[3], InterfaceGUID.Data4[4], InterfaceGUID.Data4[5], InterfaceGUID.Data4[6], InterfaceGUID.Data4[7]);
                bHaveName = TRUE;
                break;

            case 1: /* source */
                DPRINT1("Tag: source (%S)\n", argv[i + dwCurrentIndex]);
                dwError = MatchEnumTag(hDllInstance,
                                       argv[i + dwCurrentIndex],
                                       ARRAYSIZE(ptvSource),
                                       ptvSource,
                                       &dwSource);
                if (dwError != ERROR_SUCCESS)
                {
                    DPRINT1("MatchEnumTag() failed (Error %lu)\n", dwError);
                    PrintMessageFromModule(hDllInstance,
                                           IDS_ERROR_BAD_VALUE,
                                           argv[i + dwCurrentIndex],
                                           pttTags[pdwTagType[i]].pwszTag);
                    dwError = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
                DPRINT1("Source: %lu\n", dwSource);
                bHaveSource = TRUE;
                break;

            case 2: /* addr */
                DPRINT1("Tag: addr (%S)\n", argv[i + dwCurrentIndex]);
                Status = RtlIpv4StringToAddressW(argv[i + dwCurrentIndex],
                                                 TRUE,
                                                 &Term,
                                                 &Address);
                if (Status != 0 /*STATUS_SUCCESS*/)
                {
                    DPRINT1("RtlIpv4StringToAddressW() failed (Status 0x%08lx)\n", Status);
                    PrintMessageFromModule(hDllInstance,
                                           IDS_ERROR_BAD_VALUE,
                                           argv[i + dwCurrentIndex],
                                           pttTags[pdwTagType[i]].pwszTag);
                    dwError = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
                DPRINT1("IP Address: %u.%u.%u.%u\n",
                        Address.S_un.S_un_b.s_b1, Address.S_un.S_un_b.s_b2, Address.S_un.S_un_b.s_b3, Address.S_un.S_un_b.s_b4);
                bHaveAddress = TRUE;
                break;

            case 3: /* mask */
                DPRINT1("Tag: mask (%S)\n", argv[i + dwCurrentIndex]);
                Status = RtlIpv4StringToAddressW(argv[i + dwCurrentIndex],
                                                 TRUE,
                                                 &Term,
                                                 &Mask);
                if (Status != 0 /*STATUS_SUCCESS*/)
                {
                    DPRINT1("RtlIpv4StringToAddressW() failed (Status 0x%08lx)\n", Status);
                    PrintMessageFromModule(hDllInstance,
                                           IDS_ERROR_BAD_VALUE,
                                           argv[i + dwCurrentIndex],
                                           pttTags[pdwTagType[i]].pwszTag);
                    dwError = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
                DPRINT1("Subnet Mask: %u.%u.%u.%u\n",
                        Mask.S_un.S_un_b.s_b1, Mask.S_un.S_un_b.s_b2, Mask.S_un.S_un_b.s_b3, Mask.S_un.S_un_b.s_b4);
                bHaveMask = TRUE;
                break;

            case 4: /* gateway */
                DPRINT1("Tag: gateway (%S)\n", argv[i + dwCurrentIndex]);
                Status = RtlIpv4StringToAddressW(argv[i + dwCurrentIndex],
                                                 TRUE,
                                                 &Term,
                                                 &Gateway);
                if (Status != 0 /*STATUS_SUCCESS*/)
                {
                    DPRINT1("RtlIpv4StringToAddressW() failed (Status 0x%08lx)\n", Status);
                    PrintMessageFromModule(hDllInstance,
                                           IDS_ERROR_BAD_VALUE,
                                           argv[i + dwCurrentIndex],
                                           pttTags[pdwTagType[i]].pwszTag);
                    dwError = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
                DPRINT1("Gateway: %u.%u.%u.%u\n",
                        Gateway.S_un.S_un_b.s_b1, Gateway.S_un.S_un_b.s_b2, Gateway.S_un.S_un_b.s_b3, Gateway.S_un.S_un_b.s_b4);
                bHaveGateway = TRUE;
                break;

            case 5: /* gwmetric */
                DPRINT1("Tag: gwmetric (%S)\n", argv[i + dwCurrentIndex]);
                dwMetric = wcstoul(argv[i + dwCurrentIndex],
                                   (wchar_t**)&Term,
                                   10);
                if (dwMetric > 9999)
                {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                DPRINT1("Metric: %lu\n", dwMetric);
                bHaveMetric = TRUE;
                break;

            default:
                DPRINT1("Unknown tag type %lu\n", pdwTagType[i]);
                break;
        }
    }

    if (pdwTagType)
        HeapFree(GetProcessHeap(), 0, pdwTagType);

    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Check parameters */

    /* The interface name is mandatory */
    if (bHaveName == FALSE)
        return ERROR_INVALID_SYNTAX;

    if (bHaveSource)
    {
        if ((dwSource == 1) &&
            (!bHaveAddress || !bHaveMask))
            return ERROR_INVALID_SYNTAX;

        if ((dwSource == 2) &&
            (bHaveAddress || bHaveMask))
            return ERROR_INVALID_SYNTAX;
    }

    if ((bHaveGateway && !bHaveMetric) ||
        (!bHaveGateway && bHaveMetric))
        return ERROR_INVALID_SYNTAX;


    DPRINT1("IpSetAddress() done (Error %lu)\n", dwError);
    return dwError;
}


static
HRESULT
GetInterfaceProperties(
    GUID *InterfaceGuid,
    PTCPIP_PROPERTIES *ppProperties)
{
    INetCfg *pNetCfg = NULL;
    INetCfgLock *pNetCfgLock = NULL;
    INetCfgClass *pNetCfgClass = NULL;
    INetCfgComponent *pTcpipComponent = NULL;
    INetCfgComponentPrivate *pTcpipComponentPrivate = NULL;
    ITcpipProperties *pTcpipProperties = NULL;

    BOOL fWriteLocked = FALSE, fInitialized = FALSE;
    HRESULT hr;

    DPRINT("GetInterfaceProperties()\n");

    hr = CoInitialize(NULL);
    if (hr != S_OK)
    {
        DPRINT1("CoInitialize failed\n");
        goto exit;
    }

    hr = CoCreateInstance(&CLSID_CNetCfg,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_INetCfg,
                          (PVOID*)&pNetCfg);
    if (hr != S_OK)
    {
        DPRINT1("CoCreateInstance failed\n");
        goto exit;
    }

    /* Acquire the write-lock */
    hr = INetCfg_QueryInterface(pNetCfg,
                                &IID_INetCfgLock,
                                (PVOID*)&pNetCfgLock);
    if (hr != S_OK)
    {
        DPRINT1("QueryInterface failed\n");
        goto exit;
    }

    hr = INetCfgLock_AcquireWriteLock(pNetCfgLock, 5000,
                                      L"NetSh",
                                      NULL);
    if (hr != S_OK)
    {
        DPRINT1("AcquireWriteLock failed\n");
        goto exit;
    }

    fWriteLocked = TRUE;

    /* Initialize the network configuration */
    hr = INetCfg_Initialize(pNetCfg, NULL);
    if (hr != S_OK)
    {
        DPRINT1("Initialize failed\n");
        goto exit;
    }

    fInitialized = TRUE;

    GUID ClassGuid = GUID_DEVCLASS_NETTRANS;
    hr = INetCfg_QueryNetCfgClass(pNetCfg, &ClassGuid, &IID_INetCfgClass, (PVOID*)&pNetCfgClass);
    if (hr != S_OK)
    {
        DPRINT1("INetCfg_QueryNetCfgClass failed!\n");
        goto exit;
    }

    hr = INetCfgClass_FindComponent(pNetCfgClass, L"MS_TCPIP", &pTcpipComponent);
    if (hr != S_OK)
    {
        DPRINT1("INetCfgClass_FindComponent failed\n");
        goto exit;
    }

    hr = INetCfgComponent_QueryInterface(pTcpipComponent, &IID_INetCfgComponentPrivate, (PVOID*)&pTcpipComponentPrivate);
    if (hr != S_OK)
    {
        DPRINT1("INetCfgComponent_QueryInterface failed\n");
        goto exit;
    }

    hr = INetCfgComponentPrivate_Unknown1(pTcpipComponentPrivate, &IID_ITcpipProperties, (PVOID*)&pTcpipProperties);
    if (hr != S_OK)
    {
        DPRINT1("INetCfgComponentPrivate_Unknown1 failed\n");
        goto exit;
    }

    PTCPIP_PROPERTIES pInfo = NULL;
    hr = ITcpipProperties_Unknown1(pTcpipProperties, InterfaceGuid, &pInfo);
    if (hr != S_OK)
    {
        DPRINT1("ITcpipProperties_Unknown1 failed\n");
    }
    else
    {
        DPRINT("pInfo: %p\n", pInfo);
        DPRINT("dwDhcp: %lx\n", pInfo->dwDhcp);
        DPRINT("IpAddress: %p\n", pInfo->pszIpAddress);
        DPRINT("IpAddress: %S\n", pInfo->pszIpAddress);
        DPRINT("SubnetMask: %p\n", pInfo->pszSubnetMask);
        DPRINT("SubnetMask: %S\n", pInfo->pszSubnetMask);
        DPRINT("Parameters: %p\n", pInfo->pszParameters);
        DPRINT("Parameters: %S\n", pInfo->pszParameters);

        *ppProperties = pInfo;
    }

    DPRINT("Done!\n");
exit:
    if (pTcpipProperties)
        ITcpipProperties_Release(pTcpipProperties);

    if (pTcpipComponentPrivate)
        INetCfgComponentPrivate_Release(pTcpipComponentPrivate);

    if (pTcpipComponent != NULL)
        INetCfgComponent_Release(pTcpipComponent);

    if (pNetCfgClass != NULL)
        INetCfgClass_Release(pNetCfgClass);

    if (fInitialized)
        INetCfg_Uninitialize(pNetCfg);

    if (fWriteLocked)
        INetCfgLock_ReleaseWriteLock(pNetCfgLock);

    if (pNetCfgLock != NULL)
        INetCfgLock_Release(pNetCfgLock);

    if (pNetCfg != NULL)
        INetCfg_Release(pNetCfg);

    CoUninitialize();

    DPRINT("GetInterfaceProperties() done!\n");

    return hr;
}


static
PWSTR
ExtractParameterValue(
    PWSTR pszParameters,
    PWSTR pszParameter)
{
    PWSTR pToken, pStart, pEnd, pBuffer;
    INT length;

    pToken = wcsstr(pszParameters, pszParameter);
    if (pToken == NULL)
        return NULL;

    pStart = wcschr(pToken, L'=');
    if (pStart == NULL)
        return NULL;

    pStart++;
    pEnd = wcschr(pStart, L';');
    if (pEnd == NULL)
        length = wcslen(pStart);
    else
        length = pEnd - pStart;

    if (length == 0)
        return NULL;

    pBuffer = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (length + 1) * sizeof(WCHAR));
    if (pBuffer)
    {
        CopyMemory(pBuffer, pStart, length * sizeof(WCHAR));
    }

    return pBuffer;
}


static
DWORD
IpShowAdapters(
    _In_ DWORD DisplayFlags,
    _In_ PWSTR InterfaceName)
{
    IP_INTERFACE_NAME_INFO *pTable = NULL;
    DWORD dwCount = 0, i;
    DWORD dwError;
    WCHAR szFriendlyName[80];
    DWORD dwFriendlyNameSize;
    PTCPIP_PROPERTIES pProperties = NULL;
    PWSTR pBuffer, pStart, pEnd;

    dwError = NhpAllocateAndGetInterfaceInfoFromStack(&pTable,
                                                      &dwCount,
                                                      FALSE,
                                                      GetProcessHeap(),
                                                      0);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("NhpAllocateAndGetInterfaceInfoFromStack() failed (Error %lu)\n", dwError);
        return dwError;
    }

    DPRINT("\nEntries: %lu\n", dwCount);

    for (i = 0; i < dwCount; i++)
    {
        DPRINT("\nEntry %lu\n", i);
        DPRINT("Index: %lu\n", pTable[i].Index);
        DPRINT("MediaType: %lu\n", pTable[i].MediaType);
        DPRINT("ConnectionType: %u\n", pTable[i].ConnectionType);
        DPRINT("AccessType: %u\n", pTable[i].AccessType);
        DPRINT("DeviceGuid: %08lx\n", pTable[i].DeviceGuid.Data1);
        DPRINT("InterfaceGuid: %08lx\n", pTable[i].InterfaceGuid.Data1);

        dwFriendlyNameSize = sizeof(szFriendlyName);
        NhGetInterfaceNameFromGuid(&pTable[i].DeviceGuid,
                                   szFriendlyName,
                                   &dwFriendlyNameSize,
                                   0,
                                   0);

        if ((InterfaceName == NULL) || MatchToken(InterfaceName, szFriendlyName))
        {
            PrintMessageFromModule(hDllInstance, IDS_IP_HEADER, szFriendlyName);

            GetInterfaceProperties(&pTable[i].DeviceGuid, &pProperties);

            if (pProperties)
            {
                DPRINT("Dhcp %lu\n", pProperties->dwDhcp);
                DPRINT("IpAddress %S\n", pProperties->pszIpAddress);
                DPRINT("SubnetMask %S\n", pProperties->pszSubnetMask);
                DPRINT("Parameters %S\n", pProperties->pszParameters);

                if (DisplayFlags & DISPLAY_ADRESSES)
                {
                    PrintMessageFromModule(hDllInstance, (pProperties->dwDhcp) ? IDS_DHCP_ON : IDS_DHCP_OFF);

                    if (pProperties->dwDhcp == 0)
                    {
                        if (*pProperties->pszIpAddress == UNICODE_NULL)
                        {
                            PrintMessageFromModule(hDllInstance, IDS_NOIPADDRESS);
                        }
                        else
                        {
                            PrintMessageFromModule(hDllInstance, IDS_IPADDRESS, pProperties->pszIpAddress);
                        }

                        if (*pProperties->pszSubnetMask == UNICODE_NULL)
                        {
                            PrintMessageFromModule(hDllInstance, IDS_NOSUBNETMASK);
                        }
                        else
                        {
                            PrintMessageFromModule(hDllInstance, IDS_SUBNETMASK, pProperties->pszSubnetMask);
                        }

                        pBuffer = ExtractParameterValue(pProperties->pszParameters, L"DefGw");
                        if (pBuffer)
                        {
                            PrintMessageFromModule(hDllInstance, IDS_DEFAULTGATEWAY, pBuffer);
                            HeapFree(GetProcessHeap(), 0, pBuffer);
                        }

                        pBuffer = ExtractParameterValue(pProperties->pszParameters, L"GwMetric");
                        if (pBuffer)
                        {
                            PrintMessageFromModule(hDllInstance, IDS_GATEWAYMETRIC, pBuffer);
                            HeapFree(GetProcessHeap(), 0, pBuffer);
                        }
                    }

                    pBuffer = ExtractParameterValue(pProperties->pszParameters, L"IfMetric");
                    if (pBuffer)
                    {
                        PrintMessageFromModule(hDllInstance, IDS_INTERFACEMETRIC, pBuffer);
                        HeapFree(GetProcessHeap(), 0, pBuffer);
                    }
                }

                if (DisplayFlags & DISPLAY_DNS)
                {
                    if (pProperties->dwDhcp == 0)
                    {
                        pBuffer = ExtractParameterValue(pProperties->pszParameters, L"DNS");
                        if (pBuffer)
                        {
                            pEnd = wcschr(pBuffer, L',');
                            if (pEnd == NULL)
                            {
                                PrintMessageFromModule(hDllInstance, IDS_STATICNAMESERVER, pBuffer);
                            }
                            else
                            {
                                pStart = pBuffer;
                                *pEnd = UNICODE_NULL;
                                PrintMessageFromModule(hDllInstance, IDS_STATICNAMESERVER, pBuffer);
                                for (;;)
                                {
                                    pStart = pEnd + 1;
                                    pEnd = wcschr(pStart, L',');
                                    if (pEnd == NULL)
                                        break;
                                    *pEnd = UNICODE_NULL;
                                    PrintMessageFromModule(hDllInstance, IDS_EMPTYLINE, pBuffer);
                                }
                            }
                            HeapFree(GetProcessHeap(), 0, pBuffer);
                        }
                    }
                }

                CoTaskMemFree(pProperties);
                pProperties = NULL;
            }
        }
    }

    if (pTable)
        HeapFree(GetProcessHeap(), 0, pTable);

    return ERROR_SUCCESS;
}


static
DWORD
WINAPI
IpShowAddresses(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PWSTR pszInterfaceName = NULL;

    if (dwArgCount - dwCurrentIndex > 1)
        return ERROR_INVALID_PARAMETER;

    if (dwArgCount - dwCurrentIndex == 1)
        pszInterfaceName = argv[dwCurrentIndex];

    return IpShowAdapters(DISPLAY_ADRESSES, pszInterfaceName);
}


static
DWORD
WINAPI
IpShowConfig(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PWSTR pszInterfaceName = NULL;

    if (dwArgCount - dwCurrentIndex > 1)
        return ERROR_INVALID_PARAMETER;

    if (dwArgCount - dwCurrentIndex == 1)
        pszInterfaceName = argv[dwCurrentIndex];

    return IpShowAdapters(DISPLAY_ADRESSES | DISPLAY_DNS, pszInterfaceName);
}


static
DWORD
WINAPI
IpShowDns(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PWSTR pszInterfaceName = NULL;

    if (dwArgCount - dwCurrentIndex > 1)
        return ERROR_INVALID_PARAMETER;

    if (dwArgCount - dwCurrentIndex == 1)
        pszInterfaceName = argv[dwCurrentIndex];

    return IpShowAdapters(DISPLAY_DNS, pszInterfaceName);
}


static
DWORD
WINAPI
IpDumpFn(
    _In_ LPCWSTR pwszRouter,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwArgCount,
    _In_ LPCVOID pvData)
{
    IP_INTERFACE_NAME_INFO *pTable = NULL;
    DWORD dwCount = 0, i;
    DWORD dwError;
    WCHAR szFriendlyName[80];
    DWORD dwFriendlyNameSize;
    PTCPIP_PROPERTIES pProperties = NULL;
    PWSTR pBuffer;

    DPRINT("IpDumpFn(%S %p %lu %p)\n", pwszRouter, ppwcArguments, dwArgCount, pvData);

    dwError = NhpAllocateAndGetInterfaceInfoFromStack(&pTable,
                                                      &dwCount,
                                                      FALSE,
                                                      GetProcessHeap(),
                                                      0);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("NhpAllocateAndGetInterfaceInfoFromStack() failed (Error %lu)\n", dwError);
        return dwError;
    }

    PrintMessageFromModule(hDllInstance, IDS_DUMP_HEADERLINE);
    PrintMessageFromModule(hDllInstance, IDS_DUMP_IP_HEADER);
    PrintMessageFromModule(hDllInstance, IDS_DUMP_HEADERLINE);
    PrintMessage(L"pushd interface ip\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

    DPRINT("\nEntries: %lu\n", dwCount);

    for (i = 0; i < dwCount; i++)
    {
        DPRINT("\nEntry %lu\n", i);
        DPRINT("Index: %lu\n", pTable[i].Index);
        DPRINT("MediaType: %lu\n", pTable[i].MediaType);
        DPRINT("ConnectionType: %u\n", pTable[i].ConnectionType);
        DPRINT("AccessType: %u\n", pTable[i].AccessType);
        DPRINT("DeviceGuid: %08lx\n", pTable[i].DeviceGuid.Data1);
        DPRINT("InterfaceGuid: %08lx\n", pTable[i].InterfaceGuid.Data1);

        dwFriendlyNameSize = sizeof(szFriendlyName);
        NhGetInterfaceNameFromGuid(&pTable[i].DeviceGuid,
                                   szFriendlyName,
                                   &dwFriendlyNameSize,
                                   0,
                                   0);

        PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);
        PrintMessageFromModule(hDllInstance, IDS_DUMP_IP_INTERFACE, szFriendlyName);
        PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

        GetInterfaceProperties(&pTable[i].DeviceGuid, &pProperties);

        if (pProperties)
        {
            DPRINT("Dhcp %lu\n", pProperties->dwDhcp);
            DPRINT("IpAddress %S\n", pProperties->pszIpAddress);
            DPRINT("SubnetMask %S\n", pProperties->pszSubnetMask);
            DPRINT("Parameters %S\n", pProperties->pszParameters);

            if (pProperties->dwDhcp)
            {
                PrintMessage(L"set address name=\"%s\" source=dhcp\n",
                             szFriendlyName);
            }
            else
            {
                PrintMessage(L"set address name=\"%s\" source=static address=%s mask=%s\n",
                             szFriendlyName, pProperties->pszIpAddress, pProperties->pszSubnetMask);
            }

            if (pProperties->dwDhcp)
            {
                PrintMessage(L"set dns name=\"%s\" source=dhcp\n",
                             szFriendlyName);
            }
            else
            {
                pBuffer = ExtractParameterValue(pProperties->pszParameters, L"DNS");
                if (pBuffer)
                {
                    PrintMessage(L"set dns name=\"%s\" source=static address=%s\n",
                                 szFriendlyName, pBuffer);
                    HeapFree(GetProcessHeap(), 0, pBuffer);
                }
            }

            CoTaskMemFree(pProperties);
            pProperties = NULL;
        }
    }

    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);
    PrintMessage(L"popd\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_IP_FOOTER);
    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

    if (pTable)
        HeapFree(GetProcessHeap(), 0, pTable);


    return ERROR_SUCCESS;
}


static
DWORD
WINAPI
IpStart(
    _In_ const GUID *pguidParent,
    _In_ DWORD dwVersion)
{
    NS_CONTEXT_ATTRIBUTES ContextAttributes;

    DPRINT1("IpStart()\n");

    ZeroMemory(&ContextAttributes, sizeof(ContextAttributes));
    ContextAttributes.dwVersion = 1;
    ContextAttributes.pwszContext = L"ip";
    ContextAttributes.guidHelper = GUID_IFMON_IP;

    ContextAttributes.ulNumTopCmds = 0;
    ContextAttributes.pTopCmds = NULL;

    ContextAttributes.ulNumGroups = sizeof(IpGroups) / sizeof(CMD_GROUP_ENTRY);
    ContextAttributes.pCmdGroups = IpGroups;

    ContextAttributes.pfnDumpFn = IpDumpFn;

    RegisterContext(&ContextAttributes);

    return ERROR_SUCCESS;
}


DWORD
WINAPI
RegisterIpHelper(VOID)
{
    NS_HELPER_ATTRIBUTES HelperAttributes;
    GUID guidParent = GUID_IFMON_INTERFACE;

    DPRINT1("RegisterIpHelper()\n");

    ZeroMemory(&HelperAttributes, sizeof(HelperAttributes));
    HelperAttributes.dwVersion  = 1;
    HelperAttributes.guidHelper = GUID_IFMON_IP;
    HelperAttributes.pfnStart = IpStart;
    HelperAttributes.pfnStop = NULL;
    RegisterHelper(&guidParent, &HelperAttributes);

    return ERROR_SUCCESS;
}
