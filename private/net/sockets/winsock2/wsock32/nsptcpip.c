/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Nsptcpip.c

Abstract:

    This module contains support for the Name Space Provider for TCPIP,
    which includes four actual NSPs:

        - local lookup for "loopback", "localhost", and the host name.

        - hosts file lookup.

        - DNS queries

        - WINS (Netbt) lookups

Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

--*/

#if defined(CHICAGO)
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#include <nspapi.h>
#include <nspapip.h>
#include <svcguid.h>
#include <nspmisc.h>

#define  SDK_DNS_RECORD     //  DNS API gets SDK defs
#include <dnsapi.h>

#define host_aliases   ACCESS_THREAD_DATA( host_aliases, GETHOST )

extern GUID    HostnameGuid;

#define DNS_PORT (htons(53))

#define HOSTENT_CACHING_ENABLED

#if defined(CHICAGO)

struct hostent * localhostent ( void);
struct hostent * myhostent ( void);

#define UDP_NAME "UDP/IP"
#define TCP_NAME "TCP/IP"

#if PACKETSZ > 1024
#define MAXPACKET       PACKETSZ
#else
#define MAXPACKET       1024
#endif

typedef union {
    HEADER hdr;
    unsigned char buf[MAXPACKET];
} querybuf;

LPHOSTENT
GetHostentFromName(
    IN LPSOCK_THREAD pThread,
    IN LPSTR Name
    );

#define _gethtbyname(name) GetHostentFromName(pThread, (LPSTR)name)

ULONG
SockNbtResolveName (
    IN  PCHAR    Name,
    OUT WORD *   pwIpCount OPTIONAL,
    OUT PULONG * pIpArray  OPTIONAL
    );

struct hostent *
dnshostent (
    void
    );

DWORD
CopyHostentToBuffer (
    char FAR *Buffer,
    int BufferLength,
    PHOSTENT Hostent
    );

DWORD
BytesInHostent (
    PHOSTENT Hostent
    );

#endif // defined(CHICAGO)

#if defined(HOSTENT_CACHING_ENABLED)

LIST_ENTRY HostentCacheListHead = { &HostentCacheListHead, &HostentCacheListHead };
DWORD MaxHostentCacheSize = 10;
DWORD CurrentHostentCacheSize = 0;

#if defined(CHICAGO)
#define MY_LARGE_INTEGER    DWORD
#define QUADPART(x)         (x)
#define LSTRCMPI            lstrcmpi
#define REG_OPEN_KEY_EX     RegOpenKeyExA
#else
#define MY_LARGE_INTEGER    LARGE_INTEGER
#define QUADPART(x)         (x).QuadPart
#define LSTRCMPI            _stricmp
#define REG_OPEN_KEY_EX     RegOpenKeyExW
#endif

typedef struct _HOSTENT_CACHE_ENTRY {
    LIST_ENTRY HostentListEntry;
    MY_LARGE_INTEGER LastAccessTime;
    MY_LARGE_INTEGER ExpirationTime;
    HOSTENT HostEntry;
    // BYTE[*];                                 // other hostent info
} HOSTENT_CACHE_ENTRY, *PHOSTENT_CACHE_ENTRY;

VOID
CacheHostent (
    IN PHOSTENT HostEntry,
    IN INT Ttl
    );

PHOSTENT
QueryHostentCache (
    IN LPSTR Name OPTIONAL,
    IN DWORD IpAddress OPTIONAL
    );

#endif // defined(HOSTENT_CACHING_ENABLED)

#define NSP_VERSION 1

VOID
CopyToAliasBuffer (
    IN PSTR Alias,
    IN OUT LPTSTR AliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN OUT LPDWORD AliasBytesUsed
    );

INT
GuidToPort (
    IN LPGUID lpServiceType,
    IN LPINT lpiProtocols,
    OUT LPWORD Port,
    OUT LPBOOL IsTcp
    );

INT
HostentToCsaddr (
    IN PHOSTENT HostEntry,
    IN WORD Port,
    IN BOOL IsTcp,
    IN OUT LPVOID lpCsaddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPTSTR lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN OUT LPDWORD Count
    );

VOID
ReadPriorities (
    VOID
    );

INT
ServiceCsaddr (
    IN WORD Port,
    IN BOOL IsTcp,
    IN OUT LPVOID lpCsaddrBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );

INT
APIENTRY
TcpipLocalGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    );

DWORD
APIENTRY
TcpipLocalSetService (
    IN DWORD          dwOperation,
    IN DWORD          dwFlags,
    IN BOOL           fUnicodeBlob,
    IN LPSERVICE_INFO lpServiceInfo
    );

INT
APIENTRY
TcpipHostsGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    );

INT
APIENTRY
TcpipDnsGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    );

INT
APIENTRY
TcpipNetbtGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    );

DWORD LocalPriority = NS_STANDARD_FAST_PRIORITY - 1;
DWORD HostsPriority = NS_STANDARD_FAST_PRIORITY;
DWORD DnsPriority = NS_STANDARD_PRIORITY;
DWORD NetbtPriority = NS_STANDARD_PRIORITY + 1;


INT
APIENTRY
NPLoadNameSpaces (
    IN OUT LPDWORD         lpdwVersion,
    IN OUT LPNS_ROUTINE    nsrBuffer,
    IN OUT LPDWORD         lpdwBufferLength
    )
{
    DWORD bufferLengthRequired;
    PNS_ROUTINE nsRoutineLocal;
    PNS_ROUTINE nsRoutineHosts;
    PNS_ROUTINE nsRoutineDns;
    PNS_ROUTINE nsRoutineNetbt;

    //
    // Calculate the size of a buffer the caller will need to specify.
    // We support four name spaces in this DLL, each of which exports
    // a single NSP API.
    //

    bufferLengthRequired = 4 * (sizeof(NS_ROUTINE) + (4 * sizeof(LPFN_NSPAPI)) );

    //
    // If the caller specified too small a buffer, fail and tell them
    // how large a buffer is required.
    //

    if ( bufferLengthRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bufferLengthRequired;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return -1;
    }

    //
    // Read the name space priorities from the registry.
    //

    ReadPriorities( );

    //
    // Tell the caller the version of the NSP API interface which we
    // support.
    //

    *lpdwVersion = 1;

    //
    // Calculate where in the buffer we'll put the NS_ROUTINE structures
    // for each of our name spaces.
    //

    nsRoutineLocal = nsrBuffer;
    nsRoutineHosts = nsrBuffer + 1;
    nsRoutineDns = nsrBuffer + 2;
    nsRoutineNetbt = nsrBuffer + 3;

    //
    // Calculate where in the buffer each of the function pointer arrays
    // will go.
    //

    nsRoutineLocal->alpfnFunctions = (LPFN_NSPAPI *)( nsRoutineNetbt + 1 );
    nsRoutineHosts->alpfnFunctions = nsRoutineLocal->alpfnFunctions + 4;
    nsRoutineDns->alpfnFunctions = nsRoutineHosts->alpfnFunctions + 4;
    nsRoutineNetbt->alpfnFunctions = nsRoutineDns->alpfnFunctions + 4;

    //
    // Fill in each of the NS_ROUTINE structures.
    //

    nsRoutineLocal->dwFunctionCount = 4;
    nsRoutineLocal->alpfnFunctions[NSPAPI_GET_ADDRESS_BY_NAME] =
        (LPFN_NSPAPI)TcpipLocalGetAddressByName;
    nsRoutineLocal->alpfnFunctions[NSPAPI_GET_SERVICE] = NULL;
    nsRoutineLocal->alpfnFunctions[NSPAPI_SET_SERVICE] =
        (LPFN_NSPAPI)TcpipLocalSetService;
    nsRoutineLocal->alpfnFunctions[3] = NULL;
    nsRoutineLocal->dwNameSpace = NS_TCPIP_LOCAL;
    nsRoutineLocal->dwPriority = LocalPriority;

    nsRoutineHosts->dwFunctionCount = 4;
    nsRoutineHosts->alpfnFunctions[NSPAPI_GET_ADDRESS_BY_NAME] =
        (LPFN_NSPAPI)TcpipHostsGetAddressByName;
    nsRoutineHosts->alpfnFunctions[NSPAPI_GET_SERVICE] = NULL;
    nsRoutineHosts->alpfnFunctions[NSPAPI_SET_SERVICE] = NULL;
    nsRoutineHosts->alpfnFunctions[3] = NULL;
    nsRoutineHosts->dwNameSpace = NS_TCPIP_HOSTS;
    nsRoutineHosts->dwPriority = HostsPriority;

    nsRoutineDns->dwFunctionCount = 4;
    nsRoutineDns->alpfnFunctions[NSPAPI_GET_ADDRESS_BY_NAME] =
        (LPFN_NSPAPI)TcpipDnsGetAddressByName;
    nsRoutineDns->alpfnFunctions[NSPAPI_GET_SERVICE] = NULL;
    nsRoutineDns->alpfnFunctions[NSPAPI_SET_SERVICE] = NULL;
    nsRoutineDns->alpfnFunctions[3] = NULL;
    nsRoutineDns->dwNameSpace = NS_DNS;
    nsRoutineDns->dwPriority = DnsPriority;

    nsRoutineNetbt->dwFunctionCount = 4;
    nsRoutineNetbt->alpfnFunctions[NSPAPI_GET_ADDRESS_BY_NAME] =
        (LPFN_NSPAPI)TcpipNetbtGetAddressByName;
    nsRoutineNetbt->alpfnFunctions[NSPAPI_GET_SERVICE] = NULL;
    nsRoutineNetbt->alpfnFunctions[NSPAPI_SET_SERVICE] = NULL;
    nsRoutineNetbt->alpfnFunctions[3] = NULL;
    nsRoutineNetbt->dwNameSpace = NS_NETBT;
    nsRoutineNetbt->dwPriority = NetbtPriority;

    return 4;

} // NpLoadNameSpaces


VOID
ReadPriorities (
    VOID
    )
{
    HKEY tcpipKey;
    INT error;
    DWORD entryLength = sizeof(DWORD);
    DWORD type;

    //
    // First open the TCPIP network provider key.
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TCPIP_SERVICE_PROVIDER_KEY,
                0,
                KEY_READ,
                &tcpipKey
                );
    if ( error != NO_ERROR ) {
        return;
    }

    //
    // Read each of the priority values.  If any of these fails, we'll
    // just go with the default values.
    //

    RegQueryValueEx(
        tcpipKey,
        TEXT("LocalPriority"),
        NULL,
        &type,
        (PVOID)&LocalPriority,
        &entryLength
        );

    RegQueryValueEx(
        tcpipKey,
        TEXT("HostsPriority"),
        NULL,
        &type,
        (PVOID)&HostsPriority,
        &entryLength
        );

    RegQueryValueEx(
        tcpipKey,
        TEXT("DnsPriority"),
        NULL,
        &type,
        (PVOID)&DnsPriority,
        &entryLength
        );

    RegQueryValueEx(
        tcpipKey,
        TEXT("NetbtPriority"),
        NULL,
        &type,
        (PVOID)&NetbtPriority,
        &entryLength
        );

#if defined(HOSTENT_CACHING_ENABLED)
    RegQueryValueEx(
        tcpipKey,
        TEXT("MaxHostentCacheSize"),
        NULL,
        &type,
        (PVOID)&MaxHostentCacheSize,
        &entryLength
        );
#endif

    RegCloseKey( tcpipKey );
    return;

} // ReadPriorities


INT
APIENTRY
TcpipLocalGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    )
{
    WORD port;
    INT error;
    BOOL isTcp;
    PHOSTENT hostEntry;
    DWORD count;
    PSTR hostName;
    PTSTR compareHostName;

#if defined(UNICODE)
    UNICODE_STRING unicodeHostName;
    ANSI_STRING ansiHostName;
#endif
    NTSTATUS status;

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(0);
    }

    //
    // Find the port number for this service.  We do this before other
    // resolutions to save time in case this fails.
    //

    error = GuidToPort( lpServiceType, lpiProtocols, &port, &isTcp );
    if ( error != NO_ERROR ) {
        return 0;
    }

    //
    // If this is a service resolution, we don't actually want to do a
    // lookup.  Just fill in the appropriate bind address immediately.
    //

    if ( (dwResolution & RES_SERVICE) != 0 ) {
        return ServiceCsaddr( port, isTcp, lpCsaddrBuffer, lpdwBufferLength );
    }

    //
    // Check whether we have information for this name cached.  Note
    // that we don't look in the cache if RES_FIND_MULTIPLE is set since
    // we'll be going to all the providers anyway and the info returned
    // here will be extraneous and could cause problems with
    // lpdwBufferLength if the app makes multiple calls for the same
    // data.
    //

#if defined(HOSTENT_CACHING_ENABLED)

    if ( (dwResolution & RES_FIND_MULTIPLE) == 0 &&
             CurrentHostentCacheSize > 0 ) {

        //
        // Get the host name in ansi for cache comparisons.
        //

#if defined(UNICODE)
        RtlInitUnicodeString( &unicodeHostName, lpServiceName );

        status = RtlUnicodeStringToAnsiString( &ansiHostName, &unicodeHostName, TRUE );
#else
        status = STATUS_SUCCESS;
#endif

        if ( NT_SUCCESS(status) ) {

            //
            // We hold the hostent cache lock until we're completely
            // done with any returned hostent.  This prevents another
            // thread from screwing with the list or the returned
            // hostent while we're processing.
            //

            SockAcquireGlobalLockExclusive( );

            //
            // Attempt to find the name in the hostent cache.
            //

#if defined(UNICODE)
            hostEntry = QueryHostentCache( ansiHostName.Buffer, 0 );

            RtlFreeAnsiString( &ansiHostName );
#else
            hostEntry = QueryHostentCache(lpServiceName, 0);
#endif

            //
            // If we found it, copy it to the user buffer and return.
            //

            if ( hostEntry != NULL ) {

                error = HostentToCsaddr(
                            hostEntry,
                            port,
                            isTcp,
                            lpCsaddrBuffer,
                            lpdwBufferLength,
                            lpAliasBuffer,
                            lpdwAliasBufferLength,
                            &count
                            );

                SockReleaseGlobalLock( );

                if ( error != NO_ERROR ) {
                    return 0;
                }

                return count;
            }

            SockReleaseGlobalLock( );
        }
    }
#endif // defined(HOSTENT_CACHING_ENABLED)


    //
    // If the service name is present and equal to "localhost" or
    // "loopback", return 127.0.0.1.
    //

    if ( lpServiceName != NULL &&
         (_tcscmp( lpServiceName, TEXT("localhost") ) == 0 ||
          _tcscmp( lpServiceName, TEXT("loopback") ) == 0) ) {

        hostEntry = localhostent( );
        if ( hostEntry == NULL ) {
            return 0;
        }

        error = HostentToCsaddr(
                    hostEntry,
                    port,
                    isTcp,
                    lpCsaddrBuffer,
                    lpdwBufferLength,
                    lpAliasBuffer,
                    lpdwAliasBufferLength,
                    &count
                    );
        if ( error != NO_ERROR ) {
            return 0;
        }

        return count;
    }

    //
    // If the service name is NULL and the caller is requesting info on
    // DNS addresses, return information on the DNS server IP addresses
    // that we know about.
    //

    if ( lpServiceName == NULL && port == DNS_PORT ) {

        hostEntry = dnshostent( );
        if ( hostEntry == NULL ) {
            return 0;
        }

        error = HostentToCsaddr(
                    hostEntry,
                    port,
                    isTcp,
                    lpCsaddrBuffer,
                    lpdwBufferLength,
                    lpAliasBuffer,
                    lpdwAliasBufferLength,
                    &count
                    );
        if ( error != NO_ERROR ) {
            return 0;
        }

        return count;
    }

    //
    // Check whether the service name is the same as the local host
    // name.  Note that we consider lpServiceName == NULL to be the
    // local host.
    //

    hostName = ALLOCATE_HEAP( 256 );
    if ( hostName == NULL ) {
        return 0;
    }

    error = gethostname( hostName, 256 );
    if ( error ) {
        return 0;
    }

#if defined(UNICODE)
    RtlInitAnsiString( &ansiHostName, hostName );
    status = RtlAnsiStringToUnicodeString( &unicodeHostName, &ansiHostName, TRUE );
    FREE_HEAP( hostName );
    if ( !NT_SUCCESS(status) ) {
        return 0;
    }
    compareHostName = unicodeHostName.Buffer;
#else
    compareHostName = hostName;
#endif

    if ( lpServiceName == NULL ||
             _tcscmp( lpServiceName, compareHostName ) == 0 ) {

#if defined(UNICODE)
        RtlFreeUnicodeString( &unicodeHostName );
#else
        FREE_HEAP( hostName );
#endif

        hostEntry = myhostent( );
        if ( hostEntry == NULL ) {
            return 0;
        }

        error = HostentToCsaddr(
                    hostEntry,
                    port,
                    isTcp,
                    lpCsaddrBuffer,
                    lpdwBufferLength,
                    lpAliasBuffer,
                    lpdwAliasBufferLength,
                    &count
                    );
        if ( error != NO_ERROR ) {
            return 0;
        }

        return count;
    }

#if defined(UNICODE)
    RtlFreeUnicodeString( &unicodeHostName );
#else
    FREE_HEAP( hostName );
#endif

    //
    // If we're not inside gethostbyname(), check if the service name is
    // really an IP address.  We intentionally disable this for
    // gethostbyname() calls because we don't want to break apps which
    // depend on gethostname() not resolving IP addresses.  However,
    // this is a useful feature for general RNR applications.
    //

    if ( (dwResolution & RES_GETHOSTBYNAME) == 0 ) {

        PSTR serviceString;
        IN_ADDR inAddr;
        HOSTENT hostEntryStruct;
        PBYTE nullAlias = NULL;
        PBYTE addrs[2];

#if defined(UNICODE)
        RtlInitUnicodeString( &unicodeHostName, lpServiceName );
        status = RtlUnicodeStringToAnsiString( &ansiHostName, &unicodeHostName, TRUE );
        if ( !NT_SUCCESS(status) ) {
            return 0;
        }
        serviceString = ansiHostName.Buffer;
#else
        serviceString = lpServiceName;
#endif

        //
        // Attempt to translate the string into an IP address value.
        //

        inAddr.s_addr = inet_addr( serviceString );

        if ( inAddr.s_addr != INADDR_NONE ) {

            hostEntryStruct.h_name = serviceString;
            hostEntryStruct.h_aliases = &nullAlias;
            hostEntryStruct.h_addrtype = AF_INET;
            hostEntryStruct.h_length = sizeof(DWORD);
            hostEntryStruct.h_addr_list = addrs;
            addrs[0] = (char *)(&inAddr);
            addrs[1] = NULL;

            error = HostentToCsaddr(
                        &hostEntryStruct,
                        port,
                        isTcp,
                        lpCsaddrBuffer,
                        lpdwBufferLength,
                        lpAliasBuffer,
                        lpdwAliasBufferLength,
                        &count
                        );
#if defined(UNICODE)
            RtlFreeAnsiString( &ansiHostName );
#endif
            if ( error != NO_ERROR ) {
                return 0;
            }

            return count;
        }
#if defined(UNICODE)
            RtlFreeAnsiString( &ansiHostName );
#endif
    }

    return 0;

} // TcpipLocalGetAddressByName


INT
APIENTRY
TcpipHostsGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    )
{
    WORD port;
    INT error;
    BOOL isTcp;
    PHOSTENT hostEntry;
    DWORD count;
    PSTR hostName;

#if defined(CHICAGO)
    LPSOCK_THREAD pThread;
#endif

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(0);
    }

    //
    // If this is a service resolution, don't do anything here.  Only the
    // TCP/IP local NSP will handle RES_SERVICE requests.  This prevents
    // getting back multiple TCP/IP addresses to bind to, one from each
    // TCP/IP NSP.
    //

    if ( (dwResolution & RES_SERVICE) != 0 ) {
        return 0;
    }

    //
    // Find the port number for this service.  We do this before other
    // resolutions to save time in case this fails.
    //

    error = GuidToPort( lpServiceType, lpiProtocols, &port, &isTcp );
    if ( error != NO_ERROR ) {
        return 0;
    }

    //
    // Convert the service name to ANSI.
    //

    hostName = GetAnsiName( lpServiceName );
    if ( hostName == NULL ) {
        return 0;
    }

    //
    // Attempt to get a host entry from the hosts file lookup routine.
    //

    hostEntry = _gethtbyname( hostName );

    FREE_HEAP( hostName );

    if ( hostEntry == NULL ) {
        return 0;
    }

    error = HostentToCsaddr(
                hostEntry,
                port,
                isTcp,
                lpCsaddrBuffer,
                lpdwBufferLength,
                lpAliasBuffer,
                lpdwAliasBufferLength,
                &count
                );
    if ( error != NO_ERROR ) {
        return 0;
    }

    return count;

} // TcpipHostsGetAddressByName


INT
APIENTRY
TcpipDnsGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    )
{
    WORD port;
    INT error;
    BOOL isTcp;
    PHOSTENT hostEntry = NULL;
    INT count;
    PSTR hostName;

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(0);
    }

    //
    // If this is a service resolution, don't do anything here.  Only the
    // TCP/IP local NSP will handle RES_SERVICE requests.  This prevents
    // getting back multiple TCP/IP addresses to bind to, one from each
    // TCP/IP NSP.
    //

    if ( (dwResolution & RES_SERVICE) != 0 ) {
        return 0;
    }

    //
    // Find the port number for this service.  We do this before other
    // resolutions to save time in case this fails.
    //
    error = GuidToPort( lpServiceType, lpiProtocols, &port, &isTcp );
    if ( error != NO_ERROR ) {
        return 0;
    }

    //
    // Convert the service name to ANSI.
    //
    hostName = GetAnsiName( lpServiceName );

    if ( hostName == NULL )
        return 0;

    //
    // Perform query through DNS Caching Resolver Service API
    //
    hostEntry = QueryDnsCache( hostName, T_A, 0, NULL );

    FREE_HEAP( hostName );

    if ( hostEntry == NULL )
        return 0;

    error = HostentToCsaddr(
                hostEntry,
                port,
                isTcp,
                lpCsaddrBuffer,
                lpdwBufferLength,
                lpAliasBuffer,
                lpdwAliasBufferLength,
                &count
                );

    if ( error != NO_ERROR )
        return 0;

    return count;

} // TcpipDnsGetAddressByName


INT
APIENTRY
TcpipNetbtGetAddressByName (
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength,
    IN     HANDLE          hCancellationEvent
    )
{
    WORD port;
    INT error;
    BOOL isTcp;
    HOSTENT hostEntry;
    DWORD count;
    DWORD ipAddress;
    PVOID addressPtrs[2];
    PSTR hostName;
    PSTR oemHostName;

#if defined(CHICAGO)
    LPSOCK_THREAD pThread;
#endif

    //
    // If this is a service resolution, don't do anything here.  Only the
    // TCP/IP local NSP will handle RES_SERVICE requests.  This prevents
    // getting back multiple TCP/IP addresses to bind to, one from each
    // TCP/IP NSP.
    //

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(0);
    }

    if ( (dwResolution & RES_SERVICE) != 0 ) {
        return 0;
    }

    //
    // If we're not supposed to do WINS name resolution for this thread,
    // stop.
    //

    if ( SockDisableWinsNameResolution ) {
        return 0;
    }

    //
    // Find the port number for this service.  We do this before other
    // resolutions to save time in case this fails.
    //

    error = GuidToPort( lpServiceType, lpiProtocols, &port, &isTcp );
    if ( error != NO_ERROR ) {
        return 0;
    }

    //
    // Convert the service name to ANSI.
    //

    hostName = GetAnsiName( lpServiceName );
    if ( hostName == NULL ) {
        return 0;
    }

    oemHostName = GetOemName( lpServiceName );
    if ( oemHostName == NULL )
    {
        FREE_HEAP( hostName );
        return 0;
    }

    //
    // Attempt to get the IP address from the WINS client.
    //

    ipAddress = SockNbtResolveName( oemHostName, NULL, NULL );
    if ( ipAddress == INADDR_NONE ) {
        FREE_HEAP( hostName );
        FREE_HEAP( oemHostName );
        return 0;
    }

    //
    // Fake up a hostent and convert to a CSADDR structure.
    //

    hostEntry.h_name = hostName;

    hostEntry.h_addr_list = (PCHAR *)addressPtrs;
    addressPtrs[0] = &ipAddress;
    addressPtrs[1] = NULL;

    hostEntry.h_length = sizeof (unsigned long);
    hostEntry.h_addrtype = AF_INET;
    hostEntry.h_aliases = host_aliases;
    host_aliases[0] = NULL;

    error = HostentToCsaddr(
                &hostEntry,
                port,
                isTcp,
                lpCsaddrBuffer,
                lpdwBufferLength,
                lpAliasBuffer,
                lpdwAliasBufferLength,
                &count
                );
    if ( error != NO_ERROR ) {
        FREE_HEAP( hostName );
        FREE_HEAP( oemHostName );
        return 0;
    }

    //
    // Store the host's information in our cache with a 10-minute timeout.
    //

#if defined(HOSTENT_CACHING_ENABLED)
    CacheHostent( &hostEntry, 60*10 );
#endif

    FREE_HEAP( hostName );
    FREE_HEAP( oemHostName );
    return count;

} // TcpipNetbtGetAddressByName


DWORD
APIENTRY
TcpipLocalSetService (
    IN DWORD          dwOperation,
    IN DWORD          dwFlags,
    IN BOOL           fUnicodeBlob,
    IN LPSERVICE_INFO lpServiceInfo
    )
{
    DWORD err;
    HKEY hkey = NULL;
    HKEY hkeyType = NULL;

    SERVICE_TYPE_INFO *pSvcTypeInfo = (SERVICE_TYPE_INFO *)
        lpServiceInfo->ServiceSpecificInfo.pBlobData;
    LPTSTR pszSvcTypeName;
    DWORD i;
    PSERVICE_TYPE_VALUE pVal;
#if defined(UNICODE)
    UNICODE_STRING uniStr;
#endif

    //
    // We need to take action only on SERVICE_ADD_TYPE.  The rest are
    // meaningless for the static name spaces we support.
    //

    if ( dwOperation != SERVICE_ADD_TYPE ) {
        return NO_ERROR;
    }

    //
    // Get the new service type name
    //

#if defined(UNICODE)

    if ( fUnicodeBlob ) {

        pszSvcTypeName = (LPWSTR) (((LPBYTE) pSvcTypeInfo) +
                                   pSvcTypeInfo->dwTypeNameOffset );

    } else {

        ANSI_STRING ansiStr;

        RtlInitAnsiString( &ansiStr,
                           (LPSTR) (((LPBYTE) pSvcTypeInfo) +
                                    pSvcTypeInfo->dwTypeNameOffset ));

        err = RtlAnsiStringToUnicodeString( &uniStr, &ansiStr, TRUE );
        if ( err )
            return err;

        pszSvcTypeName = uniStr.Buffer;
    }
#else
    pszSvcTypeName = (LPSTR) (((LPBYTE) pSvcTypeInfo) +
                               pSvcTypeInfo->dwTypeNameOffset );
#endif

    //
    // If the service type name is an empty string, return error.
    //

    if (  ( pSvcTypeInfo->dwTypeNameOffset == 0 ) ||
          ( pszSvcTypeName == NULL ) ||
          ( *pszSvcTypeName == 0 ) ) {

        err = ERROR_INVALID_PARAMETER;
        goto CleanExit;
    }

    //
    // The following keys should have already been created
    //

    err = REG_OPEN_KEY_EX(
              HKEY_LOCAL_MACHINE,
              NSP_SERVICE_KEY_NAME,
              0,
              KEY_READ | KEY_WRITE,
              &hkey
              );

    if ( err ) {
        goto CleanExit;
    }

    err = REG_OPEN_KEY_EX(
              hkey,
              pszSvcTypeName,
              0,
              KEY_READ | KEY_WRITE,
              &hkeyType
              );

    if ( err ) {
        goto CleanExit;
    }

    //
    // Loop through all values in the specific and add them one by one
    // to the registry if it belongs to our name space
    //

    for ( i = 0, pVal = pSvcTypeInfo->Values;
          i < pSvcTypeInfo->dwValueCount;
          i++, pVal++ )
    {
        //
        // Skip over values not aimed at one of the TCPIP name spaces.
        //

        if ( pVal->dwNameSpace != NS_TCPIP_LOCAL &&
                 pVal->dwNameSpace != NS_TCPIP_HOSTS &&
                 pVal->dwNameSpace != NS_DNS &&
                 pVal->dwNameSpace != NS_NETBT ) {
            continue;
        }

        if ( fUnicodeBlob ) {

            err = RegSetValueExW(
                      hkeyType,
                      (LPWSTR) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueNameOffset),
                      0,
                      pVal->dwValueType,
                      (LPBYTE) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueOffset),
                      pVal->dwValueSize
                      );

        } else {

            err = RegSetValueExA(
                      hkeyType,
                      (LPSTR) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueNameOffset),
                      0,
                      pVal->dwValueType,
                      (LPBYTE) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueOffset),
                      pVal->dwValueSize
                      );
        }
    }

CleanExit:

#if defined(UNICODE)
    if ( !fUnicodeBlob ) {
        RtlFreeUnicodeString( &uniStr );
    }
#endif

    if ( hkeyType != NULL ) {
        RegCloseKey( hkeyType );
    }

    if ( hkey != NULL ) {
        RegCloseKey( hkey );
    }

    return err;

} // TcpipLocalSetService


INT
GuidToPort (
    IN LPGUID lpServiceType,
    IN LPINT   lpiProtocols,
    OUT LPWORD Port,
    OUT LPBOOL IsTcp
    )
{
    TCHAR nameBuffer[256];
    TCHAR keyNameBuffer[256];
    BOOL isTcp;
    BOOL isUdp;
    BOOL guidIsTcp;
    DWORD i;
    BOOL ianaGuid;
    HKEY serviceKey;
    DWORD regPort;
    DWORD type;
    INT err;
    DWORD bytesRequired;

    //
    // Determine whether they want TCP, UDP, or both.  If there was no
    // protocols list passed in, they want both.  Otherwise, they must
    // specify on or both manually.
    //

    isTcp = FALSE;
    isUdp = FALSE;

    if ( !ARGUMENT_PRESENT( lpiProtocols ) ) {

        isTcp = TRUE;
        isUdp = TRUE;

    } else {

        for ( i = 0; lpiProtocols[i] != 0; i++ ) {

            if ( lpiProtocols[i] == IPPROTO_TCP ) {
                isTcp = TRUE;
            }

            if ( lpiProtocols[i] == IPPROTO_UDP ) {
                isUdp = TRUE;
            }
        }
    }

    //
    // If they want neither TCP nor UDP, bail.
    //

    if ( !isTcp && !isUdp ) {
        return -1;
    }

    //
    // If this is the special hostname GUID, we don't need to find a
    // port.
    //

    if ( GuidEqual( lpServiceType, &HostnameGuid ) ) {
        *Port = 0;
        *IsTcp = TRUE;
        return NO_ERROR;
    }

    //
    // If this is an IANA GUID, shortcut to the answer.
    //

    ianaGuid = FALSE;

    if ( IS_SVCID_TCP( lpServiceType ) ) {
        guidIsTcp = TRUE;
        ianaGuid = TRUE;
        *Port = htons(PORT_FROM_SVCID_TCP( lpServiceType ));
    }

    if ( IS_SVCID_UDP( lpServiceType ) ) {
        guidIsTcp = FALSE;
        ianaGuid = TRUE;
        *Port = htons(PORT_FROM_SVCID_UDP( lpServiceType ));
    }

    if ( ianaGuid ) {

        //
        // If an incorrect protocol type was specified for the GUID, fail.
        //

        if ( (guidIsTcp && !isTcp) || (!guidIsTcp && !isUdp) ) {
            return -1;
        }

        *IsTcp = guidIsTcp;

        return NO_ERROR;
    }

    //
    // Convert the GUID into a service name.  We'll first check whether
    // port information is in the registry.  An application/service
    // writes this information to the registry by using SetService()
    // with SERVICE_ADD_TYPE.
    //

    err = GetNameByType( lpServiceType, nameBuffer, sizeof(nameBuffer) );
    if ( err != NO_ERROR ) {
        return -1;
    }

    //
    // Attempt to open the service type key.
    //

    _tcscpy( keyNameBuffer, NSP_SERVICE_KEY_NAME TEXT("\\") );
    _tcscat( keyNameBuffer, nameBuffer );

    err = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE,
              keyNameBuffer,
              0,
              KEY_READ,
              &serviceKey
              );

    if ( err != NO_ERROR ) {
        return -1;
    }

    //
    // Attempt to read the "TcpPort" or "UdpPort" values from this key.
    // Note that we sort of ignore errors here; we act based on whether
    // the regPort local variable changes from 0xFFFFFFFF.
    //
    // Also, we don't have any support for a single GUID which supports
    // both TCP and UDP.  This currently requires separate GUIDs.
    //

    regPort = 0xFFFFFFFF;
    guidIsTcp = TRUE;
    bytesRequired = sizeof(regPort);

    if ( isTcp ) {
        RegQueryValueEx(
            serviceKey,
            TEXT("TcpPort"),
            NULL,
            &type,
            (PVOID)&regPort,
            &bytesRequired
            );
    }

    if ( regPort == 0xFFFFFFFF && isUdp ) {
        RegQueryValueEx(
            serviceKey,
            TEXT("UdpPort"),
            NULL,
            &type,
            (PVOID)&regPort,
            &bytesRequired
            );
        guidIsTcp = FALSE;
    }

    RegCloseKey( serviceKey );

    //
    // If we got a hit, then we're all set.  Use this port.
    //

    if ( regPort != 0xFFFFFFFF ) {
        *IsTcp = guidIsTcp;
        *Port = htons( (WORD)regPort );
        return NO_ERROR;
    }

    //
    // We coultn't find a port number for this service type.  Fail.
    //

    return -1;

} // GuidToPort


INT
HostentToCsaddr (
    IN PHOSTENT HostEntry,
    IN WORD Port,
    IN BOOL IsTcp,
    IN OUT LPVOID lpCsaddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPTSTR lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN OUT LPDWORD Count
    )
{
    DWORD count;
    DWORD i;
    DWORD requiredBufferLength;
    PSOCKADDR_IN sockaddrIn;
    PCSADDR_INFO csaddrInfo;
    DWORD aliasBytesUsed;

    //
    // Count the number of IP addresses in the hostent.
    //

    for ( count = 0; HostEntry->h_addr_list[count] != NULL; count++ );

    //
    // Make sure that the buffer is large enough to hold all the entries
    // which will be necessary.
    //

    requiredBufferLength = count * (sizeof(CSADDR_INFO) + 2*sizeof(SOCKADDR_IN));

    if ( *lpdwBufferLength < requiredBufferLength ) {
        *lpdwBufferLength = requiredBufferLength;
        return (DWORD)-1;
    }

    *lpdwBufferLength = requiredBufferLength;

    //
    // For each IP address, fill in the user buffer with one entry.
    //

    sockaddrIn = (PSOCKADDR_IN)((PCSADDR_INFO)lpCsaddrBuffer + count);
    csaddrInfo = lpCsaddrBuffer;

    for ( i = 0; i < count; i++, csaddrInfo++, sockaddrIn++ ) {

        //
        // First fill in the local address.  It should remain basically
        // all zeros except for the family so that it is a "wildcard"
        // address for binding.
        //

        RtlZeroMemory( csaddrInfo, sizeof(*csaddrInfo) );

        csaddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)sockaddrIn;
        csaddrInfo->LocalAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

        RtlZeroMemory( sockaddrIn, sizeof(*sockaddrIn) );
        sockaddrIn->sin_family = AF_INET;

        //
        // Now the remote address.
        //

        sockaddrIn++;

        csaddrInfo->RemoteAddr.lpSockaddr = (PSOCKADDR)( sockaddrIn );
        csaddrInfo->RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

        sockaddrIn = (PSOCKADDR_IN)csaddrInfo->RemoteAddr.lpSockaddr;
        RtlZeroMemory( sockaddrIn, sizeof(*sockaddrIn) );
        sockaddrIn->sin_family = AF_INET;

        //
        // Fill in the remote address with the actual address, both port
        // and IP address.
        //

        sockaddrIn->sin_port = Port;
        sockaddrIn->sin_addr.s_addr =
            *((long *)(HostEntry->h_addr_list[i]));

        //
        // Lastly, fill in the protocol information.
        //

        if ( IsTcp ) {
            csaddrInfo->iSocketType = SOCK_STREAM;
            csaddrInfo->iProtocol = IPPROTO_TCP;
        } else {
            csaddrInfo->iSocketType = SOCK_DGRAM;
            csaddrInfo->iProtocol = IPPROTO_UDP;
        }
    }

    //
    // Copy the aliases to the alias buffer.  The "official" name from the
    // hostent goes as the first alias, then the actual aliases.
    //

    if ( ARGUMENT_PRESENT( lpAliasBuffer ) &&
             ARGUMENT_PRESENT( lpdwAliasBufferLength ) ) {

        //
        // Copy over the official name first.
        //

        aliasBytesUsed = 0;

        if ( HostEntry->h_name != NULL )
        {
            CopyToAliasBuffer(
                HostEntry->h_name,
                lpAliasBuffer,
                lpdwAliasBufferLength,
                &aliasBytesUsed
                );
        }

        if ( HostEntry->h_aliases != NULL ) {

            for ( i = 0; HostEntry->h_aliases[i] != NULL; i++ ) {

                CopyToAliasBuffer(
                    HostEntry->h_aliases[i],
                    lpAliasBuffer,
                    lpdwAliasBufferLength,
                    &aliasBytesUsed
                    );
            }
        }

        //
        // Set up the final zero-terminator on the alias list.
        //

        aliasBytesUsed += sizeof(TCHAR);

        if ( aliasBytesUsed <= *lpdwAliasBufferLength ) {
            lpAliasBuffer[(aliasBytesUsed / sizeof(TCHAR)) - 1] = L'\0';
        }

        //
        // Tell the caller how much of the alias buffer we used.
        //

        *lpdwAliasBufferLength = aliasBytesUsed;
    }

    *Count = count;

    return NO_ERROR;

} // HostentToCsaddr


VOID
CopyToAliasBuffer (
    IN PSTR Alias,
    IN OUT LPTSTR AliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN OUT LPDWORD AliasBytesUsed
    )
{
#if defined(UNICODE)
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
#endif

    //
    // Point to the first unused byte in the alias buffer.
    //

    AliasBuffer = (LPTSTR)( (PBYTE)AliasBuffer + *AliasBytesUsed );

    //
    // Initialize string structures for the alias.
    //

#if defined(UNICODE)
    RtlInitAnsiString( &ansiString, Alias );

    unicodeString.Buffer = AliasBuffer;
    unicodeString.MaximumLength =
        (USHORT)(*lpdwAliasBufferLength - *AliasBytesUsed);
#endif

    //
    // Make sure that there is enough room left in the alias buffer.
    //

    *AliasBytesUsed += (strlen( Alias ) + 1) * sizeof(TCHAR);
    if ( *AliasBytesUsed > *lpdwAliasBufferLength ) {
        return;
    }

    //
    // Convert the ANSI name to Unicode and copy it to the alias buffer.
    //

#if defined(UNICODE)
    RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, FALSE );
#else
    _tcscpy( AliasBuffer, Alias );
#endif

    return;

} // CopyToAliasBuffer

#if defined(HOSTENT_CACHING_ENABLED)


VOID
CacheHostent (
    IN PHOSTENT HostEntry,
    IN INT Ttl
    )
{
    MY_LARGE_INTEGER currentTime;
    MY_LARGE_INTEGER liTtl;
    DWORD bytesRequired;
    PHOSTENT_CACHE_ENTRY cacheEntry;
    PLIST_ENTRY listEntry;
    PHOSTENT_CACHE_ENTRY testCacheEntry;
    PHOSTENT_CACHE_ENTRY oldestCacheEntry;

    //
    // If the TTL is 0, do not cache the entry.
    //

    if ( ( Ttl <= 0 ) || ( MaxHostentCacheSize == 0 ) ) {
        return;
    }

    //
    // Get the current time and convert the TTL to 64 bit time.
    //

#if defined(CHICAGO)
    currentTime = time(NULL);
    liTtl = Ttl;
#else
    NtQuerySystemTime( &currentTime );
    liTtl = RtlEnlargedIntegerMultiply( Ttl, 10*1000*1000 );
#endif

    //
    // Allocate space to hold the hostent information.
    //

    bytesRequired = sizeof(*cacheEntry) + BytesInHostent( HostEntry ) + 20;

    cacheEntry = ALLOCATE_HEAP( bytesRequired );
    if ( cacheEntry == NULL ) {
        return;
    }

    //
    // Set up the cache entry.
    //

    cacheEntry->LastAccessTime = currentTime;
    QUADPART(cacheEntry->ExpirationTime) = QUADPART(currentTime) + QUADPART(liTtl);

    CopyHostentToBuffer(
        (char FAR *)&cacheEntry->HostEntry,
        bytesRequired - sizeof(*cacheEntry),
        HostEntry
        );

    //
    // Acquire the global lock exclusively to protect our lists and
    // counts, and test whether we're at the limit of the caching we'll
    // do.
    //

    SockAcquireGlobalLockExclusive( );

    if ( CurrentHostentCacheSize < MaxHostentCacheSize ) {

        CurrentHostentCacheSize++;

    } else {

        //
        // We're at our limit for cached entries.  Remove the oldest
        // entry from the cache.
        //

        oldestCacheEntry = NULL;

        for ( listEntry = HostentCacheListHead.Flink;
              listEntry != &HostentCacheListHead;
              listEntry = listEntry->Flink ) {


            testCacheEntry = CONTAINING_RECORD(
                                 listEntry,
                                 HOSTENT_CACHE_ENTRY,
                                 HostentListEntry
                                 );

            if ( oldestCacheEntry == NULL ||
                     QUADPART(testCacheEntry->LastAccessTime) <
                     QUADPART(oldestCacheEntry->LastAccessTime) ) {

                oldestCacheEntry = testCacheEntry;
            }
        }

        RemoveEntryList( &oldestCacheEntry->HostentListEntry );
        FREE_HEAP( oldestCacheEntry );
    }

    //
    // Place the new entry at the front of the global list and return;
    //

    InsertHeadList( &HostentCacheListHead, &cacheEntry->HostentListEntry );

    SockReleaseGlobalLock( );

    return;

} // CacheHostent


PHOSTENT
QueryHostentCache (
    IN LPSTR Name OPTIONAL,
    IN DWORD IpAddress OPTIONAL
    )
{
    PLIST_ENTRY listEntry;
    PHOSTENT_CACHE_ENTRY testCacheEntry;
    DWORD i;
    MY_LARGE_INTEGER currentTime;
    PHOSTENT hostEntry;

    //
    // *** It is assumed that this routine is called while the caller
    //     holds the appropriate global cache lock!
    //

    //
    // First get the current system time.  We'll use this to reset the
    // LastAccessTime if we find a hit.
    //

#if defined(CHICAGO)
    currentTime = time(NULL);
#else
    NtQuerySystemTime( &currentTime );
#endif

    //
    // Walk the host entry cache.  As soon as we find a match, quit
    // walking the list.
    //

    for ( listEntry = HostentCacheListHead.Flink;
          listEntry != &HostentCacheListHead; ) {

        testCacheEntry = CONTAINING_RECORD(
                             listEntry,
                             HOSTENT_CACHE_ENTRY,
                             HostentListEntry
                             );
        hostEntry = &testCacheEntry->HostEntry;

        //
        // If this entry has expired, remove it from the list.
        //

        if ( QUADPART(currentTime) > QUADPART(testCacheEntry->ExpirationTime) ) {

            CurrentHostentCacheSize--;
            RemoveEntryList( &testCacheEntry->HostentListEntry );
            listEntry = listEntry->Flink;
            FREE_HEAP( testCacheEntry );
            continue;
        }

        //
        // First check for a matching name.  A match on either the
        // primary name or any of the aliases results in a hit.
        //

        if ( ARGUMENT_PRESENT( Name ) ) {

            if ( LSTRCMPI( Name, hostEntry->h_name ) == 0 ) {
                testCacheEntry->LastAccessTime = currentTime;
                return hostEntry;
            }

            for ( i = 0; hostEntry->h_aliases[i] != NULL; i++ ) {
                if ( LSTRCMPI( Name, hostEntry->h_aliases[i] ) == 0 ) {
                    testCacheEntry->LastAccessTime = currentTime;
                    return hostEntry;
                }
            }
        }

        //
        // Now check for a match against any of the IP addresses in
        // the hostent.
        //

        if ( IpAddress != 0 ) {

            for ( i = 0; hostEntry->h_addr_list[i] != NULL; i++ ) {
                if ( IpAddress == *(PDWORD)hostEntry->h_addr_list[i] ) {
                    testCacheEntry->LastAccessTime = currentTime;
                    return hostEntry;
                }
            }
        }

        listEntry = listEntry->Flink;
    }

    //
    // We didn't find a match in the hostent cache.
    //

    return NULL;

} // QueryHostentCache

#endif // defined(HOSTENT_CACHING_ENABLED)


INT
ServiceCsaddr (
    IN WORD Port,
    IN BOOL IsTcp,
    IN OUT LPVOID lpCsaddrBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    DWORD requiredBufferLength;
    PCSADDR_INFO csaddrInfo;
    PSOCKADDR_IN sockaddrIn;

    //
    // We're going to return exactly one sockaddr and csaddr structure.
    // Make sure that the user buffer is large enough.
    //

    requiredBufferLength = sizeof(CSADDR_INFO) + 2*sizeof(SOCKADDR_IN);

    if ( *lpdwBufferLength < requiredBufferLength ) {
        *lpdwBufferLength = requiredBufferLength;
        return (DWORD)-1;
    }

    *lpdwBufferLength = requiredBufferLength;

    //
    // Fill in the appropriate information.  Set the local sockaddr's
    // port to the port for the service, and leave the remote's port
    // zero.  All IP addresses remain INADDR_ANY (0).
    //

    csaddrInfo = lpCsaddrBuffer;
    RtlZeroMemory( csaddrInfo, sizeof(*csaddrInfo) );

    sockaddrIn = (PSOCKADDR_IN)(csaddrInfo + 1);
    csaddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)sockaddrIn;
    csaddrInfo->LocalAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

    if ( IsTcp ) {
        csaddrInfo->iSocketType = SOCK_STREAM;
        csaddrInfo->iProtocol = IPPROTO_TCP;
    } else {
        csaddrInfo->iSocketType = SOCK_DGRAM;
        csaddrInfo->iProtocol = IPPROTO_UDP;
    }

    RtlZeroMemory( sockaddrIn, sizeof(*sockaddrIn)  );
    sockaddrIn->sin_family = AF_INET;
    sockaddrIn->sin_port = Port;

    sockaddrIn += 1;
    csaddrInfo->RemoteAddr.lpSockaddr = (LPSOCKADDR)sockaddrIn;
    csaddrInfo->RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

    RtlZeroMemory( sockaddrIn, sizeof(*sockaddrIn)  );
    sockaddrIn->sin_family = AF_INET;

    //
    // Return a count of one entry.
    //

    return 1;

} // ServiceCsaddr

#if defined(CHICAGO)


INT
WSHEnumProtocols (
    IN LPINT lpiProtocols,
    IN LPSTR lpTransportKeyName,
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    DWORD bytesRequired;
    PPROTOCOL_INFO11 tcpProtocolInfo;
    PPROTOCOL_INFO11 udpProtocolInfo;
    BOOL useTcp = FALSE;
    BOOL useUdp = FALSE;
    DWORD i;

    lpTransportKeyName;         // Avoid compiler warnings.

    //
    // Make sure that the caller cares about TCP and/or UDP.
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) ) {

        for ( i = 0; lpiProtocols[i] != 0; i++ ) {
            if ( lpiProtocols[i] == IPPROTO_TCP ) {
                useTcp = TRUE;
            }
            if ( lpiProtocols[i] == IPPROTO_UDP ) {
                useUdp = TRUE;
            }
        }

    } else {

        useTcp = TRUE;
        useUdp = TRUE;
    }

    if ( !useTcp && !useUdp ) {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = (sizeof(PROTOCOL_INFO11) * 2) +
                        ( (lstrlen( TCP_NAME ) + 1) * sizeof(TCHAR)) +
                        ( (lstrlen( UDP_NAME ) + 1) * sizeof(TCHAR));

    if ( bytesRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Fill in TCP info, if requested.
    //

    if ( useTcp ) {

        tcpProtocolInfo = lpProtocolBuffer;

        tcpProtocolInfo->dwServiceFlags = XP_GUARANTEED_DELIVERY |
                                              XP_GUARANTEED_ORDER |
                                              XP_GRACEFUL_CLOSE |
                                              XP_EXPEDITED_DATA |
                                              XP_FRAGMENTATION;
        tcpProtocolInfo->iAddressFamily = AF_INET;
        tcpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN);
        tcpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN);
        tcpProtocolInfo->iSocketType = SOCK_STREAM;
        tcpProtocolInfo->iProtocol = IPPROTO_TCP;
        tcpProtocolInfo->dwMessageSize = 0;
        tcpProtocolInfo->lpProtocol = (LPSTR)
            ( (PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                ( (lstrlen( TCP_NAME ) + 1) * sizeof(TCHAR) ) );
        lstrcpy( tcpProtocolInfo->lpProtocol, TCP_NAME );

        udpProtocolInfo = tcpProtocolInfo + 1;
        udpProtocolInfo->lpProtocol = (LPSTR)
            ( (PBYTE)tcpProtocolInfo->lpProtocol -
                ( (lstrlen( UDP_NAME ) + 1) * sizeof(TCHAR) ) );

    } else {

        udpProtocolInfo = lpProtocolBuffer;
        udpProtocolInfo->lpProtocol = (LPSTR)
            ( (PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                ( (lstrlen( UDP_NAME ) + 1) * sizeof(TCHAR) ) );
    }

    //
    // Fill in UDP info, if requested.
    //

    if ( useUdp ) {

        udpProtocolInfo->dwServiceFlags = XP_CONNECTIONLESS |
                                              XP_MESSAGE_ORIENTED |
                                              XP_SUPPORTS_BROADCAST |
                                              XP_SUPPORTS_MULTICAST |
                                              XP_FRAGMENTATION;
        udpProtocolInfo->iAddressFamily = AF_INET;
        udpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN);
        udpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN);
        udpProtocolInfo->iSocketType = SOCK_DGRAM;
        udpProtocolInfo->iProtocol = IPPROTO_UDP;
        udpProtocolInfo->dwMessageSize = 65535-68;
        lstrcpy( udpProtocolInfo->lpProtocol, UDP_NAME );
    }

    *lpdwBufferLength = bytesRequired;

    return (useTcp && useUdp) ? 2 : 1;

} // WSHEnumProtocols

#endif
