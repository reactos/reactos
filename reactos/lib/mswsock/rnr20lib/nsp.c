/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

#define ALL_LUP_FLAGS (0x0BFFF)

/* DATA **********************************************************************/

LPWSTR g_pszHostName;
LPWSTR g_pszHostFqdn;
LONG g_NspRefCount;
GUID NbtProviderId = {0};
GUID DNSProviderId = {0};
DWORD MaskOfGuids;

NSP_ROUTINE g_NspVector = {sizeof(NSP_ROUTINE),
                          1,
                          1,
                          Dns_NSPCleanup,
                          Dns_NSPLookupServiceBegin,
                          Dns_NSPLookupServiceNext,
                          Dns_NSPLookupServiceEnd,
                          Dns_NSPSetService,
                          Dns_NSPInstallServiceClass,
                          Dns_NSPRemoveServiceClass,
                          Dns_NSPGetServiceClassInfo};

/* FUNCTIONS *****************************************************************/

INT 
WINAPI
Dns_NSPStartup(IN LPGUID lpProviderId,
               IN OUT LPNSP_ROUTINE lpsnpRoutines)
{
    INT ErrorCode;
    BOOLEAN Prolog;

    /* Validate the size */
    if (lpsnpRoutines->cbSize != sizeof(NSP_ROUTINE))
    {
        /* Fail */
        SetLastError(WSAEINVALIDPROCTABLE);
        return SOCKET_ERROR;
    }

    /* Enter the prolog */
    Prolog = RNRPROV_SockEnterApi();
    if (Prolog)
    {
        /* Increase our reference count */
        InterlockedIncrement(&g_NspRefCount);

        /* Check if we don't have the hostname */
        if (!g_pszHostName)
        {
            /* Query it from DNS */
            g_pszHostName = DnsQueryConfigAllocEx(DnsConfigHostName_W, 0, 0);
        }

        /* Check if we have a hostname now, but not a Fully-Qualified Domain */
        if (g_pszHostName && !(g_pszHostFqdn))
        {
            /* Get the domain from DNS */
            g_pszHostFqdn = DnsQueryConfigAllocEx(DnsConfigFullHostName_W,
                                                  0,
                                                  0);
        }

        /* If we don't have both of them, then set error */
        if (!(g_pszHostName) || !(g_pszHostFqdn)) ErrorCode = SOCKET_ERROR;
    }

    /* Check if the Prolog or DNS Local Queries failed */
    if (!(Prolog) || (ErrorCode != NO_ERROR))
    {
        /* Fail */
        SetLastError(WSASYSNOTREADY);
        return SOCKET_ERROR;
    }

    /* Copy the Routines */
    RtlMoveMemory(lpsnpRoutines, &g_NspVector, sizeof(NSP_ROUTINE));

    /* Check if this is NBT or DNS */
    if (!memcmp(lpProviderId, &NbtProviderId, sizeof(GUID)))
    {
        /* Enable the NBT Mask */
        MaskOfGuids |= NBT_MASK;
    }
    else if (!memcmp(lpProviderId, &DNSProviderId, sizeof(GUID)))
    {
        /* Enable the DNS Mask */
        MaskOfGuids |= DNS_MASK;
    }

    /* Return success */
    return NO_ERROR;
}

VOID
WSPAPI
Nsp_GlobalCleanup(VOID)
{
    /* Cleanup the RnR Contexts */
    RnrCtx_ListCleanup();

    /* Free the hostnames, if we have them */
    if (g_pszHostName) DnsApiFree(g_pszHostName);
    if (g_pszHostFqdn) DnsApiFree(g_pszHostFqdn);
    g_pszHostFqdn = g_pszHostName = NULL;
}

INT 
WINAPI
NSPStartup(IN LPGUID lpProviderId,
           IN OUT LPNSP_ROUTINE lpsnpRoutines)
{
    INT ErrorCode;

    /* Initialize the DLL */
    ErrorCode = MSWSOCK_Initialize();
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        SetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    /* Check if this is Winsock Mobile or DNS */
    if (!memcmp(lpProviderId, &gNLANamespaceGuid, sizeof(GUID)))
    {
        /* Initialize WSM */
        return WSM_NSPStartup(lpProviderId, lpsnpRoutines);
    }
    
    /* Initialize DNS */
    return Dns_NSPStartup(lpProviderId, lpsnpRoutines);
}

INT
WINAPI
Dns_NSPCleanup(IN LPGUID lpProviderId)
{
    /* Decrement our reference count and do global cleanup if it's reached 0 */
    if (!(InterlockedDecrement(&g_NspRefCount))) Nsp_GlobalCleanup();
    
    /* Return success */
    return NO_ERROR;
}

INT
WINAPI
Dns_NSPSetService(IN LPGUID lpProviderId,
                  IN LPWSASERVICECLASSINFOW lpServiceClassInfo,
                  IN LPWSAQUERYSETW lpqsRegInfo,
                  IN WSAESETSERVICEOP essOperation,
                  IN DWORD dwControlFlags)
{
    /* Unlike NLA, DNS Services cannot be dynmically modified */
    SetLastError(ERROR_NOT_SUPPORTED);
    return SOCKET_ERROR;
}

INT
WINAPI
Dns_NSPInstallServiceClass(IN LPGUID lpProviderId,
                           IN LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    /* Unlike NLA, DNS Services cannot be dynmically modified */
    SetLastError(WSAEOPNOTSUPP);
    return SOCKET_ERROR;
};

INT
WINAPI
Dns_NSPRemoveServiceClass(IN LPGUID lpProviderId,
                          IN LPGUID lpServiceCallId)
{
    /* Unlike NLA, DNS Services cannot be dynmically modified */
    SetLastError(WSAEOPNOTSUPP);
    return SOCKET_ERROR;
}
INT
WINAPI
Dns_NSPGetServiceClassInfo(IN LPGUID lpProviderId,
                           IN OUT LPDWORD lpdwBufSize,
                           IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    /* Unlike NLA, DNS Services cannot be dynmically modified */
    SetLastError(WSAEOPNOTSUPP);
    return SOCKET_ERROR;
}

INT
WINAPI
Dns_NSPLookupServiceEnd(IN HANDLE hLookup)
{
    PRNR_CONTEXT RnrContext;

    /* Get this handle's context */
    RnrContext = RnrCtx_Get(hLookup, 0, NULL);

    /* Mark it as completed */
    RnrContext->LookupFlags |= DONE;

    /* Dereference it once for our _Get */
    RnrCtx_Release(RnrContext);

    /* And once last to delete it */
    RnrCtx_Release(RnrContext);

    /* return */
    return NO_ERROR;
}

INT
WINAPI
rnr_IdForGuid(IN LPGUID Guid)
{
    
    if (memcmp(Guid, &InetHostName, sizeof(GUID))) return 0x10000002;
    if (memcmp(Guid, &Ipv6Guid, sizeof(GUID))) return 0x10000023;
    if (memcmp(Guid, &HostnameGuid, sizeof(GUID))) return 0x1;
    if (memcmp(Guid, &AddressGuid, sizeof(GUID))) return 0x80000000;
    if (memcmp(Guid, &IANAGuid, sizeof(GUID))) return 0x2;
    if IS_SVCID_DNS(Guid) return 0x5000000;
    if IS_SVCID_TCP(Guid) return 0x1000000;
    if IS_SVCID_UDP(Guid) return 0x2000000;
    return 0;
}

PVOID
WSPAPI
FlatBuf_ReserveAlignDword(IN PFLATBUFF FlatBuffer,
                          IN ULONG Size)
{
    /* Let DNSLIB do the grunt work */
    return FlatBuf_Arg_Reserve((PVOID)FlatBuffer->BufferPos,
                               &FlatBuffer->BufferFreeSize,
                               Size,
                               sizeof(PVOID));
}

PVOID
WSPAPI
FlatBuf_WriteString(IN PFLATBUFF FlatBuffer,
                    IN PVOID String,
                    IN BOOLEAN IsUnicode)
{
    /* Let DNSLIB do the grunt work */
    return FlatBuf_Arg_WriteString((PVOID)FlatBuffer->BufferPos,
                                   &FlatBuffer->BufferFreeSize,
                                   String,
                                   IsUnicode);
}

PVOID
WSPAPI
FlatBuf_CopyMemory(IN PFLATBUFF FlatBuffer,
                   IN PVOID Buffer,
                   IN ULONG Size,
                   IN ULONG Align)
{
    /* Let DNSLIB do the grunt work */
    return FlatBuf_Arg_CopyMemory((PVOID)FlatBuffer->BufferPos,
                                  &FlatBuffer->BufferFreeSize,
                                  Buffer,
                                  Size,
                                  Align);
}

INT 
WINAPI
Dns_NSPLookupServiceBegin(LPGUID lpProviderId,
                          LPWSAQUERYSETW lpqsRestrictions,
                          LPWSASERVICECLASSINFOW lpServiceClassInfo,
                          DWORD dwControlFlags,
                          LPHANDLE lphLookup)
{
    INT ErrorCode = SOCKET_ERROR;
    PWCHAR ServiceName = lpqsRestrictions->lpszServiceInstanceName;
    LPGUID ServiceClassId;
    INT RnrId;
    ULONG LookupFlags;
    BOOL NameRequested = FALSE;
    WCHAR StringBuffer[48];
    ULONG i;
    DWORD LocalProtocols;
    ULONG ProtocolFlags;
    PSERVENT LookupServent;
    DWORD UdpPort, TcpPort;
    PRNR_CONTEXT RnrContext;
    PSOCKADDR_IN ReverseSock;

    /* Check if the Size isn't weird */
    if(lpqsRestrictions->dwSize < sizeof(WSAQUERYSETW)) 
    {
        ErrorCode = WSAEFAULT;
        goto Quickie;
    }

    /* Get the GUID */
    ServiceClassId = lpqsRestrictions->lpServiceClassId;
    if(!ServiceClassId) 
    {
        /* No GUID, fail */
        ErrorCode = WSA_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Get the RNR ID */
    RnrId = rnr_IdForGuid(ServiceClassId);

    /* Make sure that the control flags are valid */
    if ((dwControlFlags & ~ALL_LUP_FLAGS) ||
        ((dwControlFlags & (LUP_CONTAINERS | LUP_NOCONTAINERS)) ==
        (LUP_CONTAINERS | LUP_NOCONTAINERS)))
    {
        /* Either non-recognized flags or invalid combos were passed */
        ErrorCode = WSA_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Make sure that we have no context, and that LUP_CONTAINERS is not on */
    if(((lpqsRestrictions->lpszContext) &&
        (*lpqsRestrictions->lpszContext) &&
        (wcscmp(lpqsRestrictions->lpszContext,  L"\\"))) ||
       (dwControlFlags & LUP_CONTAINERS))
    {
        /* We don't support contexts or LUP_CONTAINERS */
        ErrorCode = WSANO_DATA;
        goto Quickie;
    }
    
    /* Is this a Reverse Lookup? */
    if (RnrId == 0x80000000)
    {
        /* Remember for later */
        LookupFlags = REVERSE;
    } 
    else
    {
        /* Is this a IANA Lookup? */
        if (RnrId == 0x2)
        {
            /* Mask out this flag since it's of no use now */
            dwControlFlags &= ~(LUP_RETURN_ADDR);

            /* This is a IANA lookup, remember for later */
            LookupFlags |= IANA;
        }

        /* Check if we need a name or not */
        if ((RnrId == 0x1) ||
            (RnrId == 0x10000002) ||
            (RnrId == 0x10000023) ||
            (RnrId == 0x10000022))
        {
            /* We do */
            NameRequested = TRUE;
        }   
    }

    /* Final check to make sure if we need a name or not */
    if (RnrId & 0x3000000) NameRequested = TRUE;
        
    /* No Service Name was specified */
    if(!(ServiceName) || !(*ServiceName))
    {
        /*
         * A name was requested but no Service Name was given,
         * so this is a local lookup
         */
        if(NameRequested)
        {
            /* A local Lookup */
            LookupFlags |= LOCAL;
            ServiceName = L"";
        } 
        else if((LookupFlags & REVERSE) && 
                (lpqsRestrictions->lpcsaBuffer) && 
                (lpqsRestrictions->dwNumberOfCsAddrs == 1))
        {
            /* Reverse lookup, make sure a CS Address is there */
            ReverseSock = (struct sockaddr_in*)
                          lpqsRestrictions->lpcsaBuffer->RemoteAddr.lpSockaddr;

            /* Convert address to Unicode */
            MultiByteToWideChar(CP_ACP,
                                0,
                                inet_ntoa(ReverseSock->sin_addr),
                                -1,
                                StringBuffer,
                                16);

            /* Set it as the new name */
            ServiceName = StringBuffer;
        } 
        else 
        {
            /* We can't do anything without a service name at this point */
            ErrorCode = WSA_INVALID_PARAMETER;
            goto Quickie;
        }
    } 
    else if(NameRequested) 
    {
        /* Check for meaningful DNS Names */
        if (DnsNameCompare_W(ServiceName, L"localhost") ||
            DnsNameCompare_W(ServiceName, L"loopback")) 
        {
            /* This is the local and/or loopback DNS name */
            LookupFlags |= (LOCAL | LOOPBACK);
        } 
        else if (DnsNameCompare_W(ServiceName, g_pszHostName) ||
                 DnsNameCompare_W(ServiceName, g_pszHostFqdn)) 
        {
            /* This is the local name of the computer */
            LookupFlags |= LOCAL;
        }
    }

    /* Check if any restrictions were made on the protocols */
    if(lpqsRestrictions->lpafpProtocols) 
    {
        /* Save our local copy to speed up the loop */
        LocalProtocols = lpqsRestrictions->dwNumberOfProtocols;
        ProtocolFlags = 0;

        /* Loop the protocols */
        for(i = 0; LocalProtocols--;) 
        {
            /* Make sure it's a family that we recognize */
            if ((lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_INET) ||
                (lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_INET6) ||
                (lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_UNSPEC) ||
                (lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_ATM))
            {
                /* Find which one is used */
                switch(lpqsRestrictions->lpafpProtocols[i].iProtocol) 
                {
                    case IPPROTO_UDP:
                        ProtocolFlags |= UDP;
                        break;
                    case IPPROTO_TCP:
                        ProtocolFlags |= TCP;
                        break;
                    case PF_ATM:
                        ProtocolFlags |= ATM;
                        break;
                    default:
                        break;
                }
            }
        }
        /* Make sure we have at least a valid protocol */
        if (!ProtocolFlags)
        {
            /* Fail */
            ErrorCode = WSANO_DATA;
            goto Quickie;
        }
    }
    else 
    {
        /* No restrictions, assume TCP/UDP */
        ProtocolFlags = (TCP | UDP);
    }

    /* Create the Servent from the Service String */
    UdpPort = TcpPort = -1;
    ProtocolFlags |= GetServerAndProtocolsFromString(lpqsRestrictions->lpszQueryString,
                                                     ServiceClassId,
                                                     &LookupServent);

    /* Extract the port numbers */
    if(LookupServent) 
    {
        /* Are we using UDP? */
        if(ProtocolFlags & UDP) 
        {
            /* Get the UDP Port, disable the TCP Port */
            UdpPort = ntohs(LookupServent->s_port);
            TcpPort = -1;
        } 
        else if(ProtocolFlags & TCP) 
        {
            /* Get the TCP Port, disable the UDP Port */
            TcpPort = ntohs(LookupServent->s_port);
            UdpPort = -1;
        }
    } 
    else 
    {
        /* No servent, so use the Service ID to check */
        if(ProtocolFlags & UDP)
        {
            /* Get the Port from the Service ID */
            UdpPort = FetchPortFromClassInfo(UDP,
                                             ServiceClassId,
                                             lpServiceClassInfo);
        } 
        else 
        {
            /* No UDP */
            UdpPort = -1;
        }

        /* No servent, so use the Service ID to check */
        if(ProtocolFlags & TCP)
        {
            /* Get the Port from the Service ID */
            UdpPort = FetchPortFromClassInfo(TCP,
                                             ServiceClassId,
                                             lpServiceClassInfo);
        }
        else 
        {
            /* No TCP */
            TcpPort = -1;
        }
    }

    /* Check if we still don't have a valid port by now */
    if((TcpPort == -1) && (UdpPort == -1))
    {
        /* Check if this is TCP */
        if ((ProtocolFlags & TCP) || !(ProtocolFlags & UDP))
        {
            /* Set the UDP Port to 0 */
            UdpPort = 0;
        }
        else
        {
            /* Set the TCP Port to 0 */
            TcpPort = 0;
        }
    }

    /* Allocate a Context for this Query */
    RnrContext = RnrCtx_Create(NULL, ServiceName);
    RnrContext->lpServiceClassId = *ServiceClassId;
    RnrContext->RnrId = RnrId;
    RnrContext->dwControlFlags = dwControlFlags;
    RnrContext->TcpPort = TcpPort;
    RnrContext->UdpPort = UdpPort;
    RnrContext->LookupFlags = LookupFlags;
    RnrContext->lpProviderId = *lpProviderId;
    RnrContext->dwNameSpace = lpqsRestrictions->dwNameSpace;
    RnrCtx_Release(RnrContext);

    /* Return the context as a handle */
    *lphLookup = (HANDLE)RnrContext;

    /* Check if this was a TCP, UDP or DNS Query */
    if(RnrId & 0x3000000)
    {
        /* Get the RR Type from the Service ID */
        RnrContext->RrType = RR_FROM_SVCID(ServiceClassId);
    }

    /* Return Success */
    ErrorCode = ERROR_SUCCESS;

Quickie:
    /* Check if we got here through a failure path */
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Set the last error and fail */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Return success */
    return ERROR_SUCCESS;
}

INT
WSPAPI
BuildCsAddr(IN LPWSAQUERYSETW QuerySet,
            IN PFLATBUFF FlatBuffer,
            IN PDNS_BLOB Blob,
            IN DWORD UdpPort,
            IN DWORD TcpPort,
            IN BOOLEAN ReverseLookup)
{
    return WSANO_DATA;
}

INT
WINAPI
Dns_NSPLookupServiceNext(IN HANDLE hLookup,
                         IN DWORD dwControlFlags,
                         IN OUT LPDWORD lpdwBufferLength,
                         OUT LPWSAQUERYSETW lpqsResults)
{
    INT ErrorCode;
    WSAQUERYSETW LocalResults;
    LONG Instance;
    PRNR_CONTEXT RnrContext;
    FLATBUFF FlatBuffer;
    PVOID Name;
    PDNS_BLOB Blob;
    DWORD PortNumber;
    PSERVENT ServEntry = NULL;
    PDNS_ARRAY DnsArray;
    BOOLEAN IsUnicode = TRUE;
    SIZE_T FreeSize;
    ULONG BlobSize;
    ULONG_PTR Position;
    PVOID BlobData;
    ULONG StringLength;
    LPWSTR UnicodeName;

    /* Make sure that the control flags are valid */
    if ((dwControlFlags & ~ALL_LUP_FLAGS) ||
        ((dwControlFlags & (LUP_CONTAINERS | LUP_NOCONTAINERS)) ==
        (LUP_CONTAINERS | LUP_NOCONTAINERS)))
    {
        /* Either non-recognized flags or invalid combos were passed */
        ErrorCode = WSA_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Get the Context */
    RnrContext = RnrCtx_Get(hLookup, dwControlFlags, &Instance);
    if (!RnrContext)
    {
        /* This lookup handle must be invalid */
        SetLastError(WSA_INVALID_HANDLE);
        return SOCKET_ERROR;
    }

    /* Assume success for now */
    SetLastError(NO_ERROR);

    /* Validate the query set size */
    if (*lpdwBufferLength < sizeof(WSAQUERYSETW))
    {
        /* Windows doesn't fail, but sets up a local QS for you... */
        lpqsResults = &LocalResults;
        ErrorCode = WSAEFAULT;
    }

    /* Zero out the buffer and fill out basic data */
    RtlZeroMemory(lpqsResults, sizeof(WSAQUERYSETW));
    lpqsResults->dwNameSpace = NS_DNS;
    lpqsResults->dwSize = sizeof(WSAQUERYSETW);

    /* Initialize the Buffer */
    FlatBuf_Init(&FlatBuffer,
                 lpqsResults + 1,
                 (ULONG)(*lpdwBufferLength - sizeof(WSAQUERYSETW)));

    /* Check if this is an IANA Lookup */
    if(RnrContext->LookupFlags & IANA)
    {
        /* Service Lookup */
        GetServerAndProtocolsFromString(RnrContext->ServiceName,
                                        &HostnameGuid,
                                        &ServEntry);

        /* Get the Port */
        PortNumber = ntohs(ServEntry->s_port);

        /* Use this as the name */
        Name = ServEntry->s_name;
        IsUnicode = FALSE;

        /* Override some parts of the Context and check for TCP/UDP */
        if(!_stricmp("tcp", ServEntry->s_proto))
        {
            /* Set the TCP Guid */
            SET_TCP_SVCID(&RnrContext->lpServiceClassId, PortNumber);
            RnrContext->TcpPort = PortNumber;
            RnrContext->UdpPort = -1;
        }
        else
        {
            /* Set the UDP Guid */
            SET_UDP_SVCID(&RnrContext->lpServiceClassId, PortNumber);
            RnrContext->UdpPort = PortNumber;
            RnrContext->TcpPort = -1;
        }
    } 
    else 
    {
        /* Check if the caller requested for RES_SERVICE */
        if(RnrContext->dwControlFlags & LUP_RES_SERVICE)
        {
            /* Make sure that this is the first instance */
            if (Instance)
            {
                /* Fail */
                ErrorCode = WSA_E_NO_MORE;
                goto Quickie;
            }

            /* Create the blob */
            DnsArray = NULL;
            Blob = SaBlob_CreateFromIp4(RnrContext->ServiceName,
                                        1,
                                        &DnsArray);
        } 
        else if(!(Blob = RnrContext->CachedSaBlob))
        {    
            /* An actual Host Lookup, but we don't have a cached HostEntry yet */
            if (RtlEqualMemory(&RnrContext->lpServiceClassId,
                               &HostnameGuid,
                               sizeof(GUID)) && !(RnrContext->ServiceName))
            {   
                /* Do a Regular DNS Lookup */
                Blob = Rnr_DoHostnameLookup(RnrContext);
            } 
            else if (RnrContext->LookupFlags & REVERSE)
            {
                /* Do a Reverse DNS Lookup */
                Blob = Rnr_GetHostByAddr(RnrContext);
            }
            else 
            {
                /* Do a Hostname Lookup */
                Blob = Rnr_DoDnsLookup(RnrContext);
            }

            /* Check if we got a blob, and cache it */
            if (Blob) RnrContext->CachedSaBlob = Blob;
        }

        /* We should have a blob by now */
        if (!Blob)
        {
            /* We dont, fail */
            if (ErrorCode == NO_ERROR)
            {
                /* Supposedly no error, so find it out */
                ErrorCode = GetLastError();
                if (ErrorCode == NO_ERROR) ErrorCode = WSASERVICE_NOT_FOUND;
            }
            
            /* Fail */
            goto Quickie;
        }
    }

    /* Check if this is the first instance or not */
    if(!RnrContext->Instance) 
    {
        /* It is, get the name from the blob */
        Name = Blob->Name;
    } 
    else 
    {
        /* Only accept this scenario if the caller wanted Aliases */
        if((RnrContext->dwControlFlags & LUP_RETURN_ALIASES) &&
            (Blob->AliasCount > RnrContext->Instance)) 
        {    
            /* Get the name from the Alias */
            Name = Blob->Aliases[RnrContext->Instance];

            /* Let the caller know that this is an Alias */
            lpqsResults->dwOutputFlags |= RESULT_IS_ALIAS;
        }
        else 
        {
            /* Fail */
            ErrorCode = WSA_E_NO_MORE;
            goto Quickie;
        }
    }

    /* Lookups are complete... time to return the right stuff! */
    lpqsResults->dwNameSpace = NS_DNS;

    /* Caller wants the Type back */
    if(RnrContext->dwControlFlags & LUP_RETURN_TYPE)
    {
        /* Copy into the flat buffer and point to it */
        lpqsResults->lpServiceClassId = FlatBuf_CopyMemory(&FlatBuffer,
                                                           &RnrContext->lpServiceClassId,
                                                           sizeof(GUID),
                                                           sizeof(PVOID));
    }

    /* Caller wants the Addreses Back */
    if((RnrContext->dwControlFlags & LUP_RETURN_ADDR) && (Blob))
    {
        /* Build the CS Addr for the caller */
        ErrorCode = BuildCsAddr(lpqsResults,
                                &FlatBuffer,
                                Blob,
                                RnrContext->UdpPort,
                                RnrContext->TcpPort,
                                (RnrContext->LookupFlags & REVERSE) == 1);
    }

    /* Caller wants a Blob */
    if(RnrContext->dwControlFlags & LUP_RETURN_BLOB) 
    {
        /* Save the current size and position */
        FreeSize = FlatBuffer.BufferFreeSize;
        Position = FlatBuffer.BufferPos;

        /* Allocate some space for the Public Blob */
        lpqsResults->lpBlob = FlatBuf_ReserveAlignDword(&FlatBuffer,
                                                        sizeof(BLOB));
                                                        
        /* Check for a Cached Blob */
        if((RnrContext->RrType) && (RnrContext->CachedBlob.pBlobData))
        {
            /* We have a Cached Blob, use it */
            BlobSize = RnrContext->CachedBlob.cbSize;
            BlobData = FlatBuf_ReserveAlignDword(&FlatBuffer, BlobSize);

            /* Copy into the blob */
            RtlCopyMemory(RnrContext->CachedBlob.pBlobData,
                          BlobData,
                          BlobSize);
        } 
        else if (!Blob)
        {
            /* Create an ANSI Host Entry */
            BlobData = SaBlob_CreateHostent(&FlatBuffer.BufferPos,
                                            &FlatBuffer.BufferFreeSize,
                                            &BlobSize,
                                            Blob,
                                            AnsiString,
                                            TRUE,
                                            FALSE);
        }
        else if ((RnrContext->LookupFlags & IANA) && (ServEntry))
        {
            /* Get Servent */ 
            BlobData = CopyServEntry(ServEntry,
                                     &FlatBuffer.BufferPos,
                                     &FlatBuffer.BufferFreeSize,
                                     &BlobSize,
                                     TRUE);

            /* Manually update the buffer (no SaBlob function for servents) */
            FlatBuffer.BufferPos += BlobSize;
            FlatBuffer.BufferFreeSize -= BlobSize;
        }
        else
        {
            /* We have nothing to return! */
            BlobSize = 0;
            lpqsResults->lpBlob = NULL;
            FlatBuffer.BufferPos = Position;
            FlatBuffer.BufferFreeSize = FreeSize;
        }

        /* Make sure we have a blob by here */
        if (Blob)
        {
            /* Set it */
            lpqsResults->lpBlob->pBlobData = BlobData;
            lpqsResults->lpBlob->cbSize = BlobSize;
        }
        else
        {
            /* Set the error code */
            ErrorCode = WSAEFAULT;
        }
    }

    /* Caller wants a name, and we have one */
    if((RnrContext->dwControlFlags & LUP_RETURN_NAME) && (Name))
    {
        /* Check if we have an ANSI name */
        if (!IsUnicode)
        {
            /* Convert it */
            StringLength = 512;
            Dns_StringCopy(&UnicodeName,
                           &StringLength,
                           Name,
                           0,
                           AnsiString,
                           UnicodeString);
        }
        else
        {
            /* Keep the name as is */
            UnicodeName = (LPWSTR)Name;
        }

        /* Write it to the buffer */
        Name = FlatBuf_WriteString(&FlatBuffer, UnicodeName, TRUE);

        /* Return it to the caller */
        lpqsResults->lpszServiceInstanceName = Name;
    }

Quickie:
    /* Check which path got us here */
    if (ErrorCode != NO_ERROR)
    {
        /* Set error */
        SetLastError(ErrorCode);

        /* Check if was a memory error */
        if (ErrorCode == WSAEFAULT)
        {
            /* Update buffer length */
            *lpdwBufferLength -= (DWORD)FlatBuffer.BufferFreeSize;

            /* Decrease an instance */
            RnrCtx_DecInstance(RnrContext);
        }

        /* Set the normalized error code */
        ErrorCode = SOCKET_ERROR;
    }

    /* Release the RnR Context */
    RnrCtx_Release(RnrContext);

    /* Return error code */
    return ErrorCode;
}
