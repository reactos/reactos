/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/


/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
FetchPortFromClassInfo(IN DWORD Type,
                       IN LPGUID Guid,
                       IN LPWSASERVICECLASSINFOW ServiceClassInfo)
{
    DWORD Port;

    if (Type == UDP)
    {
        if (IS_SVCID_UDP(Guid))
        {
            /* Get the Port from the Service ID */
            Port = PORT_FROM_SVCID_UDP(Guid);
        } 
        else 
        {
            /* No UDP */
            Port = -1;
        }
    }
    else if (Type == TCP)
    {
        if (IS_SVCID_TCP(Guid))
        {
            /* Get the Port from the Service ID */
            Port = PORT_FROM_SVCID_TCP(Guid);
        } 
        else 
        {
            /* No TCP */
            Port = -1;
        }
    }
    else
    {
        /* Invalid */
        Port = -1;
    }
    
    /* Return it */
    return Port;
}

WORD
WINAPI
GetDnsQueryTypeFromGuid(IN LPGUID Guid)
{
    WORD DnsType = DNS_TYPE_A;

    /* Check if this is is a DNS GUID and get the type from it */
    if (IS_SVCID_DNS(Guid)) DnsType = RR_FROM_SVCID(Guid);

    /* Return the DNS Type */
    return DnsType;
}

LPSTR
WINAPI
GetAnsiNameRnR(IN LPWSTR UnicodeName,
               IN LPSTR Domain,
               OUT PBOOL Result)
{
    SIZE_T Length = 0;
    LPSTR AnsiName;

    /* Check if we have a domain */
    if (Domain) Length = strlen(Domain);

    /* Calculate length needed and allocate it */
    Length += ((wcslen(UnicodeName) + 1) * sizeof(WCHAR) * 2);
    AnsiName = DnsApiAlloc((DWORD)Length);

    /* Convert the string */
    WideCharToMultiByte(CP_ACP,
                        0,
                        UnicodeName,
                        -1,
                        AnsiName,
                        (DWORD)Length,
                        0,
                        Result);

    /* Add the domain, if needed */
    if (Domain) strcat(AnsiName, Domain);

    /* Return the ANSI name */
    return AnsiName;
}

DWORD
WINAPI
GetServerAndProtocolsFromString(PWCHAR ServiceString,
                                LPGUID ServiceType,
                                PSERVENT *ReverseServent)
{
    PSERVENT LocalServent = NULL;
    DWORD ProtocolFlags = 0;
    PWCHAR ProtocolString;
    PWCHAR ServiceName;
    PCHAR AnsiServiceName;
    PCHAR AnsiProtocolName;
    PCHAR TempString;
    ULONG ServiceNameLength;
    ULONG PortNumber = 0;

    /* Make sure that this is valid for a Servent lookup */
    if ((ServiceString) &&
        (ServiceType) && 
        (memcmp(ServiceType, &HostnameGuid, sizeof(GUID))) &&
        (memcmp(ServiceType, &InetHostName, sizeof(GUID))))
    {
        /* Extract the Protocol */
        ProtocolString = wcschr(ServiceString, L'/');
        if (!ProtocolString) ProtocolString = wcschr(ProtocolString, L'\0');

        /* Find out the length of the service name */
        ServiceNameLength = (ULONG)(ProtocolString - ServiceString) * sizeof(WCHAR);

        /* Allocate it */
        ServiceName = DnsApiAlloc(ServiceNameLength + sizeof(UNICODE_NULL));

        /* Copy it and null-terminate */
        RtlMoveMemory(ServiceName, ServiceString, ServiceNameLength);
        ServiceName[ServiceNameLength] = UNICODE_NULL;

        /* Get the Ansi Service Name */
        AnsiServiceName = GetAnsiNameRnR(ServiceName, 0, NULL);
        DnsApiFree(ServiceName);
        if (AnsiServiceName)
        {
            /* If we only have a port number, convert it */
            for (TempString = AnsiServiceName;
                 *TempString && isdigit(*TempString);
                 TempString++);
        
            /* Convert to Port Number */
            if (!*TempString) PortNumber = atoi(AnsiServiceName);

            /* Check if we have a Protocol Name, and set it */
            if (!(*ProtocolString) || !(*++ProtocolString))
            {
                /* No protocol string, so won't have it in ANSI either */
                AnsiProtocolName = NULL;
            }
            else
            {
                /* Get it in ANSI */
                AnsiProtocolName = GetAnsiNameRnR(ProtocolString, 0, NULL);
            }

            /* Now do the actual operation */
            if (PortNumber)
            {
                /* FIXME: Get Servent by Port */
            }
            else
            {
                /* FIXME: Get Servent by Name */
            }

            /* Free the ansi names if we had them */
            if (AnsiProtocolName) DnsApiFree(AnsiProtocolName);
            if (AnsiServiceName) DnsApiFree(AnsiProtocolName);
        }
    }

    /* Return Servent */
    if (ReverseServent) *ReverseServent = LocalServent;

    /* Return Protocol */
    if (LocalServent)
    {
        /* Check if it was UDP */
        if (_stricmp("udp", LocalServent->s_proto))
        {
            /* Return UDP */
            ProtocolFlags = UDP;
        }
        else
        {
            /* Return TCP */
            ProtocolFlags = TCP;
        }
    }
    else
    {
        /* Return both, no restrictions */
        ProtocolFlags = (TCP | UDP);
    }

    /* Return the flags */
    return ProtocolFlags;
}

PSERVENT
WSPAPI
CopyServEntry(IN PSERVENT Servent,
              IN OUT PULONG_PTR BufferPos,
              IN OUT PULONG BufferFreeSize,
              IN OUT PULONG BlobSize,
              IN BOOLEAN Relative)
{
    return NULL;
}