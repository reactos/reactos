/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    gethost.c

Abstract:

    This module implements hostname -> IP address resolution routines.

Author:

    David Treadwell (davidtr)

Revision History:

    Glenn Curtis (glennc)   Integrated code with DNS Caching Resolver
                            Service.

--*/

/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  GETHOST.C
 *
 ******************************************************************/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.gethost.c
 *      @(#)gethost.c   5.3
 *
 *      Last delta created      14:09:38 3/4/91
 *      This file extracted     11:20:08 3/8/91
 *
 *      Modifications:
 *
 *              GSS     24 Jul 90       New File
 */
/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gethostnamadr.c     6.39 (Berkeley) 1/4/90";
#endif /* LIBC_SCCS and not lint */
/***************************************************************************/

#include "winsockp.h"
#include <nbtioctl.h>
#include <nb30.h>
#include <nspapi.h>
#include <svcguid.h>
#include <nspmisc.h>

#define  SDK_DNS_RECORD     //  DNS API gets SDK defs
#include <dnsapi.h>

#include <ws2atm.h>

#define ENABLE_DEBUG_LOGGING 1
#include "logit.h"

extern GUID  HostnameGuid;

#define DONBTFIRST 1

#define h_addr_ptrs       ACCESS_THREAD_DATA( h_addr_ptrs, GETHOST )
#define h_addr_ptrsBigBuf ACCESS_THREAD_DATA( h_addr_ptrsBigBuf, GETHOST )
#define host              ACCESS_THREAD_DATA( host, GETHOST )
#define host_aliases      ACCESS_THREAD_DATA( host_aliases, GETHOST )
#define hostbuf           ACCESS_THREAD_DATA( hostbuf, GETHOST )
#define host_addr         ACCESS_THREAD_DATA( host_addr, GETHOST )
#define HOSTDB            ACCESS_THREAD_DATA( HOSTDB, GETHOST )
#define hostf             ACCESS_THREAD_DATA( hostf, GETHOST )
#define hostaddr          ACCESS_THREAD_DATA( hostaddr, GETHOST )
#define hostaddrBigBuf    ACCESS_THREAD_DATA( hostaddrBigBuf, GETHOST )
#define host_addrs        ACCESS_THREAD_DATA( host_addrs, GETHOST )
#define stayopen          ACCESS_THREAD_DATA( stayopen, GETHOST )

char TCPIPLINK[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Linkage";
char TCPLINK[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcp\\Linkage";
char REGBASE[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";

//
// Globals for caching open NBT control channel handles. See the description
// of SockOpenNbt() for details.
//

DWORD SockNbtHandleCount = 0;
PHANDLE SockNbtHandles = NULL;
PHANDLE SockSparseNbtHandles = NULL;
DWORD SockNbtDeviceCount = 0;
PWSTR SockNbtDeviceNames = NULL;
BOOLEAN SockNbtFullyInitialized = FALSE;

BOOL
SockNbtResolveAddr (
    IN ULONG IpAddr,
    IN PCHAR Name
    );

DWORD
SockOpenNbt (
    PHANDLE * pphSockNbt
    );

PHANDLE
CopyNbtHandles (
    PHANDLE phSockNbt,
    DWORD   HandleCount
    );

struct hostent *
_pgethostbyaddr(
    IN  const char *addr,
    IN  int   len,
    IN  int   type,
    IN  DWORD dwControlFlags,
    IN  DWORD NbtFirst,
    OUT PBOOL pfUnicodeQuery
    );

int
_pgethostname(
    OUT char * addr,
    IN int len
    );

DWORD
DnsStatusToWS2Error (
    IN  DNS_STATUS DnsStatus
    );

struct hostent *
_pTryReverseLookup_Nbt (
    IN  ULONG address
    );

struct hostent *
_pTryReverseLookup_Dns (
    IN DWORD  dwControlFlags,
    IN LPWSTR name,
    IN ULONG  address,
    IN int    type,
    IN int    len
    );

typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;

struct hostent *
_pgethostbyaddr(
    IN  const char *addr,
    IN  int   len,
    IN  int   type,
    IN  DWORD dwControlFlags,
    IN  DWORD NbtFirst,
    OUT PBOOL pfUnicodeQuery
    )
/*++
Routine Description:
    Internal form of above
--*/
{
    register struct hostent *hp;
    WCHAR qbuf[MAXDNAME];
    DWORD dwNbtFirst = NbtFirst & DONBTFIRST;
    extern struct hostent *_gethtbyaddr();

    WS_ENTER( "GETXBYYSP_gethostbyaddr", (PVOID)addr, UIntToPtr( len ), UIntToPtr( type ), NULL );
    WS2LOG_F1( "gethost.c - _pgethostbyaddr" );

    if ( !SockEnterApi( FALSE, TRUE, TRUE ) ) {
        WS_EXIT( "GETXBYYSP_gethostbyaddr", 0, TRUE );
        return NULL;
    }

    if (type != AF_INET) {
        SockThreadProcessingGetXByY = FALSE;
        SetLastError(WSANO_RECOVERY);
        WS_EXIT( "GETXBYYSP_gethostbyaddr", 0, TRUE );
        return ((struct hostent *) NULL);
    }

    if ( dwControlFlags & LUP_RETURN_BLOB )
    {
        (void)wsprintfA( (LPSTR) qbuf, "%u.%u.%u.%u.in-addr.arpa.",
                ((unsigned)addr[3] & 0xff),
                ((unsigned)addr[2] & 0xff),
                ((unsigned)addr[1] & 0xff),
                ((unsigned)addr[0] & 0xff) );
    }
    else
    {
        (void)wsprintfW( qbuf, L"%u.%u.%u.%u.in-addr.arpa.",
                ((unsigned)addr[3] & 0xff),
                ((unsigned)addr[2] & 0xff),
                ((unsigned)addr[1] & 0xff),
                ((unsigned)addr[0] & 0xff) );
    }

    //
    // Next, try the hosts file.
    //

    IF_DEBUG(GETXBYY)
    {
        WS_PRINT(("GETXBYYSP_gethostbyaddr trying HOST\n"));
    }

    hp = _gethtbyaddr( dwControlFlags,
                       qbuf,
                       addr,
                       len,
                       type,
                       pfUnicodeQuery);

    if (hp != NULL)
    {
        IF_DEBUG(GETXBYY)
        {
            WS_PRINT(("GETXBYYSP_gethostbyaddr HOST successful\n"));
        }

        SockThreadProcessingGetXByY = FALSE;
        WS_EXIT( "GETXBYYSP_gethostbyaddr", (INT)(LONG_PTR)hp, FALSE );
        return (hp);
    }

    do
    {
        if(dwNbtFirst & DONBTFIRST)
        {
            //
            // Try NBT.
            //
            hp = _pTryReverseLookup_Nbt( *(PULONG)addr );

            if ( hp )
            {
                *pfUnicodeQuery = FALSE;
            }
        }
        else
        {
            //
            // Now try DNS to query DNS for the information.
            //
            hp = _pTryReverseLookup_Dns( dwControlFlags,
                                         qbuf,
                                         *(PULONG)addr,
                                         type,
                                         len );

            if ( hp )
            {
                if ( dwControlFlags & LUP_RETURN_BLOB )
                {
                    *pfUnicodeQuery = FALSE;
                }
                else
                {
                    *pfUnicodeQuery = TRUE;
                }
            }
        }
    } while( !hp &&
             ( ( dwNbtFirst ^= DONBTFIRST ) !=
               ( NbtFirst & DONBTFIRST ) ) );

    if( hp )
        return( hp );

    IF_DEBUG(GETXBYY)
    {
        WS_PRINT(("GETXBYYSP_gethostbyaddr unsuccessful\n"));
    }

    SockThreadProcessingGetXByY = FALSE;
    SetLastError( WSANO_DATA );
    WS_EXIT( "GETXBYYSP_gethostbyaddr", 0, TRUE );
    return ((struct hostent *) NULL);
}



void
_sethtent (
    IN int f
    )
{
    WS2LOG_F1( "gethost.c - _sethtent" );

    if (hostf == NULL) {
        hostf = SockOpenNetworkDataBase("hosts", HOSTDB, HOSTDB_SIZE, "r");

    } else {
        rewind(hostf);
    }
    stayopen |= f;

} // _sethtent


void
_endhtent (
    void
    )
{
    WS2LOG_F1( "gethost.c - _endhtent" );

    if (hostf && !stayopen) {
        (void) fclose(hostf);
        hostf = NULL;
    }

} // _endhtent


struct hostent *
_gethtent (
    void
    )
{
    char *p;
    register char *cp, **q;

    WS2LOG_F1( "gethost.c - _gethtent" );

    if (hostf == NULL && (hostf = fopen(HOSTDB, "rt" )) == NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("\tERROR: cannot open hosts database file %s\n",
                      HOSTDB));
        }
        return (NULL);
    }

again:
    if ((p = fgets(hostbuf, BUFSIZ, hostf)) == NULL) {
        return (NULL);
    }

    if (*p == '#') {
        goto again;
    }

    cp = strpbrk(p, "#\n");

    if (cp != NULL) {
        *cp = '\0';
    }

    cp = strpbrk(p, " \t");

    if (cp == NULL) {
        goto again;
    }

    *cp++ = '\0';
    /* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
    host.h_addr_list = host_addrs;
#endif
    host.h_addr = hostaddr;

    *((long *)host.h_addr) = inet_addr(p);

    //
    // If the address in the hosts file is bogus, skip over the
    // entry.
    //

    if ( *((long *)host.h_addr) == INADDR_ANY ||
         ( *((long *)host.h_addr) == INADDR_NONE &&
           _strnicmp( "255.255.255.255", p, 15 ) != 0 ) )
    {
        goto again;
    }

    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;
    while (*cp == ' ' || *cp == '\t')
            cp++;
    host.h_name = cp;
    q = host.h_aliases = host_aliases;
    cp = strpbrk(cp, " \t");

    if (cp != NULL) {
        *cp++ = '\0';
    }

    while (cp && *cp) {
        if (*cp == ' ' || *cp == '\t') {
            cp++;
            continue;
        }
        if (q < &host_aliases[MAXALIASES - 1]) {
            *q++ = cp;
        }
        cp = strpbrk(cp, " \t");
        if (cp != NULL) {
                *cp++ = '\0';
        }
    }
    *q = NULL;

    return (&host);

} // _gethtent


struct hostent *
_gethtbyname (
    IN char *name
    )
{
    register struct hostent *p;
    register char **cp;

    WS2LOG_F1( "gethost.c - _gethtbyname" );

    p = QueryDnsCache( name, T_A, DNS_QUERY_CACHE_ONLY, NULL );

    if ( p )
        return p;

    if ( GetLastError() == WSATRY_AGAIN )
    {
        _sethtent(0);

        while (p = _gethtent())
        {
            if (DnsNameCompare_A(p->h_name, name))
                break;

            for (cp = p->h_aliases; *cp != 0; cp++)
                if (DnsNameCompare_A(*cp, name))
                    goto found;
        }

        found:
            _endhtent();
    }
    return (p);

} // _gethtbyname



struct hostent *
myhostent (
    void
    )
{
    char        *bp;
    char        *ap;
    int         i;
    PUCHAR      domain;
    LONG_PTR     bytesLeft;
    int         numberOfIpAddresses;
    PIP_ARRAY   pipAddresses;
    PDNS_RECORD pDnsRecord;
    PDNS_RECORD pTempDnsRecord;
    DNS_STATUS  DnsStatus;
    char        HostName[(DNS_MAX_LABEL_LENGTH * 2) + 1];
    INT         HostNameLen;
    char        **ppHostAliases;
    LPSTR       pszPrimaryDomainName = NULL;
    DWORD       DomainNameLen = 0;
    INT         FQDNLen = 0;

    WS2LOG_F1( "gethost.c - myhostent" );

    host.h_addr_list = h_addr_ptrs;
    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;

    bp = hostbuf;
    memset( bp, 0, BUFSIZ );

    ap = hostaddr;
    memset( ap, 0, MAXADDRS );

    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    ppHostAliases = host.h_aliases;

    numberOfIpAddresses = DnsGetIpAddressList( &pipAddresses );

    if ( numberOfIpAddresses > MAXADDRS )
    {
        if ( SockBigBufSize >= numberOfIpAddresses &&
             hostaddrBigBuf &&
             h_addr_ptrsBigBuf )
        {
            ap = hostaddrBigBuf;
            host.h_addr_list = h_addr_ptrsBigBuf;
            goto PackAddresses;
        }

        if ( SockBigBufSize > 0 )
        {
            if ( hostaddrBigBuf )
            {
                FREE_HEAP( hostaddrBigBuf );
                hostaddrBigBuf = NULL;
            }

            if ( h_addr_ptrsBigBuf )
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
            }

            SockBigBufSize = 0;
        }

        h_addr_ptrsBigBuf = ALLOCATE_HEAP( ( numberOfIpAddresses + 1 ) *
                                           sizeof( char * ) );

        if ( h_addr_ptrsBigBuf )
        {
            host.h_addr_list = h_addr_ptrsBigBuf;

            hostaddrBigBuf = ALLOCATE_HEAP( ( numberOfIpAddresses + 1 ) *
                                            sizeof( ULONG ) );

            if ( hostaddrBigBuf )
            {
                ap = hostaddrBigBuf;
                SockBigBufSize = numberOfIpAddresses;
            }
            else
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
                ap = hostaddr;
                host.h_addr_list = h_addr_ptrs;
                numberOfIpAddresses = MAXADDRS;
            }
        }
        else
        {
            ap = hostaddr;
            host.h_addr_list = h_addr_ptrs;
            numberOfIpAddresses = MAXADDRS;
        }
    }

PackAddresses :

    if ( numberOfIpAddresses )
    {
        for ( i = 0; i < numberOfIpAddresses; ++i )
        {
            if ( pipAddresses->aipAddrs[i] != 0 )
            {
                host.h_addr_list[i] = ap;
                *((LPDWORD)ap)++ = pipAddresses->aipAddrs[i];
            }
        }
    }
    else
    {
        i = 0;
        host.h_addr_list[i++] = ap;
        *((LPDWORD)ap)++ = htonl(INADDR_LOOPBACK);
    }

    host.h_addr_list[i] = NULL;

    if ( pipAddresses )
        LocalFree( pipAddresses );

    bytesLeft = BUFSIZ - (bp - hostbuf);
    if ( _pgethostname(HostName, (DNS_MAX_LABEL_LENGTH * 2) + 1) )
        return NULL;

    HostNameLen = strlen( HostName );

    if ( !HostNameLen )
        return NULL;

    DnsStatus = DnsQuery_A( "",
                            DNS_TYPE_A,
                            0,
                            NULL,
                            &pDnsRecord,
                            NULL );

    pszPrimaryDomainName = DnsGetPrimaryDomainName_A();

    if ( pszPrimaryDomainName )
    {
        DomainNameLen = strlen( pszPrimaryDomainName );

        if ( DomainNameLen == 0 )
        {
            LocalFree( pszPrimaryDomainName );
            pszPrimaryDomainName = NULL;
            DomainNameLen = 0;
        }
    }

    FQDNLen = HostNameLen + DomainNameLen + 2;
    FQDNLen += sizeof( align ) - ( FQDNLen % sizeof( align ) );

    if ( bytesLeft > FQDNLen )
    {
        strcpy( bp, HostName );

        if ( pszPrimaryDomainName )
        {
            strcat( bp, "." );
            strcat( bp, pszPrimaryDomainName );
            LocalFree( pszPrimaryDomainName );
        }

        host.h_name = bp;
        bp += FQDNLen;
        bytesLeft -= FQDNLen;
    }
    else
    {
        if ( pszPrimaryDomainName )
            LocalFree( pszPrimaryDomainName );

        return NULL;
    }

    if ( DnsStatus || !pDnsRecord )
    {
        *host_aliases = NULL;
        host.h_aliases = host_aliases;

        return (&host);
    }

    pTempDnsRecord = pDnsRecord;

    while ( pTempDnsRecord )
    {
        INT length;

        if (ppHostAliases >= &host_aliases[MAXALIASES-1])
            break;

        if ( pTempDnsRecord->pName &&
             strcmp( pTempDnsRecord->pName, "." ) != 0 )
        {
            BOOL    fAddAlias = TRUE;

            length = HostNameLen + 2 + strlen( pTempDnsRecord->pName );
            length += sizeof( align ) - ( length % sizeof( align ) );

            if ( bytesLeft < length )
                break;

            strcpy( bp, HostName );
            strcat( bp, "." );
            strcat( bp, pTempDnsRecord->pName );

            if ( DnsNameCompare( bp, host.h_name ) )
            {
                fAddAlias = FALSE;
            }
            else
            {
                char ** ppTempHostAliases = host.h_aliases;

                while ( *ppTempHostAliases )
                {
                    if ( DnsNameCompare( bp, *ppTempHostAliases++ ) )
                    {
                        fAddAlias = FALSE;
                    }
                }
            }

            if ( fAddAlias )
            {
                *ppHostAliases++ = bp;
                *ppHostAliases = NULL;
                bp += length;
                bytesLeft -= length;
            }
        }

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    DnsFreeRRSet( pDnsRecord, TRUE );

    return (&host);

} // myhostent



struct hostent *
myhostent_W (
    void
    )
{
    char        *bp;
    char        *ap;
    int         i;
    LONG_PTR     bytesLeft;
    int         numberOfIpAddresses;
    PIP_ARRAY   pipAddresses;
    PDNS_RECORD pDnsRecord;
    PDNS_RECORD pTempDnsRecord;
    DNS_STATUS  DnsStatus;
    char        **ppHostAliases;
    LPWSTR      pszHostName = NULL;
    INT         HostNameLen;
    LPWSTR      pszPrimaryDomainName = NULL;
    DWORD       DomainNameLen = 0;
    INT         FQDNLen = 0;

    WS2LOG_F1( "gethost.c - myhostent_W" );

    host.h_addr_list = h_addr_ptrs;
    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;

    bp = hostbuf;
    memset( bp, 0, BUFSIZ );

    ap = hostaddr;
    memset( ap, 0, MAXADDRS );

    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    ppHostAliases = host.h_aliases;

    numberOfIpAddresses = DnsGetIpAddressList( &pipAddresses );

    if ( numberOfIpAddresses > MAXADDRS )
    {
        if ( SockBigBufSize >= numberOfIpAddresses &&
             hostaddrBigBuf &&
             h_addr_ptrsBigBuf )
        {
            ap = hostaddrBigBuf;
            host.h_addr_list = h_addr_ptrsBigBuf;
            goto PackAddresses;
        }

        if ( SockBigBufSize > 0 )
        {
            if ( hostaddrBigBuf )
            {
                FREE_HEAP( hostaddrBigBuf );
                hostaddrBigBuf = NULL;
            }

            if ( h_addr_ptrsBigBuf )
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
            }

            SockBigBufSize = 0;
        }

        h_addr_ptrsBigBuf = ALLOCATE_HEAP( ( numberOfIpAddresses + 1 ) *
                                           sizeof( char * ) );

        if ( h_addr_ptrsBigBuf )
        {
            host.h_addr_list = h_addr_ptrsBigBuf;

            hostaddrBigBuf = ALLOCATE_HEAP( ( numberOfIpAddresses + 1 ) *
                                            sizeof( ULONG ) );

            if ( hostaddrBigBuf )
            {
                ap = hostaddrBigBuf;
                SockBigBufSize = numberOfIpAddresses;
            }
            else
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
                ap = hostaddr;
                host.h_addr_list = h_addr_ptrs;
                numberOfIpAddresses = MAXADDRS;
            }
        }
        else
        {
            ap = hostaddr;
            host.h_addr_list = h_addr_ptrs;
            numberOfIpAddresses = MAXADDRS;
        }
    }

PackAddresses :

    if ( numberOfIpAddresses )
    {
        for ( i = 0; i < numberOfIpAddresses; ++i )
        {
            if ( pipAddresses->aipAddrs[i] != 0 )
            {
                host.h_addr_list[i] = ap;
                *((LPDWORD)ap)++ = pipAddresses->aipAddrs[i];
            }
        }
    }
    else
    {
        i = 0;
        host.h_addr_list[i++] = ap;
        *((LPDWORD)ap)++ = htonl(INADDR_LOOPBACK);
    }

    host.h_addr_list[i] = NULL;

    if ( pipAddresses )
        LocalFree( pipAddresses );

    bytesLeft = BUFSIZ - (bp - hostbuf);

    pszHostName = DnsGetHostName_W();

    if ( !pszHostName )
    {
        return NULL;
    }

    HostNameLen = wcslen( pszHostName );

    if ( !HostNameLen )
    {
        LocalFree( pszHostName );
        pszHostName = NULL;
        return NULL;
    }

    DnsStatus = DnsQuery_W( L"",
                            DNS_TYPE_A,
                            0,
                            NULL,
                            &pDnsRecord,
                            NULL );

    pszPrimaryDomainName = DnsGetPrimaryDomainName_W();

    if ( pszPrimaryDomainName )
    {
        DomainNameLen = wcslen( pszPrimaryDomainName );

        if ( DomainNameLen == 0 )
        {
            LocalFree( pszPrimaryDomainName );
            pszPrimaryDomainName = NULL;
            DomainNameLen = 0;
        }
    }

    FQDNLen = ( HostNameLen + DomainNameLen + 2 ) * sizeof(WCHAR);
    FQDNLen += sizeof( align ) - ( FQDNLen % sizeof( align ) );

    if ( bytesLeft > FQDNLen )
    {
        wcscpy( (LPWSTR) bp, pszHostName );

        if ( pszPrimaryDomainName )
        {
            wcscat( (LPWSTR) bp, L"." );
            wcscat( (LPWSTR) bp, pszPrimaryDomainName );
            LocalFree( pszPrimaryDomainName );
        }

        host.h_name = bp;
        bp += FQDNLen;
        bytesLeft -= FQDNLen;
    }
    else
    {
        if ( pszHostName )
        {
            LocalFree( pszHostName );
        }

        if ( pszPrimaryDomainName )
        {
            LocalFree( pszPrimaryDomainName );
        }

        return NULL;
    }

    if ( DnsStatus || !pDnsRecord )
    {
        *host_aliases = NULL;
        host.h_aliases = host_aliases;

        return (&host);
    }

    pTempDnsRecord = pDnsRecord;

    while ( pTempDnsRecord )
    {
        INT length;

        if (ppHostAliases >= &host_aliases[MAXALIASES-1])
            break;

        if ( pTempDnsRecord->pName &&
             wcscmp( (LPWSTR) pTempDnsRecord->pName, L"." ) != 0 )
        {
            BOOL    fAddAlias = TRUE;

            length = ( HostNameLen + wcslen( (LPWSTR) pTempDnsRecord->pName ) +
                       2 ) * sizeof(WCHAR);
            length += sizeof( align ) - ( length % sizeof( align ) );

            if ( bytesLeft < length )
                break;

            wcscpy( (LPWSTR) bp, pszHostName );
            wcscat( (LPWSTR) bp, L"." );
            wcscat( (LPWSTR) bp, (LPWSTR) pTempDnsRecord->pName );

            if ( DnsNameCompare_W( (LPWSTR) bp, (LPWSTR) host.h_name ) )
            {
                fAddAlias = FALSE;
            }
            else
            {
                char ** ppTempHostAliases = host.h_aliases;

                while ( *ppTempHostAliases )
                {
                    if ( DnsNameCompare_W( (LPWSTR) bp,
                                          (LPWSTR) *ppTempHostAliases++ ) )
                    {
                        fAddAlias = FALSE;
                    }
                }
            }

            if ( fAddAlias )
            {
                *ppHostAliases++ = bp;
                *ppHostAliases = NULL;
                bp += length;
                bytesLeft -= length;
            }
        }

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    LocalFree( pszHostName );

    DnsFreeRRSet( pDnsRecord, TRUE );

    return (&host);

} // myhostent_W



struct hostent *
localhostent (
    void
    )
{
    struct hostent * hent = myhostent();

    WS2LOG_F1( "gethost.c - localhostent" );

    if ( hent )
    {
        /* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
        hent->h_addr_list = host_addrs;
#endif
        hent->h_addr = hostaddr;
        hent->h_length = sizeof (unsigned long);
        hent->h_addrtype = AF_INET;
        *((long *)hent->h_addr) = htonl(INADDR_LOOPBACK);
    }

    return hent;

} // localhostent


struct hostent *
localhostent_W (
    void
    )
{
    struct hostent * hent = myhostent_W();

    WS2LOG_F1( "gethost.c - localhostent_W" );

    if ( hent )
    {
        /* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
        hent->h_addr_list = host_addrs;
#endif
        hent->h_addr = hostaddr;
        hent->h_length = sizeof (unsigned long);
        hent->h_addrtype = AF_INET;
        *((long *)hent->h_addr) = htonl(INADDR_LOOPBACK);
    }

    return hent;

} // localhostent_W



struct hostent *
dnshostent (
    void
    )
{
    WS2LOG_F1( "gethost.c - dnshostent" );

    //
    // Perform query through DNS Caching Resolver Service API
    //
    return QueryDnsCache( "", T_A, 0, NULL );
} // dnshostent


struct hostent *
_gethtbyaddr (
    IN  DWORD dwControlFlags,
    IN  LPSTR inaddr,
    IN  char *addr,
    IN  int   len,
    IN  int   type,
    OUT PBOOL pfUnicodeQuery
    )
{
    register struct hostent *p;

    WS2LOG_F1( "gethost.c - _gethtbyaddr" );

    if ( dwControlFlags & LUP_RETURN_BLOB )
    {
        p = QueryDnsCache( inaddr,
                           T_PTR,
                           DNS_QUERY_CACHE_ONLY,
                           NULL );

        *pfUnicodeQuery = FALSE;
    }
    else
    {
        p = QueryDnsCache_W( (LPWSTR) inaddr,
                             T_PTR,
                             DNS_QUERY_CACHE_ONLY,
                             NULL );

        *pfUnicodeQuery = TRUE;
    }

    if ( p )
    {
        *(PDWORD)p->h_addr_list[0] = *(ULONG *) addr;
        p->h_addr_list[1] = NULL;
        return p;
    }

    *pfUnicodeQuery = FALSE;

    if ( GetLastError() == WSATRY_AGAIN )
    {
        _sethtent(0);

        while ( p = _gethtent() )
        {
            if ( p->h_addrtype == type &&
                 p->h_length == len &&
                 !bcmp(p->h_addr, addr, len ) )
                break;
        }

        _endhtent();
    }
    return (p);

} // _gethtbyaddr


DWORD
CopyHostentToBuffer (
    char FAR *Buffer,
    int BufferLength,
    PHOSTENT Hostent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD addressCount;
    DWORD i;
    PHOSTENT outputHostent = (PHOSTENT)Buffer;

    WS2LOG_F1( "gethost.c - CopyHostentToBuffer" );

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInHostent( Hostent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        RtlZeroMemory( Buffer, requiredBufferLength );
    } else {
        RtlZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the hostent structure if it fits.
    //

    bytesFilled = sizeof(*Hostent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    RtlCopyMemory( currentLocation, Hostent, sizeof(*Hostent) );
    currentLocation = Buffer + bytesFilled;

    outputHostent->h_name = NULL;
    outputHostent->h_aliases = NULL;
    outputHostent->h_addr_list = NULL;

    //
    // Count the host's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Hostent->h_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_aliases = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Count the host's addresses and set up an array to hold pointers to
    // them.
    //

    for ( addressCount = 0;
          Hostent->h_addr_list[addressCount] != NULL;
          addressCount++ );

    bytesFilled += (addressCount+1) * sizeof(void FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_addr_list = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_addr_list = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in addresses.  Do addresses before filling in the
    // host name and aliases in order to avoid alignment problems.
    //

    for ( i = 0; i < addressCount; i++ ) {

        bytesFilled += Hostent->h_length;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_addr_list[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_addr_list[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Hostent->h_addr_list[i],
            Hostent->h_length
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_addr_list[i] = NULL;

    //
    // Copy the host name if it fits.
    //

    bytesFilled += strlen( Hostent->h_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputHostent->h_name = currentLocation;

    RtlCopyMemory( currentLocation, Hostent->h_name, strlen( Hostent->h_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Hostent->h_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_aliases[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Hostent->h_aliases[i],
            strlen( Hostent->h_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_aliases[i] = NULL;

    return requiredBufferLength;

} // CopyHostentToBuffer


DWORD
BytesInHostent (
    PHOSTENT Hostent
    )
{
    DWORD total;
    int i;

    WS2LOG_F1( "gethost.c - BytesInHostent" );

    total = sizeof(HOSTENT);
    total += strlen( Hostent->h_name ) + 1;

    //
    // Account for the NULL terminator pointers at the end of the
    // alias and address arrays.
    //

    total += sizeof(char *) + sizeof(char *);

    for ( i = 0; Hostent->h_aliases[i] != NULL; i++ ) {
        total += strlen( Hostent->h_aliases[i] ) + 1 + sizeof(char *);
    }

    for ( i = 0; Hostent->h_addr_list[i] != NULL; i++ ) {
        total += Hostent->h_length + sizeof(char *);
    }

    //
    // Pad the answer to an eight-byte boundary.
    //

    return (total + 7) & ~7;

} // BytesInHostent

typedef struct _SOCK_NBT_NAMERES_INFO {
    IO_STATUS_BLOCK IoStatus;
    struct {
        FIND_NAME_HEADER Header;
        FIND_NAME_BUFFER Buffer[MAXADDRS];
    } FindNameInfo;
    tIPADDR_BUFFER IpAddrInfo;
} SOCK_NBT_NAMERES_INFO, *PSOCK_NBT_NAMERES_INFO;


ULONG
SockNbtResolveName (
    IN  PCHAR    Name,
    OUT WORD *   pwIpCount OPTIONAL,
    OUT PULONG * pIpArray  OPTIONAL
    )
{
    NTSTATUS status;
    ULONG ipAddr = INADDR_NONE;
    ULONG i;
    ULONG j;
    DWORD completed;
    DWORD handleCount;
    DWORD waitCount;
    ULONG nameLength;
    PHANDLE events = NULL;
    PHANDLE events2;
    PHANDLE phSockNbt = NULL;
    PSOCK_NBT_NAMERES_INFO nameresInfo = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL  fPnPEventDetected = FALSE;
    BOOL  fOkayToRetry = TRUE;

    WS2LOG_F1( "gethost.c - SockNbtResolveName" );

    //
    // If the passed-in name is longer than 15 characters, then we know
    // that it can't be resolved with Netbios.
    //

    nameLength = strlen( Name );

    if ( nameLength > 15 ) {
        goto exit;
    }

RetryWithPnPEvent :

    fPnPEventDetected = FALSE;

    //
    // Open control channels to NBT.
    //

    handleCount = SockOpenNbt( &phSockNbt );

    if( handleCount == 0 ) {
        goto exit;
    }

    //
    // Open event objects for synchronization of I/O completion.
    //
    // We're use two arrays for this: the 1st one "events[]",
    // contains the complete list of events as initially created;
    // the 2nd one "events2[]", contains the contiguous list
    // of "waitable" event handles.  We'll WaitForMultObjs using
    // events2[], then map back to the original list index by
    // comparing event handles with those in events[].
    //

    events = ALLOCATE_HEAP( handleCount * 2 * sizeof(HANDLE) );
    if ( events == NULL ) {
        goto exit;
    }

    events2 = &events[handleCount];

    RtlZeroMemory( events, handleCount * sizeof(HANDLE) );

    for ( j = 0; j < handleCount; j++ ) {
        events[j] = CreateEvent( NULL, TRUE, TRUE, NULL );
        if ( events[j] == NULL ) {
            goto exit;
        }
    }

    //
    // Allocate an array of structures, one for each nameres request.
    //

    nameresInfo = ALLOCATE_HEAP( handleCount * sizeof(*nameresInfo) );
    if ( nameresInfo == NULL ) {
        goto exit;
    }

    //
    // Set up several name resolution requests.
    //

    waitCount = 0;

    for ( j = 0; j < handleCount; j++ ) {

        //
        // Set up the input buffer with the name of the host we're looking
        // for.  We upcase the name, zero pad to 15 spaces, and put a 0 in
        // the last byte to search for the redirector name.
        //

        for ( i = 0; i < nameLength; i++ )
        {
            nameresInfo[j].IpAddrInfo.Name[i] = Name[i];
        }

        while ( i < 15 ) {
            nameresInfo[j].IpAddrInfo.Name[i++] = ' ';
        }

        nameresInfo[j].IpAddrInfo.Name[15] = 0;

        //
        // Do the actual query find name.
        //

        status = NtDeviceIoControlFile(
                     phSockNbt[j],
                     events[j], // the list that doesn't change
                     NULL,
                     NULL,
                     &nameresInfo[j].IoStatus,
                     IOCTL_NETBT_FIND_NAME,
                     &nameresInfo[j].IpAddrInfo,
                     sizeof(nameresInfo[j].IpAddrInfo),
                     &nameresInfo[j].FindNameInfo,
                     sizeof(nameresInfo[j].FindNameInfo)
                     );

        //
        // If success add to the events2[] array of waitable event handles
        //
        // Else indicate the status in the current nameresInfo entry
        //

        if( NT_SUCCESS(status) )
        {
            events2[waitCount] = events[j];
            waitCount++;

            if( status != STATUS_PENDING )
            {
                nameresInfo[j].IoStatus.Status = status;
            }
        }
        else
        {
            if ( status == STATUS_INVALID_DEVICE_STATE )
            {
                fPnPEventDetected = TRUE;
            }

            nameresInfo[j].IoStatus.Status = status;
        }
    }

    if( waitCount == 0 ) {
        goto exit;
    }

    //
    // Wait for one of the requests to complete.  We'll take the first
    // successful response we get.
    //

    while (waitCount)
    {
        DWORD jj;

        completed =
            WaitForMultipleObjects( waitCount,
                                    events2, // the compacted list
                                    FALSE,
                                    INFINITE );

        //
        // If something went wrong cancel all pending io & exit
        //

        if (completed >= waitCount)
        {
            for ( j = 0; j < waitCount; j++ ) {

                DWORD k;

                //
                // Map from events2[] index to events[] index to
                // get correct index for phSockNbt[]
                //

                for (k = 0; events[k] != events2[j]; k++);

                NtCancelIoFile( phSockNbt[k], &ioStatusBlock );
            }

            WaitForMultipleObjects( waitCount, events2, TRUE, INFINITE );

            goto exit;
        }

        //
        // Map the index of the signaled event handle in events2[] to
        // the corresponding index in events[]
        //

        for (jj = 0; events[jj] != events2[completed]; jj++);

        WS_ASSERT(jj < handleCount );

        if ( NT_SUCCESS(nameresInfo[jj].IoStatus.Status) ) {

            //
            // Cancel all the outstanding requests.  This prevents the query
            // from taking a long time on a multihomed machine where only
            // one of the queries is going to succeed.
            //
            // Don't cancel io on the handle that just completed, and
            // make sure to map to the correct phSockNbt[] index.
            //

            for ( j = 0; j < waitCount; j++ ) {

                if (j != completed)
                {
                    DWORD k;

                    //
                    // Map from events2[] index to events[] index to
                    // get correct index for phSockNbt[]
                    //

                    for (k = 0; events[k] != events2[j]; k++);

                    NtCancelIoFile( phSockNbt[k], &ioStatusBlock );
                }
            }
            break;
        }

        //
        // Compact events2[] by removing the signaled event, then
        // loop back for another wait
        //

        memcpy(&events2[completed],
               &events2[completed + 1],
               (waitCount - completed - 1) * sizeof (HANDLE));
        waitCount--;
    }


    //
    // Wait for all the rest of the IO requests to complete.
    //

    if ( waitCount ) {

      WaitForMultipleObjects( waitCount, events2, TRUE, INFINITE );
    }


    //
    // Walk through the requests and see if any succeeded.
    //

    for ( j = 0; j < handleCount; j++ ) {

        if ( !NT_SUCCESS(nameresInfo[j].IoStatus.Status) ) {
            continue;
        }

        if ( nameresInfo[j].FindNameInfo.Header.unique_group != UNIQUE_NAME )
        {
            continue;
        }

        //
        // This request succeeded.  The IP address is in the rightmost
        // four bytes of the source_addr field.
        //

        if ( pwIpCount && pIpArray )
        {
            *pwIpCount = nameresInfo[j].FindNameInfo.Header.node_count;

            if ( *pwIpCount )
            {
                if ( *pwIpCount > MAXADDRS )
                {
                    *pwIpCount = MAXADDRS;
                }

                *pIpArray = ALLOCATE_HEAP( *pwIpCount * sizeof( ULONG ) );

                if ( *pIpArray )
                {
                    WORD iter;

                    for ( iter = 0; iter < *pwIpCount; iter++ )
                    {
                      (*pIpArray)[iter] = *(UNALIGNED ULONG *)
                         (&(nameresInfo[j].FindNameInfo.Buffer[iter].source_addr[2]));
                    }
                }
                else
                {
                    *pwIpCount = 0;
                }
            }
        }

        ipAddr = *(UNALIGNED ULONG *)
                  (&(nameresInfo[j].FindNameInfo.Buffer[0].source_addr[2]));

        if ( ipAddr == 0 ) {
            ipAddr = INADDR_NONE;
        }

        break;
    }

    //
    // Clean up and return.
    //

exit:

    if ( events != NULL )
    {
        for ( j = 0; j < handleCount; j++ )
        {
            if ( events[j] != NULL )
            {
                NtClose( events[j] );
            }
        }
        FREE_HEAP( events );
        events = NULL;
    }

    if ( nameresInfo != NULL )
    {
        FREE_HEAP( nameresInfo );
        nameresInfo = NULL;
    }

    if ( phSockNbt )
    {
        FREE_HEAP( phSockNbt );
        phSockNbt = NULL;
    }

    if ( ipAddr == INADDR_NONE &&
         fPnPEventDetected &&
         fOkayToRetry )
    {
        //
        // PnP event may have occured, discard current Nbt handles
        // and retry query . . .
        //
        SockAcquireGlobalLockExclusive();
        GetHostCleanup();
        SockReleaseGlobalLock();

        fOkayToRetry = FALSE;

        goto RetryWithPnPEvent;
    }

    return ipAddr;

} // SockNbtResolveName

typedef struct _SOCK_NBT_ADDRRES_INFO {
    IO_STATUS_BLOCK IoStatus;
    tIPANDNAMEINFO IpAndNameInfo;
    CHAR Buffer[2048];
} SOCK_NBT_ADDRRES_INFO, *PSOCK_NBT_ADDRRES_INFO;


BOOL
SockNbtResolveAddr (
    IN ULONG IpAddress,
    IN PCHAR Name
    )
{
    LONG Count;
    INT i;
    UINT j;
    NTSTATUS status;
    tADAPTERSTATUS *pAdapterStatus;
    NAME_BUFFER *pNames;
    ULONG SizeInput;
    PSOCK_NBT_ADDRRES_INFO addrresInfo = NULL;
    PHANDLE events = NULL;
    PHANDLE events2;
    PHANDLE phSockNbt = NULL;
    BOOL success;
    IO_STATUS_BLOCK ioStatusBlock;
    DWORD completed;
    DWORD handleCount;
    DWORD waitCount;
    BOOL  fPnPEventDetected = FALSE;
    BOOL  fOkayToRetry = TRUE;

    WS2LOG_F1( "gethost.c - SockNbtResolveAddr" );

RetryWithPnPEvent :

    success = FALSE;
    fPnPEventDetected = FALSE;

    //
    // Open control channels to NBT.
    //

    handleCount = SockOpenNbt( &phSockNbt );

    if( handleCount == 0 ) {
        goto exit;
    }

    //
    // Don't allow zero for the address since it sends a broadcast and
    // every one responds
    //

    if ((IpAddress == INADDR_NONE) || (IpAddress == 0)) {
        goto exit;
    }

    //
    // Open event objects for synchronization of I/O completion.
    //
    // We're use two arrays for this: the 1st one "events[]",
    // contains the complete list of events as initially created;
    // the 2nd one "events2[]", contains the contiguous list
    // of "waitable" event handles.  We'll WaitForMultObjs using
    // events2[], then map back to the original list index by
    // comparing event handles with those in events[].
    //

    events = ALLOCATE_HEAP( handleCount * 2 * sizeof(HANDLE) );
    if ( events == NULL ) {
        goto exit;
    }

    events2 = &events[handleCount];

    RtlZeroMemory( events, handleCount * sizeof(HANDLE) );

    for ( j = 0; j < handleCount; j++ ) {
        events[j] = CreateEvent( NULL, TRUE, TRUE, NULL );
        if ( events[j] == NULL ) {
            goto exit;
        }
    }

    //
    // Allocate an array of structures, one for each addrres request.
    //

    addrresInfo = ALLOCATE_HEAP( handleCount * sizeof(*addrresInfo) );
    if ( addrresInfo == NULL ) {
        goto exit;
    }

    //
    // Set up several name resolution requests.
    //

    waitCount = 0;

    for ( j = 0; j < handleCount; j++ ) {

        RtlZeroMemory( &addrresInfo[j].IpAndNameInfo, sizeof(tIPANDNAMEINFO) );

        addrresInfo[j].IpAndNameInfo.IpAddress = ntohl(IpAddress);
        addrresInfo[j].IpAndNameInfo.NetbiosAddress.Address[0].Address[0].NetbiosName[0] = '*';

        addrresInfo[j].IpAndNameInfo.NetbiosAddress.TAAddressCount = 1;
        addrresInfo[j].IpAndNameInfo.NetbiosAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
        addrresInfo[j].IpAndNameInfo.NetbiosAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
        addrresInfo[j].IpAndNameInfo.NetbiosAddress.Address[0].Address[0].NetbiosNameType =
                            TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        SizeInput = sizeof(tIPANDNAMEINFO);

        //
        // Do the actual query find name.
        //

        status = NtDeviceIoControlFile(
                     phSockNbt[j],
                     events[j], // the list that doesn't change
                     NULL,
                     NULL,
                     &addrresInfo[j].IoStatus,
                     IOCTL_NETBT_ADAPTER_STATUS,
                     &addrresInfo[j].IpAndNameInfo,
                     sizeof(addrresInfo[j].IpAndNameInfo),
                     addrresInfo[j].Buffer,
                     sizeof(addrresInfo[j].Buffer)
                     );

        //
        // If success add to the events2[] array of waitable event handles
        //
        // Else indicate the status in the current nameresInfo entry
        //

        if( NT_SUCCESS(status) )
        {
            events2[waitCount] = events[j];
            waitCount++;

            if( status != STATUS_PENDING )
            {
                addrresInfo[j].IoStatus.Status = status;
            }
        }
        else
        {
            if ( status == STATUS_INVALID_DEVICE_STATE )
            {
                fPnPEventDetected = TRUE;
            }

            addrresInfo[j].IoStatus.Status = status;
        }
    }

    if( waitCount == 0 ) {
        goto exit;
    }

    //
    // Wait for one of the requests to complete.  We'll take the first
    // successful response we get.
    //

    while (waitCount)
    {
        DWORD jj;

        completed =
            WaitForMultipleObjects( waitCount,
                                    events2, // the compacted list
                                    FALSE,
                                    INFINITE );

        //
        // If something went wrong cancel all pending io & exit
        //

        if (completed >= waitCount)
        {
            for ( j = 0; j < waitCount; j++ ) {

                DWORD k;

                //
                // Map from events2[] index to events[] index to
                // get correct index for phSockNbt[]
                //

                for (k = 0; events[k] != events2[j]; k++);

                NtCancelIoFile( phSockNbt[k], &ioStatusBlock );
            }

            WaitForMultipleObjects( waitCount, events2, TRUE, INFINITE );

            goto exit;
        }

        //
        // Map the index of the signaled event handle in events2[] to
        // the corresponding index in events[]
        //

        for (jj = 0; events[jj] != events2[completed]; jj++);

        WS_ASSERT(jj < handleCount );

        if ( NT_SUCCESS(addrresInfo[jj].IoStatus.Status) ) {

            //
            // Cancel all the outstanding requests.  This prevents the query
            // from taking a long time on a multihomed machine where only
            // one of the queries is going to succeed.
            //
            // Don't cancel io on the handle that just completed, and
            // make sure to map to the correct phSockNbt[] index.
            //

            for ( j = 0; j < waitCount; j++ ) {

                if (j != completed)
                {
                    DWORD k;

                    //
                    // Map from events2[] index to events[] index to
                    // get correct index for phSockNbt[]
                    //

                    for (k = 0; events[k] != events2[j]; k++);

                    NtCancelIoFile( phSockNbt[k], &ioStatusBlock );
                }
            }
            break;
        }

        //
        // Compact events2[] by removing the signaled event, then
        // loop back for another wait
        //

        memcpy(&events2[completed],
               &events2[completed + 1],
               (waitCount - completed - 1) * sizeof (HANDLE));
        waitCount--;
    }

    //
    // Wait for all the rest of the IO requests to complete.
    //

    if ( waitCount ) {

        WaitForMultipleObjects( waitCount, events2, TRUE, INFINITE );
    }


    //
    // Walk through the requests and see if any succeeded.
    //

    for ( j = 0; j < handleCount; j++ ) {

        pAdapterStatus = (tADAPTERSTATUS *)addrresInfo[j].Buffer;

        if ( !NT_SUCCESS(addrresInfo[j].IoStatus.Status) ||
                 pAdapterStatus->AdapterInfo.name_count == 0 ) {
            continue;
        }

        pNames = pAdapterStatus->Names;
        Count = pAdapterStatus->AdapterInfo.name_count;

        //
        // Look for the redirector name in the list.
        //

        while(Count--) {

            if ( pNames->name[NCBNAMSZ-1] == 0 ) {

                //
                // Copy the name up to but not including the first space.
                //

                for ( i = 0; i < NCBNAMSZ && pNames->name[i] != ' '; i++ ) {
                    *(Name + i) = pNames->name[i];
                }

                *(Name + i) = '\0';

                success = TRUE;
                goto exit;
            }

            pNames++;
        }
    }

exit:

    if ( events != NULL )
    {
        for ( j = 0; j < handleCount; j++ )
        {
            if ( events[j] != NULL )
            {
                NtClose( events[j] );
            }
        }
        FREE_HEAP( events );
        events = NULL;
    }

    if ( addrresInfo != NULL )
    {
        FREE_HEAP( addrresInfo );
        addrresInfo = NULL;
    }

    if ( phSockNbt )
    {
        FREE_HEAP( phSockNbt );
        phSockNbt = NULL;
    }

    if ( success == FALSE &&
         fPnPEventDetected &&
         fOkayToRetry )
    {
        //
        // PnP event may have occured, discard current Nbt handles
        // and retry query . . .
        //
        SockAcquireGlobalLockExclusive();
        GetHostCleanup();
        SockReleaseGlobalLock();

        fOkayToRetry = FALSE;

        goto RetryWithPnPEvent;
    }

    return success;

} // SockNbtResolveAddr


DWORD
SockOpenNbt (
    PHANDLE * pphSockNbt
    )

/*++

Routine Description:

    Opens control channel(s) to the NBT device(s).

    N.B. The Plug & Play enabled NBT creates its device objects on demand
    when the underlying media becomes available. As a result of this,
    devices in NBT's device list may not be immediately available, and
    trying to open these devices will result in error. This routine caches
    NBT control channels as they are opened. Subsequent calls attempt to
    open any control channels that are not already open.

Arguments:

    Pointer to receive a copy of the opened Nbt handle list ( a copy of
    the global list -  multi-thread safe ).

Return Value:

    DWORD - The number of control channels opened.

Notes:

    This routine caches open NBT handles for performance. The following
    global variables are used for this cache:

        SockNbtDeviceNames - Pointer to a REG_MULTI_SZ buffer containing
            all of the potential NBT names.

        SockNbtDeviceCount - The number of device names in the above buffer.

        SockNbtHandles - Pointer to an array of open HANDLEs. Note that this
            array is "packed" (all entries are non-NULL) and is thus
            suitable for passing to WaitForMultipleObjects().

        SockNbtHandleCount - The number of open HANDLEs in the above array.
            This value is always <= SockNbtDeviceCount.

        SockSparseNbtHandles - Pointer to a sparse array of HANDLEs. Each
            entry in this array is either NULL (meaning that the control
            channel has not yet been opened) or !NULL (meaning that the
            control channel has been opened). Since this array may contain
            NULL entries, it is *not* suitable for WaitForMultipleObjects().

        SockNbtFullyInitialized - Set to TRUE after all NBT devices have
            been successfully opened. This lets us short-circuit a bunch
            of tests after all control channels are open.

    Note that SockNbtHandles and SockSparseNbtHandles are allocated from
    the same heap block (saves an extra allocation).

--*/

{
    HKEY nbtKey = NULL;
    ULONG error;
    ULONG deviceNameLength;
    ULONG type;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING deviceString;
    OBJECT_ATTRIBUTES objectAttributes;
    PWSTR w;
    DWORD i;
    DWORD handleCount;

    WS2LOG_F1( "gethost.c - SockOpenNbt" );

    //
    // First determine whether we actually need to open NBT.
    //

    SockAcquireGlobalLockExclusive( );

    if( SockNbtFullyInitialized )
    {
        goto CopyHandles;
    }

    *pphSockNbt = NULL;

    //
    // Next, read the registry to obtain the device name of one of
    // NBT's device exports.
    //
    // N.B. This assumes the list of NBT device names is fairly static
    // (i.e. we only read it from the registry once per process).
    //

    if( SockNbtDeviceNames == NULL ) {

        error = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Linkage",
                    0,
                    KEY_READ,
                    &nbtKey
                    );
        if ( error != NO_ERROR ) {
            goto exit;
        }

        //
        // Determine the size of the device name.  We need this so that we
        // can allocate enough memory to hold it.
        //

        deviceNameLength = 0;

        error = RegQueryValueExW(
                    nbtKey,
                    L"Export",
                    NULL,
                    &type,
                    NULL,
                    &deviceNameLength
                    );
        if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
            goto exit;
        }

        //
        // Allocate enough memory to hold the mapping.
        //

        SockNbtDeviceNames = ALLOCATE_HEAP( deviceNameLength );
        if ( SockNbtDeviceNames == NULL ) {
            goto exit;
        }

        //
        // Get the actual device names from the registry.
        //

        error = RegQueryValueExW(
                    nbtKey,
                    L"Export",
                    NULL,
                    &type,
                    (PVOID)SockNbtDeviceNames,
                    &deviceNameLength
                    );
        if ( error != NO_ERROR ) {
            FREE_HEAP( SockNbtDeviceNames );
            SockNbtDeviceNames = NULL;
            goto exit;
        }

        //
        // Count the number of names exported by NetBT.
        //

        SockNbtDeviceCount = 0;

        for ( w = SockNbtDeviceNames; *w != L'\0'; w += wcslen(w) + 1 ) {
            SockNbtDeviceCount++;
        }

        if ( SockNbtDeviceCount == 0 ) {
            SockNbtFullyInitialized = TRUE;
            goto exit;
        }
    }

    WS_ASSERT( SockNbtDeviceNames != NULL );
    WS_ASSERT( SockNbtDeviceCount > 0 );

    //
    // Allocate space to hold all the handles.
    //

    if( SockNbtHandles == NULL ) {

        WS_ASSERT( SockSparseNbtHandles == NULL );

        SockNbtHandles = ALLOCATE_HEAP(
                             (SockNbtDeviceCount+1) * sizeof(HANDLE) * 2
                             );

        if( SockNbtHandles == NULL ) {
            goto exit;
        }

        RtlZeroMemory(
            SockNbtHandles,
            (SockNbtDeviceCount+1) * sizeof(HANDLE) * 2
            );

        SockSparseNbtHandles = SockNbtHandles + (SockNbtDeviceCount+1);

    }

    SockNbtHandleCount = 0;

    //
    // For each exported name, open a control channel handle to NBT.
    //

    for ( i = 0, w = SockNbtDeviceNames; *w != L'\0'; i++, w += wcslen(w) + 1 ) {

        WS_ASSERT( i < SockNbtDeviceCount );

        if( SockSparseNbtHandles[i] != NULL ) {
            SockNbtHandles[SockNbtHandleCount] = SockSparseNbtHandles[i];
            SockNbtHandleCount++;
            continue;
        }

        RtlInitUnicodeString( &deviceString, w );

        InitializeObjectAttributes(
            &objectAttributes,
            &deviceString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtCreateFile(
                     &SockSparseNbtHandles[i],
                     GENERIC_EXECUTE | SYNCHRONIZE,
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                     // AllocationSize
                     0L,                                       // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                     FILE_OPEN_IF,                             // CreateDisposition
                     0,                                        // CreateOptions
                     NULL,
                     0
                     );

        if( NT_SUCCESS(status) ) {
            SockNbtHandles[SockNbtHandleCount] = SockSparseNbtHandles[i];
            SockNbtHandleCount++;
        }
    }

    if( SockNbtHandleCount == SockNbtDeviceCount ) {
        SockNbtFullyInitialized = TRUE;
    }

exit:

    if ( nbtKey != NULL ) {
        RegCloseKey( nbtKey );
    }

CopyHandles :

    *pphSockNbt = CopyNbtHandles( SockNbtHandles, SockNbtHandleCount );

    if ( *pphSockNbt )
    {
        handleCount = SockNbtHandleCount;
    }
    else
    {
        handleCount = 0;
    }

    SockReleaseGlobalLock( );

    return handleCount;

} // SockOpenNbt


PHANDLE
CopyNbtHandles (
    PHANDLE phSockNbt,
    DWORD   HandleCount
    )
/*++

Routine Description:

    Copies list of control channel(s) to the NBT device(s).

Arguments:

    Pointer to the opened Nbt handle list to be copied.

Return Value:

    PHANDLE - Pointer to the Nbt handle list copy.


--*/
{
    PHANDLE pNbtHandleCopy = NULL;
    DWORD   iter;

    if ( !HandleCount )
    {
        return NULL;
    }

    pNbtHandleCopy = ALLOCATE_HEAP( ( HandleCount + 1 ) * sizeof(HANDLE) );

    if( pNbtHandleCopy == NULL )
    {
        return NULL;
    }

    RtlZeroMemory( pNbtHandleCopy,
                   ( HandleCount + 1 ) * sizeof(HANDLE) );

    for ( iter = 0; iter < HandleCount; iter++ )
    {
        pNbtHandleCopy[iter] = phSockNbt[iter];
    }

    return pNbtHandleCopy;
} // CopyNbtHandles


VOID
GetHostCleanup(
    VOID
    )
{
    DWORD i;

    WS2LOG_F1( "gethost.c - GetHostCleanup" );

    if( SockNbtDeviceNames != NULL ) {
        FREE_HEAP( SockNbtDeviceNames );
        SockNbtDeviceNames = NULL;
    }

    if( SockNbtHandles != NULL ) {
        for( i = 0 ; i < SockNbtHandleCount ; i++ ) {
            if( SockNbtHandles[i] != NULL ) {
                CloseHandle( SockNbtHandles[i] );
                SockNbtHandles[i] = NULL;
            }
        }

        FREE_HEAP( SockNbtHandles );
        SockNbtHandles = NULL;
    }

    SockNbtHandleCount = 0;
    SockNbtFullyInitialized = FALSE;

}   // GetHostCleanup


struct hostent *
QueryDnsCache(
    IN  LPSTR    pszName,
    IN  WORD     wType,
    IN  DWORD    Options,
    IN  PVOID *  ppMsg OPTIONAL
    )
{
    DNS_STATUS  DnsStatus = NO_ERROR;
    PDNS_RECORD pDnsRecord = NULL, pTempDnsRecord = NULL;
    char        * bp, *ap, **ppHostAliases, **ppAddresses;
    int         buflen, length;
    int         haveanswer = 0;
    int         count = 0;

    WS2LOG_F1( "gethost.c - QueryDnsCache" );

    //
    // Query DNS Cache/Server for records ...
    //
    DnsStatus = DnsQuery_A( pszName,
                            wType,
                            Options,
                            NULL,
                            &pDnsRecord,
                            ppMsg );

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE )
    {
        SetLastError( WSATRY_AGAIN );
        return ((struct hostent *) NULL);
    }

    if ( DnsStatus )
    {
        if ( ppMsg && *ppMsg )
        {
            LocalFree( *ppMsg );
            *ppMsg = NULL;
        }

        SetLastError( DnsStatusToWS2Error( DnsStatus ) );
        return ((struct hostent *) NULL);
    }

    pTempDnsRecord = pDnsRecord;

    while ( pTempDnsRecord )
    {
        if ( pTempDnsRecord->Flags.S.Section == DNSREC_ANSWER &&
             ( pTempDnsRecord->wType == DNS_TYPE_A ||
               pTempDnsRecord->wType == DNS_TYPE_ATMA ||
               pTempDnsRecord->wType == DNS_TYPE_AAAA ) )
            count++;

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    host.h_addr_list = h_addr_ptrs;

    bp = hostbuf;
    buflen = BUFSIZ+1;
    memset( bp, 0, BUFSIZ );

    ap = hostaddr;
    memset( ap, 0, MAXADDRS );

    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    ppHostAliases = host.h_aliases;

    if ( count > MAXADDRS )
    {
        if ( SockBigBufSize >= count &&
             hostaddrBigBuf &&
             h_addr_ptrsBigBuf )
        {
            ap = hostaddrBigBuf;
            host.h_addr_list = h_addr_ptrsBigBuf;
            goto ProcessQuery;
        }

        if ( SockBigBufSize > 0 )
        {
            if ( hostaddrBigBuf )
            {
                FREE_HEAP( hostaddrBigBuf );
                hostaddrBigBuf = NULL;
            }

            if ( h_addr_ptrsBigBuf )
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
            }

            SockBigBufSize = 0;
        }

        h_addr_ptrsBigBuf = ALLOCATE_HEAP( ( count + 1 ) *
                                           sizeof( char * ) );

        if ( h_addr_ptrsBigBuf )
        {
            host.h_addr_list = h_addr_ptrsBigBuf;

            hostaddrBigBuf = ALLOCATE_HEAP( ( count + 1 ) *
                                            sizeof( IPV6_ADDRESS ) );
            if ( hostaddrBigBuf )
            {
                ap = hostaddrBigBuf;
                SockBigBufSize = count;
            }
            else
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
                ap = hostaddr;
                host.h_addr_list = h_addr_ptrs;
                count = MAXADDRS;
            }
        }
        else
        {
            ap = hostaddr;
            host.h_addr_list = h_addr_ptrs;
            count = MAXADDRS;
        }
    }

ProcessQuery :

    ppAddresses = host.h_addr_list;
    *ppAddresses = NULL;
    host.h_addrtype = AF_INET;
    host.h_length = 0;
    host.h_name = NULL;

    pTempDnsRecord = pDnsRecord;

    while ( ( wType == DNS_TYPE_A ||
              wType == DNS_TYPE_ATMA ||
              wType == DNS_TYPE_AAAA ) &&
            pTempDnsRecord )
    {
        if ( pTempDnsRecord->Flags.S.Section == DNSREC_ANSWER &&
             ( pTempDnsRecord->wType == DNS_TYPE_A ||
               pTempDnsRecord->wType == DNS_TYPE_ATMA ||
               pTempDnsRecord->wType == DNS_TYPE_AAAA ) )
            break;

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    if ( !pTempDnsRecord )
    {
        if ( pDnsRecord )
        {
            DnsFreeRRSet( pDnsRecord, TRUE );
        }

        SetLastError( DnsStatusToWS2Error( DNS_ERROR_RCODE_NAME_ERROR ) );
        return ((struct hostent *) NULL);
    }

    if ( wType != T_PTR &&
         pTempDnsRecord->pName )
        length = strlen( pTempDnsRecord->pName ) + 1;
    else
        length = 0;

    if ( length )
    {
        length += sizeof( align ) - ( length % sizeof( align ) );

        if ( buflen < length )
        {
            if ( pDnsRecord )
            {
                DnsFreeRRSet( pDnsRecord, TRUE );
            }

            WS_PRINT(("Buffer too small!"));
            SetLastError( WSA_NOT_ENOUGH_MEMORY );
            return ((struct hostent *) NULL);
        }

        strcpy( bp, pTempDnsRecord->pName );
        host.h_name = bp;
        bp += length;
        buflen -= length;
    }
    else
    {
        if ( wType != T_PTR &&
             strcmp( pszName, "" ) )
        {
#if 0
            KdPrint(("\n\nWinSock2 RNR - QueryDnsCache problem!\n"));
            KdPrint((" Function was passed a name value of %s,\n", pszName));
            KdPrint(("yet DnsQuery_A returned a pDnsRecord with a NULL pName field for it!\n"));

            ASSERT( pTempDnsRecord->pName != NULL );
#endif // 0

            if ( pDnsRecord )
            {
                DnsFreeRRSet( pDnsRecord, TRUE );
            }

            SetLastError( DnsStatusToWS2Error( DNS_ERROR_RCODE_NAME_ERROR ) );
            return NULL;
        }
    }

    pTempDnsRecord = pDnsRecord;

    while ( pTempDnsRecord )
    {
        if ( pTempDnsRecord->Flags.S.Section != DNSREC_ANSWER ||
             ( pTempDnsRecord->Flags.S.Section == DNSREC_ANSWER &&
               ( pTempDnsRecord->wType == DNS_TYPE_A ||
                 pTempDnsRecord->wType == DNS_TYPE_ATMA ||
                 pTempDnsRecord->wType == DNS_TYPE_AAAA ) &&
               haveanswer >= count ) )
        {
            pTempDnsRecord = pTempDnsRecord->pNext;
            continue;
        }

        switch( pTempDnsRecord->wType )
        {
            case T_CNAME :
                if (ppHostAliases >= &host_aliases[MAXALIASES-1])
                    break;

                length = strlen( pTempDnsRecord->pName ) + 1;
                length += sizeof( align ) - ( length % sizeof( align ) );
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                strcpy( bp, pTempDnsRecord->pName );
                *ppHostAliases = bp;
                ppHostAliases++;
                *ppHostAliases = NULL;
                bp += length;
                buflen -= length;
                //
                // We don't have a valid hostent unless an address has
                // been found also. Bump haveanswer in T_A type and PTR
                // type only.
                //
                // haveanswer++;
                break;

            case T_PTR :
                length = strlen( pTempDnsRecord->Data.PTR.pNameHost ) + 1;
                length += sizeof( align ) - ( length % sizeof( align ) );
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                strcpy( bp, pTempDnsRecord->Data.PTR.pNameHost );

                if ( host.h_name == NULL )
                    host.h_name = bp;
                else
                {
                    *ppHostAliases = bp;
                    ppHostAliases++;
                    *ppHostAliases = NULL;
                }

                bp += length;
                buflen -= length;
                host.h_length = sizeof( unsigned long );
                host.h_addr = hostaddr;
                haveanswer++;
                break;

            case T_A :
                length = pTempDnsRecord->wDataLength;
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                memcpy( ap, &pTempDnsRecord->Data.A.ipAddress, length );
                *ppAddresses = ap;
                ppAddresses++;
                *ppAddresses = NULL;
                ap += length;
                host.h_length = sizeof( unsigned long );
                haveanswer++;
                break;

            case DNS_TYPE_AAAA :
                length = pTempDnsRecord->wDataLength;
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                memcpy( ap, &pTempDnsRecord->Data.AAAA.ipv6Address, length );
                *ppAddresses = ap;
                ppAddresses++;
                *ppAddresses = NULL;
                ap += length;
                host.h_length = sizeof( IPV6_ADDRESS );
                host.h_addrtype = AF_INET6;
                haveanswer++;
                break;

            case T_ATMA :
                length = pTempDnsRecord->wDataLength;

                if ( buflen < sizeof( ATM_ADDRESS ) )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }

                memset( ap, 0, sizeof( ATM_ADDRESS ) );
                ((ATM_ADDRESS *) ap)->AddressType =
                    pTempDnsRecord->Data.ATMA.AddressType;
                ((ATM_ADDRESS *) ap)->NumofDigits = length - sizeof( BYTE );
                memcpy( &((ATM_ADDRESS *) ap)->Addr,
                        &pTempDnsRecord->Data.ATMA.Address,
                        length );
                *ppAddresses = ap;
                ppAddresses++;
                *ppAddresses = NULL;
                ap += sizeof( ATM_ADDRESS );
                host.h_length = sizeof( ATM_ADDRESS );
                host.h_addrtype = AF_ATM;
                haveanswer++;
                break;

            default :
                IF_DEBUG(RESOLVER)
                {
                    WS_PRINT(("unexpected answer type %d\n",
                              pTempDnsRecord->wType));
                }
                break;
        }

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    DnsFreeRRSet( pDnsRecord, TRUE );

    SetLastError( 0 );

    if ( haveanswer )
        return( &host );
    else
        return( (struct hostent *) NULL );
}


struct hostent *
QueryDnsCache_W(
    IN  LPWSTR   pszName,
    IN  WORD     wType,
    IN  DWORD    Options,
    IN  PVOID *  ppMsg OPTIONAL
    )
{
    DNS_STATUS  DnsStatus = NO_ERROR;
    PDNS_RECORD pDnsRecord = NULL, pTempDnsRecord = NULL;
    char        * bp, *ap, **ppHostAliases, **ppAddresses;
    int         buflen, length;
    int         haveanswer = 0;
    int         count = 0;

    WS2LOG_F1( "gethost.c - QueryDnsCache_W" );

    //
    // Query DNS Cache/Server for records ...
    //
    DnsStatus = DnsQuery_W( pszName,
                            wType,
                            Options,
                            NULL,
                            &pDnsRecord,
                            ppMsg );

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE )
    {
        SetLastError( WSATRY_AGAIN );
        return ((struct hostent *) NULL);
    }

    if ( DnsStatus )
    {
        if ( ppMsg && *ppMsg )
        {
            LocalFree( *ppMsg );
            *ppMsg = NULL;
        }

        SetLastError( DnsStatusToWS2Error( DnsStatus ) );
        return ((struct hostent *) NULL);
    }

    pTempDnsRecord = pDnsRecord;

    while ( pTempDnsRecord )
    {
        if ( pTempDnsRecord->Flags.S.Section == DNSREC_ANSWER &&
             ( pTempDnsRecord->wType == DNS_TYPE_A ||
               pTempDnsRecord->wType == DNS_TYPE_ATMA ||
               pTempDnsRecord->wType == DNS_TYPE_AAAA ) )
            count++;

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    host.h_addr_list = h_addr_ptrs;

    bp = hostbuf;
    buflen = BUFSIZ+1;
    memset( bp, 0, BUFSIZ );

    ap = hostaddr;
    memset( ap, 0, MAXADDRS );

    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    ppHostAliases = host.h_aliases;

    if ( count > MAXADDRS )
    {
        if ( SockBigBufSize >= count &&
             hostaddrBigBuf &&
             h_addr_ptrsBigBuf )
        {
            ap = hostaddrBigBuf;
            host.h_addr_list = h_addr_ptrsBigBuf;
            goto ProcessQuery;
        }

        if ( SockBigBufSize > 0 )
        {
            if ( hostaddrBigBuf )
            {
                FREE_HEAP( hostaddrBigBuf );
                hostaddrBigBuf = NULL;
            }

            if ( h_addr_ptrsBigBuf )
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
            }

            SockBigBufSize = 0;
        }

        h_addr_ptrsBigBuf = ALLOCATE_HEAP( ( count + 1 ) *
                                           sizeof( char * ) );

        if ( h_addr_ptrsBigBuf )
        {
            host.h_addr_list = h_addr_ptrsBigBuf;

            hostaddrBigBuf = ALLOCATE_HEAP( ( count + 1 ) *
                                            sizeof( IPV6_ADDRESS ) );
            if ( hostaddrBigBuf )
            {
                ap = hostaddrBigBuf;
                SockBigBufSize = count;
            }
            else
            {
                FREE_HEAP( h_addr_ptrsBigBuf );
                h_addr_ptrsBigBuf = NULL;
                ap = hostaddr;
                host.h_addr_list = h_addr_ptrs;
                count = MAXADDRS;
            }
        }
        else
        {
            ap = hostaddr;
            host.h_addr_list = h_addr_ptrs;
            count = MAXADDRS;
        }
    }

ProcessQuery :

    ppAddresses = host.h_addr_list;
    *ppAddresses = NULL;
    host.h_addrtype = AF_INET;
    host.h_length = 0;
    host.h_name = NULL;

    pTempDnsRecord = pDnsRecord;

    while ( ( wType == DNS_TYPE_A ||
              wType == DNS_TYPE_ATMA ||
              wType == DNS_TYPE_AAAA ) &&
            pTempDnsRecord )
    {
        if ( pTempDnsRecord->Flags.S.Section == DNSREC_ANSWER &&
             ( pTempDnsRecord->wType == DNS_TYPE_A ||
               pTempDnsRecord->wType == DNS_TYPE_ATMA ||
               pTempDnsRecord->wType == DNS_TYPE_AAAA ) )
            break;

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    if ( !pTempDnsRecord )
    {
        if ( pDnsRecord )
        {
            DnsFreeRRSet( pDnsRecord, TRUE );
        }

        SetLastError( DnsStatusToWS2Error( DNS_ERROR_RCODE_NAME_ERROR ) );
        return ((struct hostent *) NULL);
    }

    if ( pTempDnsRecord->wType != T_PTR &&
         pTempDnsRecord->pName )
        length = (wcslen( (LPWSTR) pTempDnsRecord->pName ) + 1) * sizeof(WCHAR);
    else
        length = 0;

    if ( length )
    {
        length += sizeof( align ) - ( length % sizeof( align ) );

        if ( buflen < length )
        {
            if ( pDnsRecord )
            {
                DnsFreeRRSet( pDnsRecord, TRUE );
            }

            WS_PRINT(("Buffer too small!"));
            SetLastError( WSA_NOT_ENOUGH_MEMORY );
            return ((struct hostent *) NULL);
        }

        wcscpy( (LPWSTR) bp, (LPWSTR) pTempDnsRecord->pName );
        host.h_name = bp;
        bp += length;
        buflen -= length;
    }
    else
    {
        if ( pTempDnsRecord->wType != T_PTR &&
             wcscmp( pszName, L"" ) )
        {
#if 0
            KdPrint(("\n\nWinSock2 RNR - QueryDnsCache_W problem!\n"));
            KdPrint((" Function was passed a name value of %S,\n", pszName));
            KdPrint(("yet DnsQuery_A returned a pDnsRecord with a NULL pName field for it!\n"));

            ASSERT( pTempDnsRecord->pName != NULL );
#endif // 0

            if ( pDnsRecord )
            {
                DnsFreeRRSet( pDnsRecord, TRUE );
            }

            SetLastError( DnsStatusToWS2Error( DNS_ERROR_RCODE_NAME_ERROR ) );
            return NULL;
        }
    }

    pTempDnsRecord = pDnsRecord;

    while ( pTempDnsRecord )
    {
        if ( pTempDnsRecord->Flags.S.Section != DNSREC_ANSWER ||
             ( pTempDnsRecord->Flags.S.Section == DNSREC_ANSWER &&
               ( pTempDnsRecord->wType == DNS_TYPE_A ||
                 pTempDnsRecord->wType == DNS_TYPE_ATMA ||
                 pTempDnsRecord->wType == DNS_TYPE_AAAA ) &&
               haveanswer >= count ) )
        {
            pTempDnsRecord = pTempDnsRecord->pNext;
            continue;
        }

        switch( pTempDnsRecord->wType )
        {
            case T_CNAME :
                if (ppHostAliases >= &host_aliases[MAXALIASES-1])
                    break;

                length = (wcslen( (LPWSTR) pTempDnsRecord->pName ) + 1) *
                         sizeof(WCHAR);
                length += sizeof( align ) - ( length % sizeof( align ) );
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                wcscpy( (LPWSTR) bp, (LPWSTR) pTempDnsRecord->pName );
                *ppHostAliases = bp;
                ppHostAliases++;
                *ppHostAliases = NULL;
                bp += length;
                buflen -= length;
                //
                // We don't have a valid hostent unless an address has
                // been found also. Bump haveanswer in T_A type and PTR
                // type only.
                //
                // haveanswer++;
                break;

            case T_PTR :
                length = (wcslen( (LPWSTR)
                          pTempDnsRecord->Data.PTR.pNameHost ) + 1) *
                          sizeof(WCHAR);
                length += sizeof( align ) - ( length % sizeof( align ) );
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                wcscpy( (LPWSTR) bp, (LPWSTR) pTempDnsRecord->Data.PTR.pNameHost );

                if ( host.h_name == NULL )
                    host.h_name = bp;
                else
                {
                    *ppHostAliases = bp;
                    ppHostAliases++;
                    *ppHostAliases = NULL;
                }

                bp += length;
                buflen -= length;
                host.h_length = sizeof( unsigned long );
                host.h_addr = hostaddr;
                haveanswer++;
                break;

            case T_A :
                length = pTempDnsRecord->wDataLength;
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                memcpy( ap, &pTempDnsRecord->Data.A.ipAddress, length );
                *ppAddresses = ap;
                ppAddresses++;
                *ppAddresses = NULL;
                ap += length;
                host.h_length = sizeof( unsigned long );
                haveanswer++;
                break;

            case DNS_TYPE_AAAA :
                length = pTempDnsRecord->wDataLength;
                if ( buflen < length )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }
                memcpy( ap, &pTempDnsRecord->Data.AAAA.ipv6Address, length );
                *ppAddresses = ap;
                ppAddresses++;
                *ppAddresses = NULL;
                ap += length;
                host.h_length = sizeof( IPV6_ADDRESS );
                host.h_addrtype = AF_INET6;
                haveanswer++;
                break;

            case T_ATMA :
                length = pTempDnsRecord->wDataLength;

                if ( buflen < sizeof( ATM_ADDRESS ) )
                {
                    WS_PRINT(("Buffer too small!"));
                    break;
                }

                memset( ap, 0, sizeof( ATM_ADDRESS ) );
                ((ATM_ADDRESS *) ap)->AddressType =
                    pTempDnsRecord->Data.ATMA.AddressType;
                ((ATM_ADDRESS *) ap)->NumofDigits = length - sizeof( BYTE );
                memcpy( &((ATM_ADDRESS *) ap)->Addr,
                        &pTempDnsRecord->Data.ATMA.Address,
                        length );
                *ppAddresses = ap;
                ppAddresses++;
                *ppAddresses = NULL;
                ap += sizeof( ATM_ADDRESS );
                host.h_length = sizeof( ATM_ADDRESS );
                host.h_addrtype = AF_ATM;
                haveanswer++;
                break;

            default :
                IF_DEBUG(RESOLVER)
                {
                    WS_PRINT(("unexpected answer type %d\n",
                              pTempDnsRecord->wType));
                }
                break;
        }

        pTempDnsRecord = pTempDnsRecord->pNext;
    }

    DnsFreeRRSet( pDnsRecord, TRUE );

    SetLastError( 0 );

    if ( haveanswer )
        return( &host );
    else
        return( (struct hostent *) NULL );
}


struct hostent *
QueryNbtWins(
    IN  LPSTR    pszOemName,
    IN  LPSTR    pszAnsiName
    )
{
    int    LocalAddress;
    char * bp, *ap, **ppHostAliases, **ppAddresses;
    int    buflen;
    WORD   length = strlen( pszOemName );
    WORD   iter, Count = 0;
    PULONG IpArray = NULL;
    BOOL   fDotted = FALSE;

    WS2LOG_F1( "gethost.c - QueryNbtWins" );

    host.h_addr_list = h_addr_ptrs;

    bp = hostbuf;
    buflen = BUFSIZ+1;
    memset( bp, 0, BUFSIZ );

    ap = hostaddr;
    memset( ap, 0, MAXADDRS );

    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    ppHostAliases = host.h_aliases;

    ppAddresses = host.h_addr_list;
    *ppAddresses = NULL;
    host.h_addrtype = AF_INET;
    host.h_length = sizeof( unsigned long );
    host.h_name = NULL;

    if ( pszOemName[length] == '.' )
    {
        pszOemName[length] = 0;
        fDotted = TRUE;
    }

    LocalAddress = SockNbtResolveName(pszOemName, &Count, &IpArray);

    if ( fDotted )
        pszOemName[length] = '.';

    if(LocalAddress != INADDR_NONE)
    {
        //
        // WINS found it.
        //
        // Make up a host entry that has all the addresses.
        //
        length += 1;
        strcpy( bp, pszAnsiName );
        host.h_name = bp;
        bp += length;
        buflen -= length;

        for ( iter = 0; iter < Count; iter++ )
        {
            memcpy( ap, &IpArray[iter], sizeof( ULONG ) );
            *ppAddresses = ap;
            ppAddresses++;
            *ppAddresses = NULL;
            ap += sizeof( ULONG );
        }

        if ( IpArray )
        {
            FREE_HEAP( IpArray );
        }

        if ( Count )
        {
            return( &host );
        }
    }

    return( (struct hostent *) NULL );
}


DWORD
DnsStatusToWS2Error (
    IN  DNS_STATUS DnsStatus
    )
{
    WS2LOG_F1( "gethost.c - DnsStatusToWS2Error" );

    switch( DnsStatus )
    {
        case DNS_ERROR_RCODE_NO_ERROR :
            return NO_ERROR;

        case DNS_ERROR_RCODE_NOT_IMPLEMENTED :
            return ERROR_NOT_SUPPORTED;

        case DNS_INFO_NO_RECORDS :
        case DNS_ERROR_RECORD_DOES_NOT_EXIST :
            return WSANO_DATA;

        case DNS_ERROR_RCODE_NAME_ERROR :
        case DNS_ERROR_RCODE_NOTZONE :
        case DNS_ERROR_NAME_NOT_IN_ZONE :
        case DNS_ERROR_NAME_DOES_NOT_EXIST :
        case ERROR_TIMEOUT :
            return WSAHOST_NOT_FOUND;

        case DNS_ERROR_RECORD_FORMAT :
        case DNS_ERROR_UNKNOWN_RECORD_TYPE :
        case DNS_ERROR_RCODE_REFUSED :
        case DNS_ERROR_BAD_PACKET :
        case DNS_ERROR_NO_PACKET :
            return WSANO_RECOVERY;

        case DNS_ERROR_NO_MEMORY :
            return WSA_NOT_ENOUGH_MEMORY;

        case DNS_ERROR_RCODE_FORMAT_ERROR :
        case DNS_ERROR_INVALID_NAME :
        case DNS_ERROR_INVALID_DATA :
        case DNS_ERROR_INVALID_TYPE :
        case DNS_ERROR_INVALID_IP_ADDRESS :
        case DNS_ERROR_INVALID_PROPERTY :
        case ERROR_INVALID_PARAMETER :
            return WSA_INVALID_PARAMETER;

        case DNS_ERROR_RCODE_SERVER_FAILURE :
        case DNS_ERROR_TRY_AGAIN_LATER :
            return WSATRY_AGAIN;

        case DNS_ERROR_RECORD_TIMED_OUT :
            return WSAETIMEDOUT;

        case DNS_ERROR_NO_TCPIP :
            return WSAENETDOWN;

        case DNS_ERROR_NO_DNS_SERVERS :
            return WSASERVICE_NOT_FOUND;

        case RPC_S_SERVER_UNAVAILABLE :
            return WSATRY_AGAIN;

        default :
#if DBG
            KdPrint(("\n\nWinSock2 RNR - DnsStatusToWS2Error - unhandled DNS status: 0x%8x\n", DnsStatus ));
#endif
            return WSAEINVAL;
    }
}


struct hostent *
_pTryReverseLookup_Nbt (
    IN  ULONG address
    )
{
    char  OemNameBuf[MAXDNAME];
    WCHAR UnicodeNameBuf[MAXDNAME];

    WS2LOG_F1( "gethost.c - _pTryReverseLookup_Nbt" );

    if ( SockNbtResolveAddr( address, OemNameBuf ) )
    {
        if ( MultiByteToWideChar( CP_OEMCP,
                                  0,
                                  OemNameBuf,
                                  -1,
                                  UnicodeNameBuf,
                                  sizeof( UnicodeNameBuf ) ) == 0 )
        {
            return NULL;
        }

        if ( WideCharToMultiByte( CP_ACP,
                                  0,
                                  UnicodeNameBuf,
                                  -1,
                                  &hostbuf[0],
                                  BUFSIZ,
                                  0,
                                  NULL ) == 0 )
        {
            return NULL;
        }

        host.h_addr_list = h_addr_ptrs;
        host.h_addr = hostaddr;
        host.h_length = sizeof (unsigned long);
        host.h_addrtype = AF_INET;
        *(PDWORD)host.h_addr_list[0] = address;
        host.h_addr_list[1] = NULL;

        host.h_name = &hostbuf[0];
        *host_aliases = NULL;
        host.h_aliases = host_aliases;

        SockThreadProcessingGetXByY = FALSE;
        WS_EXIT( "GETXBYYSP_gethostbyaddr", 0, TRUE );

        return (&host);
    }

    return NULL;
}


struct hostent *
_pTryReverseLookup_Dns (
    IN DWORD  dwControlFlags,
    IN LPWSTR name,
    IN ULONG  address,
    IN int    type,
    IN int    len
    )
{
    register struct hostent *hp;

    WS2LOG_F1( "gethost.c - _pTryReverseLookup_Dns" );

    IF_DEBUG(RESOLVER)
    {
        WS_PRINT(("GETXBYYSP_gethostbyaddr trying DNS Caching Resolver\n"));
    }

    //
    // Perform query through DNS Caching Resolver Service API
    //
    if ( dwControlFlags & LUP_FLUSHCACHE )
    {
        if ( dwControlFlags & LUP_RETURN_BLOB )
        {
            hp = QueryDnsCache( (LPSTR) name,
                                T_PTR,
                                DNS_QUERY_BYPASS_CACHE,
                                NULL );
        }
        else
        {
            hp = QueryDnsCache_W( name,
                                  T_PTR,
                                  DNS_QUERY_BYPASS_CACHE,
                                  NULL );
        }
    }
    else
    {
        if ( dwControlFlags & LUP_RETURN_BLOB )
        {
            hp = QueryDnsCache( (LPSTR) name,
                                T_PTR,
                                0,
                                NULL );
        }
        else
        {
            hp = QueryDnsCache_W( name,
                                  T_PTR,
                                  0,
                                  NULL );
        }
    }

    if (hp != NULL)
    {
        IF_DEBUG(GETXBYY)
        {
            WS_PRINT(("GETXBYYSP_gethostbyaddr DNS successful\n"));
        }

        *(PDWORD)hp->h_addr_list[0] = address;
        hp->h_addr_list[1] = NULL;

        SockThreadProcessingGetXByY = FALSE;
        WS_EXIT( "GETXBYYSP_gethostbyaddr", (INT)(LONG_PTR)hp, FALSE );
    }

    return( hp );
}


