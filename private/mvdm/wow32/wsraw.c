/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Wsraw.h

Abstract:

    Support for raw winsock calls for WOW.

Author:

    David Treadwell (davidtr)    02-Oct-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "wsdynmc.h"

LIST_ENTRY WWS32SocketHandleListHead;
WORD WWS32SocketHandleCounter;
BOOL WWS32SocketHandleCounterWrapped;

//
// The (PCHAR) casts in the following macro force the compiler to assume
// only BYTE alignment.
//

#define SockCopyMemory(d,s,l) RtlCopyMemory( (PCHAR)(d), (PCHAR)(s), (l) )


#define WSEXIT_IF_NOT_INTIALIZED()           \
    if(!WWS32IsThreadInitialized) {          \
        SetLastError(WSANOTINITIALISED);     \
        RETURN((ULONG)SOCKET_ERROR);         \
    }


int SocketOption16To32(IN WORD SocketOption16);

DWORD WSGetWinsock32(IN  HAND16 h16,
                     OUT PULONG pul);

BOOL WSThunkAddrBufAndLen(IN  PSOCKADDR  fastSockaddr, 
                          IN  VPSOCKADDR vpSockAddr16,
                          IN  VPWORD     vpwAddrLen16,
                          OUT PINT       addressLength,
                          OUT PINT      *pAddressLength,
                          OUT PSOCKADDR *realSockaddr);

VOID WSUnThunkAddrBufAndLen(IN ULONG      ret,
                            IN VPWORD     vpwAddrLen16,
                            IN VPSOCKADDR vpSockAddr16,
                            IN INT        addressLength,
                            IN PSOCKADDR  fastSockaddr,
                            IN PSOCKADDR  realSockaddr);

BOOL WSThunkAddrBuf(IN  INT         addressLength,
                    IN  VPSOCKADDR  vpSockAddr16,
                    IN  PSOCKADDR   fastSockaddr, 
                    OUT PSOCKADDR  *realSockaddr);

VOID WSUnThunkAddrBuf(IN PSOCKADDR  fastSockaddr, 
                      IN PSOCKADDR  realSockaddr);

BOOL WSThunkRecvBuffer(IN  INT    BufferLength,
                       IN  VPBYTE vpBuf16,
                       OUT PBYTE  *buffer);

VOID WSUnthunkRecvBuffer(IN INT    cBytes,
                         IN INT    BufferLength, 
                         IN VPBYTE vpBuf16,
                         IN PBYTE  buffer);

BOOL WSThunkSendBuffer(IN  INT    BufferLength,
                       IN  VPBYTE vpBuf16,
                       OUT PBYTE  *buffer);

VOID WSUnthunkSendBuffer(IN PBYTE buffer);




/*++

 GENERIC FUNCTION PROTOTYPE:
 ==========================

ULONG FASTCALL WWS32<function name>(PVDMFRAME pFrame)
{
    ULONG ul;
    register P<function name>16 parg16;

    GETARGPTR(pFrame, sizeof(<function name>16), parg16);

    <get any other required pointers into 16 bit space>

    ALLOCVDMPTR
    GETVDMPTR
    GETMISCPTR
    et cetera

    <copy any complex structures from 16 bit -> 32 bit space>
    <ALWAYS use the FETCHxxx macros>

    ul = GET<return type>16(<function name>(parg16->f1,
                                                :
                                                :
                                            parg16->f<n>);

    <copy any complex structures from 32 -> 16 bit space>
    <ALWAYS use the STORExxx macros>

    <free any pointers to 16 bit space you previously got>

    <flush any areas of 16 bit memory if they were written to>

    FLUSHVDMPTR

    FREEARGPTR( parg16 );
    RETURN( ul );
}

NOTE:

  The VDM frame is automatically set up, with all the function parameters
  available via parg16->f<number>.

  Handles must ALWAYS be mapped for 16 -> 32 -> 16 space via the mapping tables
  laid out in WALIAS.C.

  Any storage you allocate must be freed (eventually...).

  Further to that - if a thunk which allocates memory fails in the 32 bit call
  then it must free that memory.

  Also, never update structures in 16 bit land if the 32 bit call fails.

  Be aware that the GETxxxPTR macros return the CURRENT selector-to-flat_memory
  mapping.  Calls to some 32-bit functions may indirectly cause callbacks into
  16-bit code.  These may cause 16-bit memory to move due to allocations
  made in 16-bit land.  If the 16-bit memory does move, the corresponding 32-bit
  ptr in WOW32 needs to be refreshed to reflect the NEW selector-to-flat_memory
  mapping.

--*/


ULONG FASTCALL WWS32accept(PVDMFRAME pFrame)
{
    ULONG      ul = GETWORD16(INVALID_SOCKET);
    register   PACCEPT16 parg16;
    SOCKET     s32;
    SOCKET     news32;
    HSOCKET16  news16;
    INT        addressLength;
    PINT       pAddressLength;
    SOCKADDR   fastSockaddr;
    PSOCKADDR  realSockaddr;
    VPWORD     vpwAddrLen16;
    VPSOCKADDR vpSockAddr16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PACCEPT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpwAddrLen16 = (VPWORD)FETCHDWORD(parg16->AddressLength);
    vpSockAddr16 = (VPSOCKADDR)FETCHDWORD(parg16->Address);

    // Thunk the 16-bit Address name and length buffers
    if(!WSThunkAddrBufAndLen(&fastSockaddr, 
                             vpSockAddr16,
                             vpwAddrLen16, 
                             &addressLength,
                             &pAddressLength,
                             &realSockaddr)) {
        goto exit;
    }

    // call the 32-bit API
    news32 = (*wsockapis[WOW_ACCEPT].lpfn)( s32, realSockaddr, pAddressLength);

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit Address name and length buffers
    WSUnThunkAddrBufAndLen((ULONG)news32,
                           vpwAddrLen16,
                           vpSockAddr16,
                           addressLength,
                           &fastSockaddr,
                           realSockaddr);

    //
    // If the call succeeded, alias the 32-bit socket handle we just
    // obtained into a 16-bit handle.
    //

    if ( news32 != INVALID_SOCKET ) {

        news16 = GetWinsock16( news32, 0 );

        if ( news16 == 0 ) {

            (*wsockapis[WOW_CLOSESOCKET].lpfn)( news32 );
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );

            // Note: 16-bit callbacks resulting from above function 
            //       call may have caused 16-bit memory movement

            goto exit;
        }

        ul = news16;

    }

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32accept









ULONG FASTCALL WWS32bind(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PBIND16 parg16;
    SOCKET      s32;
    SOCKADDR    fastSockaddr;
    PSOCKADDR   realSockaddr;
    INT         addressLength;
    VPSOCKADDR  vpSockAddr16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PBIND16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpSockAddr16 = (VPSOCKADDR)FETCHDWORD(parg16->Address);

    addressLength = INT32(parg16->AddressLength);

    // Thunk the 16-bit address buffer
    if(!WSThunkAddrBuf(addressLength,
                       vpSockAddr16,
                       &fastSockaddr, 
                       &realSockaddr)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_BIND].lpfn)(s32, 
                                                realSockaddr, 
                                                addressLength));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit address buffer
    WSUnThunkAddrBuf(&fastSockaddr, realSockaddr);

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32bind









ULONG FASTCALL WWS32closesocket(PVDMFRAME pFrame)
{
    ULONG     ul = GETWORD16(INVALID_SOCKET);
    register  PCLOSESOCKET16 parg16;
    SOCKET    s32;
    HSOCKET16 hSocket16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(CLOSESOCKET16), parg16);

    hSocket16 = (HSOCKET16)FETCHWORD(parg16->hSocket);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(hSocket16, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_CLOSESOCKET].lpfn)( s32 ) );

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);


exit:
    //
    // Free the space in the alias table.
    //

    FreeWinsock16( hSocket16 );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32closesocket








ULONG FASTCALL WWS32connect(PVDMFRAME pFrame)
{
    ULONG      ul = GETWORD16(INVALID_SOCKET);
    register   PCONNECT16 parg16;
    SOCKET     s32;
    SOCKADDR   fastSockaddr;
    PSOCKADDR  realSockaddr;
    INT        addressLength;
    VPSOCKADDR vpSockAddr16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PCONNECT16), parg16);

    vpSockAddr16  = (VPSOCKADDR)FETCHDWORD(parg16->Address);
    addressLength = INT32(parg16->AddressLength);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    // Thunk the 16-bit address buffer
    if(!WSThunkAddrBuf(addressLength,
                       vpSockAddr16,
                       &fastSockaddr, 
                       &realSockaddr)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_CONNECT].lpfn)(s32, 
                                                   realSockaddr, 
                                                   addressLength));


    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit address buffer
    WSUnThunkAddrBuf(&fastSockaddr, realSockaddr);

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32connect








ULONG FASTCALL WWS32getpeername(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PGETPEERNAME16 parg16;
    SOCKET      s32;
    INT         addressLength;
    PINT        pAddressLength;
    SOCKADDR    fastSockaddr;
    PSOCKADDR   realSockaddr;
    VPWORD      vpwAddrLen16;
    VPSOCKADDR  vpSockAddr16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PGETPEERNAME16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpSockAddr16 = (VPSOCKADDR)FETCHDWORD(parg16->Address);
    vpwAddrLen16 = (VPWORD)FETCHDWORD(parg16->AddressLength);

    // Thunk the 16-bit Address name and length buffers
    if(!WSThunkAddrBufAndLen(&fastSockaddr, 
                             vpSockAddr16,
                             vpwAddrLen16, 
                             &addressLength,
                             &pAddressLength,
                             &realSockaddr)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_GETPEERNAME].lpfn)(s32, 
                                                       realSockaddr, 
                                                       pAddressLength));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit Address name and length buffers
    WSUnThunkAddrBufAndLen(ul,
                           vpwAddrLen16,
                           vpSockAddr16,
                           addressLength,
                           &fastSockaddr,
                           realSockaddr);

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32getpeername








ULONG FASTCALL WWS32getsockname(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PGETSOCKNAME16 parg16;
    SOCKET      s32;
    INT         addressLength;
    PINT        pAddressLength;
    SOCKADDR    fastSockaddr;
    PSOCKADDR   realSockaddr;
    VPWORD      vpwAddrLen16;
    VPSOCKADDR  vpSockAddr16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PGETSOCKNAME16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpSockAddr16 = (VPSOCKADDR)FETCHDWORD(parg16->Address);
    vpwAddrLen16 = (VPWORD)FETCHDWORD(parg16->AddressLength);

    // Thunk the 16-bit Address name and length buffers
    if(!WSThunkAddrBufAndLen(&fastSockaddr, 
                             vpSockAddr16,
                             vpwAddrLen16, 
                             &addressLength,
                             &pAddressLength,
                             &realSockaddr)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_GETSOCKNAME].lpfn)( s32, realSockaddr, pAddressLength ) );

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit Address name and length buffers
    WSUnThunkAddrBufAndLen(ul,
                           vpwAddrLen16,
                           vpSockAddr16,
                           addressLength,
                           &fastSockaddr,
                           realSockaddr);

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32getsockname









ULONG FASTCALL WWS32getsockopt(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PGETSOCKOPT16 parg16;
    SOCKET      s32;
    WORD        UNALIGNED *optionLength16;
    WORD        actualOptionLength16;
    PBYTE       optionValue16;
    DWORD       optionLength32;
    PBYTE       optionValue32;
    VPWORD      vpwOptLen16;
    VPBYTE      vpwOptVal16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PGETSOCKOPT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpwOptLen16 = (VPWORD)FETCHDWORD(parg16->OptionLength);
    vpwOptVal16 = (VPBYTE)FETCHDWORD(parg16->OptionValue);
    GETVDMPTR( vpwOptLen16, sizeof(WORD), optionLength16 );
    GETVDMPTR( vpwOptVal16, FETCHWORD(*optionLength16), optionValue16 );

    if ( FETCHWORD(*optionLength16) < sizeof(WORD) ) {
        FREEVDMPTR( optionLength16 );
        FREEVDMPTR( optionValue16 );
        FREEARGPTR( parg16 );
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAEFAULT );
        ul = (ULONG)GETWORD16(SOCKET_ERROR );
        RETURN( ul );
    } else if ( FETCHWORD(*optionLength16) < sizeof(DWORD) ) {
        optionLength32 = sizeof(DWORD);
    } else {
        optionLength32 = FETCHWORD(*optionLength16);
    }

    optionValue32 = malloc_w(optionLength32);

    if ( optionValue32 == NULL ) {
        FREEVDMPTR( optionLength16 );
        FREEVDMPTR( optionValue16 );
        FREEARGPTR( parg16 );
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        ul = (ULONG)GETWORD16(SOCKET_ERROR );
        RETURN( ul );
    }

    SockCopyMemory( optionValue32, optionValue16, optionLength32 );

    ul = GETWORD16( (*wsockapis[WOW_GETSOCKOPT].lpfn)(
                     s32,
                     parg16->Level,
                     SocketOption16To32( parg16->OptionName ),
                     (char *)optionValue32,
                     (int *)&optionLength32));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);
    FREEVDMPTR(optionLength16);
    FREEVDMPTR(optionValue16);

    if ( ul == NO_ERROR ) {
        GETVDMPTR( vpwOptLen16, sizeof(WORD), optionLength16 );
        GETVDMPTR( vpwOptVal16, FETCHWORD(*optionLength16), optionValue16 );

        actualOptionLength16 = (WORD) min(optionLength32, FETCHWORD(*optionLength16));

        RtlMoveMemory( optionValue16, optionValue32, actualOptionLength16 );

        STOREWORD(*optionLength16, actualOptionLength16);

        FLUSHVDMPTR( vpwOptLen16, sizeof(parg16->OptionLength), optionLength16 );
        FLUSHVDMPTR( vpwOptVal16, actualOptionLength16, optionValue16 );
    }

    FREEVDMPTR( optionLength16 );
    FREEVDMPTR( optionValue16 );

exit:
    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32getsockopt









ULONG FASTCALL WWS32htonl(PVDMFRAME pFrame)
{
    ULONG ul;
    register PHTONL16 parg16;

    GETARGPTR(pFrame, sizeof(HTONL16), parg16);

    ul = (*wsockapis[WOW_HTONL].lpfn)( parg16->HostLong );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32htonl









ULONG FASTCALL WWS32htons(PVDMFRAME pFrame)
{
    ULONG ul;
    register PHTONS16 parg16;

    GETARGPTR(pFrame, sizeof(HTONS16), parg16);

    ul = GETWORD16( (*wsockapis[WOW_HTONS].lpfn)( parg16->HostShort ) );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32htons









ULONG FASTCALL WWS32inet_addr(PVDMFRAME pFrame)
{
    ULONG ul;
    register PINET_ADDR16 parg16;
    PSZ addressString;
    CHAR     szAddrStr[32];
    register PINET_ADDR16 realParg16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(INET_ADDR16), parg16);

    realParg16 = parg16;

    GETVDMPTR( parg16->cp, 1, addressString );
    strcpy(szAddrStr, addressString);
    FREEVDMPTR( addressString );

    //
    // If the thread is version 1.0 of Windows Sockets, play special
    // stack games to return a struct in_addr.
    //

    if ( WWS32IsThreadVersion10 ) {

        PDWORD inAddr16;
        ULONG inAddr32;

        ul = *((PWORD)parg16);
        ul |= pFrame->wAppDS << 16;

        parg16 = (PINET_ADDR16)( (PCHAR)parg16 + 2 );

        inAddr32 = (*wsockapis[WOW_INET_ADDR].lpfn)( szAddrStr );

        ASSERT( sizeof(IN_ADDR) == sizeof(DWORD) );
        GETVDMPTR( ul, sizeof(DWORD), inAddr16 );
        STOREDWORD( *inAddr16, inAddr32 );
        FLUSHVDMPTR( ul, sizeof(DWORD), inAddr16 );
        FREEVDMPTR( inAddr16 );

    } else {
        ul = (*wsockapis[WOW_INET_ADDR].lpfn)( szAddrStr );
    }

    FREEARGPTR( realParg16 );

    RETURN( ul );

} // WWS32inet_addr









ULONG FASTCALL WWS32inet_ntoa(PVDMFRAME pFrame)
{
    ULONG ul;
    register PINET_NTOA16 parg16;
    PSZ ipAddress;
    PSZ ipAddress16;
    IN_ADDR in32;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(INET_NTOA16), parg16);

    in32.s_addr = parg16->in;

    ipAddress = (PSZ) (*wsockapis[WOW_INET_NTOA].lpfn)( in32 );

    if ( ipAddress != NULL ) {
        GETVDMPTR( WWS32vIpAddress, strlen( ipAddress )+1, ipAddress16 );
        strcpy( ipAddress16, ipAddress );
        FLUSHVDMPTR( WWS32vIpAddress, strlen( ipAddress )+1, ipAddress16 );
        FREEVDMPTR( ipAddress16 );
        ul = WWS32vIpAddress;
    } else {
        ul = 0;
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32inet_ntoa









ULONG FASTCALL WWS32ioctlsocket(PVDMFRAME pFrame)
{
    ULONG ul;
    register PIOCTLSOCKET16 parg16;
    SOCKET s32;
    PDWORD argument16;
    DWORD argument32;
    DWORD command;
    VPDWORD vpdwArg16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(IOCTLSOCKET16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpdwArg16 = (VPDWORD)FETCHDWORD(parg16->Argument);
    GETVDMPTR( vpdwArg16, sizeof(*argument16), argument16 );

    //
    // Translate the command value as necessary.
    //

    switch ( FETCHDWORD( parg16->Command ) & IOCPARM_MASK ) {

        case 127:
            command = FIONREAD;
            break;

        case 126:
            command = FIONBIO;
            break;

        case 125:
            command = FIOASYNC;
            break;

        case 0:
            command = SIOCSHIWAT;
            break;

        case 1:
            command = SIOCGHIWAT;
            break;

        case 2:
            command = SIOCSLOWAT;
            break;

        case 3:
            command = SIOCGLOWAT;
            break;

        case 7:
            command = SIOCATMARK;
            break;

        default:
            command = 0;
            break;
    }

    argument32 = FETCHDWORD( *argument16 );

    ul = GETWORD16( (*wsockapis[WOW_IOCTLSOCKET].lpfn)(s32, 
                                                       command, 
                                                       &argument32));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR( parg16 );

    GETVDMPTR( vpdwArg16, sizeof(*argument16), argument16 );
    STOREDWORD( *argument16, argument32 );
    FLUSHVDMPTR( vpdwArg16, sizeof(*argument16), argument16 );
    FREEVDMPTR( argument16 );

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32ioctlsocket









ULONG FASTCALL WWS32listen(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLISTEN16 parg16;
    SOCKET s32;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PLISTEN6), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }


    ul = GETWORD16( (*wsockapis[WOW_LISTEN].lpfn)( s32, parg16->Backlog ) );

exit:
    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32listen









ULONG FASTCALL WWS32ntohl(PVDMFRAME pFrame)
{
    ULONG ul;
    register PNTOHL16 parg16;

    GETARGPTR(pFrame, sizeof(NTOHL16), parg16);

    ul = (*wsockapis[WOW_NTOHL].lpfn)( parg16->NetLong );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32ntohl









ULONG FASTCALL WWS32ntohs(PVDMFRAME pFrame)
{
    ULONG ul;
    register PNTOHS16 parg16;

    GETARGPTR(pFrame, sizeof(NTOHS16), parg16);

    ul = GETWORD16( (*wsockapis[WOW_NTOHS].lpfn)( parg16->NetShort ) );

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32ntohs









ULONG FASTCALL WWS32recv(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PRECV16 parg16;
    SOCKET      s32;
    PBYTE       buffer;
    INT         BufferLength;
    VPBYTE      vpBuf16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PRECV16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    BufferLength = INT32(parg16->BufferLength);
    vpBuf16      = (VPBYTE)FETCHDWORD(parg16->Buffer);

    // Thunk the 16-bit recv buffer
    if(!WSThunkRecvBuffer(BufferLength, vpBuf16, &buffer)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_RECV].lpfn)(s32, 
                                                buffer, 
                                                BufferLength, 
                                                parg16->Flags));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit recv buffer
    WSUnthunkRecvBuffer((INT)ul, BufferLength, vpBuf16, buffer);

exit:
    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32recv









ULONG FASTCALL WWS32recvfrom(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PRECVFROM16 parg16;
    SOCKET      s32;
    INT         addressLength;
    PINT        pAddressLength;
    SOCKADDR    fastSockaddr;
    PSOCKADDR   realSockaddr;
    PBYTE       buffer;
    INT         BufferLength;
    VPBYTE      vpBuf16;
    VPWORD      vpwAddrLen16;
    VPSOCKADDR  vpSockAddr16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PRECVFROM16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    vpwAddrLen16 = (VPWORD)FETCHDWORD(parg16->AddressLength);
    vpSockAddr16 = (VPSOCKADDR)FETCHDWORD(parg16->Address);
    BufferLength = INT32(parg16->BufferLength);
    vpBuf16      = (VPBYTE)parg16->Buffer;

    // Thunk the 16-bit Address name and length buffers
    if(!WSThunkAddrBufAndLen(&fastSockaddr, 
                             vpSockAddr16,
                             vpwAddrLen16, 
                             &addressLength,
                             &pAddressLength,
                             &realSockaddr)) {
        goto exit;
    }

    // Thunk the 16-bit recv buffer
    if(!WSThunkRecvBuffer(BufferLength, vpBuf16, &buffer)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_RECVFROM].lpfn)(s32,
                                                    buffer,
                                                    BufferLength,
                                                    parg16->Flags,
                                                    realSockaddr,
                                                    pAddressLength));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit Address name and length buffers
    WSUnThunkAddrBufAndLen(ul,
                           vpwAddrLen16,
                           vpSockAddr16,
                           addressLength,
                           &fastSockaddr,
                           realSockaddr);

    // Un-Thunk the 16-bit recv buffer
    WSUnthunkRecvBuffer((INT)ul, BufferLength, vpBuf16, buffer);


exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32recvfrom









ULONG FASTCALL WWS32select(PVDMFRAME pFrame)
{
    ULONG ul = (ULONG)GETWORD16( SOCKET_ERROR );
    register PSELECT16 parg16;
    PFD_SET readfds32   = NULL;
    PFD_SET writefds32  = NULL;
    PFD_SET exceptfds32 = NULL;
    PFD_SET16 readfds16;
    PFD_SET16 writefds16;
    PFD_SET16 exceptfds16;
    struct timeval timeout32;
    struct timeval *ptimeout32;
    PTIMEVAL16 timeout16;
    INT err;
    VPFD_SET16  vpreadfds16;
    VPFD_SET16  vpwritefds16;
    VPFD_SET16  vpexceptfds16;
    VPTIMEVAL16 vptimeout16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR( pFrame, sizeof(PSELECT16), parg16 );

    //
    // Get 16-bit pointers.
    //
    // !!! This sizeof(FD_SET16) here and below is wrong if the app is
    //     using more than FDSETSIZE handles!!!

    vpreadfds16   = parg16->Readfds;
    vpwritefds16  = parg16->Writefds;
    vpexceptfds16 = parg16->Exceptfds;
    vptimeout16   = parg16->Timeout;
    GETOPTPTR(vpreadfds16, sizeof(FD_SET16), readfds16);
    GETOPTPTR(vpwritefds16, sizeof(FD_SET16), writefds16);
    GETOPTPTR(vpexceptfds16, sizeof(FD_SET16), exceptfds16);
    GETOPTPTR(vptimeout16, sizeof(TIMEVAL16), timeout16);

    //
    // Translate readfds.
    //

    if ( readfds16 != NULL ) {

        readfds32 = AllocateFdSet32( readfds16 );
        if ( readfds32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            goto exit;
        }

        err = ConvertFdSet16To32( readfds16, readfds32 );
        if ( err != 0 ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( err );
            goto exit;
        }

    }

    //
    // Translate writefds.
    //

    if ( writefds16 != NULL ) {

        writefds32 = AllocateFdSet32( writefds16 );
        if ( writefds32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            goto exit;
        }

        err = ConvertFdSet16To32( writefds16, writefds32 );
        if ( err != 0 ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( err );
            goto exit;
        }

    } 

    //
    // Translate exceptfds.
    //

    if ( exceptfds16 != NULL ) {

        exceptfds32 = AllocateFdSet32( exceptfds16 );
        if ( exceptfds32 == NULL ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            goto exit;
        }

        err = ConvertFdSet16To32( exceptfds16, exceptfds32 );
        if ( err != 0 ) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( err );
            goto exit;
        }

    }

    //
    // Translate the timeout.
    //

    if ( timeout16 == NULL ) {
        ptimeout32 = NULL;
    } else {
        timeout32.tv_sec = FETCHDWORD( timeout16->tv_sec );
        timeout32.tv_usec = FETCHDWORD( timeout16->tv_usec );
        ptimeout32 = &timeout32;
    }

    //
    // Call the 32-bit select function.
    //

    ul = GETWORD16( (*wsockapis[WOW_SELECT].lpfn)(0, 
                                                  readfds32, 
                                                  writefds32, 
                                                  exceptfds32, 
                                                  ptimeout32));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);
    FREEOPTPTR(readfds16);
    FREEOPTPTR(writefds16);
    FREEOPTPTR(exceptfds16);
    FREEOPTPTR(timeout16);

    //
    // Copy 32-bit readfds back to the 16-bit readfds.
    //
    if ( readfds32 != NULL ) {
        GETOPTPTR(vpreadfds16, sizeof(FD_SET16), readfds16);
        ConvertFdSet32To16( readfds32, readfds16 );
        FLUSHVDMPTR(vpreadfds16, sizeof(FD_SET16), readfds16);
    }

    //
    // Copy 32-bit writefds back to the 16-bit writefds.
    //

    if ( writefds32 != NULL ) {
        GETOPTPTR(vpwritefds16, sizeof(FD_SET16), writefds16);
        ConvertFdSet32To16( writefds32, writefds16 );
        FLUSHVDMPTR(vpwritefds16, sizeof(FD_SET16), writefds16);
    }

    //
    // Copy 32-bit exceptfds back to the 16-bit exceptfds.
    //

    if ( exceptfds32 != NULL ) {
        GETOPTPTR(vpexceptfds16, sizeof(FD_SET16), exceptfds16);
        ConvertFdSet32To16( exceptfds32, exceptfds16 );
        FLUSHVDMPTR(vpexceptfds16, sizeof(FD_SET16), exceptfds16);
    }

exit:

    FREEOPTPTR( readfds16 );
    FREEOPTPTR( writefds16 );
    FREEOPTPTR( exceptfds16 );

    if ( readfds32 != NULL ) {
        free_w((PVOID)readfds32);
    }
    if ( writefds32 != NULL ) {
        free_w((PVOID)writefds32);
    }
    if ( exceptfds32 != NULL ) {
        free_w((PVOID)exceptfds32);
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32select









ULONG FASTCALL WWS32send(PVDMFRAME pFrame)
{
    ULONG       ul = GETWORD16(INVALID_SOCKET);
    register    PSEND16 parg16;
    SOCKET      s32;
    INT         BufferLength;
    PBYTE       buffer;
    VPBYTE      vpBuf16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PSEND16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    BufferLength = INT32(parg16->BufferLength);
    vpBuf16      = FETCHDWORD(parg16->Buffer);

    // Thunk the 16-bit send buffer
    if(!WSThunkSendBuffer(BufferLength, vpBuf16, &buffer)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_SEND].lpfn)(s32, 
                                                buffer, 
                                                BufferLength, 
                                                parg16->Flags));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit send buffer
    WSUnthunkSendBuffer(buffer);

exit:
    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32send








ULONG FASTCALL WWS32sendto(PVDMFRAME pFrame)
{
    ULONG      ul = GETWORD16(INVALID_SOCKET);
    register   PSENDTO16 parg16;
    SOCKET     s32;
    PBYTE      buffer;
    SOCKADDR   fastSockaddr;
    PSOCKADDR  realSockaddr;
    INT        addressLength;
    INT        BufferLength;
    VPSOCKADDR vpSockAddr16;
    VPBYTE     vpBuf16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PSENDTO16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    addressLength = INT32(parg16->AddressLength);
    vpSockAddr16  = (VPSOCKADDR)FETCHDWORD(parg16->Address);
    BufferLength  = INT32(parg16->BufferLength);
    vpBuf16       = (VPBYTE)FETCHDWORD(parg16->Buffer);

    // Thunk the 16-bit Address buffer
    if(!WSThunkAddrBuf(addressLength,
                       vpSockAddr16,
                       &fastSockaddr,
                       &realSockaddr)) {
        goto exit;
    }

    // Thunk the 16-bit send buffer
    if(!WSThunkSendBuffer(BufferLength, vpBuf16, &buffer)) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_SENDTO].lpfn)(s32,
                                                  buffer,
                                                  BufferLength,
                                                  parg16->Flags,
                                                  realSockaddr,
                                                  addressLength));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    // Un-Thunk the 16-bit address buffer
    WSUnThunkAddrBuf(&fastSockaddr, realSockaddr);

    // Un-Thunk the 16-bit send buffer
    WSUnthunkSendBuffer(buffer);

exit:

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32sendto









ULONG FASTCALL WWS32setsockopt(PVDMFRAME pFrame)
{
    ULONG    ul = GETWORD16(INVALID_SOCKET);
    register PSETSOCKOPT16 parg16;
    SOCKET   s32;
    PBYTE    optionValue16;
    PBYTE    optionValue32;
    DWORD    optionLength32;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PSETSOCKOPT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    GETVDMPTR( parg16->OptionValue, parg16->OptionLength, optionValue16 );

    if ( parg16->OptionLength < sizeof(DWORD) ) {
        optionLength32 = sizeof(DWORD);
    } else {
        optionLength32 = parg16->OptionLength;
    }

    optionValue32 = malloc_w(optionLength32);
    if ( optionValue32 == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );
        FREEVDMPTR( optionValue16 );
        FREEARGPTR( parg16 );
        RETURN( ul );
    }

    RtlZeroMemory( optionValue32, optionLength32 );
    RtlMoveMemory( optionValue32, optionValue16, parg16->OptionLength );

    ul = GETWORD16( (*wsockapis[WOW_SETSOCKOPT].lpfn)(
                     s32,
                     parg16->Level,
                     SocketOption16To32( parg16->OptionName ),
                     optionValue32,
                     optionLength32));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);
    FREEVDMPTR( optionValue16 );

    free_w(optionValue32);

exit:
    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32setsockopt









ULONG FASTCALL WWS32shutdown(PVDMFRAME pFrame)
{
    ULONG    ul = GETWORD16(INVALID_SOCKET);
    register PSHUTDOWN16 parg16;
    SOCKET   s32;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(PBIND16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    if((s32 = WSGetWinsock32(parg16->hSocket, &ul)) == INVALID_SOCKET) {
        goto exit;
    }

    ul = GETWORD16( (*wsockapis[WOW_SHUTDOWN].lpfn)( s32, parg16->How ) );

exit:
    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32shutdown










ULONG FASTCALL WWS32socket(PVDMFRAME pFrame)
{
    ULONG      ul = GETWORD16(INVALID_SOCKET);
    register   PSOCKET16 parg16;
    SOCKET     s32;
    HSOCKET16  s16;

    WSEXIT_IF_NOT_INTIALIZED();

    GETARGPTR(pFrame, sizeof(SOCKET16), parg16);

    s32 = (*wsockapis[WOW_SOCKET].lpfn)(INT32(parg16->AddressFamily),
                                        INT32(parg16->Type),
                                        INT32(parg16->Protocol));

    // Note: 16-bit callbacks resulting from above function 
    //       call may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    //
    // If the call succeeded, alias the 32-bit socket handle we just
    // obtained into a 16-bit handle.
    //

    if ( s32 != INVALID_SOCKET ) {

        s16 = GetWinsock16( s32, 0 );

        ul = s16;

        if ( s16 == 0 ) {
            (*wsockapis[WOW_CLOSESOCKET].lpfn)( s32 );
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
            ul = GETWORD16( INVALID_SOCKET );
        }

    } else {

        ul = GETWORD16( INVALID_SOCKET );
    }

    FREEARGPTR( parg16 );

    RETURN( ul );

} // WWS32socket










//
// Routines for converting between 16- and 32-bit FD_SET structures.
//

PFD_SET AllocateFdSet32(IN PFD_SET16 FdSet16)
{
    int bytes = 4 + (FETCHWORD(FdSet16->fd_count) * sizeof(SOCKET));

    return (PFD_SET)( malloc_w(bytes) );

} // AlloacteFdSet32









INT ConvertFdSet16To32(IN PFD_SET16 FdSet16,
                       IN PFD_SET FdSet32)
{
    int i;

    FdSet32->fd_count = UINT32( FdSet16->fd_count );

    for ( i = 0; i < (int)FdSet32->fd_count; i++ ) {

        FdSet32->fd_array[i] = GetWinsock32( FdSet16->fd_array[i] );
        if ( FdSet32->fd_array[i] == INVALID_SOCKET ) {
            return WSAENOTSOCK;
        }
    }

    return 0;

} // ConvertFdSet16To32








VOID ConvertFdSet32To16(IN PFD_SET FdSet32,
                        IN PFD_SET16 FdSet16)
{
    int i;

    STOREWORD( FdSet16->fd_count, GETWORD16( FdSet32->fd_count ) );

    for ( i = 0; i < FdSet16->fd_count; i++ ) {

        HSOCKET16 s16;

        s16 = GetWinsock16( FdSet32->fd_array[i], 0 );

        STOREWORD( FdSet16->fd_array[i], s16 );
    }

} // ConvertFdSet32To16










//
// Routines for aliasing 32-bit socket handles to 16-bit handles.
//

PWINSOCK_SOCKET_INFO FindSocketInfo16(IN SOCKET h32,
                                      IN HAND16 h16)
{
    PLIST_ENTRY listEntry;
    PWINSOCK_SOCKET_INFO socketInfo;

    //
    // It is the responsibility of the caller of this routine to enter
    // the critical section that protects the global socket list.
    //

    for ( listEntry = WWS32SocketHandleListHead.Flink;
          listEntry != &WWS32SocketHandleListHead;
          listEntry = listEntry->Flink ) {

        socketInfo = CONTAINING_RECORD(listEntry,
                                       WINSOCK_SOCKET_INFO,
                                       GlobalSocketListEntry);

        if ( socketInfo->SocketHandle32 == h32 ||
                 socketInfo->SocketHandle16 == h16 ) {
            return socketInfo;
        }
    }

    return NULL;

} // FindSocketInfo16








HAND16 AllocateUnique16BitHandle(VOID)
{

    PLIST_ENTRY listEntry;
    PWINSOCK_SOCKET_INFO socketInfo;
    HAND16 h16;
    WORD i;

    //
    // This function assumes it is called with the WWS32CriticalSection
    // lock held!
    //

    //
    // If the socket list is empty, then we can reset our socket handle
    // counter because we know there are no active sockets. We'll only
    // do this if the handle counter is above some value (just pulled
    // out of the air) so that handles are not reused too quickly.
    // (Frequent handle reuse can confuse poorly written 16-bit apps.)
    //

    if( ( WWS32SocketHandleCounter > 255 ) &&
        IsListEmpty( &WWS32SocketHandleListHead ) ) {

        WWS32SocketHandleCounter = 1;
        WWS32SocketHandleCounterWrapped = FALSE;

    }

    //
    // If the socket handle counter has not wrapped around,
    // then we can quickly return a unique handle.
    //

    if( !WWS32SocketHandleCounterWrapped ) {

        h16 = (HAND16)WWS32SocketHandleCounter++;

        if( WWS32SocketHandleCounter == 0xFFFF ) {

            WWS32SocketHandleCounter = 1;
            WWS32SocketHandleCounterWrapped = TRUE;

        }

        ASSERT( h16 != 0 );
        return h16;

    }

    //
    // There are active sockets, and the socket handle counter has
    // wrapped, so we'll need to perform a painful search for a unique
    // handle. We'll put a cap on the maximum number of times through
    // this search loop so that, if all handles from 1 to 0xFFFE are
    // in use, we won't search forever for something we'll never find.
    //

    for( i = 1 ; i <= 0xFFFE ; i++ ) {

        h16 = (HAND16)WWS32SocketHandleCounter++;

        if( WWS32SocketHandleCounter == 0xFFFF ) {

            WWS32SocketHandleCounter = 1;

        }

        for ( listEntry = WWS32SocketHandleListHead.Flink;
              listEntry != &WWS32SocketHandleListHead;
              listEntry = listEntry->Flink ) {

            socketInfo = CONTAINING_RECORD(
                             listEntry,
                             WINSOCK_SOCKET_INFO,
                             GlobalSocketListEntry
                             );

            if( socketInfo->SocketHandle16 == h16 ) {

                break;

            }

        }

        //
        // If listEntry == &WWS32SocketHandleListHead, then we have
        // scanned the entire list and found no match. This is good,
        // and we'll just return the current handle. Otherwise, there
        // was a collision, so we'll get another potential handle and
        // rescan the list.
        //

        if( listEntry == &WWS32SocketHandleListHead ) {

            ASSERT( h16 != 0 );
            return h16;

        }

    }

    //
    // If we made it this far, then there were no unique handles
    // available. Bad news.
    //

    return 0;

} // AllocateUnique16BitHandle









HAND16 GetWinsock16(IN INT h32,
                    IN INT iClass)
{
    PWINSOCK_SOCKET_INFO socketInfo;
    HAND16 h16;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    //
    // If the handle is already in the list, use it.
    //

    socketInfo = FindSocketInfo16( h32, 0 );

    if ( socketInfo != NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return socketInfo->SocketHandle16;
    }

    //
    // If this thread has not yet been initialized, then we cannot
    // create the new socket data. This should only happen if a 16-bit
    // app closes a socket while an async connect is outstanding.
    //

    if( !WWS32IsThreadInitialized ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return 0;
    }

    //
    // The handle is not in use.  Create a new entry in the list.
    //

    h16 = AllocateUnique16BitHandle();
    if( h16 == 0 ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return 0;
    }

    socketInfo = malloc_w(sizeof(*socketInfo));
    if ( socketInfo == NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return 0;
    }

    socketInfo->SocketHandle16 = h16;
    socketInfo->SocketHandle32 = h32;
    socketInfo->ThreadSerialNumber = WWS32ThreadSerialNumber;

    InsertTailList( &WWS32SocketHandleListHead, &socketInfo->GlobalSocketListEntry );

    RtlLeaveCriticalSection( &WWS32CriticalSection );

    ASSERT( h16 != 0 );
    return h16;

} // GetWinsock16








VOID FreeWinsock16(IN HAND16 h16)
{
    PWINSOCK_SOCKET_INFO socketInfo;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    socketInfo = FindSocketInfo16( INVALID_SOCKET, h16 );

    if ( socketInfo == NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return;
    }

    RemoveEntryList( &socketInfo->GlobalSocketListEntry );
    free_w((PVOID)socketInfo);
    RtlLeaveCriticalSection( &WWS32CriticalSection );

    return;

} // FreeWinsock16








DWORD GetWinsock32(IN HAND16 h16)
{
    PWINSOCK_SOCKET_INFO socketInfo;
    SOCKET socket32;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    socketInfo = FindSocketInfo16( INVALID_SOCKET, h16 );

    if ( socketInfo == NULL ) {
        RtlLeaveCriticalSection( &WWS32CriticalSection );
        return INVALID_SOCKET;
    }

    //
    // Store the socket handle in an aytumatic before leaving the critical
    // section in case the socketInfo structure is about to be freed.
    //

    socket32 = socketInfo->SocketHandle32;

    RtlLeaveCriticalSection( &WWS32CriticalSection );

    return socket32;

} // GetWinsock32








int SocketOption16To32(IN WORD SocketOption16)
{

    if ( SocketOption16 == 0xFF7F ) {
        return SO_DONTLINGER;
    }

    return (int)SocketOption16;

} // SocketOption16To32









DWORD WSGetWinsock32 (IN  HAND16 h16,
                      OUT PULONG pul)
{

    DWORD  s32;


    s32 = GetWinsock32(h16);

    if(s32 == INVALID_SOCKET) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)(WSAENOTSOCK);
        *pul = (ULONG)GETWORD16(SOCKET_ERROR);

    }

    return(s32);

}








BOOL WSThunkAddrBufAndLen(IN  PSOCKADDR  fastSockaddr, 
                          IN  VPSOCKADDR vpSockAddr16,
                          IN  VPWORD     vpwAddrLen16,
                          OUT PINT       addressLength,
                          OUT PINT      *pAddressLength,
                          OUT PSOCKADDR *realSockaddr)
{
    PWORD      addressLength16;
    PSOCKADDR  Sockaddr;
    
    GETVDMPTR(vpwAddrLen16, sizeof(*addressLength16), addressLength16);
    GETVDMPTR(vpSockAddr16, *addressLength16, Sockaddr);

    if(Sockaddr) {
        *realSockaddr = fastSockaddr;
    }
    else {
        *realSockaddr = NULL;
    }

    if (addressLength16 == NULL) {

        *pAddressLength = NULL;

    } else {

        *addressLength  = INT32(*addressLength16);
        *pAddressLength = addressLength;

        if(*addressLength > sizeof(SOCKADDR)) {

            *realSockaddr = malloc_w(*addressLength);

            if(*realSockaddr == NULL) {

                (*wsockapis[WOW_WSASETLASTERROR].lpfn)(WSAENOBUFS);
                return(FALSE);

            }
        }
    }

    FREEVDMPTR(Sockaddr);
    FREEVDMPTR(addressLength16);
    return(TRUE);
}









VOID WSUnThunkAddrBufAndLen(IN ULONG      ret,
                            IN VPWORD     vpwAddrLen16,
                            IN VPSOCKADDR vpSockAddr16,
                            IN INT        addressLength,
                            IN PSOCKADDR  fastSockaddr,
                            IN PSOCKADDR  realSockaddr)
{
    PWORD      addressLength16;
    PSOCKADDR  Sockaddr;

    GETVDMPTR(vpwAddrLen16, sizeof(*addressLength16), addressLength16);
    if((ret != SOCKET_ERROR) && addressLength16) {
        STOREWORD(*addressLength16, addressLength);
        FLUSHVDMPTR(vpwAddrLen16, sizeof(WORD), addressLength16);

        GETVDMPTR(vpSockAddr16, addressLength, Sockaddr);
        if(Sockaddr) {

            // don't copy back to the 16-bit address buffer if it's too small
            if(addressLength <= *addressLength16) {
                SockCopyMemory(Sockaddr, realSockaddr, addressLength);
                FLUSHVDMPTR(vpSockAddr16, addressLength, Sockaddr);
            }
        }
    }

    if( (realSockaddr) && (realSockaddr != fastSockaddr) ) {

        free_w(realSockaddr);

    }

    FREEVDMPTR(addressLength16);
    FREEVDMPTR(Sockaddr);
}








BOOL WSThunkAddrBuf(IN  INT         addressLength,
                    IN  VPSOCKADDR  vpSockAddr16,
                    IN  PSOCKADDR   fastSockaddr, 
                    OUT PSOCKADDR  *realSockaddr)
{
    PSOCKADDR  Sockaddr;

    GETVDMPTR(vpSockAddr16, addressLength, Sockaddr);

    if(Sockaddr) {


        if(addressLength <= sizeof(SOCKADDR)) {
            *realSockaddr = fastSockaddr;
        }
        else {

            *realSockaddr = malloc_w(addressLength);

            if(*realSockaddr == NULL) {

                (*wsockapis[WOW_WSASETLASTERROR].lpfn)(WSAENOBUFS);
                FREEVDMPTR(Sockaddr);
                return(FALSE);

            }
        }

        SockCopyMemory(*realSockaddr, Sockaddr, addressLength);
    }
    else {
        *realSockaddr = NULL;
    }

    FREEVDMPTR(Sockaddr);
    return(TRUE);

}








VOID WSUnThunkAddrBuf(IN PSOCKADDR  fastSockaddr, 
                      IN PSOCKADDR  realSockaddr)
{
        if( (realSockaddr) && (realSockaddr != fastSockaddr) ) {

            free_w(realSockaddr);

        }
}












BOOL WSThunkRecvBuffer(IN  INT    BufferLength,
                       IN  VPBYTE vpBuf16,
                       OUT PBYTE  *buffer)
{
    PBYTE  lpBuf16;

    GETVDMPTR(vpBuf16, BufferLength, lpBuf16);

    if(lpBuf16) {

        *buffer = malloc_w(BufferLength);

        if(*buffer == NULL) {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)(WSAENOBUFS);
            return(FALSE);
        }
    }
    else {
        *buffer = NULL;
    }

    return(TRUE);
        
}








VOID WSUnthunkRecvBuffer(IN INT    cBytes,
                         IN INT    BufferLength, 
                         IN VPBYTE vpBuf16,
                         IN PBYTE  buffer)
{
    PBYTE lpBuf16;

    GETVDMPTR(vpBuf16, BufferLength, lpBuf16);

    if(buffer) { 

        if( (cBytes > 0) && lpBuf16 ) {
            SockCopyMemory(lpBuf16, buffer, cBytes);
            FLUSHVDMPTR(vpBuf16, cBytes, lpBuf16);
        }

        free_w(buffer);

    }

    FREEVDMPTR(lpBuf16);

}






BOOL WSThunkSendBuffer(IN  INT    BufferLength, 
                       IN  VPBYTE vpBuf16,
                       OUT PBYTE  *buffer)
{
    PBYTE  lpBuf16;

    GETVDMPTR(vpBuf16, BufferLength, lpBuf16);

    if(lpBuf16) {

        *buffer = malloc_w(BufferLength);

        if(*buffer) {
            SockCopyMemory(*buffer, lpBuf16, BufferLength);
        }
        else {
            (*wsockapis[WOW_WSASETLASTERROR].lpfn)(WSAENOBUFS);
            return(FALSE);
        }

        FREEVDMPTR(lpBuf16);
    }
    else {
        *buffer = NULL;
    }

    return(TRUE);
        
}






VOID WSUnthunkSendBuffer(IN PBYTE buffer)
{

    if(buffer) {
        free_w(buffer);
    }
}
