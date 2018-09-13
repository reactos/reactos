/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    rnr.c

Abstract:

    This module contains the DNS RNR support routines for the
    Winsock 2 to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

Author:

    Arnold Miller (ArnoldM)  3-Jan-1996

Revision History:

    Keith Moore (keithmo)   10-Jul-1996
        Clean up, made it work with the 2:1 mapper.

--*/


#include "precomp.h"


//
// Alignment macros.
//

#define ROUND_UP_COUNT(Count,Pow2)  ( ((Count)+(Pow2)-1) & (~((Pow2)-1)) )

#define ALIGN_WORD  2
#define ALIGN_DWORD 4


//
// Bit defs for the protocols
//

#define UDP_BIT            1
#define TCP_BIT            2


typedef struct _DNS_RNR_CONTEXT {

    LIST_ENTRY ListEntry;
    LONG      lSig;
    LONG      lInUse;
    LONG      lInstance;              // counter to tell if we've
                                      //  resolved this yet
    DWORD     fFlags;                 // always nice to have
    DWORD     dwControlFlags;
    DWORD     dwUdpPort;
    DWORD     dwTcpPort;
    DWORD     dwNameSpace;
    HANDLE    Handle;                 // the corresponding RnR handle
    DWORD     nProt;
    GUID      gdType;                 // the type we are seeking
    GUID      gdProviderId;           // the provider being used
    DWORD     dwHostSize;
    struct hostent * phent;
    PHOOKER_INFORMATION HookerInfo;
    WCHAR     wcName[1];              // the name

} DNS_RNR_CONTEXT, *PDNS_RNR_CONTEXT;

#define RNR_SIG 0xaabbccdd

#define REVERSELOOK 0x02
#define LOCALLOOK   0x04
#define NEEDDOMAIN  0x08
#define IANALOOK    0x10
#define LOOPLOOK    0x20

//
// Function prototypes
//

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
    BOOL  fOffsets
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

BOOL
IsLocalComputerName(
    IN LPGUID ProviderId,
    IN LPSTR AnsiName
    );

VOID
RnR2Cleanup(
    VOID
    );

LPSTR
GetAnsiNameRnR (
    IN PWCHAR Name,
    IN LPSTR  Domain
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

DWORD
AllocateUnicodeString (
    IN     LPSTR   lpAnsi,
    IN OUT PWCHAR *lppUnicode
    );

DWORD
GetServerAndProtocolsFromString(
    PHOOKER_INFORMATION HookerInfo,
    PWCHAR lpszString,
    LPGUID lpType,
    struct servent ** pServEnt
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


//
// Definitions and data
//

LIST_ENTRY ListAnchor = {&ListAnchor, &ListAnchor};
#define CHECKANCHORQUEUE         // do queue checking on the above

GUID HostnameGuid = SVCID_HOSTNAME;
GUID AddressGuid = SVCID_INET_HOSTADDRBYINETSTRING;
GUID InetHostName = SVCID_INET_HOSTADDRBYNAME;
GUID IANAGuid = SVCID_INET_SERVICEBYNAME;

LONG  lStartupCount;

#define NAME_SIZE    50

#define NOPORTDEFINED (0xffffffff)

#define UDP_PORT  0                // look for the UDP port type
#define TCP_PORT  1                // look for the TCP port type

CRITICAL_SECTION csRnRLock;

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


//
// Get a servent for a string.
//
//

DWORD
GetServerAndProtocolsFromString(
    PHOOKER_INFORMATION HookerInfo,
    PWCHAR lpszString,
    LPGUID lpType,
    struct servent ** pServEnt
    )
{
    DWORD nProt;
    struct servent * sent;

    if(lpszString
          &&
       lpType
          &&
       (GUIDS_ARE_EQUAL(lpType, &HostnameGuid)
                  ||
        GUIDS_ARE_EQUAL(lpType, &InetHostName) ) )
    {
        PCHAR servname, protocolname;
        WCHAR wszTemp[1000];
        INT  port = 0;
        WCHAR * pwszTemp;
        DWORD dwLen;
        PCHAR pszTemp;

        //
        // the string is  of the form service/protocol. If there is no
        // protocol, just look up the name and take the first entry.
        // The service name might be a port number ...
        //

        pwszTemp = wcschr(lpszString, L'/');
        if(!pwszTemp)
        {
             pwszTemp = wcschr(lpszString, L'\0');
        }

        //
        // copy the service name so we can render it into ASCII
        //

        dwLen = (DWORD)(pwszTemp - lpszString);
        memcpy(wszTemp, lpszString, dwLen * sizeof(WCHAR));
        wszTemp[dwLen] = 0;
        servname = GetAnsiNameRnR(wszTemp, 0);

        //
        // if it's numeric, then we get a port from it.
        //

        for(pszTemp = servname;
            *pszTemp && isdigit(*pszTemp);
            pszTemp++)  ;

        if(!*pszTemp)
        {
            //
            // it's numeric. Get the number
            //

            port = atoi(servname);
        }


        //
        // now the protocol.
        //

        if(!*pwszTemp
              ||
           !*++pwszTemp)
        {
            protocolname = 0;
        }
        else
        {

             protocolname = GetAnsiNameRnR(pwszTemp, 0);

        }
        //
        // let's get the entry
        //

        SockPreApiCallout();

        if(port)
        {
            sent = HookerInfo->getservbyport(port, protocolname);
        }
        else
        {
            sent = HookerInfo->getservbyname(servname, protocolname);
        }

        SockPostApiCallout();

        if(servname)
        {
            SOCK_FREE_HEAP(servname);
        }

        if(protocolname)
        {
            SOCK_FREE_HEAP(protocolname);
        }
    }
    else
    {
          sent = 0;
    }

    if(pServEnt)
    {
        *pServEnt = sent;
    }
    if(sent)
    {
        if(_stricmp("udp", sent->s_proto))
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
    return(nProt);
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
    DWORD count, nAddresses;
    DWORD i;
    DWORD requiredBufferLength;
    PSOCKADDR_IN sockaddrIn;
    PCSADDR_INFO csaddrInfo;

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

    //
    // Count the number of IP addresses in the hostent.
    //

    for ( count = 0; HostEntry->h_addr_list[count] != NULL; count++ );

    count *= nAddresses;

    //
    // Make sure that the buffer is large enough to hold all the entries
    // which will be necessary.
    //

    requiredBufferLength = count * (sizeof(CSADDR_INFO) +
                               2*sizeof(SOCKADDR_IN));
;

    *lplBufferLength -= requiredBufferLength;
    *lpdwBytesTaken = requiredBufferLength;
    if ( *lplBufferLength < 0)
    {
        return(WSAEFAULT);
    }


    //
    // For each IP address, fill in the user buffer with one entry.
    //

    sockaddrIn = (PSOCKADDR_IN)((PCSADDR_INFO)lpCsaddrBuffer + count);
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

        for ( i = 0; i < count; i++, csaddrInfo++, sockaddrIn++ )
        {

            //
            // First fill in the local address.  It should remain basically
            // all zeros except for the family so that it is a "wildcard"
            // address for binding.
            //

            ZeroMemory( csaddrInfo, sizeof(*csaddrInfo) );

            csaddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)sockaddrIn;
            csaddrInfo->LocalAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

            ZeroMemory( sockaddrIn, sizeof(*sockaddrIn) );
            sockaddrIn->sin_family = AF_INET;

            //
            // Now the remote address.
            //

            sockaddrIn++;

            csaddrInfo->RemoteAddr.lpSockaddr = (PSOCKADDR)( sockaddrIn );
            csaddrInfo->RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_IN);

            sockaddrIn = (PSOCKADDR_IN)csaddrInfo->RemoteAddr.lpSockaddr;
            ZeroMemory( sockaddrIn, sizeof(*sockaddrIn) );
            sockaddrIn->sin_family = AF_INET;

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

            if ( IsTcp ) {
                csaddrInfo->iSocketType = SOCK_STREAM;
                csaddrInfo->iProtocol = IPPROTO_TCP;
            } else {
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
        }
    }

    *Count = count;

    return(NO_ERROR);
}

PVOID
AllocLocal(
    DWORD dwSize
    )
{
    PVOID pvMem;

    pvMem = SOCK_ALLOCATE_HEAP(dwSize);
    if(pvMem)
    {
        ZeroMemory(pvMem, dwSize);
    }
    return(pvMem);
}

LPSTR
GetAnsiNameRnR (
    IN PWCHAR Name,
    IN LPSTR  Domain
    )
{

    PCHAR pszBuffer;
    DWORD dwLen;
    WORD wLen;

    if(Domain)
    {
        wLen = (WORD)strlen(Domain);
    }
    else
    {
        wLen = 0;
    }
    dwLen = ((wcslen(Name) + 1) * sizeof(WCHAR) * 2) + wLen;
    pszBuffer = SOCK_ALLOCATE_HEAP(dwLen);
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
               0))
    {
        SOCK_FREE_HEAP( pszBuffer );
        return NULL;
    }

    if(Domain)
    {
        strcat(pszBuffer, Domain);
    }
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

    pdrc = (PDNS_RNR_CONTEXT)AllocLocal(
                                   sizeof(DNS_RNR_CONTEXT) +
                                   dwExtra);
    if(pdrc)
    {
        pdrc->lInUse = 2;
        pdrc->Handle = (hHandle ? hHandle : (HANDLE)pdrc);
        pdrc->lSig = RNR_SIG;
        pdrc->lInstance = -1;
        EnterCriticalSection(&csRnRLock);
        InsertHeadList(&ListAnchor, &pdrc->ListEntry);
        LeaveCriticalSection(&csRnRLock);
    }
    return(pdrc);
}

VOID
RnR2Cleanup()
{
    PLIST_ENTRY pdrc;

    EnterCriticalSection(&csRnRLock);

    while((pdrc = ListAnchor.Flink) != &ListAnchor)
    {
        RnR2ReleaseContext((PDNS_RNR_CONTEXT)pdrc);
    }

    LeaveCriticalSection(&csRnRLock);
    DeleteCriticalSection(&csRnRLock);
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

    EnterCriticalSection(&csRnRLock);
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

        SOCK_ASSERT(Entry != &ListAnchor);
#endif
        RemoveEntryList(&pdrc->ListEntry);
        if(pdrc->phent)
        {
            SOCK_FREE_HEAP(pdrc->phent);
        }

        if( pdrc->HookerInfo ) {
            SockDereferenceHooker( pdrc->HookerInfo );
        }

        SOCK_FREE_HEAP(pdrc);
    }
    LeaveCriticalSection(&csRnRLock);
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

    EnterCriticalSection(&csRnRLock);

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
    LeaveCriticalSection(&csRnRLock);
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
    WCHAR wszString[1000];
    PHOOKER_INFORMATION hookerInfo;
    struct servent * sent;
    INT err;

    SOCK_ENTER( "NSPLookupServiceBegin", lpProviderId, lpqsRestrictions, lpServiceClassInfo, (PVOID)dwControlFlags );

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
        SetLastError( err );
        return SOCKET_ERROR;

    }

    if( lpqsRestrictions->dwSize < sizeof(WSAQUERYSETW) ) {

        err = WSAEFAULT;
        SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
        SetLastError( err );
        return SOCKET_ERROR;

    }

    if( !lpqsRestrictions->lpServiceClassId ) {

        //
        // gotta have a class ID.
        //

        err = WSA_INVALID_PARAMETER;
        SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
        SetLastError( err );
        return SOCKET_ERROR;

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

        err = WSANO_DATA;
        SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
        SetLastError( err );
        return SOCKET_ERROR;
    }

    //
    // Try to find our hooker.
    //

    hookerInfo = SockFindAndReferenceHooker(
                     lpProviderId
                     );

    if( hookerInfo == NULL ) {

        err = WSA_INVALID_PARAMETER;
        SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
        SetLastError( err );
        return SOCKET_ERROR;

    }

    //
    // If this is a lookup of the local name
    // allow the instance
    // name to be NULL. If it is a reverse lookup, mark it as such.
    // If it is a host lookup, check for one of the several special
    // names that require returning specific local information.
    //

    if( GUIDS_ARE_EQUAL(lpqsRestrictions->lpServiceClassId, &AddressGuid) )
    {
        dwLocalFlags |= REVERSELOOK;
    }
    else if(GUIDS_ARE_EQUAL(lpqsRestrictions->lpServiceClassId, &IANAGuid))
    {
        dwLocalFlags |= IANALOOK;
        dwControlFlags &= ~(LUP_RETURN_ADDR);
    }

    //
    // Compute whether this is some sort of name lookup. Do this
    // here since we've two places below that need to test for it.
    //

    fNameLook = GUIDS_ARE_EQUAL(lpqsRestrictions->lpServiceClassId, &HostnameGuid)
                              ||
                IS_SVCID_TCP(lpqsRestrictions->lpServiceClassId)
                              ||
                IS_SVCID_UDP(lpqsRestrictions->lpServiceClassId)
                              ||
                GUIDS_ARE_EQUAL(lpqsRestrictions->lpServiceClassId, &InetHostName);

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
                DWORD err = GetLastError();

                SockDereferenceHooker( hookerInfo );
                SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
                SetLastError( err );
                return(SOCKET_ERROR);
            }
        }
        else
        {
badparm:
            SockDereferenceHooker( hookerInfo );
            err = WSA_INVALID_PARAMETER;
            SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
            SetLastError( err );
            return(SOCKET_ERROR);
        }
    }
    else if(fNameLook)
    {
        //
        // it's some kind of name lookup. So, let's see if it is a special
        // name
        //
        PCHAR pszAnsiName = GetAnsiNameRnR(pwszServiceName, 0);

        if(!_stricmp(pszAnsiName, "localhost")
                           ||
           !_stricmp(pszAnsiName, "loopback"))
        {
            dwLocalFlags |= LOCALLOOK | LOOPLOOK;
        }
        else if( IsLocalComputerName( lpProviderId, pszAnsiName ) )
        {
            dwLocalFlags |= LOCALLOOK;
        }
        SOCK_FREE_HEAP(pszAnsiName);
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
            SockDereferenceHooker( hookerInfo );
            err = WSANO_DATA;
            SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
            SetLastError( err );
            return(SOCKET_ERROR);
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

    nProt &= GetServerAndProtocolsFromString(
                 hookerInfo,
                 lpqsRestrictions->lpszQueryString,
                 lpqsRestrictions->lpServiceClassId,
                 &sent
                 );

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
            GUIDS_ARE_EQUAL(lpqsRestrictions->lpServiceClassId, &HostnameGuid)
                  ||
            GUIDS_ARE_EQUAL(lpqsRestrictions->lpServiceClassId, &InetHostName) )
        {
            dwUdpPort = NOPORTDEFINED;
            dwTcpPort = 0;

        }
        else
        {
            SockDereferenceHooker( hookerInfo );
            err = WSANO_DATA;
            SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
            SetLastError( err );
            return(SOCKET_ERROR);
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
        SockDereferenceHooker( hookerInfo );
        err = WSA_NOT_ENOUGH_MEMORY;
        SOCK_EXIT( "NSPLookupServiceBegin", SOCKET_ERROR, &err );
        SetLastError( err );
        return(SOCKET_ERROR);
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
    pdrc->HookerInfo = hookerInfo;

    wcscpy(pdrc->wcName, pwszServiceName);

    RnR2ReleaseContext(pdrc);
    SOCK_EXIT( "NSPLookupServiceBegin", NO_ERROR, NULL );

    *lphLookup = (HANDLE)pdrc;
    return(NO_ERROR);
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

    INT err;
    PDNS_RNR_CONTEXT pdrc;
    PBYTE pData = (PBYTE)(lpqsResults + 1);
    LONG lSpare = (LONG)*lpdwBufferLength - lpqsResults->dwSize;
    LPSTR pAnsiName = NULL;
    PHOSTENT hostEntry = NULL;
    PCHAR pBuff = (PCHAR)(lpqsResults + 1);
    DWORD dwTaken;
    LONG lFree,  lInstance;
    PCHAR pszDomain, pszName;
    struct hostent LocalHostEntry;
    PINT local_addr_list[2];
    INT LocalAddress;
    WSAQUERYSETW wsaqDummy;
    struct servent * sent = NULL;
    CHAR szComputerName[20];
    LONG lSizeOf;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    if(*lpdwBufferLength < sizeof(WSAQUERYSETW))
    {
        lpqsResults = &wsaqDummy;
    }
    ZeroMemory(lpqsResults, sizeof(WSAQUERYSETW));
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

    EnterCriticalSection(&csRnRLock);
    lInstance = ++pdrc->lInstance;
    if(dwControlFlags & LUP_FLUSHPREVIOUS)
    {
        lInstance = ++pdrc->lInstance;
    }
    LeaveCriticalSection(&csRnRLock);

    pszDomain = 0;

    pAnsiName = GetAnsiNameRnR(pdrc->wcName, pszDomain);
    if(!pAnsiName)
    {
        err = WSA_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    SetLastError(NO_ERROR);        // start clean

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
                                pdrc->HookerInfo,
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

                SockPreApiCallout();

                hostEntry = pdrc->HookerInfo->gethostbyaddr(
                    (PCHAR)&dwIp,
                    4,
                    AF_INET
                    );

                if( hostEntry == NULL ) {

                    err = pdrc->HookerInfo->WSAGetLastError();
                    SOCK_ASSERT( err != NO_ERROR );
                    SockPostApiCallout();
                    goto Done;

                }

                SockPostApiCallout();
            }
            else if(!(pdrc->fFlags & LOCALLOOK))
            {
                //
                // a real name lookup. See which provider type to use
                //

                //
                // if in the 2:1 mapper, just defer the call to
                // someone else.
                //

                SockPreApiCallout();

                hostEntry = pdrc->HookerInfo->gethostbyname(
                    pAnsiName
                    );

                if( hostEntry == NULL ) {

                    err = pdrc->HookerInfo->WSAGetLastError();
                    SOCK_ASSERT( err != NO_ERROR );
                    SockPostApiCallout();
                    goto Done;

                }

                SockPostApiCallout();
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

                INT result;
                CHAR szLocalHostName[256];   // allow plenty of room

                //
                // in the 2:1 mapper we have to first get our local host
                // name and then ask for the hostent structure. It's really
                // the "right" way to do this, but we take a  short-cut when
                // we own the provider. Note there's no way to know how
                // much space to allocate, so if 1000 bytes is insufficient,
                // it simply fails, but I suspect many other things fail
                // we well.
                //

                SockPreApiCallout();

                result = pdrc->HookerInfo->gethostname(
                    szLocalHostName,
                    sizeof(szLocalHostName)
                    );

                if( result == SOCKET_ERROR ) {

                    err = pdrc->HookerInfo->WSAGetLastError();
                    SOCK_ASSERT( err != NO_ERROR );
                    SockPostApiCallout();
                    goto Done;

                }

                hostEntry = pdrc->HookerInfo->gethostbyname(
                    pAnsiName
                    );

                if( hostEntry == NULL ) {

                    err = pdrc->HookerInfo->WSAGetLastError();
                    SOCK_ASSERT( err != NO_ERROR );
                    SockPostApiCallout();
                    goto Done;

                }

                SockPostApiCallout();
            }

            SOCK_ASSERT( hostEntry != NULL );

            //
            // copy the host entry so we can keep a local copy of
            // it

            pdrc->phent = hostEntry = CopyHostEntry(hostEntry,
                                                    0,
                                                    0,
                                                    &lSizeOf,
                                                    FALSE);
            pdrc->dwHostSize = (DWORD)lSizeOf;
        }

        SOCK_ASSERT( hostEntry != NULL );

        //
        // Got the data. See if we've returned all of the results yet.
        //

        if(!lInstance)
        {
            pszName = hostEntry->h_name;
        }
        else
        {
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

    if(pdrc->dwControlFlags & LUP_RETURN_BLOB)
    {
        if(GUIDS_ARE_EQUAL(&pdrc->gdType, &InetHostName)
                    ||
           GUIDS_ARE_EQUAL(&pdrc->gdType, &AddressGuid) )
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
    }

    if(pdrc->dwControlFlags & LUP_RETURN_NAME)
    {
        PWCHAR pszString;
        DWORD dwLen;

        //
        // and the caller wants the name. Make sure it fits
        //

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
            CopyMemory(
                     pBuff,
                     pszString,
                     dwLen);
            lpqsResults->lpszServiceInstanceName = (WCHAR *)pBuff;
            pBuff += dwLen;
        }
        SOCK_FREE_HEAP(pszString);
    }

Done:
    if(pAnsiName)
    {
        SOCK_FREE_HEAP(pAnsiName);
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
            EnterCriticalSection(&csRnRLock);
            --pdrc->lInstance;
            LeaveCriticalSection(&csRnRLock);
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
    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    pdrc = RnR2GetContext(hLookup);
    if(!pdrc)
    {

        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    }

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
    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    return(NO_ERROR);
}

INT WINAPI
NSPCleanup(
    IN LPGUID lpProviderId
    )
{
    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    if(!InterlockedDecrement(&lStartupCount))
    {
        RnR2Cleanup();          // zap all contexts
    }

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
    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    //
    // Since DNS is a static database, there's nothing to do
    //

    SetLastError(ERROR_NOT_SUPPORTED);
    return(SOCKET_ERROR);
}

INT WINAPI
NSPInstallServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
{

    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    //
    // Must be done manually
    //

    return(NO_ERROR);
}

INT WINAPI
NSPRemoveServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPGUID lpServiceCallId
    )
{

    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    return(NO_ERROR);
}

INT WINAPI
NSPGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
{

    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    //
    // Fetch the class info stuff from DNS
    //

    SetLastError(WSASERVICE_NOT_FOUND);
    return(SOCKET_ERROR);
}


INT WINAPI
NSPStartup(
    IN LPGUID            lpProviderId,
    IN OUT LPNSP_ROUTINE lpsnpRoutines)
{
    DWORD dwSize = min(sizeof(nsrVector), lpsnpRoutines->cbSize);
    INT err;

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        SetLastError( err );
        return SOCKET_ERROR;

    }

    //
    // BUGBUG. If no size is provided, assume it is big enough!!!!
    //

    if( InterlockedIncrement(&lStartupCount) == 1 ) {

        try {

            InitializeCriticalSection( &csRnRLock );
            err = NO_ERROR;

        } except( SOCK_EXCEPTION_FILTER() ) {

            err = GetExceptionCode();

        }

        if( err != NO_ERROR ) {

            InterlockedDecrement( &lStartupCount );
            return err;

        }

    }

    if(!dwSize)
    {
        dwSize = sizeof(nsrVector);
    }

    CopyMemory(lpsnpRoutines,
                  &nsrVector,
                  dwSize);

    return NO_ERROR;

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
GetAliasSize(PCHAR * pal, PDWORD pdwCount)
{
    DWORD dwSize;

    dwSize = sizeof(PCHAR);
    *pdwCount = 1;

    if(pal)
    {
        for(; *pal; pal++)
        {
            dwSize += sizeof(PCHAR);

            dwSize += strlen(*pal) + 1;

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
    BOOL fOffsets
    )
{
    PBYTE pb;
    struct hostent * ph;
    DWORD dwSize, dwAddCount, dwAlCount;

    dwSize = sizeof(struct hostent) +
             GetAliasSize(phent->h_aliases, &dwAlCount) +
             GetAddrSize(phent, &dwAddCount) +
             strlen(phent->h_name) + 1;

    if(!(pb = pbAllocated))
    {
        pb = (PBYTE)AllocLocal(dwSize);
    }
    else
    {
        //
        // align it first. This is done to insure that if this
        // space is within another buffer, as it will be when
        // returning a BLOB through a LookupServiceNext, that
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
                CopyMemory(pb, *pcs, phent->h_length);
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
                DWORD dwLen = strlen(*pcs) + 1;

                *pcd = (PCHAR)pb;
                CopyMemory(pb, *pcs, dwLen);
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
            strcpy(pb, phent->h_name);
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

    dwNameSize = strlen(sent->s_name) + 1;
    dwSize = sizeof(struct servent) +
             GetAliasSize(sent->s_aliases, &dwAlCount) +
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
        // returning a BLOB through a LookupServiceNext, that
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
                CopyMemory(pb, *pcs, dwLen);
                pb += dwLen;
                pcd++;
                pcs++;
            }
        }
        *pcd = 0;

        // now the two strings

        ps->s_name = (PCHAR)pb;
        MoveMemory(pb, sent->s_name, dwNameSize);
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


DWORD
AllocateUnicodeString (
    IN     LPSTR   lpAnsi,
    IN OUT PWCHAR *lppUnicode
)
/*++

Routine Description:

   Allocate a Unicode String intialized with the Ansi one.
   Caller must free with SOCK_FREE_HEAP().

Arguments:

   lpAnsi     - ANSI string that is used to init the Unicode string

   lppUnicode - address to receive pointer to Unicode string.

Return Value:

   NO_ERROR if successful. Win32 error otherwise.

--*/

{
    LPWSTR         UnicodeString;
    INT            err;
    DWORD          dwAnsiLen;

    *lppUnicode = NULL ;

    //
    // handle the trivial case
    //
    if (!lpAnsi)
    {
        return NO_ERROR ;
    }

    //
    // allocate the memory
    //
    dwAnsiLen = strlen(lpAnsi) + 1;

    UnicodeString = (LPWSTR)SOCK_ALLOCATE_HEAP(dwAnsiLen * sizeof(WCHAR));

    if (!UnicodeString)
    {
        return ERROR_NOT_ENOUGH_MEMORY ;
    }

    //
    // convert it
    //

    err = MultiByteToWideChar(
                   CP_ACP,         // better by ANSI
                   0,
                   lpAnsi,
                   -1,             // it's NULL terminated
                   UnicodeString,
                   dwAnsiLen );    // # of wide characters available

    if (!err)
    {
        SOCK_FREE_HEAP(UnicodeString) ;
        return (GetLastError());
    }

    *lppUnicode = UnicodeString;

    return NO_ERROR ;
}

BOOL
IsLocalComputerName(
    IN LPGUID ProviderId,
    IN LPSTR AnsiName
    )
{

    PHOOKER_INFORMATION hookerInfo;
    LPSTR dot;
    INT result;
    BOOL value;
    CHAR name[256];

    value = FALSE;

    hookerInfo = SockFindAndReferenceHooker( ProviderId );

    if( hookerInfo == NULL ) {

        goto exit;

    }

    SockPreApiCallout();

    result = hookerInfo->gethostname(
                 name,
                 sizeof(name)
                 );

    if( result == SOCKET_ERROR ) {

        SockPostApiCallout();
        goto exit;

    }

    SockPostApiCallout();

    if( _stricmp( name, AnsiName ) == 0 ) {

        value = TRUE;
        goto exit;

    }

    dot = name;

    while( *dot != '\0' && *dot != '.' ) {

        dot++;

    }

    *dot = '\0';

    if( _stricmp( name, AnsiName ) == 0 ) {

        value = TRUE;
        goto exit;

    }

exit:

    if( hookerInfo != NULL ) {

        SockDereferenceHooker( hookerInfo );

    }

    return value;

}   // IsLocalComputerName

