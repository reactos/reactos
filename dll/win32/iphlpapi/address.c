/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/iphlpapi/address.c
 * PURPOSE:         iphlpapi implementation - Adapter Address APIs
 * PROGRAMMERS:     Jérôme Gardou (jerome.gardou@reactos.org)
 */

#include "iphlpapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);
#ifdef GetAdaptersAddressesV2
/* Helper for GetAdaptersAddresses:
 * Retrieves the list of network adapters from tcpip.sys */
static
NTSTATUS
GetInterfacesList(
    _In_ HANDLE TcpFile,
    _Out_ TDIEntityID **EntityList,
    _Out_ ULONG* InterfaceCount)
{

    TCP_REQUEST_QUERY_INFORMATION_EX TcpQueryInfo;
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS Status;
    ULONG_PTR BufferSize;

    ZeroMemory(&TcpQueryInfo, sizeof(TcpQueryInfo));
    TcpQueryInfo.ID.toi_class = INFO_CLASS_GENERIC;
    TcpQueryInfo.ID.toi_type = INFO_TYPE_PROVIDER;
    TcpQueryInfo.ID.toi_id = ENTITY_LIST_ID;
    TcpQueryInfo.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    TcpQueryInfo.ID.toi_entity.tei_instance = 0;

    Status = NtDeviceIoControlFile(
        TcpFile,
        NULL,
        NULL,
        NULL,
        &StatusBlock,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &TcpQueryInfo,
        sizeof(TcpQueryInfo),
        NULL,
        0);
    if (Status == STATUS_PENDING)
    {
        /* So we have to wait a bit */
        Status = NtWaitForSingleObject(TcpFile, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = StatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
        return Status;

    BufferSize = StatusBlock.Information;
    *EntityList = HeapAlloc(GetProcessHeap(), 0, BufferSize);
    if (!*EntityList)
        return STATUS_NO_MEMORY;

    /* Do the real call */
    Status = NtDeviceIoControlFile(
        TcpFile,
        NULL,
        NULL,
        NULL,
        &StatusBlock,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &TcpQueryInfo,
        sizeof(TcpQueryInfo),
        *EntityList,
        BufferSize);
    if (Status == STATUS_PENDING)
    {
        /* So we have to wait a bit */
        Status = NtWaitForSingleObject(TcpFile, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = StatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        HeapFree(GetProcessHeap(), 0, *EntityList);
        return Status;
    }

    *InterfaceCount = BufferSize / sizeof(TDIEntityID);
    return Status;
}

static
NTSTATUS
GetSnmpInfo(
    _In_ HANDLE TcpFile,
    _In_ TDIEntityID InterfaceID,
    _Out_ IPSNMPInfo* Info)
{
    TCP_REQUEST_QUERY_INFORMATION_EX TcpQueryInfo;
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS Status;

    ZeroMemory(&TcpQueryInfo, sizeof(TcpQueryInfo));
    TcpQueryInfo.ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpQueryInfo.ID.toi_type = INFO_TYPE_PROVIDER;
    TcpQueryInfo.ID.toi_id = IP_MIB_STATS_ID;
    TcpQueryInfo.ID.toi_entity = InterfaceID;

    Status = NtDeviceIoControlFile(
        TcpFile,
        NULL,
        NULL,
        NULL,
        &StatusBlock,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &TcpQueryInfo,
        sizeof(TcpQueryInfo),
        Info,
        sizeof(*Info));
    if (Status == STATUS_PENDING)
    {
        /* So we have to wait a bit */
        Status = NtWaitForSingleObject(TcpFile, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = StatusBlock.Status;
    }

    return Status;
}

static
NTSTATUS
GetAddrEntries(
    _In_ HANDLE TcpFile,
    _In_ TDIEntityID InterfaceID,
    _Out_ IPAddrEntry* Entries,
    _In_ ULONG NumEntries)
{
    TCP_REQUEST_QUERY_INFORMATION_EX TcpQueryInfo;
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS Status;

    ZeroMemory(&TcpQueryInfo, sizeof(TcpQueryInfo));
    TcpQueryInfo.ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpQueryInfo.ID.toi_type = INFO_TYPE_PROVIDER;
    TcpQueryInfo.ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    TcpQueryInfo.ID.toi_entity = InterfaceID;

    Status = NtDeviceIoControlFile(
        TcpFile,
        NULL,
        NULL,
        NULL,
        &StatusBlock,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &TcpQueryInfo,
        sizeof(TcpQueryInfo),
        Entries,
        NumEntries * sizeof(Entries[0]));
    if (Status == STATUS_PENDING)
    {
        /* So we have to wait a bit */
        Status = NtWaitForSingleObject(TcpFile, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = StatusBlock.Status;
    }

    return Status;
}

/*
 * Fills the IFEntry buffer from tcpip.sys.
 * The buffer size MUST be FIELD_OFFSET(IFEntry, if_descr[MAX_ADAPTER_DESCRIPTION_LENGTH + 1]).
 * See MSDN IFEntry struct definition if you don't believe me. ;-)
 */
static
NTSTATUS
GetInterfaceEntry(
    _In_ HANDLE TcpFile,
    _In_ TDIEntityID InterfaceID,
    _Out_ IFEntry* Entry)
{
    TCP_REQUEST_QUERY_INFORMATION_EX TcpQueryInfo;
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS Status;

    ZeroMemory(&TcpQueryInfo, sizeof(TcpQueryInfo));
    TcpQueryInfo.ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpQueryInfo.ID.toi_type = INFO_TYPE_PROVIDER;
    TcpQueryInfo.ID.toi_id = IP_MIB_STATS_ID;
    TcpQueryInfo.ID.toi_entity = InterfaceID;

    Status = NtDeviceIoControlFile(
        TcpFile,
        NULL,
        NULL,
        NULL,
        &StatusBlock,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &TcpQueryInfo,
        sizeof(TcpQueryInfo),
        Entry,
        FIELD_OFFSET(IFEntry, if_descr[MAX_ADAPTER_DESCRIPTION_LENGTH + 1]));
    if (Status == STATUS_PENDING)
    {
        /* So we have to wait a bit */
        Status = NtWaitForSingleObject(TcpFile, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = StatusBlock.Status;
    }

    return Status;
}

/* Helpers to get the list of DNS for an interface */
static
VOID
EnumerateServerNameSize(
    _In_ PWCHAR Interface,
    _In_ PWCHAR NameServer,
    _Inout_ PVOID Data)
{
    ULONG* BufferSize = Data;

    /* This is just sizing here */
    UNREFERENCED_PARAMETER(Interface);
    UNREFERENCED_PARAMETER(NameServer);

    *BufferSize += sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS) + sizeof(SOCKADDR);
}

static
VOID
EnumerateServerName(
    _In_ PWCHAR Interface,
    _In_ PWCHAR NameServer,
    _Inout_ PVOID Data)
{
    PIP_ADAPTER_DNS_SERVER_ADDRESS** Ptr = Data;
    PIP_ADAPTER_DNS_SERVER_ADDRESS ServerAddress = **Ptr;

    UNREFERENCED_PARAMETER(Interface);

    ServerAddress->Length = sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS);
    ServerAddress->Address.lpSockaddr = (PVOID)(ServerAddress + 1);
    ServerAddress->Address.iSockaddrLength = sizeof(SOCKADDR);


    /* Get the address from the server name string */
    //FIXME: Only ipv4 for now...
    if (WSAStringToAddressW(
        NameServer,
        AF_INET,
        NULL,
        ServerAddress->Address.lpSockaddr,
        &ServerAddress->Address.iSockaddrLength))
    {
        /* Pass along, name conversion failed */
        ERR("%S is not a valid IP address\n", NameServer);
        return;
    }

    /* Go to next item */
    ServerAddress->Next = (PVOID)(ServerAddress->Address.lpSockaddr + 1);
    *Ptr = &ServerAddress->Next;
}

static
VOID
QueryFlags(
    _In_ PUCHAR Interface,
    _In_ DWORD InterfaceLength,
    _Out_ LPDWORD Flags)
{
    HKEY InterfaceKey;
    CHAR KeyName[256];
    DWORD Type, Size, Data;

    *Flags = 0;

    snprintf(KeyName, 256,
             "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%*s",
             InterfaceLength, Interface);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_READ, &InterfaceKey) == ERROR_SUCCESS)
    {
        Size = sizeof(DWORD);
        if (RegQueryValueExA(InterfaceKey, "EnableDHCP", NULL, &Type, (LPBYTE)&Data, &Size) == ERROR_SUCCESS &&
            Type == REG_DWORD && Data == 1)
        {
            *Flags |= IP_ADAPTER_DHCP_ENABLED;
        }

        Size = sizeof(DWORD);
        if (RegQueryValueExA(InterfaceKey, "RegisterAdapterName", NULL, &Type, (LPBYTE)&Data, &Size) == ERROR_SUCCESS &&
            Type == REG_DWORD && Data == 1)
        {
            *Flags |= IP_ADAPTER_REGISTER_ADAPTER_SUFFIX;
        }

        Size = 0;
        if (RegQueryValueExA(InterfaceKey, "NameServer", NULL, &Type, (LPBYTE)&Data, &Size) != ERROR_SUCCESS)
        {
            *Flags |= IP_ADAPTER_DDNS_ENABLED;
        }

        RegCloseKey(InterfaceKey);
    }

    // FIXME: handle 0x8 -> 0x20
}

DWORD
WINAPI
DECLSPEC_HOTPATCH
GetAdaptersAddresses(
    _In_ ULONG Family,
    _In_ ULONG Flags,
    _In_ PVOID Reserved,
    _Inout_ PIP_ADAPTER_ADDRESSES pAdapterAddresses,
    _Inout_ PULONG pOutBufLen)
{
    NTSTATUS Status;
    HANDLE TcpFile;
    TDIEntityID* InterfacesList;
    ULONG InterfacesCount;
    ULONG AdaptersCount = 0;
    ULONG i;
    ULONG TotalSize = 0, RemainingSize;
    BYTE* Ptr = (BYTE*)pAdapterAddresses;
    DWORD MIN_SIZE = 15 * 1024;
    PIP_ADAPTER_ADDRESSES PreviousAA = NULL;

    FIXME("GetAdaptersAddresses - Semi Stub: Family %u, Flags 0x%08x, Reserved %p, pAdapterAddress %p, pOutBufLen %p.\n",
        Family, Flags, Reserved, pAdapterAddresses, pOutBufLen);

    if (!pOutBufLen)
        return ERROR_INVALID_PARAMETER;

    // FIXME: the exact needed size should be computed first, BEFORE doing any write to the output buffer.
    // As suggested by MSDN, require a 15 KB buffer, which allows to React properly to length checks.
    if(!Ptr || *pOutBufLen < MIN_SIZE)
    {
        *pOutBufLen = MIN_SIZE;
        return ERROR_BUFFER_OVERFLOW;
    }

    switch(Family)
    {
        case AF_INET:
            break;
        case AF_INET6:
            /* One day maybe... */
            FIXME("IPv6 is not supported in ReactOS!\n");
            /* We got nothing to say in this case */
            return ERROR_NO_DATA;
            break;
        case AF_UNSPEC:
            WARN("IPv6 addresses ignored, IPv4 only\n");
            Family = AF_INET;
            break;
        default:
            ERR("Invalid family 0x%x\n", Family);
            return ERROR_INVALID_PARAMETER;
            break;
    }

    RemainingSize = *pOutBufLen;
    if (Ptr)
        ZeroMemory(Ptr, RemainingSize);

    /* open the tcpip driver */
    Status = openTcpFile(&TcpFile, FILE_READ_DATA);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open handle to tcpip.sys. Status %08x\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    /* Get the interfaces list */
    Status = GetInterfacesList(TcpFile, &InterfacesList, &InterfacesCount);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not get adapters list. Status %08x\n", Status);
        NtClose(TcpFile);
        return RtlNtStatusToDosError(Status);
    }

    /* Let's see if we got any adapter. */
    for (i = 0; i < InterfacesCount; i++)
    {
        PIP_ADAPTER_ADDRESSES CurrentAA = (PIP_ADAPTER_ADDRESSES)Ptr;
        ULONG CurrentAASize = 0;
        ULONG FriendlySize = 0;

        if (InterfacesList[i].tei_entity == IF_ENTITY)
        {
            BYTE EntryBuffer[FIELD_OFFSET(IFEntry, if_descr) +
                             RTL_FIELD_SIZE(IFEntry, if_descr[0]) * (MAX_ADAPTER_DESCRIPTION_LENGTH + 1)];
            IFEntry* Entry = (IFEntry*)EntryBuffer;

            /* Remember we got one */
            AdaptersCount++;

            /* Set the pointer to this instance in the previous one*/
            if(PreviousAA)
                PreviousAA->Next = CurrentAA;

            /* Of course we need some space for the base structure. */
            CurrentAASize = sizeof(IP_ADAPTER_ADDRESSES);

            /* Get the entry */
            Status = GetInterfaceEntry(TcpFile, InterfacesList[i], Entry);
            if (!NT_SUCCESS(Status))
                goto Error;

            TRACE("Got entity %*s, index %u.\n",
                Entry->if_descrlen, &Entry->if_descr[0], Entry->if_index);

            /* Add the adapter name */
            CurrentAASize += Entry->if_descrlen + sizeof(CHAR);

            /* Add the DNS suffix */
            CurrentAASize += sizeof(WCHAR);

            /* Add the description. */
            CurrentAASize += sizeof(WCHAR);

            if (!(Flags & GAA_FLAG_SKIP_FRIENDLY_NAME))
            {
                /* Get the friendly name */
                HKEY ConnectionKey;
                CHAR KeyName[256];

                snprintf(KeyName, 256,
                    "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%*s\\Connection",
                    Entry->if_descrlen, &Entry->if_descr[0]);

                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_READ, &ConnectionKey) == ERROR_SUCCESS)
                {
                    DWORD ValueType;
                    DWORD ValueSize = 0;

                    if (RegQueryValueExW(ConnectionKey, L"Name", NULL, &ValueType, NULL, &ValueSize) == ERROR_SUCCESS &&
                        ValueType == REG_SZ)
                    {
                        /* We remove the null char, it will be re-added after */
                        FriendlySize = ValueSize - sizeof(WCHAR);
                        CurrentAASize += FriendlySize;
                    }

                    RegCloseKey(ConnectionKey);
                }

                /* We always make sure to have enough room for empty string */
                CurrentAASize += sizeof(WCHAR);
            }

            if (!(Flags & GAA_FLAG_SKIP_DNS_SERVER))
            {
                /* Enumerate the name servers */
                HKEY InterfaceKey;
                CHAR KeyName[256];

                snprintf(KeyName, 256,
                    "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%*s",
                    Entry->if_descrlen, &Entry->if_descr[0]);

                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_READ, &InterfaceKey) == ERROR_SUCCESS)
                {
                    EnumNameServers(InterfaceKey, NULL, &CurrentAASize, EnumerateServerNameSize);
                    RegCloseKey(InterfaceKey);
                }
            }

            /* This is part of what we will need */
            TotalSize += CurrentAASize;

            /* Fill in the data */
            if ((CurrentAA) && (RemainingSize >= CurrentAASize))
            {
                CurrentAA->Length = sizeof(IP_ADAPTER_ADDRESSES);
                CurrentAA->IfIndex = Entry->if_index;
                CopyMemory(CurrentAA->PhysicalAddress, Entry->if_physaddr, Entry->if_physaddrlen);
                CurrentAA->PhysicalAddressLength = Entry->if_physaddrlen;
                QueryFlags(&Entry->if_descr[0], Entry->if_descrlen, &CurrentAA->Flags);
                CurrentAA->Mtu = Entry->if_mtu;
                CurrentAA->IfType = Entry->if_type;
                if(Entry->if_operstatus >= IF_OPER_STATUS_CONNECTING)
                    CurrentAA->OperStatus = IfOperStatusUp;
                else
                    CurrentAA->OperStatus = IfOperStatusDown;

                /* Next items */
                Ptr = (BYTE*)(CurrentAA + 1);

                /* Now fill in the name */
                CopyMemory(Ptr, &Entry->if_descr[0], Entry->if_descrlen);
                CurrentAA->AdapterName = (PCHAR)Ptr;
                CurrentAA->AdapterName[Entry->if_descrlen] = '\0';
                /* Next items */
                Ptr = (BYTE*)(CurrentAA->AdapterName + Entry->if_descrlen + 1);

                /* The DNS suffix */
                CurrentAA->DnsSuffix = (PWCHAR)Ptr;
                CurrentAA->DnsSuffix[0] = L'\0';
                /* Next items */
                Ptr = (BYTE*)(CurrentAA->DnsSuffix + 1);

                /* The description */
                CurrentAA->Description = (PWCHAR)Ptr;
                CurrentAA->Description[0] = L'\0';
                /* Next items */
                Ptr = (BYTE*)(CurrentAA->Description + 1);

                /* The friendly name */
                if (!(Flags & GAA_FLAG_SKIP_FRIENDLY_NAME))
                {
                    CurrentAA->FriendlyName = (PWCHAR)Ptr;

                    if (FriendlySize != 0)
                    {
                        /* Get the friendly name */
                        HKEY ConnectionKey;
                        CHAR KeyName[256];

                        snprintf(KeyName, 256,
                            "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%*s\\Connection",
                            Entry->if_descrlen, &Entry->if_descr[0]);

                        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_READ, &ConnectionKey) == ERROR_SUCCESS)
                        {
                            DWORD ValueType;
                            DWORD ValueSize = FriendlySize + sizeof(WCHAR);

                            if (RegQueryValueExW(ConnectionKey, L"Name", NULL, &ValueType, (LPBYTE)CurrentAA->FriendlyName, &ValueSize) == ERROR_SUCCESS &&
                                ValueType == REG_SZ && ValueSize == FriendlySize + sizeof(WCHAR))
                            {
                                /* We're done, next items */
                                Ptr = (BYTE*)(CurrentAA->FriendlyName + (ValueSize / sizeof(WCHAR)));
                            }
                            else
                            {
                                /* Fail */
                                ERR("Friendly name changed after probe!\n");
                                FriendlySize = 0;
                            }

                            RegCloseKey(ConnectionKey);
                        }
                        else
                        {
                            /* Fail */
                            FriendlySize = 0;
                        }
                    }

                    /* In case of failure (or no name) */
                    if (FriendlySize == 0)
                    {
                        CurrentAA->FriendlyName[0] = L'\0';
                        /* Next items */
                        Ptr = (BYTE*)(CurrentAA->FriendlyName + 1);
                    }
                }

                /* The DNS Servers */
                if (!(Flags & GAA_FLAG_SKIP_DNS_SERVER))
                {
                    /* Enumerate the name servers */
                    HKEY InterfaceKey;
                    CHAR KeyName[256];

                    snprintf(KeyName, 256,
                        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%*s",
                        Entry->if_descrlen, &Entry->if_descr[0]);

                    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_READ, &InterfaceKey) != ERROR_SUCCESS)
                    {
                        TRACE("Failed opening interface key for interface %*s\n", Entry->if_descrlen, &Entry->if_descr[0]);
                    }
                    else
                    {
                        PIP_ADAPTER_DNS_SERVER_ADDRESS* ServerAddressPtr;

                        CurrentAA->FirstDnsServerAddress = (PIP_ADAPTER_DNS_SERVER_ADDRESS)Ptr;
                        ServerAddressPtr = &CurrentAA->FirstDnsServerAddress;

                        EnumNameServers(InterfaceKey, NULL, &ServerAddressPtr, EnumerateServerName);
                        RegCloseKey(InterfaceKey);

                        /* Set the last entry in the list as having NULL next member */
                        Ptr = (BYTE*)*ServerAddressPtr;
                        *ServerAddressPtr = NULL;
                    }
                }

                /* We're done for this interface */
                PreviousAA = CurrentAA;
                RemainingSize -= CurrentAASize;
            }
        }
    }

    if (AdaptersCount == 0)
    {
        /* Uh? Not even localhost ?! */
        ERR("No Adapters found!\n");
        *pOutBufLen = 0;
        return ERROR_NO_DATA;
    }

    /* See if we have anything to add */
    // FIXME: Anycast and multicast
    if ((Flags & (GAA_FLAG_SKIP_UNICAST | GAA_FLAG_INCLUDE_PREFIX)) == GAA_FLAG_SKIP_UNICAST)
        goto Success;

    /* Now fill in the addresses */
    for (i = 0; i < InterfacesCount; i++)
    {
        /* Look for network layers */
        if ((InterfacesList[i].tei_entity == CL_NL_ENTITY)
                || (InterfacesList[i].tei_entity == CO_NL_ENTITY))
        {
            IPSNMPInfo SnmpInfo;
            PIP_ADAPTER_ADDRESSES CurrentAA = NULL;
            IPAddrEntry* AddrEntries;
            ULONG j;

            /* Get its SNMP info */
            Status = GetSnmpInfo(TcpFile, InterfacesList[i], &SnmpInfo);
            if (!NT_SUCCESS(Status))
                goto Error;

            if (SnmpInfo.ipsi_numaddr == 0)
                continue;

            /* Allocate the address entry array and get them */
            AddrEntries = HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                SnmpInfo.ipsi_numaddr * sizeof(AddrEntries[0]));
            if (!AddrEntries)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }
            Status = GetAddrEntries(TcpFile, InterfacesList[i], AddrEntries, SnmpInfo.ipsi_numaddr);
            if (!NT_SUCCESS(Status))
            {
                HeapFree(GetProcessHeap(), 0, AddrEntries);
                goto Error;
            }

            for (j = 0; j < SnmpInfo.ipsi_numaddr; j++)
            {
                /* Find the adapters struct for this address. */
                if (pAdapterAddresses)
                {
                    CurrentAA = pAdapterAddresses;
                    while (CurrentAA)
                    {
                        if (CurrentAA->IfIndex == AddrEntries[j].iae_index)
                            break;

                        CurrentAA = CurrentAA->Next;
                    }

                    if (!CurrentAA)
                    {
                        ERR("Got address for interface %u but no adapter was found for it.\n", AddrEntries[j].iae_index);
                        /* Go to the next address */
                        continue;
                    }
                }

                TRACE("address is 0x%08x, mask is 0x%08x\n", AddrEntries[j].iae_addr, AddrEntries[j].iae_mask);

                //FIXME: For now reactos only supports unicast addresses
                if (!(Flags & GAA_FLAG_SKIP_UNICAST))
                {
                    ULONG Size = sizeof(IP_ADAPTER_UNICAST_ADDRESS) + sizeof(SOCKADDR);

                    if (Ptr && (RemainingSize >= Size))
                    {
                        PIP_ADAPTER_UNICAST_ADDRESS UnicastAddress = (PIP_ADAPTER_UNICAST_ADDRESS)Ptr;

                        /* Fill in the structure */
                        UnicastAddress->Length = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
                        UnicastAddress->Next = CurrentAA->FirstUnicastAddress;

                        // FIXME: Put meaningful value here
                        UnicastAddress->Flags = 0;
                        UnicastAddress->PrefixOrigin = IpPrefixOriginOther;
                        UnicastAddress->SuffixOrigin = IpSuffixOriginOther;
                        UnicastAddress->DadState = IpDadStatePreferred;
                        UnicastAddress->ValidLifetime = 0xFFFFFFFF;
                        UnicastAddress->PreferredLifetime = 0xFFFFFFFF;

                        /* Set the address */
                        //FIXME: ipv4 only (again...)
                        UnicastAddress->Address.lpSockaddr = (LPSOCKADDR)(UnicastAddress + 1);
                        UnicastAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
                        UnicastAddress->Address.lpSockaddr->sa_family = AF_INET;
                        ((LPSOCKADDR_IN)UnicastAddress->Address.lpSockaddr)->sin_port = 0;
                        memcpy(&((LPSOCKADDR_IN)UnicastAddress->Address.lpSockaddr)->sin_addr, &AddrEntries[j].iae_addr, sizeof(AddrEntries[j].iae_addr));

                        CurrentAA->FirstUnicastAddress = UnicastAddress;
                        Ptr += Size;
                        RemainingSize -= Size;
                    }

                    TotalSize += Size;
                }

                if (Flags & GAA_FLAG_INCLUDE_PREFIX)
                {
                    ULONG Size = sizeof(IP_ADAPTER_PREFIX) + sizeof(SOCKADDR);

                    if (Ptr && (RemainingSize >= Size))
                    {
                        PIP_ADAPTER_PREFIX Prefix = (PIP_ADAPTER_PREFIX)Ptr;

                        /* Fill in the structure */
                        Prefix->Length = sizeof(IP_ADAPTER_PREFIX);
                        Prefix->Next = CurrentAA->FirstPrefix;

                        /* Set the address */
                        //FIXME: ipv4 only (again...)
                        Prefix->Address.lpSockaddr = (LPSOCKADDR)(Prefix + 1);
                        Prefix->Address.iSockaddrLength = sizeof(AddrEntries[j].iae_mask);
                        Prefix->Address.lpSockaddr->sa_family = AF_INET;
                        memcpy(Prefix->Address.lpSockaddr->sa_data, &AddrEntries[j].iae_mask, sizeof(AddrEntries[j].iae_mask));

                        /* Compute the prefix size */
                        _BitScanReverse(&Prefix->PrefixLength, AddrEntries[j].iae_mask);

                        CurrentAA->FirstPrefix = Prefix;
                        Ptr += Size;
                        RemainingSize -= Size;
                    }

                    TotalSize += Size;
                }
            }

            HeapFree(GetProcessHeap(), 0, AddrEntries);
        }
    }

Success:
    /* We're done */
    HeapFree(GetProcessHeap(), 0, InterfacesList);
    NtClose(TcpFile);
    *pOutBufLen = TotalSize;
    TRACE("TotalSize: %x\n", *pOutBufLen);
    return ERROR_SUCCESS;

Error:
    ERR("Failed! Status 0x%08x\n", Status);
    *pOutBufLen = 0;
    HeapFree(GetProcessHeap(), 0, InterfacesList);
    NtClose(TcpFile);
    return RtlNtStatusToDosError(Status);
}
#endif
