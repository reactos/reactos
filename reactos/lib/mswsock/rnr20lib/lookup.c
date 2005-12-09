/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

extern DWORD MaskOfGuids;
extern GUID NbtProviderId;

/* FUNCTIONS *****************************************************************/

PDNS_BLOB
WINAPI
Rnr_DoHostnameLookup(IN PRNR_CONTEXT RnrContext)
{
    INT ErrorCode;
    LPWSTR LocalName;
    PDNS_BLOB Blob;

    /* Query the Local Hostname */
    LocalName = (LPWSTR)DnsQueryConfigAllocEx(DnsConfigHostName_W, 0, 0);
    if (!LocalName)
    {
        /* Set error code if we got "Success" */
        ErrorCode = GetLastError();
        if (ErrorCode == NO_ERROR) ErrorCode = ERROR_OUTOFMEMORY;
        goto Fail;
    }

    /* Create a Blob */
    Blob = SaBlob_Create(0);
    if (!Blob)
    {
        /* Set error code if we got "Success" */
        ErrorCode = GetLastError();
        if (ErrorCode == NO_ERROR) ErrorCode = ERROR_OUTOFMEMORY;
        goto Fail;
    }

    /* Write the name */
    ErrorCode = SaBlob_WriteNameOrAlias(Blob, LocalName, FALSE);
    if (ErrorCode != NO_ERROR) goto Fail;

    /* Free the name and return the blob */
    DnsApiFree(LocalName);
    return Blob;

Fail:
    /* Some kind of failure... delete the blob first */
    SaBlob_Free(Blob);

    /* Free the name */
    DnsApiFree(LocalName);

    /* Set the error and fail */
    SetLastError(ErrorCode);
    return NULL;
}

PDNS_BLOB
WINAPI
Rnr_GetHostByAddr(IN PRNR_CONTEXT RnrContext)
{
    BOOLEAN Prolog;
    PDNS_BLOB Blob = NULL;
    INT ErrorCode = ERROR_SUCCESS;
    DWORD ControlFlags = RnrContext->dwControlFlags;
    IN6_ADDR Address;
    ULONG AddressSize = sizeof(IN6_ADDR);
    DWORD AddressFamily = AF_UNSPEC;
    WCHAR ReverseAddress[256];

    /* Enter the RNR Prolog */
    Prolog = RNRPROV_SockEnterApi();
    if (!Prolog) return NULL;

    /* Get an Address */
    Dns_StringToAddressW(&Address,
                         &AddressSize,
                         RnrContext->ServiceName,
                         &AddressFamily);

    /* Check the address family */
    if (AddressFamily == AF_INET)
    {
        /* Convert it to the IPv4 Reverse Name */
        Dns_Ip4AddressToReverseName_W(ReverseAddress, *(PIN_ADDR)&Address);
    }
    else if (AddressFamily == AF_INET6)
    {
        /* Convert it to the IPv6 Reverse Name */
        Dns_Ip6AddressToReverseName_W(ReverseAddress, Address);
    }

    /* Do the DNS Lookup */
    Blob = SaBlob_Query(ReverseAddress,
                        DNS_TYPE_PTR,
                        (ControlFlags & LUP_FLUSHCACHE) ?
                        DNS_QUERY_BYPASS_CACHE :
                        DNS_QUERY_STANDARD,
                        NULL,
                        AddressFamily);
    if (!Blob)
    {
        /* If this is IPv4... */
        if (AddressFamily == AF_INET)
        {
            /* Can we try NBT? */
            if (Rnr_CheckIfUseNbt(RnrContext))
            {
                /* Do NBT Resolution */
                Blob = Rnr_NbtResolveAddr(*(PIN_ADDR)&Address);
            }
        }

        /* Do we still not have a blob? */
        if (!Blob) ErrorCode = WSANO_DATA;
    }

    /* Set the error code and return */
    SetLastError(ErrorCode);
    return Blob;
}

PDNS_BLOB
WINAPI
Rnr_DoDnsLookup(IN PRNR_CONTEXT RnrContext)
{
    LPWSTR Name = RnrContext->ServiceName;
    LPGUID Guid = &RnrContext->lpServiceClassId;
    WORD DnsType;
    PVOID ReservedData = NULL;
    PVOID *Reserved = NULL;
    BOOL DoDnsQuery = TRUE;
    BOOL DoNbtQuery = TRUE;
    DWORD DnsFlags;
    PDNS_BLOB Blob;
    IN_ADDR Addr;

    /* Get the DNS Query Type */
    DnsType = GetDnsQueryTypeFromGuid(Guid);

    /* Check the request type */
    if ((DnsType != DNS_TYPE_A) ||
        (DnsType != DNS_TYPE_ATMA) ||
        (DnsType != DNS_TYPE_AAAA) ||
        (DnsType != DNS_TYPE_PTR))
    {
        /* Not a sockaddr request, so read the raw data */
        Reserved = &ReservedData;
    }

    /* Check the NS request type */
    switch (RnrContext->dwNameSpace)
    {
        /* Default flags for default, DNS or WINS Namespaces */
        case NS_DEFAULT:
        case NS_DNS:
        case NS_WINS:
            DnsFlags = 0;

        /* Set the DNS flags for a TCP/IP Local Namespace */
        case NS_TCPIP_LOCAL:
            DnsFlags = DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_NO_WIRE_QUERY;

        /* Set the DNS flags for a TCP/IP Hosts Namespace */
        case NS_TCPIP_HOSTS:
            DnsFlags = DNS_QUERY_NO_LOCAL_NAME |
                       DNS_QUERY_NO_WIRE_QUERY |
                       DNS_QUERY_BYPASS_CACHE;
    }

    /* Check if this is a DNS Server lookup or normal host lookup */
    if (!(Name) &&
        (DnsType != DNS_TYPE_A) &&
        (RnrContext->UdpPort == 53) || (RnrContext->TcpPort == 53))
    {
        /* This is actually a DNS Server lookup */
        Name = L"..DnsServers";
        DnsFlags = DNS_QUERY_NO_HOSTS_FILE |
                   DNS_QUERY_NO_WIRE_QUERY |
                   DNS_QUERY_BYPASS_CACHE;
    }
    else
    {
        /* Normal name lookup */
        DnsFlags |= 0x4000000;

        /* Check which Rr Type this request is */
        if (RnrContext->RrType = 0x10000002)
        {
            /*
             * Check if the previous value should be flushed or if this
             * is a local lookup.
             */
            if ((RnrContext->dwControlFlags & LUP_FLUSHPREVIOUS) &&
                (RnrContext->LookupFlags & LOCAL))
            {
                /* Tell DNS not to use the Hosts file */
                DnsFlags |= DNS_QUERY_NO_HOSTS_FILE;
            }

            /* Tell DNS that this is a ... request */
            DnsFlags |= 0x10000000;
        }
    }

    /* Check if flushing is enabled */
    if (RnrContext->dwControlFlags & LUP_FLUSHCACHE)
    {
        /* Bypass the Cache */
        DnsFlags |= DNS_QUERY_BYPASS_CACHE;
    }

    /* Make sure we are going to to a DNS Query */
    if (DoDnsQuery)
    {
        /* Do the DNS Query */
        Blob = SaBlob_Query(Name,
                            DnsType,
                            DnsFlags,
                            Reserved,
                            0);

        /* Check if we had reserved data */
        if (Reserved == &ReservedData)
        {
            /* Check if we need to use it */
            if (RnrContext->RnrId)
            {
                /* FIXME */
                //SaveAnswer(
            }

            /* Free it */
            DnsApiFree(ReservedData);
        }
    }

    /* Ok, did we get a blob? */
    if (Blob)
    {
        /* We did..does it have not have name yet? */
        if (!Blob->Name)
        {
            /* It doesn't... was this a Hostname GUID? */
            if (RtlEqualMemory(Guid, &HostnameGuid, sizeof(GUID)))
            {
                /* Did we not get a name? */
                if (Name || *Name)
                {
                    /* Then we must fail this request */
                    SaBlob_Free(Blob);
                    Blob = NULL;
                }
            }
        }
    }
    else if (DoNbtQuery)
    {
        /* Is this an IPv4 record? */
        if (DnsType == DNS_TYPE_A)
        {
            /* Check if we can use NBT, and use NBT to resolve it */
            if (Rnr_CheckIfUseNbt(RnrContext)) Blob = Rnr_NbtResolveName(Name);
        }
        else if (DnsType == DNS_TYPE_PTR)
        {
            /* IPv4 reverse address. Convert it */
            if (Dns_Ip4ReverseNameToAddress_W(&Addr, Name))
            {
                /* Resolve it */
                Blob = Rnr_NbtResolveAddr(Addr);
            }
        }
    }

    /* Do we not have a blob? Set the error code */
    if (!Blob) SetLastError(WSANO_ADDRESS);

    /* Return the blob */
    return Blob;
}

BOOLEAN
WINAPI
Rnr_CheckIfUseNbt(PRNR_CONTEXT RnrContext)
{
    /* If an Rr ID was specified, don't use NBT */
    if (RnrContext->RrType) return FALSE;

    /* Check if we have more then one GUID */
    if (MaskOfGuids)
    {
        /* Compare this guy's GUID with the NBT Provider GUID */
        if (!RtlEqualMemory(&RnrContext->lpProviderId,
                            &NbtProviderId,
                            sizeof(GUID)))
        {
            /* Not NBT Guid */
            return FALSE;
        }
    }

    /* Is the DNS Namespace valid for NBT? */
    if ((RnrContext->dwNameSpace == NS_ALL) ||
        (RnrContext->dwNameSpace == NS_NETBT) ||
        (RnrContext->dwNameSpace == NS_WINS))
    {
        /* Use NBT */
        return TRUE;
    }

    /* Don't use NBT */
    return FALSE;
}

PDNS_BLOB
WINAPI
Rnr_NbtResolveName(IN LPWSTR Name)
{
    /*
     * Heh...right...NBT lookups...as if!
     * Seriously, don't bother -- MS is considering to deprecate this
     * in Vista SP1 or Blackcomb. If someone complains about this, please
     * instruct them to deposit a very large check in my bank account...
     *                          - AI 03/12/05
     */
    return NULL;
}

PDNS_BLOB
WINAPI
Rnr_NbtResolveAddr(IN IN_ADDR Address)
{
    /*
     * Heh...right...NBT lookups...as if!
     * Seriously, don't bother -- MS is considering to deprecate this
     * in Vista SP1 or Blackcomb. If someone complains about this, please
     * instruct them to deposit a very large check in my bank account...
     *                          - AI 03/12/05
     */
    return NULL;
}
