/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/straddr.c
 * PURPOSE:     Functions for address<->string conversion.
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

LPWSTR
WINAPI
Dns_Ip6AddressToReverseName_W(OUT LPWSTR Name,
                              IN IN6_ADDR Address)
{
    /* FIXME */
    return NULL;
}

LPWSTR
WINAPI
Dns_Ip4AddressToReverseName_W(OUT LPWSTR Name,
                              IN IN_ADDR Address)
{
    /* Simply append the ARPA string */
    return Name + (wsprintfW(Name,
                             L"%u.%u.%u.%u.in-addr.arpa.",
                             Address.S_un.S_addr >> 24,
                             Address.S_un.S_addr >> 10,
                             Address.S_un.S_addr >> 8,
                             Address.S_un.S_addr) * sizeof(WCHAR));
}

BOOLEAN
WINAPI
Dns_Ip4ReverseNameToAddress_A(OUT PIN_ADDR Address,
                              IN LPSTR Name)
{
    /* FIXME */
    return FALSE;
}

BOOLEAN
WINAPI
Dns_Ip6ReverseNameToAddress_A(OUT PIN6_ADDR Address,
                              IN LPSTR Name)
{
    /* FIXME */
    return FALSE;
}

BOOLEAN
WINAPI
Dns_Ip6StringToAddress_A(OUT PIN6_ADDR Address,
                         IN LPSTR Name)
{
    PCHAR Terminator;
    NTSTATUS Status;

    /* Let RTL Do it for us */
    Status = RtlIpv6StringToAddressA(Name, &Terminator, Address);
    if (NT_SUCCESS(Status)) return TRUE;

    /* We failed */
    return FALSE;
}

BOOLEAN
WINAPI
Dns_Ip6StringToAddress_W(OUT PIN6_ADDR Address,
                         IN LPWSTR Name)
{
    PCHAR Terminator;
    NTSTATUS Status;

    /* Let RTL Do it for us */
    Status = RtlIpv6StringToAddressW(Name, &Terminator, Address);
    if (NT_SUCCESS(Status)) return TRUE;

    /* We failed */
    return FALSE;
}

BOOLEAN
WINAPI
Dns_Ip4StringToAddress_A(OUT PIN_ADDR Address,
                         IN LPSTR Name)
{
    ULONG Addr;

    /* Use inet_addr to convert it... */
    Addr = inet_addr(Name);
    if (Addr == -1)
    {
        /* Check if it's the wildcard (which is ok...) */
        if (strcmp("255.255.255.255", Name)) return FALSE;
    }

    /* If we got here, then we suceeded... return the address */
    Address->S_un.S_addr = Addr;
    return TRUE;
}

BOOLEAN
WINAPI
Dns_Ip4StringToAddress_W(OUT PIN_ADDR Address,
                         IN LPWSTR Name)
{
    CHAR AnsiName[16];
    ULONG Size = sizeof(AnsiName);
    INT ErrorCode;

    /* Make a copy of the name in ANSI */
    ErrorCode = Dns_StringCopy(&AnsiName,
                               &Size,
                               Name,
                               0,
                               UnicodeString,
                               AnsiString);
    if (ErrorCode)
    {
        /* Copy made sucesfully, now convert it */
        ErrorCode = Dns_Ip4StringToAddress_A(Address, AnsiName);
    }

    /* Return either 0 bytes copied (failure == false) or conversion status */
    return ErrorCode;
}

BOOLEAN
WINAPI
Dns_Ip4ReverseNameToAddress_W(OUT PIN_ADDR Address,
                              IN LPWSTR Name)
{
    CHAR AnsiName[32];
    ULONG Size = sizeof(AnsiName);
    INT ErrorCode;

    /* Make a copy of the name in ANSI */
    ErrorCode = Dns_StringCopy(&AnsiName,
                               &Size,
                               Name,
                               0,
                               UnicodeString,
                               AnsiString);
    if (ErrorCode)
    {
        /* Copy made sucesfully, now convert it */
        ErrorCode = Dns_Ip4ReverseNameToAddress_A(Address, AnsiName);
    }

    /* Return either 0 bytes copied (failure == false) or conversion status */
    return ErrorCode;
}

BOOLEAN
WINAPI
Dns_StringToAddressEx(OUT PVOID Address,
                      IN OUT PULONG AddressSize,
                      IN PVOID AddressName,
                      IN OUT PDWORD AddressFamily,
                      IN BOOLEAN Unicode,
                      IN BOOLEAN Reverse)
{
    DWORD Af = *AddressFamily;
    ULONG AddrSize = *AddressSize;
    IN6_ADDR Addr;
    BOOLEAN Return;
    INT ErrorCode = ERROR_SUCCESS;
    CHAR AnsiName[INET6_ADDRSTRLEN + sizeof("ip6.arpa.")];
    ULONG Size = sizeof(AnsiName);

    /* First check if this is a reverse address string */
    if (Reverse)
    {
        /* Convert it right now to ANSI as an optimization */
        Dns_StringCopy(AnsiName,
                       &Size,
                       AddressName,
                       0,
                       UnicodeString,
                       AnsiString);

        /* Use the ANSI Name instead */
        AddressName = AnsiName;
    }

    /* 
     * If the caller doesn't know what the family is, we'll assume IPv4 and
     * check if we failed or not. If the caller told us it's IPv4, then just
     * do IPv4...
     */
    if ((Af == AF_UNSPEC) || (Af == AF_INET))
    {
        /* Now check if the caller gave us the reverse name or not */
        if (Reverse)
        {
            /* Get the Address */
            Return = Dns_Ip4ReverseNameToAddress_A((PIN_ADDR)&Addr, AddressName);
        }
        else
        {
            /* Check if the caller gave us unicode or not */
            if (Unicode)
            {
                /* Get the Address */
                Return = Dns_Ip4StringToAddress_W((PIN_ADDR)&Addr, AddressName);
            }
            else
            {
                /* Get the Address */
                Return = Dns_Ip4StringToAddress_A((PIN_ADDR)&Addr, AddressName);
            }
        }

        /* Check if we suceeded */
        if (Return)
        {
            /* Save address family */
            Af = AF_INET;

            /* Check if the address size matches */
            if (AddrSize < sizeof(IN_ADDR))
            {
                /* Invalid match, set error code */
                ErrorCode = ERROR_MORE_DATA;
            }
            else
            {
                /* It matches, save the address! */
                *(PIN_ADDR)Address = *(PIN_ADDR)&Addr;
            }
        }
    }

    /* If we are here, either AF_INET6 was specified or IPv4 failed */
    if ((Af == AF_UNSPEC) || (Af == AF_INET6))
    {
        /* Now check if the caller gave us the reverse name or not */
        if (Reverse)
        {
            /* Get the Address */
            Return = Dns_Ip6ReverseNameToAddress_A(&Addr, AddressName);
        }
        else
        {
            /* Check if the caller gave us unicode or not */
            if (Unicode)
            {
                /* Get the Address */
                Return = Dns_Ip6StringToAddress_W(&Addr, AddressName);
            }
            else
            {
                /* Get the Address */
                Return = Dns_Ip6StringToAddress_A(&Addr, AddressName);
            }
        }

        /* Check if we suceeded */
        if (Return)
        {
            /* Save address family */
            Af = AF_INET6;

            /* Check if the address size matches */
            if (AddrSize < sizeof(IN6_ADDR))
            {
                /* Invalid match, set error code */
                ErrorCode = ERROR_MORE_DATA;
            }
            else
            {
                /* It matches, save the address! */
                *(PIN6_ADDR)Address = Addr;
            }
        }
    }
    else if (Af != AF_INET)
    {
        /* You're like.. ATM or something? Get outta here! */
        Af = AF_UNSPEC;
        ErrorCode = WSA_INVALID_PARAMETER;
    }

    /* Set error if we had one */
    if (ErrorCode) SetLastError(ErrorCode);

    /* Return the address family and size */
    *AddressFamily = Af;
    *AddressSize = AddrSize;

    /* Return success or failure */
    return (ErrorCode == ERROR_SUCCESS);
}

BOOLEAN
WINAPI
Dns_StringToAddressW(OUT PVOID Address,
                     IN OUT PULONG AddressSize,
                     IN LPWSTR AddressName,
                     IN OUT PDWORD AddressFamily)
{
    /* Call the common API */
    return Dns_StringToAddressEx(Address,
                                 AddressSize,
                                 AddressName,
                                 AddressFamily,
                                 TRUE,
                                 FALSE);
}

BOOLEAN
WINAPI
Dns_StringToDnsAddrEx(OUT PDNS_ADDRESS DnsAddr,
                      IN PVOID AddressName,
                      IN DWORD AddressFamily,
                      IN BOOLEAN Unicode,
                      IN BOOLEAN Reverse)
{
    IN6_ADDR Addr;
    BOOLEAN Return;
    INT ErrorCode = ERROR_SUCCESS;
    CHAR AnsiName[INET6_ADDRSTRLEN + sizeof("ip6.arpa.")];
    ULONG Size = sizeof(AnsiName);

    /* First check if this is a reverse address string */
    if ((Reverse) && (Unicode))
    {
        /* Convert it right now to ANSI as an optimization */
        Dns_StringCopy(AnsiName,
                       &Size,
                       AddressName,
                       0,
                       UnicodeString,
                       AnsiString);

        /* Use the ANSI Name instead */
        AddressName = AnsiName;
    }

    /* 
     * If the caller doesn't know what the family is, we'll assume IPv4 and
     * check if we failed or not. If the caller told us it's IPv4, then just
     * do IPv4...
     */
    if ((AddressFamily == AF_UNSPEC) || (AddressFamily == AF_INET))
    {
        /* Now check if the caller gave us the reverse name or not */
        if (Reverse)
        {
            /* Get the Address */
            Return = Dns_Ip4ReverseNameToAddress_A((PIN_ADDR)&Addr, AddressName);
        }
        else
        {
            /* Check if the caller gave us unicode or not */
            if (Unicode)
            {
                /* Get the Address */
                Return = Dns_Ip4StringToAddress_W((PIN_ADDR)&Addr, AddressName);
            }
            else
            {
                /* Get the Address */
                Return = Dns_Ip4StringToAddress_A((PIN_ADDR)&Addr, AddressName);
            }
        }

        /* Check if we suceeded */
        if (Return)
        {
            /* Build the IPv4 Address */
            DnsAddr_BuildFromIp4(DnsAddr, *(PIN_ADDR)&Addr, 0);

            /* So we don't go in the code below... */
            AddressFamily = AF_INET;
        }
    }

    /* If we are here, either AF_INET6 was specified or IPv4 failed */
    if ((AddressFamily == AF_UNSPEC) || (AddressFamily == AF_INET6))
    {
        /* Now check if the caller gave us the reverse name or not */
        if (Reverse)
        {
            /* Get the Address */
            Return = Dns_Ip6ReverseNameToAddress_A(&Addr, AddressName);
            if (Return)
            {
                /* Build the IPv6 Address */
                DnsAddr_BuildFromIp6(DnsAddr, &Addr, 0, 0);
            }
            else
            {
                goto Quickie;
            }
        }
        else
        {
            /* Check if the caller gave us unicode or not */
            if (Unicode)
            {
                /* Get the Address */
                if (NT_SUCCESS(RtlIpv6StringToAddressExW(AddressName,
                                                         &DnsAddr->Ip6Address.sin6_addr,
                                                         &DnsAddr->Ip6Address.sin6_scope_id,
                                                         &DnsAddr->Ip6Address.sin6_port)))
                    Return = TRUE;
                else
                    Return = FALSE;
            }
            else
            {
                /* Get the Address */
                if (NT_SUCCESS(RtlIpv6StringToAddressExA(AddressName,
                                                         &DnsAddr->Ip6Address.sin6_addr,
                                                         &DnsAddr->Ip6Address.sin6_scope_id,
                                                         &DnsAddr->Ip6Address.sin6_port)))
                   Return = TRUE;
                else
                   Return = FALSE;
            }
        }

        /* Check if we suceeded */
        if (Return)
        {
            /* Finish setting up the structure */
            DnsAddr->Ip6Address.sin6_family = AF_INET6;
            DnsAddr->AddressLength = sizeof(SOCKADDR_IN6);
        }
    }
    else if (AddressFamily != AF_INET)
    {
        /* You're like.. ATM or something? Get outta here! */
        RtlZeroMemory(DnsAddr, sizeof(DNS_ADDRESS));
        SetLastError(WSA_INVALID_PARAMETER);
    }

Quickie:
    /* Return success or failure */
    return (ErrorCode == ERROR_SUCCESS);
}

BOOLEAN
WINAPI
Dns_ReverseNameToDnsAddr_W(OUT PDNS_ADDRESS DnsAddr,
                           IN LPWSTR Name)
{
    /* Call the common API */
    return Dns_StringToDnsAddrEx(DnsAddr,
                                 Name,
                                 AF_UNSPEC,
                                 TRUE,
                                 TRUE);
}

