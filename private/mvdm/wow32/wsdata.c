/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Wsraw.h

Abstract:

    Support for database winsock calls for WOW.

Author:

    David Treadwell (davidtr)    02-Oct-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "wsdynmc.h"

#define FIND_16_OFFSET_FROM_32(base16, base32, actual32) \
            ( (DWORD)base16 + ( (DWORD)actual32 - (DWORD)base32 ) )

DWORD
BytesInHostent32 (
    PHOSTENT Hostent32
    );

DWORD
CopyHostent32To16 (
    PHOSTENT16 Hostent16,
    VPHOSTENT16 VHostent16,
    int BufferLength,
    PHOSTENT Hostent32
    );

DWORD
BytesInProtoent32 (
    PPROTOENT Protoent32
    );

DWORD
CopyProtoent32To16 (
    PPROTOENT16 Protoent16,
    VPPROTOENT16 VProtoent16,
    int BufferLength,
    PPROTOENT Protoent32
    );

DWORD
BytesInServent32 (
    PSERVENT Servent32
    );

DWORD
CopyServent32To16 (
    PSERVENT16 Servent16,
    VPSERVENT16 VServent16,
    int BufferLength,
    PSERVENT Servent32
    );

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

    FREEARGPTR(parg16);
    RETURN(ul);
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

ULONG FASTCALL WWS32gethostbyaddr(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETHOSTBYADDR16 parg16;
    PDWORD paddr16;
    PHOSTENT hostent32;
    PHOSTENT16 hostent16;
    DWORD bytesRequired;
    DWORD addr32;    // address must be in PF_INET format (length == 4 bytes)

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETHOSTBYADDR16), parg16 );
    GETVDMPTR( parg16->Address, sizeof(DWORD), paddr16 );

    addr32 = *paddr16;  // copy the 4-byte address

    hostent32 = (PHOSTENT) (*wsockapis[WOW_GETHOSTBYADDR].lpfn)((CHAR *)&addr32,
                                                                parg16->Length, 
                                                                parg16->Type);
    // Note: 16-bit callbacks resulting from above function
    //       call may have caused 16-bit memory movement
    FREEVDMPTR(paddr16);
    FREEARGPTR(parg16);

    if ( hostent32 != NULL ) {

        GETVDMPTR( WWS32vHostent, MAXGETHOSTSTRUCT, hostent16 );
        bytesRequired = CopyHostent32To16(
                            hostent16,
                            WWS32vHostent,
                            MAXGETHOSTSTRUCT,
                            hostent32
                            );
        ASSERT( bytesRequired < MAXGETHOSTSTRUCT );

        FLUSHVDMPTR( WWS32vHostent, (USHORT) bytesRequired, hostent16 );
        FREEVDMPTR( hostent16 );
        ul = WWS32vHostent;

    } else {

        ul = 0;
    }

    FREEVDMPTR( paddr16 );
    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32gethostbyaddr

ULONG FASTCALL WWS32gethostbyname(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETHOSTBYNAME16 parg16;
    PHOSTENT hostent32;
    PHOSTENT16 hostent16;
    PSZ name32 = NULL;
    PSZ name16;
    DWORD bytesRequired;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETHOSTBYNAME16), parg16 );

    GETVDMPTR( parg16->Name, 32, name16 );
    
    if(name16) {
        name32 = malloc_w(strlen(name16)+1);
        strcpy(name32, name16);
    }

    hostent32 = (PHOSTENT) (*wsockapis[WOW_GETHOSTBYNAME].lpfn)( name32 );

    // Note: 16-bit callbacks resulting from above function
    //       call may have caused 16-bit memory movement
    FREEVDMPTR(name16);
    FREEARGPTR(parg16);

    if(name32) {
        free_w(name32);
    }

    if ( hostent32 != NULL ) {

        GETVDMPTR( WWS32vHostent, MAXGETHOSTSTRUCT, hostent16 );
        bytesRequired = CopyHostent32To16(
                            hostent16,
                            WWS32vHostent,
                            MAXGETHOSTSTRUCT,
                            hostent32
                            );
        ASSERT( bytesRequired < MAXGETHOSTSTRUCT );

        FLUSHVDMPTR( WWS32vHostent, (USHORT) bytesRequired, hostent16 );
        FREEVDMPTR( hostent16 );
        ul = WWS32vHostent;

    } else {

        ul = 0;
    }

    FREEVDMPTR( name32 );
    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32gethostbyname

ULONG FASTCALL WWS32gethostname(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETHOSTNAME16 parg16;
    PCHAR name32 = NULL;
    PCHAR name16;
    INT   NameLength;
    VPSZ  vpszName;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETHOSTNAME16), parg16 );

    vpszName = FETCHDWORD(parg16->Name);
    NameLength = INT32(parg16->NameLength);

    if(vpszName) {
        name32 = malloc_w(NameLength);
    }

    ul = GETWORD16( (*wsockapis[WOW_GETHOSTNAME].lpfn)( name32, NameLength ) );

    // Note: 16-bit callbacks resulting from above function
    //       call may have caused 16-bit memory movement
    FREEVDMPTR(name16);
    FREEARGPTR(parg16);

    GETVDMPTR( vpszName, NameLength, name16 );
    if(name16 && name32) {
        strcpy(name16, name32);
    }
    FLUSHVDMPTR( vpszName, NameLength, name16 );

    FREEVDMPTR( name16 );
    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32gethostname

ULONG FASTCALL WWS32WSAAsyncGetHostByAddr(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCGETHOSTBYADDR16 parg16;
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PVOID buffer32;
    PDWORD paddr16;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN(0);
    }

    GETARGPTR( pFrame, sizeof(WSAASYNCGETHOSTBYADDR16), parg16 );
    GETVDMPTR( parg16->Address, sizeof(DWORD), paddr16 );

    //
    // Set up locals so we know how to clean up on exit.
    //

    context = NULL;
    buffer32 = NULL;
    ul = 0;

    //
    // Allocate a context block and 32-bit buffer to use for the request.
    //

    context = malloc_w( sizeof(*context) );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    buffer32 = malloc_w( MAXGETHOSTSTRUCT );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    //
    // Fill in entries in the context buffer.
    //

    context->Buffer32 = buffer32;
    context->vBuffer16 = parg16->Buffer;
    context->Buffer16Length = parg16->BufferLength;

    //
    // Enter a critical section to synchronize access to the context block
    // and their global list.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    context->AsyncTaskHandle32 = (HANDLE) (*wsockapis[WOW_WSAASYNCGETHOSTBYADDR].lpfn)(
                                     (HWND)HWND32(parg16->hWnd),
                                     (parg16->wMsg << 16) |
                                     WWS32_MESSAGE_ASYNC_GETHOST,
                                     (char *)paddr16,
                                     parg16->Length,
                                     parg16->Type,
                                     buffer32,
                                     MAXGETHOSTSTRUCT);

    if ( context->AsyncTaskHandle32 != 0 ) {

        //
        // The call succeeded so get a 16-bit task handle for this
        // request and place the context block on the global list.  The
        // resources will be freed by WWS32PostAsyncGetHost.
        //

        ul = WWS32GetAsyncTaskHandle16( );
        context->AsyncTaskHandle16 = (HAND16)ul;

        InsertTailList(
            &WWS32AsyncContextBlockListHead,
            &context->ContextBlockListEntry
            );
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

exit:

    if ( ul == 0 ) {

        if ( context != NULL ) {
            free_w( (PVOID)context );
        }

        if ( buffer32 != NULL ) {
            free_w( buffer32 );
        }
    }

    FREEVDMPTR( paddr16 );
    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAAsyncGetHostByAddr

ULONG FASTCALL WWS32WSAAsyncGetHostByName(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCGETHOSTBYNAME16 parg16;
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PVOID buffer32;
    PCHAR name32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN(0);
    }

    GETARGPTR( pFrame, sizeof(WSAASYNCGETHOSTBYNAME16), parg16 );
    GETVDMPTR( parg16->Name, 32, name32 );

    //
    // Set up locals so we know how to clean up on exit.
    //

    context = NULL;
    buffer32 = NULL;
    ul = 0;

    //
    // Allocate a context block and 32-bit buffer to use for the request.
    //

    context = malloc_w( sizeof(*context) );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    buffer32 = malloc_w( MAXGETHOSTSTRUCT );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    //
    // Fill in entries in the context buffer.
    //

    context->Buffer32 = buffer32;
    context->vBuffer16 = parg16->Buffer;
    context->Buffer16Length = parg16->BufferLength;

    //
    // Enter a critical section to synchronize access to the context block
    // and their global list.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    context->AsyncTaskHandle32 = (HANDLE) (*wsockapis[WOW_WSAASYNCGETHOSTBYNAME].lpfn)(
                                     (HWND)HWND32(parg16->hWnd),
                                     (parg16->wMsg << 16) |
                                         WWS32_MESSAGE_ASYNC_GETHOST,
                                     name32,
                                     buffer32,
                                     MAXGETHOSTSTRUCT
                                     );

    if ( context->AsyncTaskHandle32 != 0 ) {

        //
        // The call succeeded so get a 16-bit task handle for this
        // request and place the context block on the global list.  The
        // resources will be freed by WWS32PostAsyncGetHost.
        //

        ul = WWS32GetAsyncTaskHandle16( );
        context->AsyncTaskHandle16 = (HAND16)ul;

        InsertTailList(
            &WWS32AsyncContextBlockListHead,
            &context->ContextBlockListEntry
            );
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

exit:

    if ( ul == 0 ) {

        if ( context != NULL ) {
            free_w( (PVOID)context );
        }

        if ( buffer32 != NULL ) {
            free_w( buffer32 );
        }
    }

    FREEVDMPTR( name32 );
    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAAsyncGetHostByName


BOOL
WWS32PostAsyncGetHost (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    BOOL ret;
    PVOID buffer16;
    DWORD bytesRequired;

    context = WWS32FindAndRemoveAsyncContext( (HANDLE)wParam );
    ASSERT( context != NULL );

    //
    // If the call was successful, copy the 32-bit buffer to the
    // 16-bit buffer specified by the application.
    //

    if ( WSAGETASYNCERROR( lParam ) == 0 ) {

        //
        // Copy the 32-bit structure to 16-bit buffer.
        //

        GETVDMPTR( context->vBuffer16, context->Buffer16Length, buffer16 );

        bytesRequired = CopyHostent32To16(
                            buffer16,
                            context->vBuffer16,
                            context->Buffer16Length,
                            context->Buffer32
                            );

        //
        // If the application's buffer was too small, return an error
        // and information aqbout the buffer size required.
        //

        if ( bytesRequired > context->Buffer16Length ) {
            lParam = WSAMAKEASYNCREPLY( (WORD)bytesRequired, WSAENOBUFS );
        }
    }

    //
    // Post the completion message to the 16-bit application.
    //

    ret = PostMessage(
              hWnd,
              Msg >> 16,
              context->AsyncTaskHandle16,
              lParam
              );

    //
    // Free resources and return.
    //

    free_w( context->Buffer32 );
    free_w( (PVOID)context );

    return ret;

} // WWS32PostAsyncGetHost


DWORD
CopyHostent32To16 (
    PHOSTENT16 Hostent16,
    VPHOSTENT16 VHostent16,
    int BufferLength,
    PHOSTENT Hostent32
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = (PCHAR)Hostent16;
    DWORD aliasCount;
    DWORD addressCount;
    DWORD i;
    VPBYTE *addrList16;
    VPSZ *aliases16;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInHostent32( Hostent32 );

    //
    // Copy over the hostent structure if it fits.
    //

    bytesFilled = sizeof(*Hostent32);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    STOREWORD( Hostent16->h_addrtype, Hostent32->h_addrtype );
    STOREWORD( Hostent16->h_length, Hostent32->h_length );
    currentLocation = (PCHAR)Hostent16 + bytesFilled;

    //
    // Count the host's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Hostent32->h_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent32->h_aliases = NULL;
        return requiredBufferLength;
    }

    Hostent16->h_aliases =
        FIND_16_OFFSET_FROM_32( VHostent16, Hostent16, currentLocation );
    aliases16 = (VPSZ *)currentLocation;
    currentLocation = (PCHAR)Hostent16 + bytesFilled;

    //
    // Count the host's addresses and set up an array to hold pointers to
    // them.
    //

    for ( addressCount = 0;
          Hostent32->h_addr_list[addressCount] != NULL;
          addressCount++ );

    bytesFilled += (addressCount+1) * sizeof(void FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent32->h_addr_list = NULL;
        return requiredBufferLength;
    }

    Hostent16->h_addr_list =
        FIND_16_OFFSET_FROM_32( VHostent16, Hostent16, currentLocation );
    addrList16 = (VPBYTE *)currentLocation;
    currentLocation = (PCHAR)Hostent16 + bytesFilled;

    //
    // Start filling in addresses.  Do addresses before filling in the
    // host name and aliases in order to avoid alignment problems.
    //

    for ( i = 0; i < addressCount; i++ ) {

        bytesFilled += Hostent32->h_length;

        if ( bytesFilled > (DWORD)BufferLength ) {
            STOREDWORD( addrList16[i], 0 );
            return requiredBufferLength;
        }

        STOREDWORD(
            addrList16[i],
            FIND_16_OFFSET_FROM_32( VHostent16, Hostent16, currentLocation )
            );

        RtlMoveMemory(
            currentLocation,
            Hostent32->h_addr_list[i],
            Hostent32->h_length
            );

        currentLocation = (PCHAR)Hostent16 + bytesFilled;
    }

    STOREDWORD( addrList16[i], 0 );

    //
    // Copy the host name if it fits.
    //

    bytesFilled += strlen( Hostent32->h_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    Hostent16->h_name =
        FIND_16_OFFSET_FROM_32( VHostent16, Hostent16, currentLocation );

    RtlMoveMemory( currentLocation, Hostent32->h_name, strlen( Hostent32->h_name ) + 1 );
    currentLocation = (PCHAR)Hostent16 + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Hostent32->h_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            STOREDWORD( aliases16[i], 0 );
            return requiredBufferLength;
        }

        STOREDWORD(
            aliases16[i],
            FIND_16_OFFSET_FROM_32( VHostent16, Hostent16, currentLocation )
            );

        RtlMoveMemory(
            currentLocation,
            Hostent32->h_aliases[i],
            strlen( Hostent32->h_aliases[i] ) + 1
            );

        currentLocation = (PCHAR)Hostent16 + bytesFilled;
    }

    STOREDWORD( aliases16[i], 0 );

    return requiredBufferLength;

} // CopyHostentToBuffer


DWORD
BytesInHostent32 (
    PHOSTENT Hostent32
    )
{
    DWORD total;
    int i;

    total = sizeof(HOSTENT);
    total += strlen( Hostent32->h_name ) + 1;
    total += sizeof(char *) + sizeof(char *);

    for ( i = 0; Hostent32->h_aliases[i] != NULL; i++ ) {
        total += strlen( Hostent32->h_aliases[i] ) + 1 + sizeof(char *);
    }

    for ( i = 0; Hostent32->h_addr_list[i] != NULL; i++ ) {
        total += Hostent32->h_length + sizeof(char *);
    }

    return total;

} // BytesInHostent

ULONG FASTCALL WWS32getprotobyname(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETPROTOBYNAME16 parg16;
    PPROTOENT protoent32;
    PPROTOENT16 protoent16;
    PSZ name32 = NULL;
    PBYTE name16;
    DWORD bytesRequired;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETPROTOBYNAME16), parg16 );
    GETVDMPTR( parg16->Name, 32, name16 );

    if(name16) {
        name32 = malloc_w(strlen(name16)+1);
        strcpy(name32, name16);
    }

    protoent32 = (PPROTOENT) (*wsockapis[WOW_GETPROTOBYNAME].lpfn)( name32 );

    // Note: 16-bit callbacks resulting from above function
    //       call may have caused 16-bit memory movement
    FREEVDMPTR(name16);
    FREEARGPTR(parg16);

    if(name32) {
        free_w( name32 );
    }

    if ( protoent32 != NULL ) {

        GETVDMPTR( WWS32vProtoent, MAXGETHOSTSTRUCT, protoent16 );
        bytesRequired = CopyProtoent32To16(
                            protoent16,
                            WWS32vProtoent,
                            MAXGETHOSTSTRUCT,
                            protoent32
                            );
        ASSERT( bytesRequired < MAXGETHOSTSTRUCT );

        FLUSHVDMPTR( WWS32vProtoent, (USHORT) bytesRequired, protoent16 );
        FREEVDMPTR( protoent16 );
        ul = WWS32vProtoent;

    } else {

        ul = 0;
    }

    FREEVDMPTR(name16);
    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32getprotobyname

ULONG FASTCALL WWS32getprotobynumber(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETPROTOBYNUMBER16 parg16;
    PPROTOENT protoent32;
    PPROTOENT16 protoent16;
    DWORD bytesRequired;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETPROTOBYNUMBER16), parg16 );

    protoent32 = (PPROTOENT) (*wsockapis[WOW_GETPROTOBYNUMBER].lpfn)( parg16->Protocol );

    if ( protoent32 != NULL ) {

        GETVDMPTR( WWS32vProtoent, MAXGETHOSTSTRUCT, protoent16 );
        bytesRequired = CopyProtoent32To16(
                            protoent16,
                            WWS32vProtoent,
                            MAXGETHOSTSTRUCT,
                            protoent32
                            );
        ASSERT( bytesRequired < MAXGETHOSTSTRUCT );

        FLUSHVDMPTR( WWS32vProtoent, (USHORT) bytesRequired, protoent16 );
        FREEVDMPTR( protoent16 );
        ul = WWS32vProtoent;

    } else {

        ul = 0;
    }

    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32getprotobynumber

ULONG FASTCALL WWS32WSAAsyncGetProtoByName(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCGETPROTOBYNAME16 parg16;
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PVOID buffer32;
    PSZ name32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN(0);
    }

    GETARGPTR( pFrame, sizeof(WSAASYNCGETPROTOBYNAME16), parg16 );
    GETVDMPTR( parg16->Name, 32, name32 );

    //
    // Set up locals so we know how to clean up on exit.
    //

    context = NULL;
    buffer32 = NULL;
    ul = 0;

    //
    // Allocate a context block and 32-bit buffer to use for the request.
    //

    context = malloc_w( sizeof(*context) );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    buffer32 = malloc_w( MAXGETHOSTSTRUCT );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    //
    // Fill in entries in the context buffer.
    //

    context->Buffer32 = buffer32;
    context->vBuffer16 = parg16->Buffer;
    context->Buffer16Length = parg16->BufferLength;

    //
    // Enter a critical section to synchronize access to the context block
    // and their global list.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    context->AsyncTaskHandle32 = (HANDLE) (*wsockapis[WOW_WSAASYNCGETPROTOBYNAME].lpfn)(
                                     (HWND)HWND32(parg16->hWnd),
                                     (parg16->wMsg << 16) |
                                         WWS32_MESSAGE_ASYNC_GETPROTO,
                                     name32,
                                     buffer32,
                                     MAXGETHOSTSTRUCT
                                     );

    if ( context->AsyncTaskHandle32 != 0 ) {

        //
        // The call succeeded so get a 16-bit task handle for this
        // request and place the context block on the global list.  The
        // resources will be freed by WWS32PostAsyncGetProto.
        //

        ul = WWS32GetAsyncTaskHandle16( );
        context->AsyncTaskHandle16 = (HAND16)ul;

        InsertTailList(
            &WWS32AsyncContextBlockListHead,
            &context->ContextBlockListEntry
            );
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

exit:

    if ( ul == 0 ) {

        if ( context != NULL ) {
            free_w( (PVOID)context );
        }

        if ( buffer32 != NULL ) {
            free_w( buffer32 );
        }
    }

    FREEARGPTR( name32 );
    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAAsyncGetProtoByName

ULONG FASTCALL WWS32WSAAsyncGetProtoByNumber(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCGETPROTOBYNUMBER16 parg16;
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PVOID buffer32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN(0);
    }

    GETARGPTR( pFrame, sizeof(WSAASYNCGETPROTOBYNUMBER16), parg16 );

    //
    // Set up locals so we know how to clean up on exit.
    //

    context = NULL;
    buffer32 = NULL;
    ul = 0;

    //
    // Allocate a context block and 32-bit buffer to use for the request.
    //

    context = malloc_w( sizeof(*context) );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    buffer32 = malloc_w( MAXGETHOSTSTRUCT );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    //
    // Fill in entries in the context buffer.
    //

    context->Buffer32 = buffer32;
    context->vBuffer16 = parg16->Buffer;
    context->Buffer16Length = parg16->BufferLength;

    //
    // Enter a critical section to synchronize access to the context block
    // and their global list.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    context->AsyncTaskHandle32 = (HANDLE) (*wsockapis[WOW_WSAASYNCGETPROTOBYNUMBER].lpfn)(
                                     (HWND)HWND32(parg16->hWnd),
                                     (parg16->wMsg << 16) |
                                         WWS32_MESSAGE_ASYNC_GETPROTO,
                                     parg16->Number,
                                     buffer32,
                                     MAXGETHOSTSTRUCT
                                     );

    if ( context->AsyncTaskHandle32 != 0 ) {

        //
        // The call succeeded so get a 16-bit task handle for this
        // request and place the context block on the global list.  The
        // resources will be freed by WWS32PostAsyncGetProto.
        //

        ul = WWS32GetAsyncTaskHandle16( );
        context->AsyncTaskHandle16 = (HAND16)ul;

        InsertTailList(
            &WWS32AsyncContextBlockListHead,
            &context->ContextBlockListEntry
            );
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

exit:

    if ( ul == 0 ) {

        if ( context != NULL ) {
            free_w( (PVOID)context );
        }

        if ( buffer32 != NULL ) {
            free_w( buffer32 );
        }
    }

    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAAsyncGetProtoByNumber


BOOL
WWS32PostAsyncGetProto (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    BOOL ret;
    PVOID buffer16;
    DWORD bytesRequired;

    context = WWS32FindAndRemoveAsyncContext( (HANDLE)wParam );
    ASSERT( context != NULL );

    //
    // If the call was successful, copy the 32-bit buffer to the
    // 16-bit buffer specified by the application.
    //

    if ( WSAGETASYNCERROR( lParam ) == 0 ) {

        //
        // Copy the 32-bit structure to 16-bit buffer.
        //

        GETVDMPTR( context->vBuffer16, context->Buffer16Length, buffer16 );

        bytesRequired = CopyProtoent32To16(
                            buffer16,
                            context->vBuffer16,
                            context->Buffer16Length,
                            context->Buffer32
                            );

        //
        // If the application's buffer was too small, return an error
        // and information aqbout the buffer size required.
        //

        if ( bytesRequired > context->Buffer16Length ) {
            lParam = WSAMAKEASYNCREPLY( (WORD)bytesRequired, WSAENOBUFS );
        }
    }

    //
    // Post the completion message to the 16-bit application.
    //

    ret = PostMessage(
              hWnd,
              Msg >> 16,
              context->AsyncTaskHandle16,
              lParam
              );

    //
    // Free resources and return.
    //

    free_w( context->Buffer32 );
    free_w( (PVOID)context );

    return ret;

} // WWS32PostAsyncGetProto


DWORD
CopyProtoent32To16 (
    PPROTOENT16 Protoent16,
    VPPROTOENT16 VProtoent16,
    int BufferLength,
    PPROTOENT Protoent32
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = (PCHAR)Protoent16;
    DWORD aliasCount;
    DWORD i;
    VPBYTE *aliases16;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInProtoent32( Protoent32 );

    //
    // Copy over the protoent structure if it fits.
    //

    bytesFilled = sizeof(*Protoent16);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    STOREWORD( Protoent16->p_proto, Protoent32->p_proto );
    currentLocation = (PCHAR)Protoent16 + bytesFilled;

    //
    // Count the protocol's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Protoent32->p_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Protoent16->p_aliases = 0;
        return requiredBufferLength;
    }

    Protoent16->p_aliases =
        FIND_16_OFFSET_FROM_32( VProtoent16, Protoent16, currentLocation );
    aliases16 = (VPBYTE *)currentLocation;
    currentLocation = (PCHAR)Protoent16 + bytesFilled;

    //
    // Copy the protocol name if it fits.
    //

    bytesFilled += strlen( Protoent32->p_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    Protoent16->p_name =
        FIND_16_OFFSET_FROM_32( VProtoent16, Protoent16, currentLocation );

    RtlMoveMemory( currentLocation, Protoent32->p_name, strlen( Protoent32->p_name ) + 1 );
    currentLocation = (PCHAR)Protoent16 + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Protoent32->p_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            STOREDWORD( aliases16[i], 0 );
            return requiredBufferLength;
        }

        STOREDWORD(
            aliases16[i],
            FIND_16_OFFSET_FROM_32( VProtoent16, Protoent16, currentLocation )
            );

        RtlMoveMemory(
            currentLocation,
            Protoent32->p_aliases[i],
            strlen( Protoent32->p_aliases[i] ) + 1
            );

        currentLocation = (PCHAR)Protoent16 + bytesFilled;
    }

    STOREDWORD( aliases16[i], 0 );

    return requiredBufferLength;

} // CopyProtoent32To16


DWORD
BytesInProtoent32 (
    PPROTOENT Protoent32
    )
{
    DWORD total;
    int i;

    total = sizeof(PROTOENT);
    total += strlen( Protoent32->p_name ) + 1;
    total += sizeof(char *);

    for ( i = 0; Protoent32->p_aliases[i] != NULL; i++ ) {
        total += strlen( Protoent32->p_aliases[i] ) + 1 + sizeof(char *);
    }

    return total;

} // BytesInProtoent32

ULONG FASTCALL WWS32getservbyname(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSERVBYNAME16 parg16;
    PSERVENT servent32;
    PSERVENT16 servent16;
    PSZ name32;
    PSZ protocol32;
    DWORD bytesRequired;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETSERVBYNAME16), parg16 );

    GETVDMPTR( parg16->Name, 32, name32 );
    GETVDMPTR( parg16->Protocol, 32, protocol32 );

    servent32 = (PSERVENT) (*wsockapis[WOW_GETSERVBYNAME].lpfn)( name32, protocol32 );

    if ( servent32 != NULL ) {

        GETVDMPTR( WWS32vServent, MAXGETHOSTSTRUCT, servent16 );
        bytesRequired = CopyServent32To16(
                            servent16,
                            WWS32vServent,
                            MAXGETHOSTSTRUCT,
                            servent32
                            );
        ASSERT( bytesRequired < MAXGETHOSTSTRUCT );

        FLUSHVDMPTR( WWS32vServent, (USHORT) bytesRequired, servent16 );
        FREEVDMPTR( servent16 );
        ul = WWS32vServent;

    } else {

        ul = 0;
    }

    FREEVDMPTR( name32 );
    FREEVDMPTR( protocol32 );
    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32getservbyname

ULONG FASTCALL WWS32getservbyport(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSERVBYPORT16 parg16;
    PSERVENT servent32;
    PSERVENT16 servent16;
    PSZ protocol32;
    DWORD bytesRequired;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    GETARGPTR( pFrame, sizeof(GETSERVBYPORT16), parg16 );

    GETVDMPTR( parg16->Protocol, 32, protocol32 );

    servent32 = (PSERVENT) (*wsockapis[WOW_GETSERVBYPORT].lpfn)( parg16->Port, protocol32 );

    if ( servent32 != NULL ) {

        GETVDMPTR( WWS32vServent, MAXGETHOSTSTRUCT, servent16 );
        bytesRequired = CopyServent32To16(
                            servent16,
                            WWS32vServent,
                            MAXGETHOSTSTRUCT,
                            servent32
                            );
        ASSERT( bytesRequired < MAXGETHOSTSTRUCT );

        FLUSHVDMPTR( WWS32vServent, (USHORT) bytesRequired, servent16 );
        FREEVDMPTR( servent16 );
        ul = WWS32vServent;

    } else {

        ul = 0;
    }

    FREEVDMPTR( protocol32 );
    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32getservbyport

ULONG FASTCALL WWS32WSAAsyncGetServByPort(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCGETSERVBYPORT16 parg16;
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PVOID buffer32;
    PSZ proto32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)0);
    }

    GETARGPTR( pFrame, sizeof(WSAASYNCGETSERVBYPORT16), parg16 );
    GETVDMPTR( parg16->Protocol, 32, proto32 );

    //
    // Set up locals so we know how to clean up on exit.
    //

    context = NULL;
    buffer32 = NULL;
    ul = 0;

    //
    // Allocate a context block and 32-bit buffer to use for the request.
    //

    context = malloc_w( sizeof(*context) );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    buffer32 = malloc_w( MAXGETHOSTSTRUCT );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    //
    // Fill in entries in the context buffer.
    //

    context->Buffer32 = buffer32;
    context->vBuffer16 = parg16->Buffer;
    context->Buffer16Length = parg16->BufferLength;

    //
    // Enter a critical section to synchronize access to the context block
    // and their global list.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    context->AsyncTaskHandle32 = (HANDLE) (*wsockapis[WOW_WSAASYNCGETSERVBYPORT].lpfn)(
                                     (HWND)HWND32(parg16->hWnd),
                                     (parg16->wMsg << 16) |
                                         WWS32_MESSAGE_ASYNC_GETSERV,
                                     parg16->Port,
                                     proto32,
                                     buffer32,
                                     MAXGETHOSTSTRUCT
                                     );

    if ( context->AsyncTaskHandle32 != 0 ) {

        //
        // The call succeeded so get a 16-bit task handle for this
        // request and place the context block on the global list.  The
        // resources will be freed by WWS32PostAsyncGetServ.
        //

        ul = WWS32GetAsyncTaskHandle16( );
        context->AsyncTaskHandle16 = (HAND16)ul;

        InsertTailList(
            &WWS32AsyncContextBlockListHead,
            &context->ContextBlockListEntry
            );
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

exit:

    if ( ul == 0 ) {

        if ( context != NULL ) {
            free_w( (PVOID)context );
        }

        if ( buffer32 != NULL ) {
            free_w( buffer32 );
        }
    }

    FREEARGPTR( proto32 );
    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAAsyncGetServByPort

ULONG FASTCALL WWS32WSAAsyncGetServByName(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCGETSERVBYNAME16 parg16;
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PVOID buffer32;
    PSZ name32;
    PSZ proto32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)0);
    }

    GETARGPTR( pFrame, sizeof(WSAASYNCGETSERVBYNAME16), parg16 );
    GETVDMPTR( parg16->Name, 32, name32 );
    GETVDMPTR( parg16->Protocol, 32, proto32 );

    //
    // Set up locals so we know how to clean up on exit.
    //

    context = NULL;
    buffer32 = NULL;
    ul = 0;

    //
    // Allocate a context block and 32-bit buffer to use for the request.
    //

    context = malloc_w( sizeof(*context) );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    buffer32 = malloc_w( MAXGETHOSTSTRUCT );
    if ( context == NULL ) {
        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOBUFS );
        goto exit;
    }

    //
    // Fill in entries in the context buffer.
    //

    context->Buffer32 = buffer32;
    context->vBuffer16 = parg16->Buffer;
    context->Buffer16Length = parg16->BufferLength;

    //
    // Enter a critical section to synchronize access to the context block
    // and their global list.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    context->AsyncTaskHandle32 = (HANDLE) (*wsockapis[WOW_WSAASYNCGETSERVBYNAME].lpfn)(
                                     (HWND)HWND32(parg16->hWnd),
                                     (parg16->wMsg << 16) |
                                         WWS32_MESSAGE_ASYNC_GETSERV,
                                     name32,
                                     proto32,
                                     buffer32,
                                     MAXGETHOSTSTRUCT
                                     );

    if ( context->AsyncTaskHandle32 != 0 ) {

        //
        // The call succeeded so get a 16-bit task handle for this
        // request and place the context block on the global list.  The
        // resources will be freed by WWS32PostAsyncGetServ.
        //

        ul = WWS32GetAsyncTaskHandle16( );
        context->AsyncTaskHandle16 = (HAND16)ul;

        InsertTailList(
            &WWS32AsyncContextBlockListHead,
            &context->ContextBlockListEntry
            );
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

exit:

    if ( ul == 0 ) {

        if ( context != NULL ) {
            free_w( (PVOID)context );
        }

        if ( buffer32 != NULL ) {
            free_w( buffer32 );
        }
    }

    FREEARGPTR( proto32 );
    FREEARGPTR( name32 );
    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAAsyncGetServByName


BOOL
WWS32PostAsyncGetServ (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    BOOL ret;
    PVOID buffer16;
    DWORD bytesRequired;

    context = WWS32FindAndRemoveAsyncContext( (HANDLE)wParam );
    ASSERT( context != NULL );

    //
    // If the call was successful, copy the 32-bit buffer to the
    // 16-bit buffer specified by the application.
    //

    if ( WSAGETASYNCERROR( lParam ) == 0 ) {

        //
        // Copy the 32-bit structure to 16-bit buffer.
        //

        GETVDMPTR( context->vBuffer16, context->Buffer16Length, buffer16 );

        bytesRequired = CopyServent32To16(
                            buffer16,
                            context->vBuffer16,
                            context->Buffer16Length,
                            context->Buffer32
                            );

        //
        // If the application's buffer was too small, return an error
        // and information aqbout the buffer size required.
        //

        if ( bytesRequired > context->Buffer16Length ) {
            lParam = WSAMAKEASYNCREPLY( (WORD)bytesRequired, WSAENOBUFS );
        }
    }

    //
    // Post the completion message to the 16-bit application.
    //

    ret = PostMessage(
              hWnd,
              Msg >> 16,
              context->AsyncTaskHandle16,
              lParam
              );

    //
    // Free resources and return.
    //

    free_w( context->Buffer32 );
    free_w( (PVOID)context );

    return ret;

} // WWS32PostAsyncGetServ


DWORD
CopyServent32To16 (
    PSERVENT16 Servent16,
    VPSERVENT16 VServent16,
    int BufferLength,
    PSERVENT Servent32
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = (PCHAR)Servent16;
    DWORD aliasCount;
    DWORD i;
    VPBYTE *aliases16;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInServent32( Servent32 );

    //
    // Copy over the servent structure if it fits.
    //

    bytesFilled = sizeof(*Servent16);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    STOREWORD( Servent16->s_port, Servent32->s_port );
    currentLocation = (PCHAR)Servent16 + bytesFilled;

    //
    // Count the service's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Servent32->s_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        STOREDWORD( Servent32->s_aliases, 0 );
        return requiredBufferLength;
    }

    Servent16->s_aliases =
        FIND_16_OFFSET_FROM_32( VServent16, Servent16, currentLocation );
    aliases16 = (VPBYTE *)currentLocation;
    currentLocation = (PCHAR)Servent16 + bytesFilled;

    //
    // Copy the service name if it fits.
    //

    bytesFilled += strlen( Servent32->s_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    Servent16->s_name =
        FIND_16_OFFSET_FROM_32( VServent16, Servent16, currentLocation );

    RtlMoveMemory( currentLocation, Servent32->s_name, strlen( Servent32->s_name ) + 1 );
    currentLocation = (PCHAR)Servent16 + bytesFilled;

    //
    // Copy the protocol name if it fits.
    //

    bytesFilled += strlen( Servent32->s_proto ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    Servent16->s_proto =
        FIND_16_OFFSET_FROM_32( VServent16, Servent16, currentLocation );

    RtlMoveMemory( currentLocation, Servent32->s_proto, strlen( Servent32->s_proto ) + 1 );
    currentLocation = (PCHAR)Servent16 + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Servent32->s_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            STOREDWORD( aliases16[i], NULL );
            return requiredBufferLength;
        }

        STOREDWORD(
            aliases16[i],
            FIND_16_OFFSET_FROM_32( VServent16, Servent16, currentLocation )
            );

        RtlMoveMemory(
            currentLocation,
            Servent32->s_aliases[i],
            strlen( Servent32->s_aliases[i] ) + 1
            );

        currentLocation = (PCHAR)Servent16 + bytesFilled;
    }

    STOREDWORD( aliases16[i], NULL );

    return requiredBufferLength;

} // CopyServent32To16


DWORD
BytesInServent32 (
    IN PSERVENT Servent32
    )
{
    DWORD total;
    int i;

    total = sizeof(SERVENT);
    total += strlen( Servent32->s_name ) + 1;
    total += strlen( Servent32->s_proto ) + 1;
    total += sizeof(char *);

    for ( i = 0; Servent32->s_aliases[i] != NULL; i++ ) {
        total += strlen( Servent32->s_aliases[i] ) + 1 + sizeof(char *);
    }

    return total;

} // BytesInServent32

ULONG FASTCALL WWS32WSACancelAsyncRequest(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSACANCELASYNCREQUEST16 parg16;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(WSACANCELASYNCREQUEST16), parg16);

    //ul = GETWORD16((*wsockapis[WOW_WSACANCELASYNCREQUEST].lpfn)(
    //                    ));

    FREEARGPTR(parg16);

    ul = (ULONG) SOCKET_ERROR;
    SetLastError( WSAEINVAL );

    RETURN(ul);

} // WWS32WSACancelAsyncRequest


PWINSOCK_ASYNC_CONTEXT_BLOCK
WWS32FindAndRemoveAsyncContext (
    IN HANDLE AsyncTaskHandle32
    )
{
    PWINSOCK_ASYNC_CONTEXT_BLOCK context;
    PLIST_ENTRY listEntry;

    RtlEnterCriticalSection( &WWS32CriticalSection );

    //
    // Walk the global list of async context blocks, looking for
    // one that matches the specified task handle.
    //

    for ( listEntry = WWS32AsyncContextBlockListHead.Flink;
          listEntry != &WWS32AsyncContextBlockListHead;
          listEntry = listEntry->Flink ) {

        context = CONTAINING_RECORD(
                      listEntry,
                      WINSOCK_ASYNC_CONTEXT_BLOCK,
                      ContextBlockListEntry
                      );

        if ( context->AsyncTaskHandle32 == AsyncTaskHandle32 ) {

            //
            // Found a match.  Remove it from the global list, leave
            // the critical section, and return the context block.
            //

            RemoveEntryList( &context->ContextBlockListEntry );
            RtlLeaveCriticalSection( &WWS32CriticalSection );

            return context;
        }
    }

    //
    // A matching context block was not found on the list.
    //

    RtlLeaveCriticalSection( &WWS32CriticalSection );

    return NULL;

} // WWS32FindAndRemoveAsyncContext


HAND16
WWS32GetAsyncTaskHandle16 (
    VOID
    )
{
    HAND16 asyncTaskHandle16;

    // *** this routine *must* be called from within the WWS32 critical
    //     section!

    ASSERT( WWS32AsyncTaskHandleCounter != 0 );

    asyncTaskHandle16 = (HAND16)WWS32AsyncTaskHandleCounter;

    WWS32AsyncTaskHandleCounter++;

    //
    // 0 is an invalid task handle value; if the counter has wrapped to
    // zero, set it to 1.
    //

    if ( WWS32AsyncTaskHandleCounter == 0 ) {
        WWS32AsyncTaskHandleCounter = 1;
    }

    return WWS32AsyncTaskHandleCounter;

} // WWS32GetAsyncTaskHandle16
