/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2_32/misc/ns.c
 * PURPOSE:     Namespace APIs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */

#include "ws2_32.h"

#include <stdio.h>
#include <stdlib.h>
#include <winnls.h>

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
    DWORD size;
    CHAR buffer[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */
    CHAR *p;

    if (!lpsaAddress) return SOCKET_ERROR;
    if (!lpszAddressString || !lpdwAddressStringLength) return SOCKET_ERROR;

    switch(lpsaAddress->sa_family)
    {
    case AF_INET:
        if (dwAddressLength < sizeof(SOCKADDR_IN)) return SOCKET_ERROR;
        sprintf( buffer, "%u.%u.%u.%u:%u",
               (unsigned int)(ntohl( ((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr ) >> 24 & 0xff),
               (unsigned int)(ntohl( ((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr ) >> 16 & 0xff),
               (unsigned int)(ntohl( ((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr ) >> 8 & 0xff),
               (unsigned int)(ntohl( ((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr ) & 0xff),
               ntohs( ((SOCKADDR_IN *)lpsaAddress)->sin_port ) );

        p = strchr( buffer, ':' );
        if (!((SOCKADDR_IN *)lpsaAddress)->sin_port) *p = 0;
        break;
    default:
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    size = strlen( buffer ) + 1;

    if (*lpdwAddressStringLength <  size)
    {
        *lpdwAddressStringLength = size;
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    *lpdwAddressStringLength = size;
    strcpy( lpszAddressString, buffer );
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
    INT   ret;
    DWORD size;
    WCHAR buffer[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */
    CHAR bufAddr[54];

    size = *lpdwAddressStringLength;
    ret = WSAAddressToStringA(lpsaAddress, dwAddressLength, NULL, bufAddr, &size);

    if (ret) return ret;

    MultiByteToWideChar( CP_ACP, 0, bufAddr, size, buffer, sizeof( buffer )/sizeof(WCHAR));

    if (*lpdwAddressStringLength <  size)
    {
        *lpdwAddressStringLength = size;
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    *lpdwAddressStringLength = size;
    lstrcpyW( lpszAddressString, buffer );
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassA(IN  LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassW(IN  LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceEnd(IN  HANDLE hLookup)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSARemoveServiceClass(IN  LPGUID lpServiceClassId)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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
    WSAPROTOCOL_INFOW ProtoInfoW;

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
        memcpy(&ProtoInfoW,
               lpProtocolInfo,
               FIELD_OFFSET(WSAPROTOCOL_INFOA, szProtocol));

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpProtocolInfo->szProtocol,
                            -1,
                            ProtoInfoW.szProtocol,
                            WSAPROTOCOL_LEN + 1);
    }

    ret = WSAStringToAddressW(szTemp,
                              AddressFamily,
                              &ProtoInfoW,
                              lpAddress,
                              lpAddressLength);

    HeapFree(GetProcessHeap(),
             0,
             szTemp );

    WSASetLastError(ret);
    return ret;
}



/*
 * @implemented
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
    SOCKADDR_IN *sockaddr;

    if (!lpAddressLength || !lpAddress || !AddressString)
    {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    sockaddr = (SOCKADDR_IN *) lpAddress;

    /* Set right adress family */
    if (lpProtocolInfo!=NULL)
        sockaddr->sin_family = lpProtocolInfo->iAddressFamily;
    else
        sockaddr->sin_family = AddressFamily;

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
            // translate ip string to ip

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
        new_he->h_aliases = NULL;
        new_he->h_addrtype = 0; // AF_INET
        new_he->h_length = 0;   // sizeof(in_addr)
        new_he->h_addr_list = HeapAlloc(GlobalHeap,
                                        HEAP_ZERO_MEMORY,
                                        sizeof(char *) * 2);

        *he = new_he;
    }
}

void populate_hostent(struct hostent *he, char* name, IP4_ADDRESS addr)
{
    ASSERT(he);

    //he = HeapAlloc(GlobalHeap, 0, sizeof(struct hostent));
    //he->h_name = HeapAlloc(GlobalHeap, 0, MAX_HOSTNAME_LEN+1);

    strncpy(he->h_name,
            name,
            MAX_HOSTNAME_LEN);

    if( !he->h_aliases ) {
       he->h_aliases = HeapAlloc(GlobalHeap, 0, sizeof(char *));
       he->h_aliases[0] = NULL;
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
                  &addr,
                  sizeof(addr));

    he->h_addr_list[1] = NULL;
}

void free_hostent(struct hostent *he)
{
    int i;

    if (he)
    {
        if (he->h_name)
            HeapFree(GlobalHeap, 0, he->h_name);
        if (he->h_aliases)
        {
            for (i = 0; he->h_aliases[i]; i++)
                HeapFree(GlobalHeap, 0, he->h_aliases[i]);
            HeapFree(GlobalHeap, 0, he->h_aliases);
        }
        if (he->h_addr_list)
        {
            for (i = 0; he->h_addr_list[i]; i++)
                HeapFree(GlobalHeap, 0, he->h_addr_list[i]);
            HeapFree(GlobalHeap, 0, he->h_addr_list);
        }
        HeapFree(GlobalHeap, 0, he);
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
    int i;

    if (s)
    {
        if (s->s_name)
            HeapFree(GlobalHeap, 0, s->s_name);
        if (s->s_aliases)
        {
            for (i = 0; s->s_aliases[i]; i++)
                HeapFree(GlobalHeap, 0, s->s_aliases[i]);
            HeapFree(GlobalHeap, 0, s->s_aliases);
        }
        if (s->s_proto)
            HeapFree(GlobalHeap, 0, s->s_proto);
        HeapFree(GlobalHeap, 0, s);
    }
}

/* This function is far from perfect but it works enough */
static
LPHOSTENT
FindEntryInHosts(IN CONST CHAR FAR* name)
{
    BOOL Found = FALSE;
    HANDLE HostsFile;
    CHAR HostsDBData[BUFSIZ] = { 0 };
    PCHAR SystemDirectory = HostsDBData;
    PCHAR HostsLocation = "\\drivers\\etc\\hosts";
    PCHAR AddressStr, DnsName = NULL, AddrTerm, NameSt, NextLine, ThisLine, Comment;
    UINT SystemDirSize = sizeof(HostsDBData) - 1, ValidData = 0;
    DWORD ReadSize;
    ULONG Address;
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    /* We assume that the parameters are valid */

    if (!GetSystemDirectoryA(SystemDirectory, SystemDirSize))
    {
        WSASetLastError(WSANO_RECOVERY);
        WS_DbgPrint(MIN_TRACE, ("Could not get windows system directory.\n"));
        return NULL; /* Can't get system directory */
    }

    strncat(SystemDirectory,
            HostsLocation,
            SystemDirSize );

    HostsFile = CreateFileA(SystemDirectory,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
    if (HostsFile == INVALID_HANDLE_VALUE)
    {
        WSASetLastError(WSANO_RECOVERY);
        return NULL;
    }

    while(!Found &&
          ReadFile(HostsFile,
                   HostsDBData + ValidData,
                   sizeof(HostsDBData) - ValidData,
                   &ReadSize,
                   NULL))
    {
        ValidData += ReadSize;
        ReadSize = 0;
        NextLine = ThisLine = HostsDBData;

        /* Find the beginning of the next line */
        while(NextLine < HostsDBData + ValidData &&
              *NextLine != '\r' && *NextLine != '\n' )
        {
            NextLine++;
        }

        /* Zero and skip, so we can treat what we have as a string */
        if( NextLine > HostsDBData + ValidData )
            break;

        *NextLine = 0; NextLine++;

        Comment = strchr( ThisLine, '#' );
        if( Comment ) *Comment = 0; /* Terminate at comment start */

        AddressStr = ThisLine;
        /* Find the first space separating the IP address from the DNS name */
        AddrTerm = strchr(ThisLine, ' ');
        if (AddrTerm)
        {
           /* Terminate the address string */
           *AddrTerm = 0;

           /* Find the last space before the DNS name */
           NameSt = strrchr(ThisLine, ' ');

           /* If there is only one space (the one we removed above), then just use the address terminator */
           if (!NameSt)
               NameSt = AddrTerm;

           /* Move from the space to the first character of the DNS name */
           NameSt++;

           DnsName = NameSt;

           if (!strcmp(name, DnsName))
           {
               Found = TRUE;
               break;
           }
        }

        /* Get rid of everything we read so far */
        while( NextLine <= HostsDBData + ValidData &&
               isspace (*NextLine))
        {
            NextLine++;
        }

        if (HostsDBData + ValidData - NextLine <= 0)
            break;

        WS_DbgPrint(MAX_TRACE,("About to move %d chars\n",
                    HostsDBData + ValidData - NextLine));

        memmove(HostsDBData,
                NextLine,
                HostsDBData + ValidData - NextLine );
        ValidData -= NextLine - HostsDBData;
        WS_DbgPrint(MAX_TRACE,("Valid bytes: %d\n", ValidData));
    }

    CloseHandle(HostsFile);

    if (!Found)
    {
        WS_DbgPrint(MAX_TRACE,("Not found\n"));
        WSASetLastError(WSANO_DATA);
        return NULL;
    }

    if (strstr(AddressStr, ":"))
    {
       DbgPrint("AF_INET6 NOT SUPPORTED!\n");
       WSASetLastError(WSAEINVAL);
       return NULL;
    }

    Address = inet_addr(AddressStr);
    if (Address == INADDR_NONE)
    {
        WSASetLastError(WSAEINVAL);
        return NULL;
    }

    populate_hostent(p->Hostent, DnsName, Address);

    return p->Hostent;
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
    PWINSOCK_THREAD_BLOCK p;
    LPHOSTENT Hostent;

    addr = GH_INVALID;

    p = NtCurrentTeb()->WinSockData;

    if (!p || !WSAINITIALIZED)
    {
        WSASetLastError( WSANOTINITIALISED );
        return NULL;
    }

    check_hostent(&p->Hostent);   /*XXX alloc_hostent*/

    /* Hostname NULL - behave like gethostname */
    if(name == NULL)
    {
        ret = gethostname(p->Hostent->h_name, MAX_HOSTNAME_LEN);
        if(ret)
        {
            WSASetLastError( WSAHOST_NOT_FOUND ); //WSANO_DATA  ??
            return NULL;
        }
        return p->Hostent;
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

        /* Look for the DNS name in the hosts file */
        Hostent = FindEntryInHosts(name);
        if (Hostent)
           return Hostent;

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
                populate_hostent(p->Hostent,
                                 (PCHAR)curr->pName,
                                 curr->Data.A.IpAddress);
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

static CHAR *no_aliases = 0;
static PROTOENT protocols[] =
{
    {"icmp",&no_aliases, IPPROTO_ICMP},
    {"tcp", &no_aliases, IPPROTO_TCP},
    {"udp", &no_aliases, IPPROTO_UDP},
    {NULL, NULL, 0}
};

LPPROTOENT
EXPORT
getprotobyname(IN  CONST CHAR FAR* name)
{
    UINT i;
    for (i = 0; protocols[i].p_name; i++)
    {
       if (_stricmp(protocols[i].p_name, name) == 0)
         return &protocols[i];
    }
    return NULL;
}

/*
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobynumber(IN  INT number)
{
    UINT i;
    for (i = 0; protocols[i].p_name; i++)
    {
       if (protocols[i].p_proto == number)
         return &protocols[i];
    }
    return NULL;
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
    ProtocolStr = 0, Comment = 0, EndValid;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = { 0 };
    UINT i,SizeNeeded = 0,
    SystemDirSize = sizeof(ServiceDBData) - 1;
    DWORD ReadSize = 0;
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if (!p || !WSAINITIALIZED)
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
    * We will be share the buffer on the lines. If the line does not fit in
    * the buffer, then moving it to the beginning of the buffer and read
    * the remnants of line from file.
    */

    /* Initial Read */
    ReadFile(ServicesFile,
                   ServiceDBData,
                   sizeof( ServiceDBData ) - 1,
                   &ReadSize, NULL );
    ThisLine = NextLine = ServiceDBData;
    EndValid = ServiceDBData + ReadSize;
    ServiceDBData[sizeof(ServiceDBData) - 1] = '\0';

    while(ReadSize)
    {
        for(; *NextLine != '\r' && *NextLine != '\n'; NextLine++)
        {
            if(NextLine == EndValid)
            {
                int LineLen = NextLine - ThisLine;

                if(ThisLine == ServiceDBData)
                {
                    WS_DbgPrint(MIN_TRACE,("Line too long"));
                    WSASetLastError( WSANO_RECOVERY );
                    return NULL;
                }

                memmove(ServiceDBData, ThisLine, LineLen);

                ReadFile(ServicesFile, ServiceDBData + LineLen,
                         sizeof( ServiceDBData )-1 - LineLen,
                         &ReadSize, NULL );

                EndValid = ServiceDBData + LineLen + ReadSize;
                NextLine = ServiceDBData + LineLen;
                ThisLine = ServiceDBData;

                if(!ReadSize) break;
            }
        }

        *NextLine = '\0';
        Comment = strchr( ThisLine, '#' );
        if( Comment ) *Comment = '\0'; /* Terminate at comment start */

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
        NextLine++;
        ThisLine = NextLine;
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

    memcpy(p->Getservbyname->Aliases,
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

    if( !p || !WSAINITIALIZED)
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
        if( NextLine > ServiceDBData + ValidData )
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

    memcpy(p->Getservbyport->Aliases,Aliases,sizeof(Aliases));

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

    if (!p)
    {
        WSASetLastError(WSAEFAULT);
        return INADDR_NONE;
    }

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
 * @implemented
 */
VOID
EXPORT
freeaddrinfo(struct addrinfo *pAddrInfo)
{
    struct addrinfo *next, *cur;
    cur = pAddrInfo;
    while (cur)
    {
        next = cur->ai_next;
        if (cur->ai_addr)
          HeapFree(GetProcessHeap(), 0, cur->ai_addr);
        if (cur->ai_canonname)
          HeapFree(GetProcessHeap(), 0, cur->ai_canonname);
        HeapFree(GetProcessHeap(), 0, cur);
        cur = next;
    }
}


struct addrinfo *
new_addrinfo(struct addrinfo *prev)
{
    struct addrinfo *ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct addrinfo));
    if (prev)
      prev->ai_next = ret;
    return ret;
}

/*
 * @implemented
 */
INT
EXPORT
getaddrinfo(const char FAR * nodename,
            const char FAR * servname,
            const struct addrinfo FAR * hints,
            struct addrinfo FAR * FAR * res)
{
    struct addrinfo *ret = NULL, *ai;
    ULONG addr;
    USHORT port;
    PCHAR pc;
    struct servent *se;
    char *proto;
    LPPROTOENT pent;
    DNS_STATUS dns_status;
    PDNS_RECORD dp, currdns;
    struct sockaddr_in *sin;
    INT error;

    if (res == NULL)
    {
        error = WSAEINVAL;
        goto End;
    }
    if (nodename == NULL && servname == NULL)
    {
        error = WSAHOST_NOT_FOUND;
        goto End;
    }

    if (!WSAINITIALIZED)
    {
        error = WSANOTINITIALISED;
        goto End;
    }

    if (servname)
    {
        /* converting port number */
        port = strtoul(servname, &pc, 10);
        /* service name was specified? */
        if (*pc != ANSI_NULL)
        {
            /* protocol was specified? */
            if (hints && hints->ai_protocol)
            {
                pent = getprotobynumber(hints->ai_protocol);
                if (pent == NULL)
                {
                  error = WSAEINVAL;
                  goto End;
                }
                proto = pent->p_name;
            }
            else
                proto = NULL;
            se = getservbyname(servname, proto);
            if (se == NULL)
            {
                error = WSATYPE_NOT_FOUND;
                goto End;
            }
            port = se->s_port;
        }
        else
            port = htons(port);
    }
    else
        port = 0;

    if (nodename)
    {
        /* Is it an IPv6 address? */
        if (strstr(nodename, ":"))
        {
            error = WSAHOST_NOT_FOUND;
            goto End;
        }

        /* Is it an IPv4 address? */
        addr = inet_addr(nodename);
        if (addr != INADDR_NONE)
        {
            ai = new_addrinfo(NULL);
            ai->ai_family = PF_INET;
            ai->ai_addrlen = sizeof(struct sockaddr_in);
            ai->ai_addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ai->ai_addrlen);
            sin = (struct sockaddr_in *)ai->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_port = port;
            RtlCopyMemory(&sin->sin_addr, &addr, sizeof(sin->sin_addr));
            if (hints)
            {
                if (ai->ai_socktype == 0)
                    ai->ai_socktype = hints->ai_socktype;
                if (ai->ai_protocol == 0)
                    ai->ai_protocol = hints->ai_protocol;
            }
            ret = ai;
        }
        else
        {
           /* resolving host name */
            dns_status = DnsQuery_A(nodename,
                                    DNS_TYPE_A,
                                    DNS_QUERY_STANDARD,
                                    0,
                                    /* extra dns servers */ &dp,
                                    0);

            if (dns_status == 0)
            {
                ai = NULL;
                for (currdns = dp; currdns; currdns = currdns->pNext )
                {
                    /* accept only A records */
                    if (currdns->wType != DNS_TYPE_A) continue;

                    ai = new_addrinfo(ai);
                    if (ret == NULL)
                      ret = ai;
                    ai->ai_family = PF_INET;
                    ai->ai_addrlen = sizeof(struct sockaddr_in);
                    ai->ai_addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ai->ai_addrlen);
                    sin = (struct sockaddr_in *)ret->ai_addr;
                    sin->sin_family = AF_INET;
                    sin->sin_port = port;
                    RtlCopyMemory(&sin->sin_addr, &currdns->Data.A.IpAddress, sizeof(sin->sin_addr));
                    if (hints)
                    {
                        if (ai->ai_socktype == 0)
                            ai->ai_socktype = hints->ai_socktype;
                        if (ai->ai_protocol == 0)
                            ai->ai_protocol = hints->ai_protocol;
                    }
                }
                DnsRecordListFree(dp, DnsFreeRecordList);
            }
        }
    }
    else
    {
        ai = new_addrinfo(NULL);
        ai->ai_family = PF_INET;
        ai->ai_addrlen = sizeof(struct sockaddr_in);
        ai->ai_addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ai->ai_addrlen);
        sin = (struct sockaddr_in *)ai->ai_addr;
        sin->sin_family = AF_INET;
        sin->sin_port = port;
        if (hints)
        {
            if (!(hints->ai_flags & AI_PASSIVE))
            {
                sin->sin_addr.S_un.S_un_b.s_b1 = 127;
                sin->sin_addr.S_un.S_un_b.s_b2 = 0;
                sin->sin_addr.S_un.S_un_b.s_b3 = 0;
                sin->sin_addr.S_un.S_un_b.s_b4 = 1;
            }
            if (ai->ai_socktype == 0)
                ai->ai_socktype = hints->ai_socktype;
            if (ai->ai_protocol == 0)
                ai->ai_protocol = hints->ai_protocol;
        }
        ret = ai;
    }

    if (ret == NULL)
    {
        error = WSAHOST_NOT_FOUND;
        goto End;
    }

    if (hints && hints->ai_family != PF_UNSPEC && hints->ai_family != PF_INET)
    {
        freeaddrinfo(ret);
        error = WSAEAFNOSUPPORT;
        goto End;
    }

    *res = ret;
    error = 0;

End:
    WSASetLastError(error);
    return error;
}

/* EOF */
