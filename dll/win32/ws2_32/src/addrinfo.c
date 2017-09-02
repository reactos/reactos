/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/addrinfo.c
 * PURPOSE:     Protocol-Independent Address Resolution
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#include <ws2tcpip.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *******************************************************************/

#define Swap(a, b, c)       { (c) = (a); (a) = (b); (b) = (c); }
#define FreeAddrInfoW(a)    freeaddrinfo((LPADDRINFO)a)

/* FUNCTIONS *****************************************************************/

/* FIXME: put into dnslib */
VOID
WSAAPI
Dns_Ip4AddressToReverseName_W(IN LPWSTR AddressBuffer,
                              IN IN_ADDR Address)
{
    /* Convert the address into IPv4 format */
    wsprintfW(AddressBuffer, L"%u.%u.%u.%u.in-addr.arpa.",
              Address.S_un.S_un_b.s_b4,
              Address.S_un.S_un_b.s_b3,
              Address.S_un.S_un_b.s_b2,
              Address.S_un.S_un_b.s_b1);
}

VOID
WINAPI
Dns_SplitHostFromDomainNameW(IN LPWSTR DomainName)
{
    /* FIXME */
}

static
INT
WINAPI
ConvertAddrinfoFromUnicodeToAnsi(IN PADDRINFOW Addrinfo)
{
    LPSTR AnsiName;
    LPWSTR *UnicodeName;

    /* Make sure we have a valid pointer */
    if (Addrinfo)
    {
        do
        {
            /* Get the name */
            UnicodeName = &Addrinfo->ai_canonname;

            /* Check if it exists */
            if (*UnicodeName)
            {
                /* Convert it */
                AnsiName = AnsiDupFromUnicode(*UnicodeName);
                if (AnsiName)
                {
                    /* Free the old one */
                    HeapFree(WsSockHeap, 0, *UnicodeName);

                    /* Set the new one */
                    *UnicodeName = (LPWSTR)AnsiName;
                }
                else
                {
                    return GetLastError();
                }
            }
        } while ((Addrinfo = Addrinfo->ai_next));
    }

    /* All done */
    return ERROR_SUCCESS;
}

static
BOOL
WINAPI
ParseV4Address(IN PCWSTR AddressString,
               OUT PDWORD pAddress)
{
    CHAR AnsiAddressString[MAX_HOSTNAME_LEN];
    CHAR * cp = AnsiAddressString;
    DWORD val, base;
    unsigned char c;
    DWORD parts[4], *pp = parts;
    if (!AddressString)
        return FALSE;
    WideCharToMultiByte(CP_ACP,
                        0,
                        AddressString,
                        -1,
                        AnsiAddressString,
                        sizeof(AnsiAddressString),
                        NULL,
                        0);
    if (!isdigit(*cp)) return FALSE;

again:
    /*
    * Collect number up to ``.''.
    * Values are specified as for C:
    * 0x=hex, 0=octal, other=decimal.
    */
    val = 0; base = 10;
    if (*cp == '0') {
        if (*++cp == 'x' || *cp == 'X')
            base = 16, cp++;
        else
            base = 8;
    }
    while ((c = *cp)) {
        if (isdigit(c)) {
            val = (val * base) + (c - '0');
            cp++;
            continue;
        }
        if (base == 16 && isxdigit(c)) {
            val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
            cp++;
            continue;
        }
        break;
    }
    if (*cp == '.') {
        /*
        * Internet format:
        *    a.b.c.d
        */
        if (pp >= parts + 4) return FALSE;
        *pp++ = val;
        cp++;
        goto again;
    }
    /*
    * Check for trailing characters.
    */
    if (*cp) return FALSE;

    if (pp >= parts + 4) return FALSE;
    *pp++ = val;
    /*
    * Concoct the address according to
    * the number of parts specified.
    */
    if ((DWORD)(pp - parts) != 4) return FALSE;
    if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xff || parts[3] > 0xff) return FALSE;
    val = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];

    if (pAddress)
        *pAddress = htonl(val);

    return TRUE;
}

static
PADDRINFOW
WINAPI
NewAddrInfo(IN INT SocketType,
            IN INT Protocol,
            IN WORD Port,
            IN DWORD Address)
{
    PADDRINFOW AddrInfo;
    PSOCKADDR_IN SockAddress;

    /* Allocate a structure */
    AddrInfo = HeapAlloc(WsSockHeap, 0, sizeof(ADDRINFOW));
    if (!AddrInfo) return NULL;

    /* Allocate a sockaddr */
    SockAddress = HeapAlloc(WsSockHeap, 0, sizeof(SOCKADDR_IN));
    if (!SockAddress)
    {
        /* Free the addrinfo and fail */
        HeapFree(WsSockHeap, 0, AddrInfo);
        return NULL;
    }

    /* Write data for socket address */
    SockAddress->sin_family = AF_INET;
    SockAddress->sin_port = Port;
    SockAddress->sin_addr.s_addr = Address;
    ZeroMemory(SockAddress->sin_zero, sizeof(SockAddress->sin_zero));

    /* Fill out the addrinfo */
    AddrInfo->ai_family = PF_INET;
    AddrInfo->ai_socktype = SocketType;
    AddrInfo->ai_protocol = Protocol;
    AddrInfo->ai_flags = 0;
    AddrInfo->ai_next = 0;
    AddrInfo->ai_canonname = NULL;
    AddrInfo->ai_addrlen = sizeof(SOCKADDR_IN);
    AddrInfo->ai_addr = (PSOCKADDR)SockAddress;

    /* Return it */
    return AddrInfo;
}

static
INT
WINAPI
CloneAddrInfo(IN WORD Port,
              IN PADDRINFOW ptResult)
{
    PADDRINFOW Next = NULL;
    PADDRINFOW New  = NULL;

    /* Loop the chain */
    for (Next = ptResult; Next;)
    {
        /* Create a new structure */
        New = NewAddrInfo(SOCK_DGRAM,
                          Next->ai_protocol,
                          Port,
                          ((PSOCKADDR_IN)Next->ai_addr)->sin_addr.s_addr);
        if (!New) break;

        /* Link them */
        New->ai_next = Next->ai_next;
        Next->ai_next = New;
        Next = New->ai_next;
    }

    /* Check if we ran out of memory */
    if (Next) return EAI_MEMORY;

    /* Return success */
    return 0;
}

static
INT
WINAPI
QueryDNS(IN LPCSTR NodeName,
         IN INT SocketType,
         IN INT Protocol,
         IN WORD Port,
         OUT CHAR Alias[NI_MAXHOST],
         OUT PADDRINFOW *pptResult)
{
    PADDRINFOW *Next = pptResult;
    PHOSTENT Hostent = NULL;
    PCHAR *Addresses;

    /* Assume nothing found */
    *Next = NULL;
    Alias[0] = '\0';

    /* Get the hostent */
    Hostent = gethostbyname(NodeName);
    if (Hostent)
    {
        /* Check for valid addresses */
        if ((Hostent->h_addrtype == AF_INET) &&
            (Hostent->h_length == sizeof(IN_ADDR)))
        {
            /* Loop every address in it */
            for (Addresses = Hostent->h_addr_list; *Addresses; Addresses++)
            {
                /* Create an addrinfo structure for it*/
                *Next = NewAddrInfo(SocketType,
                                    Protocol,
                                    Port,
                                    ((PIN_ADDR)*Addresses)->s_addr);
                if (!*Next) return EAI_MEMORY;

                /* Move to the next entry */
                Next = &((*Next)->ai_next);
            }
        }

        /* Copy the canonical name */
        strcpy(Alias, Hostent->h_name);

        /* Return success */
        return 0;
    }

    /* Find out what the error was */
    switch (GetLastError())
    {
        /* Convert the Winsock Error to an EAI error */
        case WSAHOST_NOT_FOUND: return EAI_NONAME;
        case WSATRY_AGAIN: return EAI_AGAIN;
        case WSANO_RECOVERY: return EAI_FAIL;
        case WSANO_DATA: return EAI_NODATA;
        default: return EAI_NONAME;
    }
}

static
INT
WINAPI
LookupNodeByAddr(IN LPWSTR pNodeBuffer,
                 IN DWORD NodeBufferSize,
                 IN BOOLEAN OnlyNodeName,
                 IN PVOID Addr,
                 IN DWORD AddrSize,
                 IN INT AddressFamily)
{
    GUID LookupGuid = SVCID_DNS_TYPE_PTR;
    PIN_ADDR Ip4Addr = Addr;
    WCHAR ReverseBuffer[76];
    WSAQUERYSETW Restrictions, Reply;
    DWORD BufferLength;
    INT ErrorCode;
    HANDLE LookupHandle;

    /* Validate the address */
    if (!Addr) return WSAEFAULT;

    /* Make sure the family and address size match */
    if (AddressFamily == AF_INET6)
    {
        /* Check the address size for this type */
        if (AddrSize != sizeof(IN6_ADDR)) return WSAEFAULT;
        Ip4Addr = (PIN_ADDR)&((PIN6_ADDR)Addr)->u.Byte[12];
    }
    else if (AddressFamily == AF_INET)
    {
        /* Check the address size for this type */
        if (AddrSize != sizeof(IN_ADDR)) return WSAEFAULT;
    }
    else
    {
        /* Address family not supported */
        return WSAEAFNOSUPPORT;
    }

    /* Check if this is a mapped V4 IPv6 or pure IPv4 */
    if (((AddressFamily == AF_INET6) && (IN6_IS_ADDR_V4MAPPED(Addr))) ||
        (AddressFamily == AF_INET))
    {
        /* Get the reverse name */
        Dns_Ip4AddressToReverseName_W(ReverseBuffer, *Ip4Addr);
    }
    /* FIXME: Not implemented for now
    else if ( */

    /* By this point we have the Reverse Name, so prepare for lookup */
    RtlZeroMemory(&Restrictions, sizeof(Restrictions));
    Restrictions.dwSize = sizeof(Restrictions);
    Restrictions.lpszServiceInstanceName = ReverseBuffer;
    Restrictions.lpServiceClassId = &LookupGuid;
    Restrictions.dwNameSpace = NS_DNS;

    /* Now do the lookup */
    ErrorCode = WSALookupServiceBeginW(&Restrictions,
                                       LUP_RETURN_NAME,
                                       &LookupHandle);
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Lookup successful, now get the data */
        BufferLength = (NI_MAXHOST - 1) * sizeof(WCHAR) + sizeof(Restrictions);
        ErrorCode = WSALookupServiceNextW(LookupHandle,
                                          0,
                                          &BufferLength,
                                          &Restrictions);
        if (ErrorCode == ERROR_SUCCESS)
        {
            /* Now check if we have a name back */
            Reply = Restrictions;
            if (!Reply.lpszServiceInstanceName)
            {
                /* Fail */
                ErrorCode = WSAHOST_NOT_FOUND;
            }
            else
            {
                /* Check if the caller only wants the node name */
                if (OnlyNodeName)
                {
                    /* Split it and get only the partial host name */
                    Dns_SplitHostFromDomainNameW(Reply.lpszServiceInstanceName);
                }

                /* Check the length and see if it's within our limit */
                if (wcslen(Reply.lpszServiceInstanceName) + 1 >
                    NodeBufferSize)
                {
                    /* It's not, so fail */
                    ErrorCode = WSAEFAULT;
                }
                else
                {
                    /* It will fit, let's copy it*/
                    wcscpy(pNodeBuffer, Reply.lpszServiceInstanceName);
                }
            }
        }
    }
    else if (ErrorCode == WSASERVICE_NOT_FOUND)
    {
        /* Normalize the error code */
        ErrorCode = WSAHOST_NOT_FOUND;
    }

    /* Finish the lookup if one was in progress */
    if (LookupHandle) WSALookupServiceEnd(LookupHandle);

    /* Return the error code */
    return ErrorCode;
}

static
INT
WINAPI
GetServiceNameForPort(IN LPWSTR pServiceBuffer,
                      IN DWORD ServiceBufferSize,
                      IN WORD Port,
                      IN DWORD Flags)
{
    return ERROR_SUCCESS;
}

static
INT
WINAPI
LookupAddressForName(IN LPCSTR NodeName,
                     IN INT SocketType,
                     IN INT Protocol,
                     IN WORD Port,
                     IN BOOL bAI_CANONNAME,
                     OUT PADDRINFOW *pptResult)
{
    INT iError = 0;
    INT AliasCount = 0;
    CHAR szFQDN1[NI_MAXHOST] = "";
    CHAR szFQDN2[NI_MAXHOST] = "";
    PCHAR Name = szFQDN1;
    PCHAR Alias = szFQDN2;
    PCHAR Scratch = NULL;

    /* Make a copy of the name */
    strcpy(Name, NodeName);

    /* Loop */
    while (TRUE)
    {
        /* Do a DNS Query for the name */
        iError = QueryDNS(NodeName,
                          SocketType,
                          Protocol,
                          Port,
                          Alias,
                          pptResult);
        if (iError) break;

        /* Exit if we have a result */
        if (*pptResult) break;

        /* Don't loop continuously if this is a DNS misconfiguration */
        if ((!strlen(Alias)) || (!strcmp(Name, Alias)) || (++AliasCount == 16))
        {
            /* Exit the loop with a failure */
            iError = EAI_FAIL;
            break;
        }

        /* Restart loopup if we got a CNAME */
        Swap(Name, Alias, Scratch);
    }

    /* Check if we succeeded and the canonical name is requested */
    if (!iError && bAI_CANONNAME)
    {
        /* Allocate memory for a copy */
        (*pptResult)->ai_canonname = HeapAlloc(WsSockHeap, 0, 512);

        /* Check if we had enough memory */
        if (!(*pptResult)->ai_canonname)
        {
            /* Set failure code */
            iError = EAI_MEMORY;
        }
        else
        {
            /* Convert the alias to UNICODE */
            MultiByteToWideChar(CP_ACP,
                                0,
                                Alias,
                                -1,
                                (*pptResult)->ai_canonname,
                                256);
        }
    }

    /* Return to caller */
    return iError;
}

/*
 * @implemented
 */
INT
WSAAPI
GetAddrInfoW(IN PCWSTR pszNodeName,
             IN PCWSTR pszServiceName,
             IN const ADDRINFOW *ptHints,
             OUT PADDRINFOW *pptResult)
{
    INT iError = 0;
    INT iFlags = 0;
    INT iFamily = PF_UNSPEC;
    INT iSocketType = 0;
    INT iProtocol = 0;
    WORD wPort = 0;
    DWORD dwAddress = 0;
    PSERVENT ptService = NULL;
    PCHAR pc = NULL;
    BOOL bClone = FALSE;
    WORD wTcpPort = 0;
    WORD wUdpPort = 0;
    WCHAR CanonicalName[0x42];
    CHAR AnsiServiceName[256];
    CHAR AnsiNodeName[256];
    DPRINT("GetAddrInfoW: %S, %S, %p, %p\n", pszNodeName, pszServiceName, ptHints, pptResult);

    /* Assume error */
    *pptResult  = NULL;

    /* We must have at least one name to work with */
    if (!(pszNodeName) && !(pszServiceName))
    {
        /* Fail */
        SetLastError(EAI_NONAME);
        return EAI_NONAME;
    }

    /* Check if we got hints */
    if (ptHints)
    {
        /* Make sure these are empty */
        if ((ptHints->ai_addrlen) ||
            (ptHints->ai_canonname) ||
            (ptHints->ai_addr) ||
            (ptHints->ai_next))
        {
            /* Fail if they aren't */
            SetLastError(EAI_FAIL);
            return EAI_FAIL;
        }

        /* Save the flags and validate them */
        iFlags = ptHints->ai_flags;
        if ((iFlags & AI_CANONNAME) && !pszNodeName)
        {
            SetLastError(EAI_BADFLAGS);
            return EAI_BADFLAGS;
        }

        /* Save family and validate it */
        iFamily = ptHints->ai_family;
        if ((iFamily != PF_UNSPEC) && (iFamily != PF_INET))
        {
            SetLastError(EAI_FAMILY);
            return EAI_FAMILY;
        }

        /* Save socket type and validate it */
        iSocketType = ptHints->ai_socktype;
        if ((iSocketType != 0) &&
            (iSocketType != SOCK_STREAM) &&
            (iSocketType != SOCK_DGRAM) &&
            (iSocketType != SOCK_RAW))
        {
            SetLastError(EAI_SOCKTYPE);
            return EAI_SOCKTYPE;
        }

        /* Save the protocol */
        iProtocol = ptHints->ai_protocol;
    }

    /* Check if we have a service name */
    if (pszServiceName)
    {
        /* We need to convert it to ANSI */
        WideCharToMultiByte(CP_ACP,
                            0,
                            pszServiceName,
                            -1,
                            AnsiServiceName,
                            sizeof(AnsiServiceName),
                            NULL,
                            0);

        /* Get the port */
        wPort = (WORD)strtoul(AnsiServiceName, &pc, 10);

        /* Check if the port string is numeric */
        if (*pc == '\0')
        {
            /* Get the port directly */
            wPort = wTcpPort = wUdpPort = htons(wPort);

#if 0
            /* Check if this is both TCP and UDP */
            if (iSocketType == 0)
            {
                /* Set it to TCP for now, but remember to clone */
                bClone = TRUE;
                iSocketType = SOCK_STREAM;
            }
#endif
        }
        else
        {
            wPort = 0;
            /* The port name was a string. Check if this is a UDP socket */
            if ((iSocketType == 0) || (iSocketType == SOCK_DGRAM))
            {
                /* It's UDP, do a getservbyname */
                ptService = getservbyname(AnsiServiceName, "udp");

                /* If we got a servent, return the port from it */
                if (ptService) wPort = wUdpPort = ptService->s_port;
            }

            /* Check if this is a TCP socket */
            if ((iSocketType == 0) || (iSocketType == SOCK_STREAM))
            {
                /* It's TCP, do a getserbyname */
                ptService = getservbyname(AnsiServiceName, "tcp");

                /* Return the port from the servent */
                if (ptService) wPort = wTcpPort = ptService->s_port;
            }

            /* If we got 0, then fail */
            if (wPort == 0)
            {
                SetLastError(EAI_SERVICE);
                return EAI_SERVICE;
            }

            /* Check if this was for both */
            if (iSocketType == 0)
            {
                /* Do the TCP case right now */
                if (wTcpPort && !wUdpPort)
                    iSocketType = SOCK_STREAM;
                if (!wTcpPort && wUdpPort)
                    iSocketType = SOCK_DGRAM;
                //bClone = (wTcpPort && wUdpPort);
            }
        }
    }

    /* Check if no node was given or if this is is a valid IPv4 address */
    if ((!pszNodeName) || (ParseV4Address(pszNodeName, &dwAddress)))
    {
        /* Check if we don't have a node name */
        if (!pszNodeName)
        {
            /* Make one up based on the flags */
            dwAddress = htonl((iFlags & AI_PASSIVE) ?
                              INADDR_ANY : INADDR_LOOPBACK);
        }

        /* Create the Addr Info */
        *pptResult = NewAddrInfo(iSocketType, iProtocol, wPort, dwAddress);

        /* If we didn't get one back, assume out of memory */
        if (!(*pptResult)) iError = EAI_MEMORY;

        /* Check if we have success and a nodename */
        if (!iError && pszNodeName)
        {
            /* Set AI_NUMERICHOST since this is a numeric string */
            (*pptResult)->ai_flags |= AI_NUMERICHOST;

            /* Check if the canonical name was requested */
            if (iFlags & AI_CANONNAME)
            {
                /* Get the canonical name */
                GetNameInfoW((*pptResult)->ai_addr,
                             (socklen_t)(*pptResult)->ai_addrlen,
                             CanonicalName,
                             0x41,
                             NULL,
                             0,
                             2);

                /* Allocate memory for a copy */
                (*pptResult)->ai_canonname = HeapAlloc(WsSockHeap,
                                                       0,
                                                       wcslen(CanonicalName));

                if (!(*pptResult)->ai_canonname)
                {
                    /* No memory for the copy */
                    iError = EAI_MEMORY;
                }
                else
                {
                    /* Duplicate the string */
                    RtlMoveMemory((*pptResult)->ai_canonname,
                                  CanonicalName,
                                  wcslen(CanonicalName));
                }
            }
        }
    }
    else if (iFlags & AI_NUMERICHOST)
    {
        /* No name for this request (we have a non-numeric name) */
        iError = EAI_NONAME;
    }
    else
    {
        /* We need to convert it to ANSI */
        WideCharToMultiByte(CP_ACP,
                            0,
                            pszNodeName,
                            -1,
                            AnsiNodeName,
                            sizeof(AnsiNodeName),
                            NULL,
                            0);

        /* Non-numeric name, do DNS lookup */
        iError = LookupAddressForName(AnsiNodeName,
                                      iSocketType,
                                      iProtocol,
                                      wPort,
                                      (iFlags & AI_CANONNAME),
                                      pptResult);
    }

    /* If all was good and the caller requested UDP and TCP */
    if (!iError && bClone)
    {
        /* Process UDP now, we already did TCP */
        iError = CloneAddrInfo(wUdpPort, *pptResult);
    }

    /* If we've hit an error till here */
    if (iError)
    {
        /* Free the address info and return nothing */
        FreeAddrInfoW(*pptResult);
        *pptResult = NULL;
    }

    /* Return to caller */
    SetLastError(iError);
    return iError;
}

#undef freeaddrinfo
/*
 * @implemented
 */
VOID
WINAPI
freeaddrinfo(PADDRINFOA AddrInfo)
{
    PADDRINFOA NextInfo;

    /* Loop the chain of structures */
    for (NextInfo = AddrInfo; NextInfo; NextInfo = AddrInfo)
    {
        /* Check if there is a canonical name */
        if (NextInfo->ai_canonname)
        {
            /* Free it */
            HeapFree(WsSockHeap, 0, NextInfo->ai_canonname);
        }

        /* Check if there is an address */
        if (NextInfo->ai_addr)
        {
            /* Free it */
            HeapFree(WsSockHeap, 0, NextInfo->ai_addr);
        }

        /* Move to the next entry */
        AddrInfo = NextInfo->ai_next;

        /* Free this entry */
        HeapFree(WsSockHeap, 0, NextInfo);
    }
}

#undef getaddrinfo
/*
 * @implemented
 */
INT
WINAPI
getaddrinfo(const char FAR *nodename,
            const char FAR *servname,
            const struct addrinfo FAR *hints,
            struct addrinfo FAR * FAR *res)
{
    INT ErrorCode;
    LPWSTR UnicodeNodeName = NULL;
    LPWSTR UnicodeServName = NULL;
    DPRINT("getaddrinfo: %s, %s, %p, %p\n", nodename, servname, hints, res);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) != ERROR_SUCCESS) return ErrorCode;

    /* Convert the node name */
    if (nodename)
    {
        UnicodeNodeName = UnicodeDupFromAnsi(nodename);
        if (!UnicodeNodeName)
        {
            /* Prepare to fail */
            ErrorCode = GetLastError();
            goto Quickie;
        }
    }

    /* Convert the servname too, if we have one */
    if (servname)
    {
        UnicodeServName = UnicodeDupFromAnsi(servname);
        if (!UnicodeServName)
        {
            /* Prepare to fail */
            ErrorCode = GetLastError();
            goto Quickie;
        }
    }

    /* Now call the unicode function */
    ErrorCode = GetAddrInfoW(UnicodeNodeName,
                             UnicodeServName,
                             (PADDRINFOW)hints,
                             (PADDRINFOW*)res);

    /* Convert it to ANSI if we succeeded */
    if (ErrorCode == ERROR_SUCCESS) ConvertAddrinfoFromUnicodeToAnsi((PADDRINFOW)*res);

Quickie:
    /* Check if we have a unicode node name and serv name */
    if (UnicodeNodeName) HeapFree(WsSockHeap, 0, UnicodeNodeName);
    if (UnicodeServName) HeapFree(WsSockHeap, 0, UnicodeServName);

    /* Check if we are in error */
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Free the structure and return nothing */
        freeaddrinfo(*res);
        *res = NULL;
    }

    /* Set the last error and return */
    SetLastError(ErrorCode);
    return ErrorCode;
}

/*
 * @implemented
 */
INT
WSAAPI
GetNameInfoW(IN CONST SOCKADDR *pSockaddr,
             IN socklen_t SockaddrLength,
             OUT PWCHAR pNodeBuffer,
             IN DWORD NodeBufferSize,
             OUT PWCHAR pServiceBuffer,
             IN DWORD ServiceBufferSize,
             IN INT Flags)
{
    DWORD AddressLength, AddrSize;
    PVOID Addr;
    SOCKADDR_IN6 Address;
    INT ErrorCode = ERROR_SUCCESS;

    DPRINT("GetNameInfoW: %p, %d, %p, %ld, %p, %ld, %d\n",
           pSockaddr,
           SockaddrLength,
           pNodeBuffer,
           NodeBufferSize,
           pServiceBuffer,
           ServiceBufferSize,
           Flags);

    /* Check for valid socket */
    if (!pSockaddr)
        return WSAEFAULT;

    /* Check which family this is */
    if (pSockaddr->sa_family == AF_INET)
    {
        /* IPv4 */
        AddressLength = sizeof(SOCKADDR_IN);
        Addr = &((PSOCKADDR_IN)pSockaddr)->sin_addr;
        AddrSize = sizeof(IN_ADDR);
    }
    else if (pSockaddr->sa_family == AF_INET6)
    {
        /* IPv6 */
        AddressLength = sizeof(SOCKADDR_IN6);
        Addr = &((PSOCKADDR_IN6)pSockaddr)->sin6_addr;
        AddrSize = sizeof(IN6_ADDR);
    }
    else
    {
        /* Unsupported family */
        SetLastError(EAI_FAMILY);
        return EAI_FAMILY;
    }

    /* Check for valid socket address length */
    if ((DWORD)SockaddrLength < AddressLength)
        return WSAEFAULT;

    /* Check if we have a node name */
    if (pNodeBuffer)
    {
        /* Check if only the numeric host is wanted */
        if (!(Flags & NI_NUMERICHOST))
        {
            /* Do the lookup by addr */
            ErrorCode = LookupNodeByAddr(pNodeBuffer,
                                         NodeBufferSize,
                                         Flags & NI_NOFQDN,
                                         Addr,
                                         AddrSize,
                                         pSockaddr->sa_family);
            /* Check if we failed */
            if (ErrorCode != ERROR_SUCCESS)
            {
                /* Fail if the caller REALLY wants the NAME itself? */
                if (Flags & NI_NAMEREQD) goto quickie;
            }
            else
            {
                /* We succeeded, no need to get the numeric address */
                goto SkipNumeric;
            }
        }

        /* Copy the address */
        RtlMoveMemory(&Address, pSockaddr, AddressLength);

        /* Get the numeric address */
        if (pSockaddr->sa_family == AF_INET)
        {
            /* IPv4 */
            ((PSOCKADDR_IN)&Address)->sin_port = 0;
        }
        else if (pSockaddr->sa_family == AF_INET6)
        {
            /* IPv6 */
            ((PSOCKADDR_IN6)&Address)->sin6_port = 0;
        }
        ErrorCode = WSAAddressToStringW((LPSOCKADDR)&Address,
                                        AddressLength,
                                        NULL,
                                        pNodeBuffer,
                                        &NodeBufferSize);
        if (ErrorCode == SOCKET_ERROR)
        {
            /* Get the error code and exit with it */
            ErrorCode = GetLastError();
            goto quickie;
        }
    }

SkipNumeric:
    /* Check if we got a service name */
    if (pServiceBuffer)
    {
        /* Handle this request */
        ErrorCode = GetServiceNameForPort(pServiceBuffer,
                                          ServiceBufferSize,
                                          ((PSOCKADDR_IN)pSockaddr)->sin_port,
                                          Flags);
    }

    /* Set the error and return it (or success) */
quickie:
    SetLastError(ErrorCode);
    return ErrorCode;
}

#undef getnameinfo
/*
 * @implemented
 */
INT
WINAPI
getnameinfo(const struct sockaddr FAR *sa,
            socklen_t salen,
            char FAR *host,
            DWORD hostlen,
            char FAR *serv,
            DWORD servlen,
            INT flags)
{
    INT ErrorCode;
    WCHAR Buffer[256];
    WCHAR ServiceBuffer[17];
    DWORD HostLength = 0, ServLength = 0;
    PWCHAR ServiceString = NULL, HostString = NULL;
    DPRINT("getnameinfo: %p, %p, %p, %lx\n", host, serv, sa, salen);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) != ERROR_SUCCESS) return ErrorCode;

    /* Check if we have a host pointer */
    if (host)
    {
        /* Setup the data for it */
        HostString = Buffer;
        HostLength = sizeof(Buffer) / sizeof(WCHAR);
    }

    /* Check if we have a service pointer */
    if (serv)
    {
        /* Setup the data for it */
        ServiceString = ServiceBuffer;
        ServLength = sizeof(ServiceBuffer) / sizeof(WCHAR);
    }

    /* Now call the unicode function */
    ErrorCode = GetNameInfoW(sa,
                             salen,
                             HostString,
                             HostLength,
                             ServiceString,
                             ServLength,
                             flags);

    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Check if we had a host pointer */
        if (HostString)
        {
            /* Convert it back to ANSI */
            ErrorCode = WideCharToMultiByte(CP_ACP,
                                            0,
                                            HostString,
                                            -1,
                                            host,
                                            hostlen,
                                            NULL,
                                            NULL);
            if (!ErrorCode) goto Quickie;
        }

        /* Check if we have a service pointer */
        if (ServiceString)
        {
            /* Convert it back to ANSI */
            ErrorCode = WideCharToMultiByte(CP_ACP,
                                            0,
                                            ServiceString,
                                            -1,
                                            serv,
                                            servlen,
                                            NULL,
                                            NULL);
            if (!ErrorCode) goto Quickie;
        }

        /* Return success */
        return ERROR_SUCCESS;
    }

    /* Set the last error and return */
Quickie:
    SetLastError(ErrorCode);
    return ErrorCode;
}
