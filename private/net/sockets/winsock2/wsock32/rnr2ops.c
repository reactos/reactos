/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rnr2ops.c

Abstract:

    This module contains support for the DNS RnR2 provider

Author:

    Arnold Miller (ArnoldM)  3-Jan-1996

Revision History:

--*/

//
// The SPI interface is UNICODE only.
//

//
// A note on providers. This code handles NS_DNS NS_NBT and NS_HOSTS all-at-once
// If a single NS_DNS is desired to do all of these, don't define
// a provider for the others and the NS_DNS provider will consider
// NS_ALL to include the others. If, however, other providers exist,
// then the code will the explicitly correct thing. All that is needed
// is to fill in the provider IDs for NbtProviderId and LclProviderId.


#define UNICODE
#define _UNICODE

#include <winsockp.h>
#include <tchar.h>
#include <ws2spi.h>
#include "rnrdefs.h"
#include "svcguid.h"
#include <align.h>
#include <rpc.h>
#include <lm.h>

#define  SDK_DNS_RECORD     //  DNS API gets SDK defs
#include <dnsapi.h>
#include "..\..\dns\dnslib\dnslib.h"

#include <ws2atm.h>
#include <ws2tcpip.h>

#define ENABLE_DEBUG_LOGGING 1
#include "logit.h"


#define REGISTRY_WORKS 0       // no registry code for now

//
// Publics from RNRUTIL.C.
//

extern GUID HostnameGuid;
extern GUID AddressGuid;
extern GUID InetHostName;
extern GUID IANAGuid;
extern GUID AtmaGuid;
extern GUID Ipv6Guid;

DWORD   g_NbtFirst = 0;

#define DONBTFIRST 1

extern
DWORD
AllocateUnicodeString (
    IN     LPSTR   lpAnsi,
    IN OUT PWCHAR *lppUnicode
);


//
// Function prototypes
//

PVOID
AllocLocal(
    DWORD dwSize
    );

DWORD
RnRGetPortByType (
    IN     LPGUID          lpServiceType,
    IN     DWORD           dwType
    );

WORD
GetDnsQueryTypeFromGuid(
    IN  LPGUID gdType
    );

INT
RnR2AddServiceType(
    IN  PWSASERVICECLASSINFOW psci
    );

struct servent *
CopyServEntry(
    struct servent * phent,
    PBYTE pbAllocated,
    LONG lSizeOf,
    PLONG plTaken,
    BOOL fOffsets
    );

struct hostent *
CopyHostEntry(
    struct hostent * phent,
    PBYTE pbAllocated,
    LONG  lSizeOf,
    PLONG plTaken,
    BOOL  fOffsets,
    BOOL  fUnicode
    );

PDNS_RNR_CONTEXT
RnR2GetContext(
    HANDLE hContext
    );

PDNS_RNR_CONTEXT
RnR2MakeContext(
    IN HANDLE hContext,
    IN DWORD  dwExtra
    );

INT WINAPI
NSPLookupServiceBegin(
    IN  LPGUID               lpProviderId,
    IN  LPWSAQUERYSETW       lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN  DWORD                dwControlFlags,
    OUT LPHANDLE             lphLookup
    );

INT WINAPI
NSPLookupServiceNext(
    IN     HANDLE          hLookup,
    IN     DWORD           dwControlFlags,
    IN OUT LPDWORD         lpdwBufferLength,
    OUT    LPWSAQUERYSETW  lpqsResults
    );

INT WINAPI
NSPUnInstallNameSpace(
    IN LPGUID lpProviderId
    );

INT WINAPI
NSPCleanup(
    IN LPGUID lpProviderId
    );

INT WINAPI
NSPLookupServiceEnd(
    IN HANDLE hLookup
    );

INT WINAPI
NSPSetService(
    IN  LPGUID               lpProviderId,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN LPWSAQUERYSETW lpqsRegInfo,
    IN WSAESETSERVICEOP essOperation,
    IN DWORD          dwControlFlags
    );

INT WINAPI
NSPInstallServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
    );

INT WINAPI
NSPRemoveServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPGUID lpServiceCallId
    );

INT WINAPI
NSPGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
    );

VOID
RnR2Cleanup(
    VOID
    );

LPSTR
GetAnsiNameRnR (
    IN  PWCHAR Name,
    IN  LPSTR  Domain,
    OUT PBOOL  pfConversionFailed
    );

LPSTR
GetOemNameRnR (
    IN  PWCHAR Name,
    OUT PBOOL  pfConversionFailed
    );

DWORD
FetchPortFromClassInfo(
    IN    DWORD           dwType,
    IN    LPGUID          lpType,
    IN    LPWSASERVICECLASSINFOW lpServiceClassInfo
    );

VOID
RnR2ReleaseContext(
    IN PDNS_RNR_CONTEXT pdrc
    );

DWORD
PackCsAddr (
    IN PHOSTENT HostEntry,
    IN DWORD UdpPort,
    IN DWORD TcpPort,
    IN OUT LPVOID lpCsaddrBuffer,
    IN OUT PLONG lplBufferLength,
    OUT    LPDWORD lpdwBytesTaken,
    IN OUT LPDWORD Count,
    BOOL   fReversi
    );

BOOL
IsLocalQuery (
    struct hostent *,
    DWORD,
    char *
    );

struct servent *
_pgetservebyport (
    IN const int port,
    IN const char *name
    );

struct servent *
_pgetservebyname (
    IN const char *name,
    IN const char *proto
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

struct hostent *
myhostent (
    void
    );

struct hostent *
myhostent_W (
    void
    );

struct hostent *
dnshostent(
    void
    );

DWORD
GetServerAndProtocolsFromString(PWCHAR lpszString,
                                LPGUID lpType,
                                struct servent ** pServEnt);

#define ALL_LUP_FLAGS   (LUP_DEEP                 | \
                        LUP_CONTAINERS            | \
                        LUP_NOCONTAINERS          | \
                        LUP_NEAREST               | \
                        LUP_RETURN_NAME           | \
                        LUP_RETURN_TYPE           | \
                        LUP_RETURN_VERSION        | \
                        LUP_RETURN_COMMENT        | \
                        LUP_RETURN_ADDR           | \
                        LUP_RETURN_BLOB           | \
                        LUP_RETURN_ALIASES        | \
                        LUP_RETURN_QUERY_STRING   | \
                        LUP_RETURN_ALL            | \
                        LUP_RES_SERVICE           | \
                        LUP_FLUSHCACHE            | \
                        LUP_FLUSHPREVIOUS)

#define GuidEqual(x,y) RtlEqualMemory(x,y, sizeof(GUID))


// Definitions and data
//

LIST_ENTRY ListAnchor = {&ListAnchor, &ListAnchor};
#define CHECKANCHORQUEUE         // do queue checking on the above

#define DNS_PORT (53)

LONG  lStartupCount;

#define NSP_SERVICE_KEY_NAME        TEXT("SYSTEM\\CurrentControlSet\\Control\\ServiceProvider\\ServiceTypes")

DWORD MaskOfGuids;

#define NBTGUIDSEEN 0x1
#define DNSGUIDSEEN 0x2

//
// The provider Ids. Each of the providers implemented in this DLL
// is defined below. These match the GUIDs that setup puts in the registry.
//
GUID DNSProviderId = DNSGUID;

GUID NbtProviderId = NBTGUID;

#define NAME_SIZE    DNS_MAX_LABEL_LENGTH * 2

WCHAR  szLocalComputerName[NAME_SIZE] = {0};
LPWSTR pszFullName = NULL;

#define NOPORTDEFINED (0xffffffff)

#define UDP_PORT  0                // look for the UDP port type
#define TCP_PORT  1                // look for the TCP port type

#define FreeLocal(x)    FREE_HEAP(x)


struct hostent *
_gethtbyname (
    IN char *name
    );

struct hostent *
localhostent();

struct hostent *
localhostent_W();

NSP_ROUTINE nsrVector = {
    sizeof(NSP_ROUTINE),
    1,                                    // major version
    1,                                    // minor version
    NSPCleanup,
    NSPLookupServiceBegin,
    NSPLookupServiceNext,
    NSPLookupServiceEnd,
    NSPSetService,
    NSPInstallServiceClass,
    NSPRemoveServiceClass,
    NSPGetServiceClassInfo
    };

//
// Function Bodies
//

VOID
SaveAnswer(querybuf * query, int len, PDNS_RNR_CONTEXT pdrc)
{
    WS2LOG_F1( "rnr2ops.c - SaveAnswer" );

    if(pdrc->blAnswer.pBlobData)
    {
        FreeLocal(pdrc->blAnswer.pBlobData);
    }
    pdrc->blAnswer.cbSize = len;
    pdrc->blAnswer.pBlobData = AllocLocal(len);
    if(pdrc->blAnswer.pBlobData)
    {
        memcpy(pdrc->blAnswer.pBlobData, query->buf, len);
    }
}

//
// the following three functions are used to determine which provider action
// this should honor. It allows there to be but one provider to handle all of
// the actions or distinct providers.
//
BOOL
DoDnsProvider(PDNS_RNR_CONTEXT pdrc)
{
    WS2LOG_F1( "rnr2ops.c - DoDnsProvider" );

    if(!MaskOfGuids
              ||
       GuidEqual(&pdrc->gdProviderId, &DNSProviderId))
    {
       return(TRUE);
    }
    return(FALSE);
}

BOOL
DoNbtProvider(PDNS_RNR_CONTEXT pdrc, PCHAR pszName)
{
    WS2LOG_F1( "rnr2ops.c - DoNbtProvider" );

    if(!pdrc->DnsRR
                   &&
       (!MaskOfGuids
                   ||
       GuidEqual(&pdrc->gdProviderId, &NbtProviderId))
                   &&
        (pdrc->dwNameSpace == NS_ALL) )
    {
        if( strlen(pszName) <= LM20_SNLEN )
            return(TRUE);
    }
    return(FALSE);
}

BOOL
DoLclProvider(PDNS_RNR_CONTEXT pdrc)
{
    WS2LOG_F1( "rnr2ops.c - DoLclProvider" );

    if(!pdrc->DnsRR
           &&
       DoDnsProvider(pdrc) )
    {
        return(TRUE);
    }
    return(FALSE);
}


#if 0
WORD
GetDnsQueryTypeFromGuid(
    IN  LPGUID gdType
    )
{
    if ( GuidEqual( gdType, &HostnameGuid )
             ||
         GuidEqual( gdType, &InetHostName ) )
        return DNS_TYPE_A;

    if ( ! IS_SVCID_TCP( gdType ) )
        return DNS_TYPE_ZERO;

    if ( PORT_FROM_SVCID_TCP( gdType ) != 53 )
        return DNS_TYPE_ZERO;

    return RR_FROM_SVCID( gdType );
}
#endif


WORD
GetDnsQueryTypeFromGuid(
    IN      LPGUID          pGuid
    )
/*++

Routine Description:

    Determine DNS type to query for given GUID.

Arguments:

    pGuid -- GUID of lookup type

Return:

    DNS query type corresponding to GUID.

--*/
{
    WORD    wtype = DNS_TYPE_A;

    //  note, since defaulting to type A, there's no need to specifically
    //  check for standard GUIDs for Hostname lookup
    //
    //  if ( GuidEqual( pGuid, &HostnameGuid ) ||
    //       GuidEqual( pGuid, &InetHostName ) )
    //  {
    //      //  no-op, type defaults to DNS_TYPE_A above
    //      //return DNS_TYPE_A;
    //  }

    //
    //  GUID for specific DNS type lookup?
    //
    //  with NT5 GUIDs are available for querying specific DNS types
    //  (see svcguid.h);  catch those GUIDs here;
    //  note that encoding of these GUIDs is SVCID_TCP with port==53
    //  and RR set for query type
    //

    if ( IS_SVCID_DNS(pGuid) )
    {
        wtype = RR_FROM_SVCID( pGuid );
        if ( wtype == 0 )
        {
            wtype = DNS_TYPE_A;
        }
    }

    //
    //  note default return is DNS_TYPE_A
    //
    //  two reasons here
    //      1) glenn has no error return checking in any of the calls to
    //         this function and we don't want to go to the wire with type==0
    //      2) apparently some apps do call WSALookupServiceNext with a GUID
    //         for some service (eg. FTP), instead of having the app call
    //         with a name resolution GUID;  these services seemed to be able
    //         to get away with this in NT4, which means we somehow hit the
    //         wire with type==A query
    //

    return( wtype );
}


VOID
TryDns(PDNS_RNR_CONTEXT pdrc,
       struct hostent ** pphostEntry,
       PCHAR pszName,
       DWORD dwControlFlags,
       PBOOL pfHaveGlobalLock
      )
/*++
Routine Description:
    Called to try to resolve pszName using DNS.

   pphostent  -- pointer to a pointer to the resultant hostent
   pszName    -- the name to resolve
   pfHaveGlobalLock -- BOOL indicating whether the name was found
                       in the cache and therefore the lock is held

Returns: All side effects. If it succeeds, pphostEntry will by filled in.
--*/
{
    struct hostent * hostEntry = 0;

    WS2LOG_F1( "rnr2ops.c - TryDns" );

    if ( DoDnsProvider(pdrc) )
    {
        WORD         wType = GetDnsQueryTypeFromGuid( &pdrc->gdType );
        PDNS_MSG_BUF pMsg = NULL;
        PDNS_MSG_BUF * ppMsg = NULL;

        if ( wType != DNS_TYPE_A &&
             wType != DNS_TYPE_ATMA &&
          // wType != DNS_TYPE_AAAA &&   // The IPv6 guys still want raw
          //                             // wire bytes instead of the parsed
          //                             // hostent.
             wType != DNS_TYPE_PTR )
        {
            ppMsg = &pMsg;
        }

        //
        // Perform query through DNS Caching Resolver Service
        //
        if ( dwControlFlags & LUP_FLUSHCACHE )
            hostEntry = QueryDnsCache( pszName,
                                       wType,
                                       DNS_QUERY_BYPASS_CACHE,
                                       ppMsg );
        else
            hostEntry = QueryDnsCache( pszName,
                                       wType,
                                       ( ppMsg != 0 ) ?
                                       DNS_QUERY_BYPASS_CACHE :
                                       0,
                                       ppMsg );

        if ( pMsg && pdrc->DnsRR )
        {
            SWAP_COUNT_BYTES( &pMsg->MessageHead );
            SaveAnswer( (querybuf *) &pMsg->MessageHead, pMsg->MessageLength, pdrc );
        }

        if ( pMsg )
        {
            LocalFree( pMsg );
        }

        if ( !hostEntry &&
             pdrc->blAnswer.pBlobData &&
             (pdrc->dwControlFlags & LUP_RETURN_BLOB) )
        {
            //
            // getanswer failed, but the caller
            // wants the raw answer. So only
            // return that, but also conjure
            // a valid hostEntry for later on
            //
            pdrc->dwControlFlags &= LUP_RETURN_BLOB;
            hostEntry = myhostent();
        }
    }

    //
    // return whatever we have in hostEntry.
    //
    *pphostEntry = hostEntry;
}


VOID
TryNbt(PDNS_RNR_CONTEXT pdrc,
       struct hostent ** pphostEntry,
       PCHAR pszOemName,
       PCHAR pszAnsiName
      )
/*++
Routine Description:
   Called to resolve pszName using NetBT.

   pszName    -- the name to resolve

Returns: All side effects. If it succeeds, pphostEntry will by filled in.
--*/
{
    struct hostent * hostEntry = 0;

    WS2LOG_F1( "rnr2ops.c - TryNbt" );

    if(DoNbtProvider(pdrc, pszAnsiName))
    {
        hostEntry = QueryNbtWins( pszOemName, pszAnsiName );
    }

    *pphostEntry = hostEntry;
}

//
// Get a servent for a string.
//

DWORD
GetServerAndProtocolsFromString(
    IN      PWCHAR              lpszString,
    IN      LPGUID              lpType,
    OUT     struct servent **   ppServEnt
    )
{
    DWORD   nProt;
    struct  servent * pservent = NULL;

    WS2LOG_F1( "rnr2ops.c - GetServerAndProtocolsFromString" );

    if ( lpszString &&
         lpType &&
         ( GuidEqual(lpType, &HostnameGuid) ||
           GuidEqual(lpType, &InetHostName) ) )
    {
        PCHAR   servname;
        PCHAR   protocolname;
        WCHAR   wszTemp[1000];
        INT     port = 0;
        PWCHAR  pwszTemp;
        DWORD   lenServName;
        PCHAR   pszTemp;

        //
        //  input string is of the form service/protocol
        //
        //      - the service name may be a port number, in which case
        //      lookup by port
        //
        //      - the protocol is optional, if doesn't exists lookup name
        //      and use first protocol found
        //

        //
        //  separate service from protocol (if any)
        //      - save ptr to protocol piece (if any)
        //      - copy off service piece and convert to ANSI
        //

        pwszTemp = wcschr(lpszString, L'/');
        if ( !pwszTemp )
        {
             pwszTemp = wcschr(lpszString, L'\0');
        }

        lenServName = (DWORD)(pwszTemp - lpszString);
        memcpy( wszTemp, lpszString, lenServName * sizeof(WCHAR));
        wszTemp[ lenServName ] = 0;
        servname = GetAnsiNameRnR( wszTemp, 0, NULL );

        //
        //  found service name, do lookup
        //

        if ( servname )
        {
            //  if service name is completely numeric,
            //  convert it to a port number

            for( pszTemp = servname;
                 *pszTemp && isdigit(*pszTemp);
                 pszTemp++ );

            if( !*pszTemp )
            {
                port = atoi(servname);
            }

            //
            //  protocol optionally follows service name in original string
            //      - if protocol exists pwszTemp points at separating "\"
            //      - otherwise it points at NULL terminator
            //

            if ( !*pwszTemp || !*++pwszTemp )
            {
                protocolname = NULL;
            }
            else
            {
                protocolname =  GetAnsiNameRnR( pwszTemp, 0, NULL );
            }

            //
            //  get the servent
            //

            if ( port )
            {
                pservent = _pgetservebyport(port, protocolname);
            }
            else
            {
                pservent = _pgetservebyname(servname, protocolname);
            }

            if ( protocolname )
            {
                FREE_HEAP( protocolname );
            }
            if ( servname )
            {
                FREE_HEAP( servname );
            }
        }
    }

    //  set servent out param

    if ( ppServEnt )
    {
        *ppServEnt = pservent;
    }

    //  if found servent, return it's protocol

    if ( pservent )
    {
        if ( _stricmp("udp", pservent->s_proto) )
        {
            nProt = UDP_BIT;
        }
        else
        {
            nProt = TCP_BIT;
        }
    }
    else
    {
        nProt = UDP_BIT | TCP_BIT;
    }
    return( nProt );
}


DWORD
PackCsAddr (
    IN PHOSTENT HostEntry,
    IN DWORD UdpPort,
    IN DWORD TcpPort,
    IN OUT LPVOID lpCsaddrBuffer,
    IN OUT PLONG lplBufferLength,
    OUT    LPDWORD lpdwBytesTaken,
    IN OUT LPDWORD Count,
    BOOL fReversi
    )
{
    DWORD         count, nAddresses;
    DWORD         i;
    DWORD         requiredBufferLength;
    PSOCKADDR_IN  sockaddrIn;
    PSOCKADDR_ATM sockaddrAtm;
    PSOCKADDR_IN6 sockaddrIpv6;
    PCSADDR_INFO  csaddrInfo;
    BOOL          fATMAddr = FALSE;
    BOOL          fIPv6Addr = FALSE;

    WS2LOG_F1( "rnr2ops.c - PackCsAddr" );

    //
    // figure out how many addresse types we have to return
    //

    nAddresses = 0;
    if(UdpPort != NOPORTDEFINED)
    {
        nAddresses++;
    }
    if(TcpPort != NOPORTDEFINED)
    {
        nAddresses++;
    }

    if ( HostEntry->h_addrtype == AF_ATM )
        fATMAddr = TRUE;

    if ( HostEntry->h_addrtype == AF_INET6 )
        fIPv6Addr = TRUE;

    //
    // Count the number of IP addresses in the hostent.
    //

    for ( count = 0; HostEntry->h_addr_list[count] != NULL; count++ );

    count *= nAddresses;

    //
    // Make sure that the buffer is large enough to hold all the entries
    // which will be necessary.
    //

    if ( fATMAddr )
    {
        requiredBufferLength = count * (sizeof(CSADDR_INFO) +
                                   2*sizeof(SOCKADDR_ATM));
    }
    else if ( fIPv6Addr )
    {
        requiredBufferLength = count * (sizeof(CSADDR_INFO) +
                                   2*sizeof(SOCKADDR_IN6));
    }
    else
    {
        requiredBufferLength = count * (sizeof(CSADDR_INFO) +
                                   2*sizeof(SOCKADDR_IN));
    }


    *lplBufferLength -= requiredBufferLength;
    *lpdwBytesTaken = requiredBufferLength;
    if ( *lplBufferLength < 0)
    {
        return(WSAEFAULT);
    }

    //
    // For each IP address, fill in the user buffer with one entry.
    //

    if ( fATMAddr )
    {
        sockaddrAtm = (PSOCKADDR_ATM)((PCSADDR_INFO)lpCsaddrBuffer + count);
    }
    else if ( fIPv6Addr )
    {
        sockaddrIpv6 = (PSOCKADDR_IN6)((PCSADDR_INFO)lpCsaddrBuffer + count);
    }
    else
    {
        sockaddrIn = (PSOCKADDR_IN)((PCSADDR_INFO)lpCsaddrBuffer + count);
    }

    csaddrInfo = lpCsaddrBuffer;

    while(nAddresses--)
    {
        BOOL IsTcp;
        WORD Port;

        if(UdpPort != NOPORTDEFINED)
        {
            Port = (WORD)UdpPort;
            IsTcp = TRUE;
            UdpPort = 0;
        }
        else if(TcpPort != NOPORTDEFINED)
        {
            Port = (WORD)TcpPort;
            IsTcp = FALSE;
            TcpPort = 0;
        }

        for ( i = 0; i < count; i++ )
        {
            //
            // First fill in the local address.  It should remain basically
            // all zeros except for the family so that it is a "wildcard"
            // address for binding.
            //

            RtlZeroMemory( csaddrInfo, sizeof(*csaddrInfo) );

            if ( fATMAddr )
            {
                csaddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)sockaddrAtm;
                csaddrInfo->LocalAddr.iSockaddrLength = sizeof(SOCKADDR_ATM);
                RtlZeroMemory( sockaddrAtm, sizeof(*sockaddrAtm) );
                sockaddrAtm->satm_family = AF_ATM;
                sockaddrAtm++;
            }
            else if ( fIPv6Addr )
            {
                csaddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)sockaddrIpv6;
                csaddrInfo->LocalAddr.iSockaddrLength = sizeof(SOCKADDR_IN6);
                RtlZeroMemory( sockaddrIpv6, sizeof(*sockaddrIpv6) );
                sockaddrIpv6->sin6_family = AF_INET6;
                sockaddrIpv6++;
            }
            else
            {
                csaddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)sockaddrIn;
                csaddrInfo->LocalAddr.iSockaddrLength = sizeof(SOCKADDR_IN);
                RtlZeroMemory( sockaddrIn, sizeof(*sockaddrIn) );
                sockaddrIn->sin_family = AF_INET;
                sockaddrIn++;
            }

            //
            // Now the remote address.
            //

            if ( fATMAddr )
            {
                csaddrInfo->RemoteAddr.lpSockaddr = (PSOCKADDR)( sockaddrAtm );
                csaddrInfo->RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_ATM);

                sockaddrAtm = (PSOCKADDR_ATM)csaddrInfo->RemoteAddr.lpSockaddr;
                RtlZeroMemory( sockaddrAtm, sizeof(*sockaddrAtm) );
                sockaddrAtm->satm_family = AF_ATM;
            }
            else if ( fIPv6Addr )
            {
                csaddrInfo->RemoteAddr.lpSockaddr = (PSOCKADDR)( sockaddrIpv6 );
                csaddrInfo->RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_IN6);

                sockaddrIpv6 = (PSOCKADDR_IN6)csaddrInfo->RemoteAddr.lpSockaddr;
                RtlZeroMemory( sockaddrIpv6, sizeof(*sockaddrIpv6) );
                sockaddrIpv6->sin6_family = AF_INET6;
            }
            else
            {
                csaddrInfo->RemoteAddr.lpSockaddr = (PSOCKADDR)( sockaddrIn );
                csaddrInfo->RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

                sockaddrIn = (PSOCKADDR_IN)csaddrInfo->RemoteAddr.lpSockaddr;
                RtlZeroMemory( sockaddrIn, sizeof(*sockaddrIn) );
                sockaddrIn->sin_family = AF_INET;
            }

            if ( fATMAddr )
            {
                //
                // Fill in the remote address with the actual address, both port
                // and IP address.
                //

                sockaddrAtm->satm_blli.Layer2Protocol = SAP_FIELD_ABSENT;
                sockaddrAtm->satm_blli.Layer2UserSpecifiedProtocol =
                    SAP_FIELD_ABSENT;
                sockaddrAtm->satm_blli.Layer3Protocol = SAP_FIELD_ABSENT;
                sockaddrAtm->satm_blli.Layer3UserSpecifiedProtocol =
                    SAP_FIELD_ABSENT;
                sockaddrAtm->satm_blli.Layer3IPI = SAP_FIELD_ABSENT;

                sockaddrAtm->satm_bhli.HighLayerInfoType = SAP_FIELD_ABSENT;

                memcpy( &sockaddrAtm->satm_number,
                        HostEntry->h_addr_list[i],
                        sizeof( ATM_ADDRESS ) );

                //
                // Lastly, fill in the protocol information.
                //

                csaddrInfo->iSocketType = SOCK_RAW;
                csaddrInfo->iProtocol = PF_ATM;

                if(fReversi)
                {
                    PSOCKADDR temp = csaddrInfo->RemoteAddr.lpSockaddr;

                    csaddrInfo->RemoteAddr.lpSockaddr =
                         csaddrInfo->LocalAddr.lpSockaddr;

                    csaddrInfo->LocalAddr.lpSockaddr = temp;
                }

                sockaddrAtm++;
            }
            else if ( fIPv6Addr )
            {
                //
                // Fill in the remote address with the actual address, both port
                // and IPv6 address.
                //

                sockaddrIpv6->sin6_family = AF_INET6;
                sockaddrIpv6->sin6_port = 0;
                sockaddrIpv6->sin6_flowinfo = 0;

                memcpy( &sockaddrIpv6->sin6_addr,
                        HostEntry->h_addr_list[i],
                        sizeof( IPV6_ADDRESS ) );

                //
                // Lastly, fill in the protocol information.
                //

                csaddrInfo->iSocketType = SOCK_RAW;
                csaddrInfo->iProtocol = PF_INET6;

                if(fReversi)
                {
                    PSOCKADDR temp = csaddrInfo->RemoteAddr.lpSockaddr;

                    csaddrInfo->RemoteAddr.lpSockaddr =
                         csaddrInfo->LocalAddr.lpSockaddr;

                    csaddrInfo->LocalAddr.lpSockaddr = temp;
                }

                sockaddrIpv6++;
            }
            else
            {
                //
                // Fill in the remote address with the actual address, both port
                // and IP address.
                //

                sockaddrIn->sin_port = htons(Port);
                sockaddrIn->sin_addr.s_addr =
                    *((long *)(HostEntry->h_addr_list[i]));

                //
                // Lastly, fill in the protocol information.
                //

                if ( IsTcp )
                {
                    csaddrInfo->iSocketType = SOCK_STREAM;
                    csaddrInfo->iProtocol = IPPROTO_TCP;
                }
                else
                {
                    csaddrInfo->iSocketType = SOCK_DGRAM;
                    csaddrInfo->iProtocol = IPPROTO_UDP;
                }

                if(fReversi)
                {
                    PSOCKADDR temp = csaddrInfo->RemoteAddr.lpSockaddr;

                    csaddrInfo->RemoteAddr.lpSockaddr =
                         csaddrInfo->LocalAddr.lpSockaddr;

                    csaddrInfo->LocalAddr.lpSockaddr = temp;
                }

                sockaddrIn++;
            }

            csaddrInfo++;
        }
    }

    *Count = count;

    return(NO_ERROR);
}


BOOL
IsLocalQuery (
    struct hostent * pHost,
    DWORD dwIp,
    char * name
    )
{
    BOOL fReturn = FALSE;

    if ( ! pHost )
        return fReturn;

    if ( dwIp && name )
        return fReturn;

    if ( !dwIp && !name )
        return fReturn;

    if ( dwIp )
    {
        DWORD iter = 0;

        while ( pHost->h_addr_list[iter] )
        {
            if ( (*(LPDWORD) pHost->h_addr_list[iter]) == dwIp )
            {
                fReturn = TRUE;
                break;
            }

            iter++;
        }
    }
    else
    {
        char ** ppHostAliases = pHost->h_aliases;

        if ( DnsNameCompare( pHost->h_name, name ) )
            fReturn = TRUE;
        else
        {
            while ( *ppHostAliases )
            {
                if ( DnsNameCompare( *ppHostAliases++, name ) )
                {
                    fReturn = TRUE;
                    break;
                }
            }
        }
    }

    return fReturn;
}


PVOID
AllocLocal(
    DWORD dwSize
    )
{
    PVOID pvMem;

    WS2LOG_F1( "rnr2ops.c - AllocLocal" );

    pvMem = ALLOCATE_HEAP(dwSize);
    if(pvMem)
    {
        RtlZeroMemory(pvMem, dwSize);
    }
    return(pvMem);
}

LPSTR
GetAnsiNameRnR (
    IN  PWCHAR Name,
    IN  LPSTR  Domain,
    OUT PBOOL  pfConversionFailed
    )
{

    PCHAR pszBuffer;
    DWORD dwLen;
    WORD wLen;

    WS2LOG_F1( "rnr2ops.c - GetAnsiNameRnR" );

    if(Domain)
    {
        wLen = (WORD)strlen(Domain);
    }
    else
    {
        wLen = 0;
    }
    dwLen = ((wcslen(Name) + 1) * sizeof(WCHAR) * 2) + wLen;
    pszBuffer = ALLOCATE_HEAP(dwLen);
    if ( pszBuffer == NULL ) {
        return NULL;
    }

    if(!WideCharToMultiByte(
               CP_ACP,
               0,
               Name,
               -1,
               pszBuffer,
               dwLen,
               0,
               pfConversionFailed))
    {
        FREE_HEAP( pszBuffer );
        return NULL;
    }

    if(Domain)
    {
        strcat(pszBuffer, Domain);
    }
    return(pszBuffer);
}


LPSTR
GetOemNameRnR (
    IN  PWCHAR Name,
    OUT PBOOL  pfConversionFailed
    )
{
    PWCHAR pszTemp;
    PCHAR  pszBuffer;
    DWORD  dwCount;
    DWORD  dwLen;

    WS2LOG_F1( "rnr2ops.c - GetOemNameRnR" );

    dwCount = LCMapStringW( LOCALE_SYSTEM_DEFAULT,
                            LCMAP_UPPERCASE,
                            Name,
                            -1,
                            NULL,
                            0 );

    dwLen = (dwCount + 1) * sizeof(WCHAR);

    pszTemp = ALLOCATE_HEAP(dwLen);

    if ( pszTemp == NULL )
    {
        return NULL;
    }

    if(!LCMapStringW( LOCALE_SYSTEM_DEFAULT,
                      LCMAP_UPPERCASE,
                      Name,
                      -1,
                      pszTemp,
                      dwCount ))
    {
        FREE_HEAP( pszTemp );
        return NULL;
    }

    dwLen = WideCharToMultiByte( CP_OEMCP,
                                 0,
                                 pszTemp,
                                 -1,
                                 NULL,
                                 0,
                                 0,
                                 NULL );

    pszBuffer = ALLOCATE_HEAP( dwLen );

    if ( pszBuffer == NULL )
    {
        FREE_HEAP( pszTemp );
        return NULL;
    }

    if( !WideCharToMultiByte( CP_OEMCP,
                              0,
                              pszTemp,
                              -1,
                              pszBuffer,
                              dwLen,
                              0,
                              pfConversionFailed ) )
    {
        FREE_HEAP( pszBuffer );
        FREE_HEAP( pszTemp );
        return NULL;
    }

    FREE_HEAP( pszTemp );

    return(pszBuffer);
}


PDNS_RNR_CONTEXT
RnR2MakeContext(
    IN HANDLE hHandle,
    IN DWORD  dwExtra
    )
/*++
Routine Description:
    Allocate memory for a context, and enqueue it on the list
--*/
{
    PDNS_RNR_CONTEXT pdrc;

    WS2LOG_F1( "rnr2ops.c - RnR2MakeContext" );

    pdrc = (PDNS_RNR_CONTEXT)AllocLocal(
                                   sizeof(DNS_RNR_CONTEXT) +
                                   dwExtra);
    if(pdrc)
    {
        pdrc->lInUse = 2;
        pdrc->Handle = (hHandle ? hHandle : (HANDLE)pdrc);
        pdrc->lSig = RNR_SIG;
        pdrc->lInstance = -1;
        pdrc->fUnicodeHostent = FALSE;
        EnterCriticalSection(pcsRnRLock);
        InsertHeadList(&ListAnchor, &pdrc->ListEntry);
        LeaveCriticalSection(pcsRnRLock);
    }
    return(pdrc);
}

VOID
RnR2Cleanup()
{
    PLIST_ENTRY pdrc;

    WS2LOG_F1( "rnr2ops.c - RnR2Cleanup" );

    EnterCriticalSection(pcsRnRLock);

    while((pdrc = ListAnchor.Flink) != &ListAnchor)
    {
        RnR2ReleaseContext((PDNS_RNR_CONTEXT)pdrc);
    }

    if(pszFullName)
    {
        FREE_HEAP(pszFullName);
        pszFullName = 0;
    }

    szLocalComputerName[0] = 0;

    //
    // If Win95, delete this now. On NT, this is done when the
    // process detaches from the DLL
    //
    LeaveCriticalSection(pcsRnRLock);
}

VOID
RnR2ReleaseContext(
    IN PDNS_RNR_CONTEXT pdrc
    )
{
/*++
Routine Description:

    Dereference an RNR Context and free it if it is no longer referenced.

--*/

    WS2LOG_F1( "rnr2ops.c - RnR2ReleaseContext" );

    EnterCriticalSection(pcsRnRLock);
    if(--pdrc->lInUse == 0)
    {
        PLIST_ENTRY Entry;

        //
        // remove from queue
        //

#ifdef CHECKANCHORQUEUE

        for(Entry = ListAnchor.Flink;
            Entry != &ListAnchor;
            Entry = Entry->Flink)
        {
            if((PDNS_RNR_CONTEXT)Entry == pdrc)
            {
                break;
            }
        }

        ASSERT(Entry != &ListAnchor);
#endif
        RemoveEntryList(&pdrc->ListEntry);
        if(pdrc->phent)
        {
            FreeLocal(pdrc->phent);
        }
        if(pdrc->blAnswer.pBlobData)
        {
            FreeLocal(pdrc->blAnswer.pBlobData);
        }
        FreeLocal(pdrc);
    }
    LeaveCriticalSection(pcsRnRLock);
}

PDNS_RNR_CONTEXT
RnR2GetContext(
    IN  HANDLE Handle
   )
{
/*++

Routine Description:

    This routine checks the existing DNS contexts to see if we have one
    for this call.

Arguments:

    Handle    - the RnR handle

--*/
    PDNS_RNR_CONTEXT pdrc = 0;
    PLIST_ENTRY Entry;

    WS2LOG_F1( "rnr2ops.c - RnR2GetContext" );

    EnterCriticalSection(pcsRnRLock);

    for(Entry = ListAnchor.Flink;
        Entry != &ListAnchor;
        Entry = Entry->Flink)
    {
        PDNS_RNR_CONTEXT pdrc1;

        pdrc1 = (PDNS_RNR_CONTEXT)Entry;
        if(pdrc1 == (PDNS_RNR_CONTEXT)Handle)
        {
            pdrc = pdrc1;
            break;
        }
    }

    if(pdrc)
    {
        ++pdrc->lInUse;
    }
    LeaveCriticalSection(pcsRnRLock);
    return(pdrc);
}

DWORD
TryFetchClass(
    IN   LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN   PWCHAR     pwszMatch)
{
/*++
Routine Description:
   Helper routine to rummage through the Class Info entries looking
   for the desired one. If none is found, return NOPORTDEFINED. If
   one is found, return the value as a port number.
--*/
    WS2LOG_F1( "rnr2ops.c - TryFetchClass" );

    if(lpServiceClassInfo)
    {
        DWORD dwNumClassInfos = lpServiceClassInfo->dwCount;
        LPWSANSCLASSINFOW lpClassInfoBuf = lpServiceClassInfo->lpClassInfos;

        for(; dwNumClassInfos; dwNumClassInfos--, lpClassInfoBuf++)
        {
            if(!_wcsicmp(pwszMatch, lpClassInfoBuf->lpszName)
                              &&
               (lpClassInfoBuf->dwValueType == REG_DWORD)
                              &&
               (lpClassInfoBuf->dwValueSize >= sizeof(WORD)) )
            {
                return(*(PWORD)lpClassInfoBuf->lpValue);
            }
        }
    }
    return(NOPORTDEFINED);
}

DWORD
FetchPortFromClassInfo(
    IN    DWORD           dwType,
    IN    LPGUID          lpType,
    IN    LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
/*++
Routine Description:
   Find the port number corresponding to the connection type.
--*/
{
    DWORD dwPort;

    WS2LOG_F1( "rnr2ops.c - FetchPortFromClassInfo" );

    switch(dwType)
    {
        case UDP_PORT:

            if(IS_SVCID_UDP( lpType))
            {
                dwPort = PORT_FROM_SVCID_UDP(lpType);
            }
            else
            {
                dwPort = TryFetchClass(
                                       lpServiceClassInfo,
                                       SERVICE_TYPE_VALUE_UDPPORTW);
            }
            break;

        case TCP_PORT:

            if(IS_SVCID_TCP( lpType))
            {
                dwPort = PORT_FROM_SVCID_TCP(lpType);
            }
            else
            {
                dwPort = TryFetchClass(
                                       lpServiceClassInfo,
                                       SERVICE_TYPE_VALUE_TCPPORTW);
            }

            break;

        default:
            dwPort = NOPORTDEFINED;
    }
    if(dwPort == NOPORTDEFINED)
    {
          //
          // this was taken out because there was no time
          // to test it for NT4
#if REGISTRY_WORKS
        dwPort = RnRGetPortByType(lpType, dwType);
#endif
    }
    return(dwPort);
}

INT WINAPI
NSPLookupServiceBegin(
    IN  LPGUID               lpProviderId,
    IN  LPWSAQUERYSETW       lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN  DWORD                dwControlFlags,
    OUT LPHANDLE             lphLookup
    )
/*++
Routine Description:
    This is the RnR routine that begins a lookup.
--*/
{
    PDNS_RNR_CONTEXT pdrc;
    BOOL fNameLook;
    DWORD nProt;
    DWORD dwTcpPort, dwUdpPort;
    DWORD dwHostLen;
    DWORD dwLocalFlags = 0;
    WCHAR * pwszServiceName = lpqsRestrictions->lpszServiceInstanceName;
    WCHAR *wszString = NULL;
    struct servent * sent;
    int iResult = SOCKET_ERROR;

    WS2LOG_F1( "rnr2ops.c - NSPLookupServiceBegin" );

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(SOCKET_ERROR);
    }

    if(lpqsRestrictions->dwSize < sizeof(WSAQUERYSETW))
    {
        SetLastError(WSAEFAULT);
        return(SOCKET_ERROR);
    }

    if(!lpqsRestrictions->lpServiceClassId)
    {
        //
        // gotta have a class ID.
        //
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    //
    // Validate dwControlFlags param
    //

    if ((dwControlFlags & ~ALL_LUP_FLAGS) ||

        (dwControlFlags & (LUP_CONTAINERS | LUP_NOCONTAINERS) ==
            (LUP_CONTAINERS | LUP_NOCONTAINERS)))
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    //
    // Check that no context is specified.
    //
    if((lpqsRestrictions->lpszContext
                      &&
        (lpqsRestrictions->lpszContext[0] != 0)
                      &&
        wcscmp(&lpqsRestrictions->lpszContext[0],  L"\\") )
                  ||
       (dwControlFlags & LUP_CONTAINERS) )

    {
        //
        // if not the default context or need containers, can't handle it
        //
        SetLastError(WSANO_DATA);
        return(SOCKET_ERROR);
    }

    //
    // Alloc a buf to work with (this was previously on the stack)
    //

    if(!(wszString = ALLOCATE_HEAP (1000 * sizeof (WCHAR))))
    {
        //
        // if out of memory, give up now
        //
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return(SOCKET_ERROR);
    }

    //
    // If this is a lookup of the local name
    // allow the instance
    // name to be NULL. If it is a reverse lookup, mark it as such.
    // If it is a host lookup, check for one of the several special
    // names that require returning specific local information.
    //

    if( GuidEqual(lpqsRestrictions->lpServiceClassId, &AddressGuid) )
    {
        dwLocalFlags |= REVERSELOOK;
    }
    else if(GuidEqual(lpqsRestrictions->lpServiceClassId, &IANAGuid))
    {
        dwLocalFlags |= IANALOOK;
        dwControlFlags &= ~(LUP_RETURN_ADDR);
    }

    //
    // Compute whether this is some sort of name lookup. Do this
    // here since we've two places below that need to test for it.
    //

    fNameLook = GuidEqual(lpqsRestrictions->lpServiceClassId, &HostnameGuid)
                              ||
                IS_SVCID_TCP(lpqsRestrictions->lpServiceClassId)
                              ||
                IS_SVCID_UDP(lpqsRestrictions->lpServiceClassId)
                              ||
                GuidEqual(lpqsRestrictions->lpServiceClassId, &InetHostName)
                              ||
                GuidEqual(lpqsRestrictions->lpServiceClassId, &Ipv6Guid)
                              ||
                GuidEqual(lpqsRestrictions->lpServiceClassId, &AtmaGuid);

    if(!pwszServiceName
           ||
       !pwszServiceName[0])
    {
        //
        // the service name is NULL. Only allow this in certain cases
        //
        if(fNameLook)
        {
            dwLocalFlags |= LOCALLOOK;
            pwszServiceName = L"";
        }
        else if((dwLocalFlags & REVERSELOOK)
                      &&
                lpqsRestrictions->lpcsaBuffer
                      &&
                (lpqsRestrictions->dwNumberOfCsAddrs == 1))
        {
            //
            // there had better be an address in the CSADDR
            //

            struct sockaddr_in * psock;
            PCHAR pszAddr;

            psock = (struct  sockaddr_in *)
                       lpqsRestrictions->lpcsaBuffer->RemoteAddr.lpSockaddr;

            pszAddr = inet_ntoa(psock->sin_addr);
            if(!pszAddr)
            {
                goto badparm;
            }

            pwszServiceName = wszString;

            if(!MultiByteToWideChar(
                      CP_ACP,
                      0,
                      pszAddr,
                      -1,
                      pwszServiceName,
                      1000))
            {
                goto exit;
            }
        }
        else
        {
badparm:
            SetLastError(WSA_INVALID_PARAMETER);
            goto exit;
        }
    }
    else if(fNameLook)
    {
        //
        // it's some kind of name lookup. So, let's see if it is a special
        // name
        //

        if(DnsNameCompare_W(pwszServiceName, L"localhost")
                           ||
           DnsNameCompare_W(pwszServiceName, L"loopback"))
        {
            dwLocalFlags |= LOCALLOOK | LOOPLOOK;
        }
        else if( DnsNameCompare_W( pwszServiceName, szLocalComputerName ) ||
                 DnsNameCompare_W( pwszServiceName, pszFullName ) )
        {
            dwLocalFlags |= LOCALLOOK;
        }
    }

    //
    // Compute protocols to return, or return them all
    //
    if(lpqsRestrictions->lpafpProtocols)
    {
        //
        // Make certain at least one TCP/IP protocol is being requested
        //

        INT i;
        DWORD dwNum = lpqsRestrictions->dwNumberOfProtocols;

        nProt = 0;

        for(i = 0; dwNum--;)
        {
            if((lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_INET)
                                  ||
               (lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_UNSPEC)
                                  ||
               (lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_ATM)
              )

            {
                switch(lpqsRestrictions->lpafpProtocols[i].iProtocol)
                {
                    case IPPROTO_UDP:
                        nProt |= UDP_BIT;
                        break;
                    case IPPROTO_TCP:
                        nProt |= TCP_BIT;
                        break;
                    case PF_ATM:
                        nProt |= ATM_BIT;
                        break;
                    default:
                        break;
                }
            }
        }
        if(!nProt)
        {
            //
            // if the caller doesn't want addresses, why bother?
            //
            SetLastError(WSANO_DATA);
            goto exit;
        }
    }
    else
    {
        nProt = UDP_BIT | TCP_BIT;
    }

    //
    // Complete computation of protocols. We might have information in
    // the query string that helps us, and if so, we need
    // to fetch the servent for that specification.
    //

    nProt &= GetServerAndProtocolsFromString(lpqsRestrictions->lpszQueryString,
                                             lpqsRestrictions->lpServiceClassId,
                                             &sent);

    if(sent)
    {
        if(nProt & UDP_BIT)
        {
            dwUdpPort = ntohs(sent->s_port);
            dwTcpPort = NOPORTDEFINED;
        }
        else if(nProt & TCP_BIT)
        {
            dwTcpPort = ntohs(sent->s_port);
            dwUdpPort = NOPORTDEFINED;
        }
    }
    else
    {
       if(nProt & UDP_BIT)
       {
           dwUdpPort = FetchPortFromClassInfo(UDP_PORT,
                                             lpqsRestrictions->lpServiceClassId,
                                             lpServiceClassInfo);
       }
       else
       {
           dwUdpPort = NOPORTDEFINED;
       }
       if(nProt & TCP_BIT)
       {
           dwTcpPort = FetchPortFromClassInfo(TCP_PORT,
                                             lpqsRestrictions->lpServiceClassId,
                                             lpServiceClassInfo);
       }
       else
       {
           dwTcpPort = NOPORTDEFINED;
       }
    }

    //
    // if no protocol info so far, then this has to be a host name lookup
    // of some sort or it is an error
    //
    if((dwTcpPort == NOPORTDEFINED) && (dwUdpPort == NOPORTDEFINED))
    {
        if((dwLocalFlags & (REVERSELOOK | LOCALLOOK | IANALOOK))
                  ||
            GuidEqual(lpqsRestrictions->lpServiceClassId, &HostnameGuid)
                  ||
            GuidEqual(lpqsRestrictions->lpServiceClassId, &InetHostName)
                  ||
            GuidEqual(lpqsRestrictions->lpServiceClassId, &Ipv6Guid)
                  ||
            GuidEqual(lpqsRestrictions->lpServiceClassId, &AtmaGuid) )
        {
            dwUdpPort = NOPORTDEFINED;
            dwTcpPort = 0;
        }
        else
        {
            SetLastError(WSANO_DATA);
            goto exit;
        }
    }

    //
    // one final check. If the name does not contain a dotted part,
    // add the home domain to it.
    //

    if(!(dwLocalFlags & (IANALOOK | LOCALLOOK))
                 &&
       !wcschr(pwszServiceName, L'.'))
    {
        //
        // not dotted. Mark we need the domain name
        //


        if(dwLocalFlags & REVERSELOOK)
        {
            goto badparm;
        }
        dwLocalFlags |= NEEDDOMAIN;
    }

    //
    // It has passed muster. Allocate a context for it.
    //

    dwHostLen = (wcslen(pwszServiceName)  + 1) *
                      sizeof(WCHAR);


    pdrc = RnR2MakeContext(0, dwHostLen);

    if(!pdrc)
    {
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    //
    // save things in the context
    //

    pdrc->gdType = *lpqsRestrictions->lpServiceClassId;
    pdrc->dwControlFlags = dwControlFlags;
    pdrc->dwTcpPort = dwTcpPort;
    pdrc->dwUdpPort = dwUdpPort;
    pdrc->fFlags = dwLocalFlags;
    pdrc->gdProviderId = *lpProviderId;
    pdrc->dwNameSpace = lpqsRestrictions->dwNameSpace;
    wcscpy(pdrc->wcName, pwszServiceName);
    RnR2ReleaseContext(pdrc);
    *lphLookup = (HANDLE)pdrc;

    //
    // If necessary, compute the DNS lookup type
    //

    if(IS_SVCID_TCP(lpqsRestrictions->lpServiceClassId)
                              ||
       IS_SVCID_UDP(lpqsRestrictions->lpServiceClassId))
    {
        pdrc->DnsRR = RR_FROM_SVCID(lpqsRestrictions->lpServiceClassId);

        //
        // BUGBUG. Should a query value of T_A be ignored? Note that
        // specifying T_A will cause all of the local optimizations
        // to be bypassed. Hence, it seems as if it is a good idea
        // to ingnore it. What is the harm?
        //

        if(pdrc->DnsRR == T_A)
        {
            pdrc->DnsRR = 0;
        }
    }

//
// The following is no longer needed since the dwControlFlag option
// LUP_FLUSHCACHE is handled in the LookupServiceNext routine.
//
//    if (dwControlFlags & LUP_FLUSHCACHE) {
//        NewDNREpoch();
//    }

    iResult = NO_ERROR; // if here, then successful

exit:

    if (wszString) {

        FREE_HEAP (wszString);

    }

    return iResult;
}

INT WINAPI
NSPLookupServiceNext(
    IN     HANDLE          hLookup,
    IN     DWORD           dwControlFlags,
    IN OUT LPDWORD         lpdwBufferLength,
    OUT    LPWSAQUERYSETW  lpqsResults
    )
{
/*++
Routine Description:
    The continuation of LookupBegin. This tries to lookup the service
    based on the parameters in the context
--*/

    DWORD err = NO_ERROR;
    PDNS_RNR_CONTEXT pdrc;
    PBYTE pData = (PBYTE)(lpqsResults + 1);
    LONG lSpare = 0;
    LPSTR pAnsiName = NULL;
    LPSTR pOemName = NULL;
    PHOSTENT hostEntry = 0;
    PCHAR pBuff = (PCHAR)(lpqsResults + 1);
    DWORD dwTaken;
    LONG lFree,  lInstance;
    PCHAR pszDomain, pszName;
    struct hostent LocalHostEntry;
    PINT local_addr_list[MAXADDRS + 1];
    INT LocalAddress;
    WSAQUERYSETW wsaqDummy;
    struct servent * sent = 0;
    WCHAR  szComputerName[(DNS_MAX_LABEL_LENGTH * 2) + 2];
    BOOL fHaveGlobalLock = FALSE;
    BOOL fUnicodeQuery = FALSE;
    BOOL fUnicodeOem = FALSE;
    BOOL fAllowGetHostNameHack = TRUE;

    WS2LOG_F1( "rnr2ops.c - NSPLookupServiceNext" );

    if ( lpqsResults && *lpdwBufferLength )
        lSpare = (LONG)*lpdwBufferLength - lpqsResults->dwSize;

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(SOCKET_ERROR);
    }

    //
    // Validate dwControFlags param
    //

    if ((dwControlFlags & ~ALL_LUP_FLAGS) ||

        (dwControlFlags & (LUP_CONTAINERS | LUP_NOCONTAINERS) ==
            (LUP_CONTAINERS | LUP_NOCONTAINERS)))
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    if(*lpdwBufferLength < sizeof(WSAQUERYSETW))
    {
        lpqsResults = &wsaqDummy;
        err = WSAEFAULT; // Bug fix for #85973
    }
    RtlZeroMemory(lpqsResults, sizeof(WSAQUERYSETW));
    lpqsResults->dwNameSpace = NS_DNS;
    lpqsResults->dwSize = sizeof(WSAQUERYSETW);

    lFree = (LONG)(*lpdwBufferLength - sizeof(WSAQUERYSETW));

    pdrc = RnR2GetContext(hLookup);
    if(!pdrc)
    {
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    }

    //
    // a valid context.  Get the instance
    //
    EnterCriticalSection(pcsRnRLock);
    lInstance = ++pdrc->lInstance;
    if(dwControlFlags & LUP_FLUSHPREVIOUS)
    {
        lInstance = ++pdrc->lInstance;
    }
    LeaveCriticalSection(pcsRnRLock);

    pszDomain = 0;

    pAnsiName = GetAnsiNameRnR(pdrc->wcName, pszDomain, &fUnicodeQuery);
    if(!pAnsiName)
    {
        err = WSA_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    SetLastError(NO_ERROR);        // start clean

    //
    // GBUGBUG - WinSock2 UNICODE support in this module was poorly
    // implemented in earlier versions of NT. Calling GetAnsiNameRnR
    // and doing all name resolution as ANSI and using hostent
    // structures does not allow for easy support of UNICODE. This
    // problem was discovered late in NT5.
    //
    // The work around code change is to specifically plumb a call to
    // DNSQuery_W and return the found record information from DNS.
    // If the query was for some other type of look up such as LOOPLOOK,
    // LOCALLOOK, LUP_RES_SERVICE, IANALOOK, REVERSELOOK, we will
    // fail the LookupServiceNext call since we can't do anything with
    // the pAnsiName since it's translation from UNICODE is wrong.

    if ( fUnicodeQuery )
    {
        if ( ( pdrc->fFlags & IANALOOK ) ||
             ( pdrc->dwControlFlags & LUP_RES_SERVICE ) ||
             ( pdrc->fFlags & LOOPLOOK ) ||
             ( pdrc->fFlags & REVERSELOOK ) )
        {
            FREE_HEAP( pAnsiName );
            pAnsiName = NULL;

            //
            // None of these types of queries are supported for
            // service query names that are UNICODE that don't map
            // to ANSI because the particular foreign language code page
            // is not installed
            //

            err = WSA_E_NO_MORE;
            goto Done;
        }

        if ( pdrc->fFlags & LOCALLOOK )
        {
            strcpy( pAnsiName, "" );
            fAllowGetHostNameHack = FALSE;
        }
        else
        {
            FREE_HEAP( pAnsiName );
            pAnsiName = NULL;
        }
    }

    //
    // After all these years, it turns out that the NBT driver
    // is expecting OEM strings for the name lookup calls.
    //
    pOemName = GetOemNameRnR(pdrc->wcName, &fUnicodeOem);
    if(!pOemName)
    {
        err = WSA_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    if ( fUnicodeOem )
    {
        FREE_HEAP( pOemName );
        pOemName = NULL;
    }

    //
    // call the proper function to do this
    //

    if(pdrc->fFlags & IANALOOK)
    {
        USHORT port;

        //
        // This is a lookup of an IANA service name and the caller
        // wants the type returned. The RNR conventions are that
        // these names look like 21/ftp, that being something our
        // bind code can't handle. So, check for such a name and
        // ignore the prefix port  for the lookup. If the service
        // cannot be found, then use that port and assume tcp.
        //

        BOOL IsTcp;
        DWORD nProt;

        nProt = GetServerAndProtocolsFromString(
                                pdrc->wcName,
                                &HostnameGuid,
                                &sent);


        if(!sent)
        {
             err = WSATYPE_NOT_FOUND;
             goto Done;
        }
        else
        {
            //
            // found it. Get the information from the entry
            //
            port = ntohs(sent->s_port);
            IsTcp = (_stricmp("tcp", sent->s_proto) == 0);
            pszName = sent->s_name;
        }

        //
        // Make a GUID and copy it into the context.

        if(IsTcp)
        {
            SET_TCP_SVCID(&pdrc->gdType, port);
            pdrc->dwTcpPort = (DWORD)port;
            pdrc->dwUdpPort = NOPORTDEFINED;
        }
        else
        {
            SET_UDP_SVCID(&pdrc->gdType, port);
            pdrc->dwUdpPort = (DWORD)port;
            pdrc->dwTcpPort = NOPORTDEFINED;
        }
    }
    else
    {
        //
        // If this is a RES_SERVICE, just return the address.
        //

        if(pdrc->dwControlFlags & LUP_RES_SERVICE)
        {
            if(lInstance)
            {
NoMoreData:
                err = WSA_E_NO_MORE;
                goto Done;
            }
            //
            // Make up a host entry that has one address, of all zero.
            // fill in the other items just in case.
            //
            hostEntry = &LocalHostEntry;
            hostEntry->h_name = pAnsiName;
            hostEntry->h_aliases = 0;
            hostEntry->h_addr_list = (PCHAR *)local_addr_list;
            local_addr_list[0] = &LocalAddress;
            LocalAddress = 0;
            local_addr_list[1] = 0;
            hostEntry->h_addrtype = AF_INET;
            hostEntry->h_length = sizeof(ULONG);
        }
        else if(!(hostEntry = pdrc->phent))
        {
            //
            // It's some form of host lookup and we don't yet have
            // a saved hostent for it. See if we can get one
            //
            if(pdrc->fFlags & REVERSELOOK)
            {
                DWORD dwIp = inet_addr(pAnsiName);

                if ( dwIp == INADDR_ANY )
                {
                    if ( pdrc->dwControlFlags & LUP_RETURN_BLOB )
                    {
                        hostEntry = myhostent();
                        fUnicodeQuery = FALSE;
                    }
                    else
                    {
                        hostEntry = myhostent_W();
                        fUnicodeQuery = TRUE;
                    }
                }
                else if ( dwIp == htonl(INADDR_LOOPBACK) )
                {
                    if ( pdrc->dwControlFlags & LUP_RETURN_BLOB )
                    {
                        hostEntry = localhostent();
                        fUnicodeQuery = FALSE;
                    }
                    else
                    {
                        hostEntry = localhostent_W();
                        fUnicodeQuery = TRUE;
                    }
                }
                else
                {
                    if ( pdrc->dwControlFlags & LUP_RETURN_BLOB )
                    {
                        hostEntry = myhostent();
                        fUnicodeQuery = FALSE;
                    }
                    else
                    {
                        hostEntry = myhostent_W();
                        fUnicodeQuery = TRUE;
                    }

                    if ( ! IsLocalQuery( hostEntry, dwIp, NULL ) )
                    {
                        hostEntry = _pgethostbyaddr( (PCHAR)&dwIp,
                                                     4,
                                                     AF_INET,
                                                     pdrc->dwControlFlags,
                                                     g_NbtFirst,
                                                     &fUnicodeQuery );
                    }
                }
            }
            else if(pdrc->DnsRR || !(pdrc->fFlags & LOCALLOOK))
            {
                //
                // a real name lookup. See which provider type to use
                //

                if ( fUnicodeQuery )
                {
                    WORD wType = GetDnsQueryTypeFromGuid( &pdrc->gdType );
                    PDNS_MSG_BUF pMsg = NULL;
                    PDNS_MSG_BUF * ppMsg = NULL;

                    if ( wType == DNS_TYPE_AAAA )
                    {
                        ppMsg = &pMsg;
                    }

                    if ( wType != DNS_TYPE_A &&
                         wType != DNS_TYPE_ATMA &&
                      // wType != DNS_TYPE_AAAA &&
                      //
                      // The IPv6 guys still want raw
                      // wire bytes instead of the parsed
                      // hostent.
                      //
                         wType != DNS_TYPE_PTR )
                    {
                        ppMsg = &pMsg;
                    }

                    //
                    // If this is a query for a name that could not
                    // be converted to the ANSI code page (cleanly),
                    // because the specific code page is not installed on
                    // the current system. Do a pure UNICODE DNS query
                    // with the origical wide char string . . .
                    //
                    // Perform query through DNS Caching Resolver Service
                    //
                    if ( pdrc->dwControlFlags & LUP_FLUSHCACHE )
                    {
                        hostEntry = QueryDnsCache_W( pdrc->wcName,
                                                     wType,
                                                     DNS_QUERY_BYPASS_CACHE,
                                                     ppMsg );
                    }
                    else
                    {
                        hostEntry = QueryDnsCache_W( pdrc->wcName,
                                                     wType,
                                                     ( ppMsg != 0 ) ?
                                                     DNS_QUERY_BYPASS_CACHE :
                                                     0,
                                                     ppMsg );
                    }

                    if ( pMsg && pdrc->DnsRR )
                    {
                        SWAP_COUNT_BYTES( &pMsg->MessageHead );
                        SaveAnswer( (querybuf *) &pMsg->MessageHead,
                                    pMsg->MessageLength,
                                    pdrc );

                    }

                    if ( pMsg )
                    {
                        LocalFree( pMsg );
                    }
                }

                if ( DoLclProvider( pdrc ) && !fUnicodeQuery )
                {
                    //
                    // Always try the local database first.
                    //

                    hostEntry = _gethtbyname(pAnsiName);
                }

                if ( !hostEntry && !fUnicodeQuery )
                {
                    //
                    // Test to see if name query is for local machine
                    //
                    hostEntry = myhostent();

                    if ( ! IsLocalQuery( hostEntry, 0, pAnsiName ) )
                        hostEntry = NULL;
                }

                if ( !hostEntry && !fUnicodeQuery )
                {
                    DWORD dwNbtFirst = g_NbtFirst & DONBTFIRST;

                    //
                    // local didn't find it. See if NBT can find it
                    // Depending on g_NbtFirst, try Nbt first or DNS
                    // first.

                    do
                    {
                        if( ( dwNbtFirst & DONBTFIRST ) &&
                            pOemName )
                        {
                            TryNbt(pdrc,
                                   &hostEntry,
                                   pOemName,
                                   pAnsiName);
                        }
                        else
                        {
                            TryDns(pdrc,
                                   &hostEntry,
                                   pAnsiName,
                                   pdrc->dwControlFlags,
                                   &fHaveGlobalLock
                                  );
                        }
                    } while(!hostEntry
                               &&
                            ((dwNbtFirst ^= DONBTFIRST) !=
                                (g_NbtFirst & DONBTFIRST)) );
                }
            }
            else
            {
                //
                // the caller is looking for information about the
                // local machine. So return either the localhostent
                // information, the WINS form of the DNS information,
                // or the real DNS information. This is important to
                // get right in order for apps to work properly
                //

                if(pdrc->fFlags & LOOPLOOK)
                {
                    if ( pdrc->dwControlFlags & LUP_RETURN_BLOB )
                    {
                        hostEntry = localhostent();
                        fUnicodeQuery = FALSE;
                    }
                    else
                    {
                        hostEntry = localhostent_W();
                        fUnicodeQuery = TRUE;
                    }
                }
                else
                {
                    //
                    // this code will return the machine's
                    // DNS addresses when the lookup is on a NULL name
                    // and the port is the DNS port.
                    //
                    if(!*pAnsiName
                           &&
                       ((pdrc->dwUdpPort == DNS_PORT)
                              ||
                       (pdrc->dwTcpPort == DNS_PORT)) )
                    {
                        //
                        // special DNS address-list hack
                        //

                        hostEntry = dnshostent();
                    }
                    else
                    {
                        if ( ( GuidEqual(&pdrc->gdType, &InetHostName) &&
                             ! ( pdrc->dwControlFlags & LUP_RETURN_BLOB ) ) ||
                             GuidEqual(&pdrc->gdType, &HostnameGuid) )
                        {
                            fUnicodeQuery = TRUE;
                            hostEntry = myhostent_W();

                            if ( hostEntry && fAllowGetHostNameHack )
                            {
                                if( GuidEqual(&pdrc->gdType, &HostnameGuid) ||
                                    ( GuidEqual(&pdrc->gdType, &InetHostName) &&
                                      !*pdrc->wcName ) )
                                {
                                    LPWSTR pszDot;
                                    //
                                    // this is a gethostname call. So return
                                    // the machine name part only. Do
                                    // this by copying the machine-name
                                    // prefix to a local. It really doesn't
                                    // matter since just below the
                                    // hostEntry is copied and saved in the
                                    // lookup context so we don't care
                                    // about what we got back from myhostent.
                                    //

                                    pszDot = wcschr((LPWSTR)hostEntry->h_name, L'.');
                                    if(pszDot)
                                    {
                                        DWORD dwDot = (DWORD)(pszDot -
                                                  (LPWSTR)hostEntry->h_name);

                                        memcpy( szComputerName,
                                                hostEntry->h_name,
                                                dwDot*sizeof(WCHAR) );

                                        szComputerName[dwDot] = 0;
                                        hostEntry->h_name = (LPSTR) szComputerName;
                                    }

                                    //
                                    // Don't return any aliases with
                                    // a HOSTNAME query.
                                    //
                                    *hostEntry->h_aliases = NULL;
                                }
                            }
                        }
                        else
                        {
                            hostEntry = myhostent();

                            if ( hostEntry && fAllowGetHostNameHack )
                            {
                                if( GuidEqual(&pdrc->gdType, &HostnameGuid) ||
                                    ( GuidEqual(&pdrc->gdType, &InetHostName) &&
                                      !*pdrc->wcName ) )
                                {
                                    LPSTR pszDot;
                                    //
                                    // this is a gethostname call. So return
                                    // the machine name part only. Do
                                    // this by copying the machine-name
                                    // prefix to a local. It really doesn't
                                    // matter since just below the
                                    // hostEntry is copied and saved in the
                                    // lookup context so we don't care
                                    // about what we got back from myhostent.
                                    //

                                    pszDot = strchr(hostEntry->h_name, L'.');
                                    if(pszDot)
                                    {
                                        DWORD dwDot = (DWORD)(pszDot -
                                                      hostEntry->h_name);

                                        memcpy( (LPSTR) szComputerName,
                                                hostEntry->h_name,
                                                dwDot*sizeof(char) );

                                        ((LPSTR) szComputerName)[dwDot] = 0;
                                        hostEntry->h_name = (LPSTR) szComputerName;
                                    }

                                    //
                                    // Don't return any aliases with
                                    // a HOSTNAME query.
                                    //
                                    *hostEntry->h_aliases = NULL;
                                }
                            }
                        }
                    }
                }
            }

            if(hostEntry)
            {
                LONG lSizeOf;

                //
                // copy the host entry so we can keep a local copy of
                // it

                pdrc->phent = hostEntry = CopyHostEntry(hostEntry,
                                                        0,
                                                        0,
                                                        &lSizeOf,
                                                        FALSE,
                                                        fUnicodeQuery);
                pdrc->dwHostSize = (DWORD)lSizeOf;
                pdrc->fUnicodeHostent = fUnicodeQuery;

                //
                // if this is from the hostent cache, free the lock now
                //
                if(fHaveGlobalLock)
                {
                    SockReleaseGlobalLock();
                    fHaveGlobalLock = FALSE;   // keep things neat.
                }
            }
        }

        if(!hostEntry)
        {
            err = GetLastError();
            if ( err == NO_ERROR )
                err = WSASERVICE_NOT_FOUND;

            goto Done;
        }

        //
        // Got the data. See if we've returned all of the results yet.
        //


        if(!lInstance)
        {
            pszName = hostEntry->h_name;
        }
        else
        {
            fUnicodeQuery = pdrc->fUnicodeHostent;

            if((pdrc->dwControlFlags & LUP_RETURN_ALIASES)
                        &&
               hostEntry->h_aliases)
            {
                LONG x;

                for(x = 0; x < lInstance; x++)
                {
                    if(hostEntry->h_aliases[x] == NULL)
                    {
                        goto NoMoreData;
                    }
                }
                pszName = hostEntry->h_aliases[x - 1];
                lpqsResults->dwOutputFlags |= RESULT_IS_ALIAS;
             }
             else
             {
                 goto NoMoreData;
             }
        }
    }   // IANALOOK

    lpqsResults->dwNameSpace = NS_DNS;

    //
    // we've an answer. So make the response
    //

    if(pdrc->dwControlFlags & LUP_RETURN_TYPE)
    {
        lFree -= sizeof(GUID);
        if(lFree < 0)
        {
            err = WSAEFAULT;
        }
        else
        {
            lpqsResults->lpServiceClassId = (LPGUID)pBuff;
            *lpqsResults->lpServiceClassId = pdrc->gdType;
            pBuff += sizeof(GUID);
        }
    }

    if(pdrc->dwControlFlags & LUP_RETURN_ADDR)
    {
        lpqsResults->lpcsaBuffer = (PVOID)pBuff;
        err = PackCsAddr(
                  hostEntry,
                  pdrc->dwUdpPort,
                  pdrc->dwTcpPort,
                  (PVOID)pBuff,
                  &lFree,
                  &dwTaken,
                  &lpqsResults->dwNumberOfCsAddrs,
                  (pdrc->fFlags & REVERSELOOK) == 1);
        if(err == NO_ERROR)
        {
            pBuff += dwTaken;
        }
    }

    if( ( pdrc->dwControlFlags & LUP_RETURN_BLOB ) &&
          !fUnicodeQuery )
    {
        if(GuidEqual(&pdrc->gdType, &InetHostName)
                    ||
           GuidEqual(&pdrc->gdType, &AddressGuid)
                    ||
           GuidEqual(&pdrc->gdType, &AtmaGuid))
        {
            //
            // return the raw hostent for those that like that
            //

            LONG lTaken;

            lpqsResults->lpBlob = (LPBLOB)pBuff;
            lFree -= sizeof(BLOB);
            pBuff += sizeof(BLOB);
            if(CopyHostEntry(hostEntry,
                             pBuff,
                             lFree,
                             &lTaken,
                             TRUE,
                             FALSE)) // Hostent blobs are always returned
                                     // non-UNICODE
            {
                lpqsResults->lpBlob->pBlobData = pBuff;
                lpqsResults->lpBlob->cbSize = lTaken;
                pBuff += lTaken;
            }
            else
            {
                err = WSAEFAULT;
            }
            lFree -= lTaken;
        }
        else if((pdrc->fFlags & IANALOOK)
                        &&
                sent)
        {
            //
            // a service lookup. Return the servent
            //

            LONG lTaken;

            lpqsResults->lpBlob = (LPBLOB)pBuff;
            lFree -= sizeof(BLOB);
            pBuff += sizeof(BLOB);
            if(CopyServEntry(sent,
                             pBuff,
                             lFree,
                             &lTaken,
                             TRUE))
            {
                lpqsResults->lpBlob->pBlobData = pBuff;
                lpqsResults->lpBlob->cbSize = lTaken;
                pBuff += lTaken;
            }
            else
            {
                err = WSAEFAULT;
            }
            lFree -= lTaken;
        }
        else if(pdrc->DnsRR && pdrc->blAnswer.pBlobData)
        {
             LONG lLen = sizeof(BLOB) + pdrc->blAnswer.cbSize;

             //
             // return the raw answer, if it fits
             //

            lFree -= lLen;
            if(lFree >= 0)
            {
                lpqsResults->lpBlob = (LPBLOB)pBuff;
                pBuff += lLen;
                lpqsResults->lpBlob->pBlobData = pBuff;
                lpqsResults->lpBlob->cbSize = pdrc->blAnswer.cbSize;
                memcpy(pBuff,
                       pdrc->blAnswer.pBlobData,
                       pdrc->blAnswer.cbSize);
            }
            else
            {
                err = WSAEFAULT;
            }
        }
    }

    if(pdrc->dwControlFlags & LUP_RETURN_NAME)
    {
        PWCHAR pszString;
        DWORD dwLen;

        //
        // and the caller wants the name. Make sure it fits
        //
        if ( pszName )
        {
            if ( fUnicodeQuery )
            {
                dwLen = (wcslen((LPWSTR) pszName) + 1) * sizeof(WCHAR);
                lFree -= dwLen;
                if(lFree < 0)
                {
                     err = WSAEFAULT;
                }
                else
                {
                    RtlCopyMemory(
                             pBuff,
                             pszName,
                             dwLen);
                    lpqsResults->lpszServiceInstanceName = (WCHAR *)pBuff;
                    pBuff += dwLen;
                }
            }
            else
            {
                if(AllocateUnicodeString(pszName, &pszString) != NO_ERROR)
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    goto Done;
                }

                dwLen = (wcslen(pszString) + 1) * sizeof(WCHAR);
                lFree -= dwLen;
                if(lFree < 0)
                {
                     err = WSAEFAULT;
                }
                else
                {
                    RtlCopyMemory(
                             pBuff,
                             pszString,
                             dwLen);
                    lpqsResults->lpszServiceInstanceName = (WCHAR *)pBuff;
                    pBuff += dwLen;
                }
                FREE_HEAP(pszString);
            }
        }
    }

Done:
    if(pAnsiName)
    {
        FREE_HEAP(pAnsiName);
    }
    if(pOemName)
    {
        FREE_HEAP(pOemName);
    }
    if(err != NO_ERROR)
    {
        SetLastError(err);
        if(err == WSAEFAULT)
        {
            //
            // If this is the error, lFree should be the  value
            // -(extra bytes needed)
            //
            *lpdwBufferLength -= lFree;
            EnterCriticalSection(pcsRnRLock);
            --pdrc->lInstance;
            LeaveCriticalSection(pcsRnRLock);
        }
        err = (DWORD)SOCKET_ERROR;
    }
    RnR2ReleaseContext(pdrc);
    return(err);
}

INT WINAPI
NSPLookupServiceEnd(
    IN HANDLE hLookup
    )
{
    PDNS_RNR_CONTEXT pdrc;

    WS2LOG_F1( "rnr2ops.c - NSPLookupServiceEnd" );

    pdrc = RnR2GetContext(hLookup);
    if(!pdrc)
    {

        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    }

    pdrc->fFlags |= DNS_F_END_CALLED;
    RnR2ReleaseContext(pdrc);         // get rid of it
    RnR2ReleaseContext(pdrc);         // and close it. Context cleanup is
                                     //  done on the last derefernce.
    return(NO_ERROR);
}

INT WINAPI
NSPUnInstallNameSpace(
    IN LPGUID lpProviderId
    )
{
    WS2LOG_F1( "rnr2ops.c - NSPUnInstallNameSpace" );

    return(NO_ERROR);
}

INT WINAPI
NSPCleanup(
    IN LPGUID lpProviderId
    )
{
    WS2LOG_F1( "rnr2ops.c - NSPCleanup" );

    if(!InterlockedDecrement(&lStartupCount))
    {
        RnR2Cleanup();          // zap all contexts
    }
//    WSACleanup();
    return(NO_ERROR);
}

INT WINAPI
NSPSetService(
    IN  LPGUID               lpProviderId,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN LPWSAQUERYSETW lpqsRegInfo,
    IN WSAESETSERVICEOP essOperation,
    IN DWORD          dwControlFlags
    )
{
/*++
    Since DNS is a static database, there's nothing to do
--*/
    WS2LOG_F1( "rnr2ops.c - NSPSetService" );

    SetLastError(ERROR_NOT_SUPPORTED);
    return(SOCKET_ERROR);
}

INT WINAPI
NSPInstallServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
{
/*++
    Must be done manually
--*/
    WS2LOG_F1( "rnr2ops.c - NSPInstallServiceClass" );

    //
    // This was taken out because there was no time to test
    // it for NT4.
    //
#if REGISTRY_WORKS
    return(RnR2AddServiceType(lpServiceClassInfo));
#else
    SetLastError(WSAEOPNOTSUPP);
    return(SOCKET_ERROR);
#endif
}

INT WINAPI
NSPRemoveServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPGUID lpServiceCallId
    )
{
    WS2LOG_F1( "rnr2ops.c - NSPRemoveServiceClass" );

    SetLastError(WSAEOPNOTSUPP);
    return(SOCKET_ERROR);
}

INT WINAPI
NSPGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
{
/*++
Routine Description:
    Fetch the class info stuff from DNS
--*/
    WS2LOG_F1( "rnr2ops.c - NSPGetServiceClassInfo" );

    SetLastError(WSAEOPNOTSUPP);
    return(SOCKET_ERROR);
}


INT WINAPI
NSPStartup(
    IN     LPGUID        lpProviderId,
    IN OUT LPNSP_ROUTINE lpsnpRoutines)
{
    DWORD dwSize = min(sizeof(nsrVector), lpsnpRoutines->cbSize);

    WS2LOG_F1( "rnr2ops.c - NSPStartup" );

    //
    // BUGBUG. If no size is provided, assume it is big enough!!!!
    //

    if(!SockEnterApi(FALSE, TRUE, TRUE))
    {
        return(SOCKET_ERROR);
    }


    InterlockedIncrement(&lStartupCount);

    if(!szLocalComputerName[0])
    {
        struct hostent * myent = myhostent_W();
        LPWSTR pszDot, pszDest;
        DWORD  dwLen;
        HKEY   Key;

        //
        // Get the computer name and stash it for comparison.
        //

        if(!myent)
        {
Error:
            //
            // if we can't get the local computer name or
            // the local host entry, something is very amiss
            // and we should refuse to load.
            //
            if(!GetLastError())
            {
                //
                // insure there is an error to examine
                //
                SetLastError(WSASYSNOTREADY);
            }
            return(SOCKET_ERROR);
        }

        //
        // save it
        //

        dwLen = (wcslen((LPWSTR)myent->h_name) + 1) * sizeof(WCHAR);
        pszFullName = (LPWSTR)ALLOCATE_HEAP(dwLen);
        if(!pszFullName)
        {
            goto Error;
        }
        wcscpy(pszFullName, (LPWSTR)myent->h_name);

        //
        // Now compute the name without the domain suffix.
        //

        pszDot = pszFullName;
        pszDest = szLocalComputerName;
        while(*pszDot && (*pszDot != L'.'))
        {
            *pszDest++ = *pszDot++;
        }
        *pszDest = 0;

        //
        // get the lookup order from the registry
        //

        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TCPKEY,
                        0,
                        KEY_QUERY_VALUE,
                        &Key) == NO_ERROR)
        {
            DWORD dwSize = sizeof(g_NbtFirst);
            DWORD dwType;

            RegQueryValueEx(Key,
                            LOOKUPORDER,
                            0,
                            &dwType,
                            (PCHAR)&g_NbtFirst,
                            &dwSize);
            RegCloseKey(Key);
        }
    }

    if(!dwSize)
    {
        dwSize = sizeof(nsrVector);
    }


    RtlCopyMemory(lpsnpRoutines,
                  &nsrVector,
                  dwSize);

    if(GuidEqual(lpProviderId, &NbtProviderId))
    {
        MaskOfGuids |= NBTGUIDSEEN;
    }
    else if(GuidEqual(lpProviderId, &DNSProviderId))
    {
        MaskOfGuids |= DNSGUIDSEEN;
    }

    return(NO_ERROR);
}

//
// Two helper routines to assist CopyHostEntry. These compute
// the  space needed to  hold the alias and address structures
// respectively. They are called only once each, and therefore
// could have been in-line, but doing it this way makes it easier
// to read. The extra cost is small, so readbility won out. Now if
// C had an inline directive ...
//
DWORD
GetAliasSize(PCHAR * pal, PDWORD pdwCount, BOOL fUnicode)
{
    DWORD dwSize;

    dwSize = sizeof(PCHAR);
    *pdwCount = 1;

    if(pal)
    {
        for(; *pal; pal++)
        {
            dwSize += sizeof(PCHAR);

            if ( fUnicode )
            {
                dwSize += (wcslen((LPWSTR)*pal) + 1) * sizeof(WCHAR);
            }
            else
            {
                dwSize += strlen(*pal) + 1;
            }

            *pdwCount += 1;
        }
    }
    return(dwSize);
}

DWORD
GetAddrSize(struct hostent * ph, PDWORD pdwAddCount)
{
    DWORD dwSize = sizeof(PCHAR);
    PCHAR * paddr;

    *pdwAddCount = 1;
    for(paddr = ph->h_addr_list; *paddr; paddr++)
    {
        dwSize += ph->h_length;
        dwSize += sizeof(PCHAR);
        *pdwAddCount += 1;
    }
    return(dwSize);
}


//
// Turn a list of addresses into a list of offsets
//

VOID
FixList(PCHAR ** List,
        PCHAR Base)
{
    PCHAR * Addr = *List;

    WS2LOG_F1( "rnr2ops.c - FixList" );

    *List = (PCHAR *)((PCHAR)*List - Base);

    while(*Addr)
    {
        *Addr = (PCHAR)((PCHAR)*Addr - Base);
        Addr++;
    }
}

//
// Copy a hostent structure into a freshly allocate block of memory or
// into a provided block of size lSizeoOf.
// Args work as follows:
//
// phent -- the stuff to be copied
// pbAllocated -- if non-NULL, the memory in which to make a copy of phent
//                if NULL, allocate memory to hold this.
// lSizeOf -- if pbAllocated in non-NULL, the size of the memory at
//            that address
// plTake -- a pointer to a LONG where the amount of memory needed to make
//           a copy of phent is returned.
//
// the copied hostent is placed in a contiguous chunk of memory so
// that if pbAllocated in non-NULL the first *plTaken bytes will
// have been used to hold the structure.
//
// Return:
//   0 -- insufficient memory to do the copy
//   != 0 -- the address of the new hostent
//

struct hostent *
CopyHostEntry(
    struct hostent * phent,
    PBYTE pbAllocated,
    LONG lSizeOf,
    PLONG plTaken,
    BOOL fOffsets,
    BOOL fUnicode
    )
{
    PBYTE pb;
    struct hostent * ph;
    DWORD dwSize, dwAddCount, dwAlCount;
    DWORD dwNameLen = 0;

    WS2LOG_F1( "rnr2ops.c - CopyHostEntry" );

    if ( phent->h_name )
    {
        if ( fUnicode )
        {
            dwNameLen = (wcslen( (LPWSTR) phent->h_name ) + 1) * sizeof(WCHAR);
        }
        else
        {
            dwNameLen = strlen(phent->h_name);
        }
    }

    dwSize = sizeof(struct hostent) +
             GetAliasSize(phent->h_aliases, &dwAlCount, fUnicode) +
             GetAddrSize(phent, &dwAddCount) +
             dwNameLen + 1;

    if(!(pb = pbAllocated))
    {
        pb = (PBYTE)AllocLocal(dwSize);
    }
    else
    {
        //
        // align it first. This is done to insure that if this
        // space is within another buffer, as it will be when
        // returning a BLOB through a LoookupServiceNext, that
        // the buffer is left aligned to hold a string. Note it
        // is not left aligned to hold a structure on the assumption
        // that any structures are packed ahead of this. If
        // it is ever necessary to guarantee address alignment,
        // replace the ALIGN_WORD with ALIGN_DWORD.
        //

        dwSize = ROUND_UP_COUNT(dwSize, ALIGN_WORD);
        if(lSizeOf < (LONG)dwSize)
        {
            pb = 0;          // insufficient space provided.
        }
    }

    *plTaken = (LONG)dwSize;

    if(pb)
    {
        PCHAR *pcs, *pcd;

        ph = (struct hostent *)pb;
        ph->h_addr_list = (PCHAR *)(ph + 1);
        ph->h_aliases = &ph->h_addr_list[dwAddCount];
        pb = (PBYTE)&ph->h_aliases[dwAlCount];

        //
        // copy the basic structure
        //

        ph->h_addrtype = phent->h_addrtype;
        ph->h_length = phent->h_length;

        //
        // The layout in the string space is the addresses first, then
        // the aliases, then the name.
        //

        pcs = phent->h_addr_list;
        pcd = ph->h_addr_list;

        if(pcs)
        {
            while(*pcs)
            {
                *pcd = (PCHAR)pb;
                RtlCopyMemory(pb, *pcs, phent->h_length);
                pb += phent->h_length;
                pcd++;
                pcs++;
            }
        }
        *pcd = 0;

        //
        // copy the aliases
        //

        pcs = phent->h_aliases;
        pcd = ph->h_aliases;
        if(pcs)
        {
            while(*pcs)
            {
                DWORD dwLen;

                if ( fUnicode )
                {
                    dwLen = (wcslen( (LPWSTR) *pcs ) + 1) * sizeof(WCHAR);
                }
                else
                {
                    dwLen = strlen(*pcs) + 1;
                }

                *pcd = (PCHAR)pb;
                RtlCopyMemory(pb, *pcs, dwLen);
                pb += dwLen;
                pcd++;
                pcs++;
            }
        }
        *pcd = 0;

        //
        // finally the name
        //

        if(phent->h_name)
        {
            ph->h_name = (PCHAR)pb;
            if ( fUnicode )
            {
                wcscpy( (LPWSTR)pb,  (LPWSTR)phent->h_name );
            }
            else
            {
                strcpy(pb, phent->h_name);
            }
        }
        else
        {
            ph->h_name = NULL;
        }
    }
    else
    {
        ph = 0;
    }

    //
    // if relative offsets are needed, go through the address to make them so
    //

    if(ph && fOffsets)
    {
        ph->h_name = (PCHAR)(ph->h_name - (PCHAR)ph);
        FixList(&ph->h_aliases, (PCHAR)ph);
        FixList(&ph->h_addr_list, (PCHAR)ph);
    }
    return(ph);
}

//
// Copy a servent. Same comments as apply above for hostent copying
//
struct servent *
CopyServEntry(
    struct servent * sent,
    PBYTE pbAllocated,
    LONG lSizeOf,
    PLONG plTaken,
    BOOL fOffsets
    )
{
    PBYTE pb;
    struct servent * ps;
    DWORD dwSize, dwAlCount, dwNameSize;

    WS2LOG_F1( "rnr2ops.c - CopyServEntry" );

    dwNameSize = strlen(sent->s_name) + 1;
    dwSize = sizeof(struct servent) +
             GetAliasSize(sent->s_aliases, &dwAlCount, FALSE) +
             dwNameSize +
             strlen(sent->s_proto) + 1;


    if(!(pb = pbAllocated))
    {
        pb = (PBYTE)AllocLocal(dwSize);
    }
    else
    {
        //
        // align it first. This is done to insure that if this
        // space is within another buffer, as it will be when
        // returning a BLOB through a LoookupServiceNext, that
        // the buffer is left aligned to hold a string. Note it
        // is not left aligned to hold a structure on the assumption
        // that any structures are packed ahead of this. If
        // it is ever necessary to guarantee address alignment,
        // replace the ALIGN_WORD with ALIGN_DWORD.
        //

        dwSize = ROUND_UP_COUNT(dwSize, ALIGN_WORD);
        if(lSizeOf < (LONG)dwSize)
        {
            pb = 0;          // insufficient space provided.
        }
    }

    *plTaken = (LONG)dwSize;

    if(pb)
    {
        PCHAR *pcs, *pcd;

        ps= (struct servent *)pb;
        ps->s_aliases = (PCHAR *)(ps + 1);
        pb = (PBYTE)&ps->s_aliases[dwAlCount];

        //
        // copy the basic structure
        //

        ps->s_port = sent->s_port;

        //
        // The layout in the string space is the aliases first, then
        // the name, then the protocol string
        //


        // copy the aliases
        //
        pcs = sent->s_aliases;
        pcd = ps->s_aliases;
        if(pcs)
        {
            while(*pcs)
            {
                DWORD dwLen = strlen(*pcs) + 1;

                *pcd = (PCHAR)pb;
                RtlCopyMemory(pb, *pcs, dwLen);
                pb += dwLen;
                pcd++;
                pcs++;
            }
        }
        *pcd = 0;

        // now the two strings

        ps->s_name = (PCHAR)pb;
        RtlMoveMemory(pb, sent->s_name, dwNameSize);
        pb += dwNameSize;
        ps->s_proto = (PCHAR)pb;
        strcpy(pb, sent->s_proto);
    }
    else
    {
        ps = 0;
    }
    if(ps && fOffsets)
    {
        ps->s_name = (PCHAR)(ps->s_name - (PCHAR)ps);
        ps->s_proto = (PCHAR)(ps->s_proto - (PCHAR)ps);
        FixList(&ps->s_aliases, (PCHAR)ps);
    }

    return(ps);
}

//
// The following is not used, but is here. It was implemented for NT 4
// but too late to make it into the release.
//

#if REGISTRY_WORKS


INT
RnRGetTypeByName (
    IN     LPTSTR          lpServiceName,
    IN OUT LPGUID          lpServiceType
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT err;
    HKEY serviceTypesKey;
    HKEY serviceKey;
    TCHAR guidString[100];
    DWORD length;
    DWORD type;

    WS2LOG_F1( "rnr2ops.c - RnRGetTypeByName" );

    //
    // Open the key that stores the name space provider info.
    //

    err = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE,
              NSP_SERVICE_KEY_NAME,
              0,
              KEY_READ,
              &serviceTypesKey
              );

    if ( err == NO_ERROR ) {

        //
        // Open the key for this particular service.
        //

        err = RegOpenKeyEx(
                  serviceTypesKey,
                  lpServiceName,
                  0,
                  KEY_READ,
                  &serviceKey
                  );
        RegCloseKey( serviceTypesKey );

        //
        // If the key exists then we will read the GUID from the registry.
        //

        if ( err == NO_ERROR ) {

            //
            // Query the GUID value for the service.
            //

            length = sizeof(guidString);

            err = RegQueryValueEx(
                      serviceKey,
                      TEXT("GUID"),
                      NULL,
                      &type,
                      (PVOID)guidString,
                      &length
                      );
            RegCloseKey( serviceKey );
            if ( err != NO_ERROR ) {
                SetLastError( err );
                return -1;
            }

            //
            // Convert the Guid string to a proper Guid representation.
            // Before calling the conversion routine, we must strip the
            // leading and trailing braces from the string.
            //

            guidString[_tcslen(guidString) - 1] = L'\0';

            err = UuidFromString( guidString + 1, lpServiceType );

            if ( err != NO_ERROR ) {
                SetLastError( err );
                return -1;
            }

            return NO_ERROR;
        }
    }
    return -1;

} //RnR GetTypeByName


DWORD
RnRGetPortByType (
    IN     LPGUID          lpServiceType,
    IN     DWORD           dwType
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT err;
    HKEY serviceTypesKey;
    DWORD keyIndex;
    TCHAR serviceName[255];
    DWORD nameLength;
    FILETIME lastWriteTime;
    GUID guid;

    WS2LOG_F1( "rnr2ops.c - RnRGetPortByType" );

    //
    // Open the key that stores the name space provider info.
    //

    err = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE,
              NSP_SERVICE_KEY_NAME,
              0,
              KEY_READ,
              &serviceTypesKey
              );

    if ( err == NO_ERROR ) {

        //
        // Walk through the service keys, checking whether each one
        // corresponds to the Guid we're checking against.
        //

        keyIndex = 0;
        nameLength = sizeof(serviceName);

        while ( (err = RegEnumKeyEx(
                           serviceTypesKey,
                           keyIndex,
                           serviceName,
                           &nameLength,
                           NULL,
                           NULL,
                           NULL,
                           &lastWriteTime) == NO_ERROR ) ) {

            //
            // Get the Guid for this service type. This is pretty lazy
            // but it makes it modular
            //

            err = RnRGetTypeByName( serviceName, &guid );

            if ( err == NO_ERROR ) {

                //
                // Check whether this Guid matches the one we're looking for.
                //

                if ( GuidEqual( lpServiceType, &guid ) ) {
                    HKEY key1;
                    DWORD length = sizeof(DWORD);
                    DWORD dwPort;

                    //
                    // We have a match.  See if we can find the
                    // desired port
                    //

                    err = RegOpenKeyEx(
                              serviceTypesKey,
                              serviceName,
                              0,
                              KEY_READ,
                              &key1
                              );

                    RegCloseKey( serviceTypesKey );

                    if(err == NO_ERROR)
                    {
                        LPTSTR pwszValue;
                        DWORD type;

                        if(dwType == UDP_PORT)
                        {
                            pwszValue = SERVICE_TYPE_VALUE_UDPPORT;
                        }
                        else
                        {
                            pwszValue = SERVICE_TYPE_VALUE_TCPPORT;
                        }

                        err = RegQueryValueEx(
                                  key1,
                                  pwszValue,
                                  NULL,
                                  &type,
                                  (PVOID)&dwPort,
                                  &length
                                  );

                        RegCloseKey(key1);
                        if(err == NO_ERROR)
                        {
                            return dwPort;
                        }
                    }
                    return(NOPORTDEFINED);         // not here.
                }
            }

            //
            // Update locals for the next call to RegEnumKeyEx.
            //

            keyIndex++;
            nameLength = sizeof(serviceName);
        }

        RegCloseKey( serviceTypesKey );
    }

    return NOPORTDEFINED;

} // GetNameByType


//
// Worker to add ClassInfo
//

INT
RnR2AddServiceType(
    IN  PWSASERVICECLASSINFOW psci
    )
{
    HKEY hKey, hKeyService;
    LPTSTR pwszUuid;
    INT err;
    DWORD dwDisposition;
    TCHAR  wszUuid[36 + 1 + 2];    // to hold the GUID

    WS2LOG_F1( "rnr2ops.c - RnR2AddServiceType" );

    err = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                           NSP_SERVICE_KEY_NAME,
                           0,
                           TEXT(""),
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hKey,
                           &dwDisposition );

    if(err)
    {
        return(SOCKET_ERROR);
    }

    //
    // Open the key corresponding to the service (create if not there).
    //

    err = RegCreateKeyEx( hKey,
                          psci->lpszServiceClassName,
                          0,
                          TEXT(""),
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &hKeyService,
                          &dwDisposition
                          );

    if(!err)
    {
        //
        // ready to put the GUID value in.
        //

        UuidToString( psci->lpServiceClassId, &pwszUuid);
        wszUuid[0] = TEXT('{');
        memcpy(&wszUuid[1], pwszUuid, 36 * sizeof(TCHAR));
        wszUuid[37] = TEXT('}');
        wszUuid[38] = 0;

        lpRpcStringFree(&pwszUuid);

        //
        // write it
        err = RegSetValueEx(
                     hKeyService,
                     TEXT("GUID"),
                     0,
                     REG_SZ,
                     (LPBYTE)wszUuid,
                     39 * sizeof(TCHAR));

        if(!err)
        {
            //
            // add the appropriate items from the Class Info structures

            PWSANSCLASSINFOW pci = psci->lpClassInfos;
            DWORD dwCount = psci->dwCount;

            while(dwCount--)
            {
                if(pci->dwNameSpace == NS_DNS)
                {
                    //
                    // it's ours
                    //
                    err = RegSetValueEx( hKeyService,
                                         pci->lpszName,
                                         0,
                                         pci->dwValueType,
                                         (LPBYTE)pci->lpValue,
                                         pci->dwValueSize);

                   if(err)
                   {
                       break;
                   }
                }
                pci++;
            }

        }
        RegCloseKey(hKeyService);
    }
    RegCloseKey(hKey);
    if(err)
    {
        err = SOCKET_ERROR;
    }
    return(err);
}

#endif   // if REGISTRY_WORKS. Code not included in NT4 because of time

