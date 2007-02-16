/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/ns.c
 * PURPOSE:     Namespace APIs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#define __NO_CTYPE_INLINES
#include <ctype.h>
#include <ws2_32.h>
#include <winbase.h>

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif/*BUFSIZ*/

#ifndef MAX_HOSTNAME_LEN
#define MAX_HOSTNAME_LEN 256
#endif

/* Name resolution APIs */

/*
 * @unimplemented
 */
INT
EXPORT
WSAAddressToStringA(IN      LPSOCKADDR lpsaAddress,
                    IN      DWORD dwAddressLength,
                    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
                    OUT     LPSTR lpszAddressString,
                    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAAddressToStringW(IN      LPSOCKADDR lpsaAddress,
                    IN      DWORD dwAddressLength,
                    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
                    OUT     LPWSTR lpszAddressString,
                    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumNameSpaceProvidersA(IN OUT  LPDWORD lpdwBufferLength,
                           OUT     LPWSANAMESPACE_INFOA lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumNameSpaceProvidersW(IN OUT  LPDWORD lpdwBufferLength,
                           OUT     LPWSANAMESPACE_INFOW lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassInfoA(IN      LPGUID lpProviderId,
                        IN      LPGUID lpServiceClassId,
                        IN OUT  LPDWORD lpdwBufferLength,
                        OUT     LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassInfoW(IN      LPGUID lpProviderId,
                        IN      LPGUID lpServiceClassId,
                        IN OUT  LPDWORD lpdwBufferLength,
                        OUT     LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassNameByClassIdA(IN      LPGUID lpServiceClassId,
                                 OUT     LPSTR lpszServiceClassName,
                                 IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassNameByClassIdW(IN      LPGUID lpServiceClassId,
                                 OUT     LPWSTR lpszServiceClassName,
                                 IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassA(IN  LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassW(IN  LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceBeginA(IN  LPWSAQUERYSETA lpqsRestrictions,
                       IN  DWORD dwControlFlags,
                       OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceBeginW(IN  LPWSAQUERYSETW lpqsRestrictions,
                       IN  DWORD dwControlFlags,
                       OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceEnd(IN  HANDLE hLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceNextA(IN      HANDLE hLookup,
                      IN      DWORD dwControlFlags,
                      IN OUT  LPDWORD lpdwBufferLength,
                      OUT     LPWSAQUERYSETA lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceNextW(IN      HANDLE hLookup,
                      IN      DWORD dwControlFlags,
                      IN OUT  LPDWORD lpdwBufferLength,
                      OUT     LPWSAQUERYSETW lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSARemoveServiceClass(IN  LPGUID lpServiceClassId)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASetServiceA(IN  LPWSAQUERYSETA lpqsRegInfo,
               IN  WSAESETSERVICEOP essOperation,
               IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASetServiceW(IN  LPWSAQUERYSETW lpqsRegInfo,
               IN  WSAESETSERVICEOP essOperation,
               IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAStringToAddressA(IN     LPSTR AddressString,
                    IN     INT AddressFamily,
                    IN     LPWSAPROTOCOL_INFOA lpProtocolInfo,
                    OUT    LPSOCKADDR lpAddress,
                    IN OUT LPINT lpAddressLength)
{
    INT ret, len;
    LPWSTR szTemp;
    LPWSAPROTOCOL_INFOW lpProtoInfoW = NULL;

    len = MultiByteToWideChar(CP_ACP,
                              0,
                              AddressString,
                              -1,
                              NULL,
                              0);

    szTemp = HeapAlloc(GetProcessHeap(),
                       0,
                       len * sizeof(WCHAR));

    MultiByteToWideChar(CP_ACP,
                        0,
                        AddressString,
                        -1,
                        szTemp,
                        len);

    if (lpProtocolInfo)
    {
        len =   WSAPROTOCOL_LEN+1;
        lpProtoInfoW = HeapAlloc(GetProcessHeap(),
                                 0,
                                 len * sizeof(WCHAR) );

        memcpy(lpProtoInfoW,
               lpProtocolInfo,
               sizeof(LPWSAPROTOCOL_INFOA));

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpProtocolInfo->szProtocol,
                            -1,
                            lpProtoInfoW->szProtocol,
                            len);
    }

    ret = WSAStringToAddressW(szTemp,
                              AddressFamily,
                              lpProtoInfoW,
                              lpAddress,
                              lpAddressLength);

    HeapFree(GetProcessHeap(),
             0,
             szTemp );

    if (lpProtocolInfo)
        HeapFree(GetProcessHeap(),
                 0,
                 lpProtoInfoW);

    WSASetLastError(ret);
        return ret;
}



/*
 * @implement
 */
INT
EXPORT
WSAStringToAddressW(IN      LPWSTR AddressString,
                    IN      INT AddressFamily,
                    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
                    OUT     LPSOCKADDR lpAddress,
                    IN OUT  LPINT lpAddressLength)
{
    int pos=0;
    int res=0;
    LONG inetaddr = 0;
    LPWSTR *bp=NULL;

    SOCKADDR_IN *sockaddr = (SOCKADDR_IN *) lpAddress;

    if (!lpAddressLength || !lpAddress)
        return SOCKET_ERROR;

    if (AddressString==NULL)
        return WSAEINVAL;

    /* Set right adress family */
    if (lpProtocolInfo!=NULL)
       sockaddr->sin_family = lpProtocolInfo->iAddressFamily;

    else sockaddr->sin_family = AddressFamily;

    /* Report size */
    if (AddressFamily == AF_INET)
    {
        if (*lpAddressLength < (INT)sizeof(SOCKADDR_IN))
        {
            *lpAddressLength = sizeof(SOCKADDR_IN);
            res = WSAEFAULT;
        }
        else
        {
            if (!lpAddress)
                res = WSAEINVAL;
            else
            {
                // translate now ip string to ip

                /* rest sockaddr.sin_addr.s_addr
                   for we need to be sure it is zero when we come to while */
                memset(lpAddress,0,sizeof(SOCKADDR_IN));

                /* Set right adress family */
                sockaddr->sin_family = AF_INET;

                /* Get port number */
                pos = wcscspn(AddressString,L":") + 1;

                if (pos < (int)wcslen(AddressString))
                    sockaddr->sin_port = wcstol(&AddressString[pos],
                                                bp,
                                                10);

                else
                    sockaddr->sin_port = 0;

                /* Get ip number */
                pos=0;
                inetaddr=0;

                while (pos < (int)wcslen(AddressString))
                {
                    inetaddr = (inetaddr<<8) + ((UCHAR)wcstol(&AddressString[pos],
                                                              bp,
                                                              10));
                    pos += wcscspn( &AddressString[pos],L".") +1 ;
                }

                res = 0;
                sockaddr->sin_addr.s_addr = inetaddr;
            }

        }
    }

    WSASetLastError(res);
    if (!res) return 0;
        return SOCKET_ERROR;
}

void check_hostent(struct hostent **he)
{
    struct hostent *new_he;

    WS_DbgPrint(MID_TRACE,("*he: %x\n",*he));

    if(!*he)
    {
        new_he = HeapAlloc(GlobalHeap,
                           0,
                           sizeof(struct hostent) + MAX_HOSTNAME_LEN + 1);

        new_he->h_name = (PCHAR)(new_he + 1);
        new_he->h_aliases = 0;
        new_he->h_addrtype = 0; // AF_INET
        new_he->h_length = 0;   // sizeof(in_addr)
        new_he->h_addr_list = HeapAlloc(GlobalHeap,
                                        0,
                                        sizeof(char *) * 2);

        RtlZeroMemory(new_he->h_addr_list,
                      sizeof(char *) * 2);
        *he = new_he;
    }
}

void populate_hostent(struct hostent *he, char* name, DNS_A_DATA addr)
{
    ASSERT(he);

    //he = HeapAlloc(GlobalHeap, 0, sizeof(struct hostent));
    //he->h_name = HeapAlloc(GlobalHeap, 0, MAX_HOSTNAME_LEN+1);

    strncpy(he->h_name,
            name,
            MAX_HOSTNAME_LEN);

    if( !he->h_aliases ) {
       he->h_aliases = HeapAlloc(GlobalHeap, 0, sizeof(char *));
       he->h_aliases[0] = 0;
    }
    he->h_addrtype = AF_INET;
    he->h_length = sizeof(IN_ADDR); //sizeof(struct in_addr);

    if( he->h_addr_list[0] )
    {
        HeapFree(GlobalHeap,
                 0,
                 he->h_addr_list[0]);
    }

    he->h_addr_list[0] = HeapAlloc(GlobalHeap,
                                   0,
                                   MAX_HOSTNAME_LEN + 1);

    WS_DbgPrint(MID_TRACE,("he->h_addr_list[0] %x\n", he->h_addr_list[0]));

    RtlCopyMemory(he->h_addr_list[0],
                  (char*)&addr.IpAddress,
                  sizeof(addr.IpAddress));

    he->h_addr_list[1] = 0;
}


#define HFREE(x) if(x) { HeapFree(GlobalHeap, 0, (x)); x=0; }
void free_hostent(struct hostent *he)
{
    if(he)
    {
       char *next = 0;
        HFREE(he->h_name);
        if(he->h_aliases) 
       { 
           next = he->h_aliases[0];
           while(next) { HFREE(next); next++; }
       }
       if(he->h_addr_list)
       {
           next = he->h_addr_list[0];
           while(next) { HFREE(next); next++; }
       }
        HFREE(he->h_addr_list);
       HFREE(he->h_aliases);
        HFREE(he);
    }
}

/* WinSock 1.1 compatible name resolution APIs */

/*
 * @unimplemented
 */
LPHOSTENT
EXPORT
gethostbyaddr(IN  CONST CHAR FAR* addr,
              IN  INT len,
              IN  INT type)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}

/*
  Assumes rfc 1123 - adam *
   addr[1] = 0;
    addr[0] = inet_addr(name);
    strcpy( hostname, name );
    if(addr[0] == 0xffffffff) return NULL;
    he.h_addr_list = (void *)addr;
    he.h_name = hostname;
    he.h_aliases = NULL;
    he.h_addrtype = AF_INET;
    he.h_length = sizeof(addr);
    return &he;

<RANT>
From the MSDN Platform SDK: Windows Sockets 2
"The gethostbyname function cannot resolve IP address strings passed to it.
Such a request is treated exactly as if an unknown host name were passed."
</RANT>

Defferring to the the documented behaviour, rather than the unix behaviour
What if the hostname is in the HOSTS file? see getservbyname

 * @implemented
 */

/* DnsQuery -- lib/dnsapi/dnsapi/query.c */
   /* see ws2_32.h, winsock2.h*/
    /*getnetworkparameters - iphlp api */
/*
REFERENCES

servent -- w32api/include/winsock2.h
PWINSOCK_THREAD_BLOCK -- ws2_32.h
dllmain.c -- threadlocal memory allocation / deallocation
lib/dnsapi


*/
      /* lib/adns/src/adns.h XXX */


/*
struct  hostent {
        char    *h_name;
        char    **h_aliases;
        short   h_addrtype;
        short   h_length;
        char    **h_addr_list;
#define h_addr h_addr_list[0]
};
struct  servent {
        char    *s_name;
        char    **s_aliases;
        short   s_port;
        char    *s_proto;
};


struct hostent defined in w32api/include/winsock2.h
*/

void free_servent(struct servent* s)
{
    HFREE(s->s_name);
    char* next = s->s_aliases[0];
    while(next) { HFREE(next); next++; }
    s->s_port = 0;
    HFREE(s->s_proto);
    HFREE(s);
}



LPHOSTENT
EXPORT
gethostbyname(IN  CONST CHAR FAR* name)
{
    enum addr_type
    {
        GH_INVALID,
        GH_IPV6,
        GH_IPV4,
        GH_RFC1123_DNS
    };
    typedef enum addr_type addr_type;
    addr_type addr;
    int ret = 0;
    char* found = 0;
    DNS_STATUS dns_status = {0};
    /* include/WinDNS.h -- look up DNS_RECORD on MSDN */
    PDNS_RECORD dp = 0;

    addr = GH_INVALID;

    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if( !p )
    {
        WSASetLastError( WSANOTINITIALISED );
        return NULL;
    }

    check_hostent(&p->Hostent);   /*XXX alloc_hostent*/

    /* Hostname NULL - behave like gethostname */
    if(name == NULL)
    {
        ret = gethostname(p->Hostent->h_name, MAX_HOSTNAME_LEN);
        return p->Hostent;
    }

    if(ret)
    {
        WSASetLastError( WSAHOST_NOT_FOUND ); //WSANO_DATA  ??
        return NULL;
    }

    /* Is it an IPv6 address? */
    found = strstr(name, ":");
    if( found != NULL )
    {
        addr = GH_IPV6;
        goto act;
    }

    /* Is it an IPv4 address? */
    if (!isalpha(name[0]))
    {
        addr = GH_IPV4;
        goto act;
    }

    addr = GH_RFC1123_DNS;

 /* Broken out in case we want to get fancy later */
 act:
    switch(addr)
    {
        case GH_IPV6:
            WSASetLastError(STATUS_NOT_IMPLEMENTED);
            return NULL;
        break;

        case GH_INVALID:
            WSASetLastError(WSAEFAULT);
            return NULL;
        break;

        /* Note: If passed an IP address, MSDN says that gethostbyname()
                 treats it as an unknown host.
           This is different from the unix implementation. Use inet_addr()
        */
        case GH_IPV4:
        case GH_RFC1123_DNS:
        /* DNS_TYPE_A: include/WinDNS.h */
        /* DnsQuery -- lib/dnsapi/dnsapi/query.c */
        dns_status = DnsQuery_A(name,
                                DNS_TYPE_A,
                                DNS_QUERY_STANDARD,
                                0,
                                /* extra dns servers */ &dp,
                                0);

        if(dns_status == 0)
        {
            //ASSERT(dp->wType == DNS_TYPE_A);
            //ASSERT(dp->wDataLength == sizeof(DNS_A_DATA));
            PDNS_RECORD curr;
            for(curr=dp;
                curr != NULL && curr->wType != DNS_TYPE_A;
                curr = curr->pNext )
            {
                WS_DbgPrint(MID_TRACE,("wType: %i\n", curr->wType));
                /*empty */
            }

            if(curr)
            {
                WS_DbgPrint(MID_TRACE,("populating hostent\n"));
                WS_DbgPrint(MID_TRACE,("pName is (%s)\n", curr->pName));
                populate_hostent(p->Hostent, (PCHAR)curr->pName, curr->Data.A);
                DnsRecordListFree(dp, DnsFreeRecordList);
                return p->Hostent;
            }
            else
            {
                DnsRecordListFree(dp, DnsFreeRecordList);
            }
        }

        WS_DbgPrint(MID_TRACE,("Called DnsQuery, but host not found. Err: %i\n",
                    dns_status));
        WSASetLastError(WSAHOST_NOT_FOUND);
        return NULL;

        break;

        default:
            WSASetLastError(WSANO_RECOVERY);
            return NULL;
        break;
    }

    WSASetLastError(WSANO_RECOVERY);
    return NULL;
}

/*
 * @implemented
 */
INT
EXPORT
gethostname(OUT CHAR FAR* name,
            IN  INT namelen)
{
    DWORD size = namelen;

    int ret = GetComputerNameExA(ComputerNameDnsHostname,
                                 name,
                                 &size);
    if(ret == 0)
    {
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }
    else
    {
        name[namelen-1] = '\0';
        return 0;
    }
}


/*
 * XXX arty -- Partial implementation pending a better one.  This one will
 * do for normal purposes.#include <ws2_32.h>
 *
 * Return the address of a static LPPROTOENT corresponding to the named
 * protocol.  These structs aren't very interesting, so I'm not too ashamed
 * to have this function work on builtins for now.
 *
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobyname(IN  CONST CHAR FAR* name)
{
    static CHAR *udp_aliases = 0;
    static PROTOENT udp = { "udp", &udp_aliases, 17 };
    static CHAR *tcp_aliases = 0;
    static PROTOENT tcp = { "tcp", &tcp_aliases, 6 };

    if(!_stricmp(name, "udp"))
    {
        return &udp;
    }
    else if (!_stricmp( name, "tcp"))
    {
        return &tcp;
    }

    return 0;
}

/*
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobynumber(IN  INT number)
{
    UNIMPLEMENTED

    return (LPPROTOENT)NULL;
}

#define SKIPWS(ptr,act) \
{while(*ptr && isspace(*ptr)) ptr++; if(!*ptr) act;}
#define SKIPANDMARKSTR(ptr,act) \
{while(*ptr && !isspace(*ptr)) ptr++; \
 if(!*ptr) {act;} else { *ptr = 0; ptr++; }}


static BOOL
DecodeServEntFromString(IN  PCHAR ServiceString,
                        OUT PCHAR *ServiceName,
                        OUT PCHAR *PortNumberStr,
                        OUT PCHAR *ProtocolStr,
                        IN  PCHAR *Aliases,
                        IN  DWORD MaxAlias)
{
    UINT NAliases = 0;

    WS_DbgPrint(MAX_TRACE, ("Parsing service ent [%s]\n", ServiceString));

    SKIPWS(ServiceString, return FALSE);
    *ServiceName = ServiceString;
    SKIPANDMARKSTR(ServiceString, return FALSE);
    SKIPWS(ServiceString, return FALSE);
    *PortNumberStr = ServiceString;
    SKIPANDMARKSTR(ServiceString, ;);

    while( *ServiceString && NAliases < MaxAlias - 1 )
    {
        SKIPWS(ServiceString, break);
        if( *ServiceString )
        {
            SKIPANDMARKSTR(ServiceString, ;);
            if( strlen(ServiceString) )
            {
                WS_DbgPrint(MAX_TRACE, ("Alias: %s\n", ServiceString));
                *Aliases++ = ServiceString;
                NAliases++;
            }
        }
    }
    *Aliases = NULL;

    *ProtocolStr = strchr(*PortNumberStr,'/');
    if( !*ProtocolStr ) return FALSE;
    **ProtocolStr = 0; (*ProtocolStr)++;

    WS_DbgPrint(MAX_TRACE, ("Parsing done: %s %s %s %d\n",
                *ServiceName, *ProtocolStr, *PortNumberStr,
                NAliases));

    return TRUE;
}

#define ADJ_PTR(p,b1,b2) p = (p - b1) + b2

/*
 * @implemented
 */
LPSERVENT
EXPORT
getservbyname(IN  CONST CHAR FAR* name,
              IN  CONST CHAR FAR* proto)
{
    BOOL  Found = FALSE;
    HANDLE ServicesFile;
    CHAR ServiceDBData[BUFSIZ] = { 0 };
    PCHAR SystemDirectory = ServiceDBData; /* Reuse this stack space */
    PCHAR ServicesFileLocation = "\\drivers\\etc\\services";
    PCHAR ThisLine = 0, NextLine = 0, ServiceName = 0, PortNumberStr = 0,
    ProtocolStr = 0, Comment = 0;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = { 0 };
    UINT i,SizeNeeded = 0,
    SystemDirSize = sizeof(ServiceDBData) - 1;
    DWORD ReadSize = 0, ValidData = 0;
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if( !p )
    {
        WSASetLastError( WSANOTINITIALISED );
        return NULL;
    }

    if( !name )
    {
        WSASetLastError( WSANO_RECOVERY );
        return NULL;
    }

    if( !GetSystemDirectoryA( SystemDirectory, SystemDirSize ) )
    {
        WSASetLastError( WSANO_RECOVERY );
        WS_DbgPrint(MIN_TRACE, ("Could not get windows system directory.\n"));
        return NULL; /* Can't get system directory */
    }

    strncat(SystemDirectory,
            ServicesFileLocation,
            SystemDirSize );

    ServicesFile = CreateFileA(SystemDirectory,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL );

    if( ServicesFile == INVALID_HANDLE_VALUE )
    {
        WSASetLastError( WSANO_RECOVERY );
        return NULL;
    }

    /* Scan the services file ...
     *
     * We will read up to BUFSIZ bytes per pass, until the buffer does not
     * contain a full line, then we will try to read more.
     *
     * We fall from the loop if the buffer does not have a line terminator.
     */

    /* Initial Read */
    while(!Found &&
          ReadFile(ServicesFile,
                   ServiceDBData + ValidData,
                   sizeof( ServiceDBData ) - ValidData,
                   &ReadSize,
                   NULL))
    {
        ValidData += ReadSize;
        ReadSize = 0;
        NextLine = ThisLine = ServiceDBData;

        /* Find the beginning of the next line */
        while(NextLine < ServiceDBData + ValidData &&
              *NextLine != '\r' && *NextLine != '\n' )
        {
            NextLine++;
        }

        /* Zero and skip, so we can treat what we have as a string */
        if( NextLine >= ServiceDBData + ValidData )
            break;

        *NextLine = 0; NextLine++;

        Comment = strchr( ThisLine, '#' );
        if( Comment ) *Comment = 0; /* Terminate at comment start */

        if(DecodeServEntFromString(ThisLine,
                                   &ServiceName,
                                   &PortNumberStr,
                                   &ProtocolStr,
                                   Aliases,
                                   WS2_INTERNAL_MAX_ALIAS) &&
           !strcmp( ServiceName, name ) &&
           (proto ? !strcmp( ProtocolStr, proto ) : TRUE) )
        {

            WS_DbgPrint(MAX_TRACE,("Found the service entry.\n"));
            Found = TRUE;
            SizeNeeded = sizeof(WINSOCK_GETSERVBYNAME_CACHE) +
                (NextLine - ThisLine);
            break;
        }

        /* Get rid of everything we read so far */
        while( NextLine <= ServiceDBData + ValidData &&
               isspace( *NextLine ) )
        {
            NextLine++;
        }

        WS_DbgPrint(MAX_TRACE,("About to move %d chars\n",
                    ServiceDBData + ValidData - NextLine));

        memmove(ServiceDBData,
                NextLine,
                ServiceDBData + ValidData - NextLine );
        ValidData -= NextLine - ServiceDBData;
        WS_DbgPrint(MAX_TRACE,("Valid bytes: %d\n", ValidData));
    }

    /* This we'll do no matter what */
    CloseHandle( ServicesFile );

    if( !Found )
    {
        WS_DbgPrint(MAX_TRACE,("Not found\n"));
        WSASetLastError( WSANO_DATA );
        return NULL;
    }

    if( !p->Getservbyname || p->Getservbyname->Size < SizeNeeded )
    {
        /* Free previous getservbyname buffer, allocate bigger */
        if( p->Getservbyname )
            HeapFree(GlobalHeap, 0, p->Getservbyname);
        p->Getservbyname = HeapAlloc(GlobalHeap, 0, SizeNeeded);
        if( !p->Getservbyname )
        {
            WS_DbgPrint(MIN_TRACE,("Couldn't allocate %d bytes\n",
                        SizeNeeded));
            WSASetLastError( WSATRY_AGAIN );
            return NULL;
        }
        p->Getservbyname->Size = SizeNeeded;
    }

    /* Copy the data */
    memmove(p->Getservbyname->Data,
            ThisLine,
            NextLine - ThisLine );

    ADJ_PTR(ServiceName,ThisLine,p->Getservbyname->Data);
    ADJ_PTR(ProtocolStr,ThisLine,p->Getservbyname->Data);
    WS_DbgPrint(MAX_TRACE, ("ServiceName: %s, Protocol: %s\n",
                ServiceName,
                ProtocolStr));

    for( i = 0; Aliases[i]; i++ )
    {
        ADJ_PTR(Aliases[i],ThisLine,p->Getservbyname->Data);
        WS_DbgPrint(MAX_TRACE,("Aliase %d: %s\n", i, Aliases[i]));
    }

    memcpy(p->Getservbyname,
           Aliases,
           sizeof(Aliases));

    /* Create the struct proper */
    p->Getservbyname->ServerEntry.s_name = ServiceName;
    p->Getservbyname->ServerEntry.s_aliases = p->Getservbyname->Aliases;
    p->Getservbyname->ServerEntry.s_port = htons(atoi(PortNumberStr));
    p->Getservbyname->ServerEntry.s_proto = ProtocolStr;

    return &p->Getservbyname->ServerEntry;
}


/*
 * @implemented
 */
LPSERVENT
EXPORT
getservbyport(IN  INT port,
              IN  CONST CHAR FAR* proto)
{
    BOOL  Found = FALSE;
    HANDLE ServicesFile;
    CHAR ServiceDBData[BUFSIZ] = { 0 };
    PCHAR SystemDirectory = ServiceDBData; /* Reuse this stack space */
    PCHAR ServicesFileLocation = "\\drivers\\etc\\services";
    PCHAR ThisLine = 0, NextLine = 0, ServiceName = 0, PortNumberStr = 0,
    ProtocolStr = 0, Comment = 0;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = { 0 };
    UINT i,SizeNeeded = 0,
    SystemDirSize = sizeof(ServiceDBData) - 1;
    DWORD ReadSize = 0, ValidData = 0;
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if( !p )
    {
        WSASetLastError( WSANOTINITIALISED );
        return NULL;
    }

    if ( !port )
    {
        WSASetLastError( WSANO_RECOVERY );
        return NULL;
    }

    if( !GetSystemDirectoryA( SystemDirectory, SystemDirSize ) )
    {
        WSASetLastError( WSANO_RECOVERY );
        WS_DbgPrint(MIN_TRACE, ("Could not get windows system directory.\n"));
        return NULL; /* Can't get system directory */
    }

    strncat(SystemDirectory,
            ServicesFileLocation,
            SystemDirSize );

    ServicesFile = CreateFileA(SystemDirectory,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL );

    if( ServicesFile == INVALID_HANDLE_VALUE )
    {
        WSASetLastError( WSANO_RECOVERY );
        return NULL;
    }

    /* Scan the services file ...
     *
     * We will read up to BUFSIZ bytes per pass, until the buffer does not
     * contain a full line, then we will try to read more.
     *
     * We fall from the loop if the buffer does not have a line terminator.
     */

    /* Initial Read */
    while(!Found &&
          ReadFile(ServicesFile,
                   ServiceDBData + ValidData,
                   sizeof( ServiceDBData ) - ValidData,
                   &ReadSize, NULL ) )
    {
        ValidData += ReadSize;
        ReadSize = 0;
        NextLine = ThisLine = ServiceDBData;

        /* Find the beginning of the next line */
        while( NextLine < ServiceDBData + ValidData &&
               *NextLine != '\r' && *NextLine != '\n' ) NextLine++;

        /* Zero and skip, so we can treat what we have as a string */
        if( NextLine >= ServiceDBData + ValidData )
            break;

        *NextLine = 0; NextLine++;

        Comment = strchr( ThisLine, '#' );
        if( Comment ) *Comment = 0; /* Terminate at comment start */

        if(DecodeServEntFromString(ThisLine,
                                   &ServiceName,
                                   &PortNumberStr,
                                   &ProtocolStr,
                                   Aliases,
                                   WS2_INTERNAL_MAX_ALIAS ) &&
           (htons(atoi( PortNumberStr )) == port ) &&
           (proto ? !strcmp( ProtocolStr, proto ) : TRUE) )
        {

            WS_DbgPrint(MAX_TRACE,("Found the port entry.\n"));

            Found = TRUE;
            SizeNeeded = sizeof(WINSOCK_GETSERVBYPORT_CACHE) +
            (NextLine - ThisLine);
            break;
        }

        /* Get rid of everything we read so far */
        while( NextLine <= ServiceDBData + ValidData &&
               isspace( *NextLine ) )
        {
            NextLine++;
        }

        WS_DbgPrint(MAX_TRACE,("About to move %d chars\n",
                    ServiceDBData + ValidData - NextLine));

        memmove(ServiceDBData,
                NextLine,
                ServiceDBData + ValidData - NextLine );
        ValidData -= NextLine - ServiceDBData;
        WS_DbgPrint(MAX_TRACE,("Valid bytes: %d\n", ValidData));
    }

    /* This we'll do no matter what */
    CloseHandle( ServicesFile );

    if( !Found )
    {
        WS_DbgPrint(MAX_TRACE,("Not found\n"));
        WSASetLastError( WSANO_DATA );
        return NULL;
    }

    if( !p->Getservbyport || p->Getservbyport->Size < SizeNeeded )
    {
        /* Free previous getservbyport buffer, allocate bigger */
        if( p->Getservbyport )
            HeapFree(GlobalHeap, 0, p->Getservbyport);
        p->Getservbyport = HeapAlloc(GlobalHeap,
                                     0,
                                     SizeNeeded);
        if( !p->Getservbyport )
        {
            WS_DbgPrint(MIN_TRACE,("Couldn't allocate %d bytes\n",
                        SizeNeeded));
            WSASetLastError( WSATRY_AGAIN );
            return NULL;
        }
        p->Getservbyport->Size = SizeNeeded;
    }
    /* Copy the data */
    memmove(p->Getservbyport->Data,
            ThisLine,
            NextLine - ThisLine );

    ADJ_PTR(PortNumberStr,ThisLine,p->Getservbyport->Data);
    ADJ_PTR(ProtocolStr,ThisLine,p->Getservbyport->Data);
    WS_DbgPrint(MAX_TRACE, ("Port Number: %s, Protocol: %s\n",
                PortNumberStr, ProtocolStr));

    for( i = 0; Aliases[i]; i++ )
    {
        ADJ_PTR(Aliases[i],ThisLine,p->Getservbyport->Data);
        WS_DbgPrint(MAX_TRACE,("Aliases %d: %s\n", i, Aliases[i]));
    }

    memcpy(p->Getservbyport,Aliases,sizeof(Aliases));

    /* Create the struct proper */
    p->Getservbyport->ServerEntry.s_name = ServiceName;
    p->Getservbyport->ServerEntry.s_aliases = p->Getservbyport->Aliases;
    p->Getservbyport->ServerEntry.s_port = port;
    p->Getservbyport->ServerEntry.s_proto = ProtocolStr;

    WS_DbgPrint(MID_TRACE,("s_name: %s\n", ServiceName));

    return &p->Getservbyport->ServerEntry;

}


/*
 * @implemented
 */
ULONG
EXPORT
inet_addr(IN  CONST CHAR FAR* cp)
/*
 * FUNCTION: Converts a string containing an IPv4 address to an unsigned long
 * ARGUMENTS:
 *     cp = Pointer to string with address to convert
 * RETURNS:
 *     Binary representation of IPv4 address, or INADDR_NONE
 */
{
    UINT i;
    PCHAR p;
    ULONG u = 0;

    p = (PCHAR)cp;

    if (strlen(p) == 0)
        return INADDR_NONE;

    if (strcmp(p, " ") == 0)
        return 0;

    for (i = 0; i <= 3; i++)
    {
        u += (strtoul(p, &p, 0) << (i * 8));

        if (strlen(p) == 0)
            return u;

        if (p[0] != '.')
            return INADDR_NONE;

        p++;
    }

    return u;
}


/*
 * @implemented
 */
CHAR FAR*
EXPORT
inet_ntoa(IN  IN_ADDR in)
{
    CHAR b[10];
    PCHAR p;

    p = ((PWINSOCK_THREAD_BLOCK)NtCurrentTeb()->WinSockData)->Intoa;
    _itoa(in.S_un.S_addr & 0xFF, b, 10);
    strcpy(p, b);
    _itoa((in.S_un.S_addr >> 8) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 16) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 24) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);

    return (CHAR FAR*)p;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetHostByAddr(IN  HWND hWnd,
                      IN  UINT wMsg,
                      IN  CONST CHAR FAR* addr,
                      IN  INT len,
                      IN  INT type,
                      OUT CHAR FAR* buf,
                      IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetHostByName(IN  HWND hWnd,
                      IN  UINT wMsg,
                      IN  CONST CHAR FAR* name,
                      OUT CHAR FAR* buf,
                      IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetProtoByName(IN  HWND hWnd,
                       IN  UINT wMsg,
                       IN  CONST CHAR FAR* name,
                       OUT CHAR FAR* buf,
                       IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetProtoByNumber(IN  HWND hWnd,
                         IN  UINT wMsg,
                         IN  INT number,
                         OUT CHAR FAR* buf,
                         IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}

/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetServByName(IN  HWND hWnd,
                      IN  UINT wMsg,
                      IN  CONST CHAR FAR* name,
                      IN  CONST CHAR FAR* proto,
                      OUT CHAR FAR* buf,
                      IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetServByPort(IN  HWND hWnd,
                      IN  UINT wMsg,
                      IN  INT port,
                      IN  CONST CHAR FAR* proto,
                      OUT CHAR FAR* buf,
                      IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}

/* EOF */

