/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/dnsaddr.c
 * PURPOSE:     Functions dealing with DNS_ADDRESS and DNS_ARRAY addresses.
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PDNS_ARRAY
WINAPI
DnsAddrArray_Create(ULONG Count)
{
    PDNS_ARRAY DnsAddrArray;

    /* Allocate space for the array and the addresses within it */
    DnsAddrArray = Dns_AllocZero(sizeof(DNS_ARRAY) +
                                 (Count * sizeof(DNS_ADDRESS)));

    /* Write the allocated address count */
    if (DnsAddrArray) DnsAddrArray->AllocatedAddresses = Count;

    /* Return it */
    return DnsAddrArray;
}

VOID
WINAPI
DnsAddrArray_Free(IN PDNS_ARRAY DnsAddrArray)
{
    /* Just free the entire array */
    Dns_Free(DnsAddrArray);
}

BOOL
WINAPI
DnsAddrArray_AddIp4(IN PDNS_ARRAY DnsAddrArray,
                    IN IN_ADDR Address,
                    IN DWORD AddressType)
{
    DNS_ADDRESS DnsAddress;

    /* Build the DNS Address */
    DnsAddr_BuildFromIp4(&DnsAddress, Address, 0);

    /* Add it to the array */
    return DnsAddrArray_AddAddr(DnsAddrArray, &DnsAddress, 0, AddressType);
}

BOOL
WINAPI
DnsAddrArray_AddAddr(IN PDNS_ARRAY DnsAddrArray,
                     IN PDNS_ADDRESS DnsAddress,
                     IN WORD AddressFamily OPTIONAL,
                     IN DWORD AddressType OPTIONAL)
{
    /* Make sure we have an array */
    if (!DnsAddrArray) return FALSE;
    
    /* Check if we should validate the Address Family */
    if (AddressFamily)
    {
        /* Validate it */
        if (AddressFamily != DnsAddress->AddressFamily) return TRUE;
    }

    /* Check if we should validate the Address Type */
    if (AddressType)
    {
        /* Make sure that this array contains this type of addresses */
        if (!DnsAddrArray_ContainsAddr(DnsAddrArray, DnsAddress, AddressType))
        {
            /* Won't be adding it */
            return TRUE;
        }
    }

    /* Make sure we have space in the array */
    if (DnsAddrArray->AllocatedAddresses < DnsAddrArray->UsedAddresses)
    {
        return FALSE; 
    }

    /* Now add the address */
    RtlCopyMemory(&DnsAddrArray->Addresses[DnsAddrArray->UsedAddresses],
                  DnsAddress,
                  sizeof(DNS_ADDRESS));

    /* Return success */
    return TRUE;
}

VOID
WINAPI
DnsAddr_BuildFromIp4(IN PDNS_ADDRESS DnsAddress,
                     IN IN_ADDR Address,
                     IN WORD Port)
{
    /* Clear the address */
    RtlZeroMemory(DnsAddress, sizeof(DNS_ADDRESS));

    /* Write data */
    DnsAddress->Ip4Address.sin_family = AF_INET;
    DnsAddress->Ip4Address.sin_port  = Port;
    DnsAddress->Ip4Address.sin_addr = Address;
    DnsAddress->AddressLength = sizeof(SOCKADDR_IN);
}

VOID
WINAPI
DnsAddr_BuildFromIp6(IN PDNS_ADDRESS DnsAddress,
                     IN PIN6_ADDR Address,
                     IN ULONG ScopeId,
                     IN WORD Port)
{
    /* Clear the address */
    RtlZeroMemory(DnsAddress, sizeof(DNS_ADDRESS));

    /* Write data */
    DnsAddress->Ip6Address.sin6_family = AF_INET6;
    DnsAddress->Ip6Address.sin6_port  = Port;
    DnsAddress->Ip6Address.sin6_addr = *Address;
    DnsAddress->Ip6Address.sin6_scope_id = ScopeId;
    DnsAddress->AddressLength = sizeof(SOCKADDR_IN6);
}

VOID
WINAPI
DnsAddr_BuildFromAtm(IN PDNS_ADDRESS DnsAddress,
                     IN DWORD AddressType,
                     IN PVOID AddressData)
{
    ATM_ADDRESS Address;

    /* Clear the address */
    RtlZeroMemory(DnsAddress, sizeof(DNS_ADDRESS));

    /* Build an ATM Address */
    Address.AddressType = AddressType;
    Address.NumofDigits = DNS_ATMA_MAX_ADDR_LENGTH;
    RtlCopyMemory(&Address.Addr, AddressData, DNS_ATMA_MAX_ADDR_LENGTH);

    /* Write data */
    DnsAddress->AtmAddress = Address;
    DnsAddress->AddressLength = sizeof(ATM_ADDRESS);
}

BOOLEAN
WINAPI
DnsAddr_BuildFromDnsRecord(IN PDNS_RECORD DnsRecord,
                           OUT PDNS_ADDRESS DnsAddr)
{
    /* Check what kind of record this is */
    switch(DnsRecord->wType)
    {
        /* IPv4 */
        case DNS_TYPE_A:
            /* Create the DNS Address */
            DnsAddr_BuildFromIp4(DnsAddr,
                                 *(PIN_ADDR)&DnsRecord->Data.A.IpAddress,
                                 0);
            break;

        /* IPv6 */
        case DNS_TYPE_AAAA:
            /* Create the DNS Address */
            DnsAddr_BuildFromIp6(DnsAddr,
                                 (PIN6_ADDR)&DnsRecord->Data.AAAA.Ip6Address,
                                 DnsRecord->dwReserved,
                                 0);
            break;

        /* ATM */
        case DNS_TYPE_ATMA:
            /* Create the DNS Address */
            DnsAddr_BuildFromAtm(DnsAddr,
                                 DnsRecord->Data.Atma.AddressType,
                                 &DnsRecord->Data.Atma.Address);
            break;
    }

    /* Done! */
    return TRUE;
}

BOOL
WINAPI
DnsAddrArray_ContainsAddr(IN PDNS_ARRAY DnsAddrArray,
                          IN PDNS_ADDRESS DnsAddress,
                          IN DWORD AddressType)
{
    /* FIXME */
    return TRUE;
}

