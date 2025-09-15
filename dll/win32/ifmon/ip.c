/*
 * PROJECT:    ReactOS IF Monitor DLL
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    IP context functions
 * COPYRIGHT:  Copyright 2025 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#include <guiddef.h>

#define NDEBUG
#include <debug.h>

#include "guid.h"
#include "resource.h"

#define DISPLAY_ADRESSES   0x1
#define DISPLAY_DNS        0x2

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
    BOOL *pbDone);

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
    BOOL *pbDone);

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
    BOOL *pbDone);


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
    {L"show", IDS_HLP_IP_SHOW, sizeof(IpShowCommands) / sizeof(CMD_ENTRY), 0, IpShowCommands, NULL},
};


static
BOOL
FormatIPv4Address(
    _Out_ PWCHAR pBuffer,
    _In_ PSOCKET_ADDRESS SocketAddress)
{
    if (SocketAddress->lpSockaddr->sa_family == AF_INET)
    {
        struct sockaddr_in *si = (struct sockaddr_in *)(SocketAddress->lpSockaddr);
        RtlIpv4AddressToStringW(&(si->sin_addr), pBuffer);
        return TRUE;
    }

    return FALSE;
}


static
DWORD
IpShowAdapters(
    _In_ DWORD DisplayFlags,
    _In_ PWSTR InterfaceName)
{
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL, Ptr;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddress;
    PIP_ADAPTER_PREFIX pPrefix;
    PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsServer;
    WCHAR IpBuffer[17];
    BOOL First;
    ULONG adaptOutBufLen = 15000;
    DWORD Error = ERROR_SUCCESS;
    ULONG Flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_PREFIX;

    /* set required buffer size */
    pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(adaptOutBufLen);
    if (pAdapterAddresses == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    Error = GetAdaptersAddresses(AF_INET, Flags, NULL, pAdapterAddresses, &adaptOutBufLen);
    if (Error == ERROR_BUFFER_OVERFLOW)
    {
       free(pAdapterAddresses);
       pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(adaptOutBufLen);
       if (pAdapterAddresses == NULL)
       {
           Error = ERROR_NOT_ENOUGH_MEMORY;
           goto done;
       }
    }

    Error = GetAdaptersAddresses(AF_INET, Flags, NULL, pAdapterAddresses, &adaptOutBufLen);
    if (Error != ERROR_SUCCESS)
        goto done;

    Ptr = pAdapterAddresses;
    while (Ptr)
    {
        if (InterfaceName == NULL || MatchToken(InterfaceName, Ptr->FriendlyName))
        {
            PrintMessageFromModule(hDllInstance, IDS_IP_HEADER, Ptr->FriendlyName);

            if (DisplayFlags & DISPLAY_ADRESSES)
            {
                PrintMessageFromModule(hDllInstance, (Ptr->Flags & IP_ADAPTER_DHCP_ENABLED) ? IDS_DHCP_ON : IDS_DHCP_OFF);

                if (Ptr->FirstUnicastAddress == NULL)
                {
                    PrintMessageFromModule(hDllInstance, IDS_NOIPADDRESS);
                }
                else
                {
                    First = TRUE;
                    pUnicastAddress = Ptr->FirstUnicastAddress;
                    while (pUnicastAddress)
                    {
                        if (FormatIPv4Address(IpBuffer, &pUnicastAddress->Address))
                        {
                            if (First)
                            {
                                PrintMessageFromModule(hDllInstance, (pUnicastAddress->Next)? IDS_IPADDRESSES : IDS_IPADDRESS, IpBuffer);
                            }
                            else
                            {
                                PrintMessageFromModule(hDllInstance, IDS_EMPTYLINE, IpBuffer);
                            }
                            First = FALSE;
                        }

                        pUnicastAddress = pUnicastAddress->Next;
                    }
                }

                if (Ptr->FirstPrefix == NULL)
                {
                    PrintMessage(L"    SubnetMask:                           %s\n", L"None");
                }
                else
                {
                    First = TRUE;
                    pPrefix = Ptr->FirstPrefix;
                    while (pPrefix)
                    {
                        if (FormatIPv4Address(IpBuffer, &pPrefix->Address))
                        {
                            if (First)
                            {
                                if (pPrefix->Next)
                                    PrintMessage(L"    SubnetMasks:                          %s/%lu\n", IpBuffer, pPrefix->PrefixLength);
                                else
                                    PrintMessage(L"    IP Address:                           %s/%lu\n", IpBuffer, pPrefix->PrefixLength);
                            }
                            else
                            {
                                PrintMessage(L"                                          %s/%lu\n", IpBuffer, pPrefix->PrefixLength);
                            }
                            First = FALSE;
                        }

                        pPrefix = pPrefix->Next;
                    }
                }

//                PrintMessage(L"    Default Gateway:                      %s\n", L"---");
//                PrintMessage(L"    Gateway Metric:                       %s\n", L"---");
//                PrintMessage(L"    Interface Metric:                     %s\n", L"---");
            }

            if (DisplayFlags & DISPLAY_DNS)
            {
                if (Ptr->FirstDnsServerAddress == NULL)
                {
                    if (Ptr->Flags & IP_ADAPTER_DHCP_ENABLED)
                        PrintMessage(L"    DNS servers configured through DHCP:  %s\n", L"None");
                    else
                        PrintMessage(L"    Statically configured DNS Servers:    %s\n", L"None");
                }
                else
                {
                    First = TRUE;
                    pDnsServer = Ptr->FirstDnsServerAddress;
                    while (pDnsServer)
                    {
                        if (FormatIPv4Address(IpBuffer, &pDnsServer->Address))
                        {
                            if (First == TRUE)
                            {
                                if (Ptr->Flags & IP_ADAPTER_DHCP_ENABLED)
                                    PrintMessage(L"    DNS servers configured through DHCP:  %s\n", IpBuffer);
                                else
                                    PrintMessage(L"    Statically configured DNS Servers:    %s\n", IpBuffer);
                            }
                            else
                            {
                                PrintMessage(L"                                          %s\n", IpBuffer);
                            }

                            First = FALSE;
                        }

                        pDnsServer = pDnsServer->Next;
                    }
                }

//                PrintMessage(L"    Register with which suffix:\n");
            }
        }

        Ptr = Ptr->Next;
    }

    PrintMessage(L"\n");

done:
    if (pAdapterAddresses)
        free(pAdapterAddresses);

    return Error;
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
