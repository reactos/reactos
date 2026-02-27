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


static FN_HANDLE_CMD IpShowAddresses;
static FN_HANDLE_CMD IpShowConfig;
static FN_HANDLE_CMD IpShowDns;


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
BOOL
FormatIPv4NetMask(
    _Out_ PWCHAR pBuffer,
    _In_ ULONG PrefixLength)
{
    ULONG i;
    IN_ADDR NetMask;

    NetMask.S_un.S_addr = 0;
    for (i = 0; i < PrefixLength; i++)
        NetMask.S_un.S_addr = NetMask.S_un.S_addr | (1 << (31 - i));

    RtlIpv4AddressToStringW(&NetMask, pBuffer);
    return TRUE;
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
    WCHAR IpBuffer[17], MaskBuffer[17];
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
                            PrintMessageFromModule(hDllInstance, IDS_IPADDRESS, IpBuffer);
#if 0
                            if (First)
                            {
                                PrintMessageFromModule(hDllInstance, (pUnicastAddress->Next)? IDS_IPADDRESSES : IDS_IPADDRESS, IpBuffer);
                            }
                            else
                            {
                                PrintMessageFromModule(hDllInstance, IDS_EMPTYLINE, IpBuffer);
                            }
                            First = FALSE;
#endif
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
                        FormatIPv4NetMask(MaskBuffer, pPrefix->PrefixLength);
                        PrintMessage(L"    SubnetMask:                           %s\n", MaskBuffer);
#if 0
                        if (FormatIPv4Address(IpBuffer, &pPrefix->Address))
                        {
                            FormatIPv4NetMask(MaskBuffer, pPrefix->PrefixLength);

                            if (First)
                            {
                                if (pPrefix->Next)
                                    PrintMessage(L"    SubnetMasks:                          %s/%lu (Mask: %s)\n", IpBuffer, pPrefix->PrefixLength, MaskBuffer);
                                else
                                    PrintMessage(L"    SubnetMask:                           %s/%lu (Mask: %s)\n", IpBuffer, pPrefix->PrefixLength, MaskBuffer);
                            }
                            else
                            {
                                PrintMessage(L"                                          %s/%lu (Mask: %s)\n", IpBuffer, pPrefix->PrefixLength, MaskBuffer);
                            }
                            First = FALSE;
                        }
#endif
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
IpDumpFn(
    _In_ LPCWSTR pwszRouter,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwArgCount,
    _In_ LPCVOID pvData)
{
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL, Ptr;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddress;
//    PIP_ADAPTER_PREFIX pPrefix;
    PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsServer;
    WCHAR AddressBuffer[17], MaskBuffer[17];
    ULONG adaptOutBufLen = 15000;
    ULONG Flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_PREFIX;
    DWORD Error = ERROR_SUCCESS;

    DPRINT("IpDumpFn(%S %p %lu %p)\n", pwszRouter, ppwcArguments, dwArgCount, pvData);

    PrintMessageFromModule(hDllInstance, IDS_DUMP_HEADERLINE);
    PrintMessage(L"# Interface IP Configuration\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_HEADERLINE);
    PrintMessage(L"pushd interface ip\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

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
        if (Ptr->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
        {
            PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);
            PrintMessageFromModule(hDllInstance, IDS_DUMP_IP_HEADER, Ptr->FriendlyName);
            PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

            if (Ptr->Flags & IP_ADAPTER_DHCP_ENABLED)
            {
                PrintMessage(L"set address name=\"%s\" source=dhcp\n",
                             Ptr->FriendlyName);
            }
            else
            {
                pUnicastAddress = Ptr->FirstUnicastAddress;
                while (pUnicastAddress)
                {
                    FormatIPv4Address(AddressBuffer, &pUnicastAddress->Address);
                    wcscpy(MaskBuffer, L"?");

                    PrintMessage(L"set address name=\"%s\" source=static address=%s mask=%s\n",
                                 Ptr->FriendlyName, AddressBuffer, MaskBuffer);

                    pUnicastAddress = pUnicastAddress->Next;
                }
            }

            if (Ptr->Flags & IP_ADAPTER_DHCP_ENABLED)
            {
                PrintMessage(L"set dns name=\"%s\" source=dhcp\n",
                             Ptr->FriendlyName);
            }
            else
            {
                pDnsServer = Ptr->FirstDnsServerAddress;
                while (pDnsServer)
                {
                    FormatIPv4Address(AddressBuffer, &pDnsServer->Address);

                    PrintMessage(L"set dns name=\"%s\" source=%s address=%s\n",
                                 Ptr->FriendlyName);

                    pDnsServer = pDnsServer->Next;
                }

            }

            PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);
        }

        Ptr = Ptr->Next;
    }

done:
    if (pAdapterAddresses)
        free(pAdapterAddresses);

    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);
    PrintMessage(L"popd\n");
    PrintMessage(L"# End of Interface IP Configuration\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

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
