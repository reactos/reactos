/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    socket.c

Abstract:

    Implements the sock, port, and state commands.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#include "msafdext.h"
#include "newctx.h"

#define TABLE_OFFSET_FROM_HANDLE(_h,_mask) \
            FIELD_OFFSET(struct _CONTEXT_TABLE,Tables[((((ULONG_PTR)_h) >> 2) & (_mask))])

#define HASH_BUCKET_OFFSET_FROM_HANDLE(_h,_num) \
            FIELD_OFFSET(CTX_HASH_TABLE,Buckets[(((ULONG_PTR)_h) % (_num))])

//
// Private prototypes.
//

BOOL
DumpSocketCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    );

BOOL
FindStateCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    );

BOOL
FindPortCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    );

BOOL
FindRportCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    );


//
// Public functions.
//

DECLARE_API( Mssock )

/*++

Routine Description:

    Dumps the SOCKET_INFORMATION structure at the specified address, if
    given or all sockets.

Arguments:

    None.

Return Value:

    None.

--*/

{

    CHAR    expr[256];
    DWORD   i;
    ULONG   result;
    ULONG_PTR tablePtr;
    ULONG_PTR tableAddr;
    ULONG   mask;
    CTX_LOOKUP_TABLE table;
    ULONG   numBuckets;
    ULONG_PTR hashTableAddr;
    ULONG_PTR socketAddr;
    ULONG_PTR handle;
    SOCKET_INFORMATION sockInfo;

    //
    // Snag the address from the command line.
    //

    if ((sscanf( args, "%s%n", expr, &i )==1) &&
            (handle = GetExpression (expr))!=0) {

        tablePtr = GetExpression( "msafd!SockContextTable" );

        if( tablePtr == 0 ) {

            dprintf(( "Mssock: cannot find msafd!SockContextTable\n" ));
            return;

        }

        if( !ReadMemory(
                tablePtr,
                &tableAddr,
                sizeof(tableAddr),
                &result
                ) ) {

            dprintf((
                "Mssock: cannot read msafd!SockContextTable @ %p\n",
                tablePtr
                ));
            return;

        }

        if( !ReadMemory(
                tableAddr,
                &mask,
                sizeof(mask),
                &result
                ) ) {

            dprintf((
                "Mssock: cannot read context table mask @ %p\n",
                tableAddr
                ));
            return;

        }

        tableAddr += TABLE_OFFSET_FROM_HANDLE(handle, mask);

        if( !ReadMemory(
                tableAddr,
                &table,
                sizeof(table),
                &result
                ) ) {

            dprintf((
                "Mssock: cannot read lookup table @ %p\n",
                tableAddr
                ));
            return;
        }

        if (table.HashTable==NULL) {
            //dprintf((
            //    "Mssock: hash table for socket %p is NULL!\n", handle));
            return;
        }

        hashTableAddr = (ULONG_PTR)table.HashTable;

        if( !ReadMemory(
                hashTableAddr,
                &numBuckets,
                sizeof(numBuckets),
                &result
                ) ) {

            dprintf((
                "Mssock: cannot read hash table @ %p\n",
                table.HashTable
                ));
            return;
        }

        if (numBuckets==0) {
            dprintf((
                "Mssock: Sizeof hash table for socket %p is 0!\n", handle));
            return;
        }
        hashTableAddr += HASH_BUCKET_OFFSET_FROM_HANDLE(handle,numBuckets);
        if( !ReadMemory(
                hashTableAddr,
                &socketAddr,
                sizeof(socketAddr),
                &result
                ) ) {

            dprintf((
                "Mssock: cannot read hash table entry @ %p\n",
                hashTableAddr
                ));
            return;
        }
        if (socketAddr==0) {
            //dprintf((
            //    "Mssock: Hash table entry for socket %p is NULL!\n", handle));
            return;
        }
        if( !ReadMemory(
                socketAddr,
                &sockInfo,
                sizeof(sockInfo),
                &result
                ) ) {

            dprintf ((
                "Mssock: cannot read SOCKET_INFORMATION @ %08lx\n",
                socketAddr
                ));

            return;
        }

        DumpSocket(
            &sockInfo,
            socketAddr
            );

    }
    else {
        EnumSockets(
            DumpSocketCallback,
            NULL
            );
    }

}   // Mssock


//
// Pure hack to make this work under ntsd as well.
// NTSD always loads lowercased entry point.
//
DECLARE_API_N( mssock ) {
    if (ExtensionApisInitialized) {
        // We are running under KD, just call the real entry point
        // We are assuming that parameter sizes on the stack are the
        // same for pointers and DWORDs
        Mssock (hCurrentProcess,hCurrentThread,dwCurrentPc,PtrToUlong(lpExtensionApis),lpArgumentString);
    }
    else {
        INIT_API_N();
        Mssock (hCurrentProcess,hCurrentThread,dwCurrentPc,0,lpArgumentString);
    }
}



DECLARE_API( Port )

/*++

Routine Description:

    Dumps the SOCKET_INFORMATION structure of all sockets bound to the
    specified port.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR    expr[256];
    ULONG_PTR port = 0;

    //
    // Snag the port from the command line.
    //

    if (sscanf( args, "%s", expr)!=1) {
        dprintf ((
            "use: port port\n"
            ));
        return;
    }

    port = GetExpression (expr);

    dprintf (("Looking for socket bound to port 0x%lx (%ld)....\n", port, port));

    EnumSockets(
        FindPortCallback,
        (LPVOID)port
        );

}   // port

//
// Pure hack to make this work under ntsd as well.
// NTSD always loads lowercased entry point.
//
DECLARE_API_N( port ) {
    if (ExtensionApisInitialized) {
        // We are running under KD, just call the real entry point
        // We are assuming that parameter sizes on the stack are the
        // same for pointers and DWORDs
        Port (hCurrentProcess,hCurrentThread,dwCurrentPc,PtrToUlong(lpExtensionApis),lpArgumentString);
    }
    else {
        INIT_API_N();
        Port (hCurrentProcess,hCurrentThread,dwCurrentPc,0,lpArgumentString);
    }
}

DECLARE_API( Rport )

/*++

Routine Description:

    Dumps the SOCKET_INFORMATION structure of all sockets connected to the
    specified port.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR    expr[256];
    ULONG_PTR port = 0;

    //
    // Snag the port from the command line.
    //

    if (sscanf( args, "%s", expr)!=1) {
        dprintf ((
            "use: rport port\n"
            ));
        return;
    }

    port = GetExpression (expr);

    dprintf (("Looking for socket connected to port 0x%lx (%ld)....\n", port, port));

    EnumSockets(
        FindRportCallback,
        (LPVOID)port
        );

}   // port

//
// Pure hack to make this work under ntsd as well.
// NTSD always loads lowercased entry point.
//
DECLARE_API_N( rport ) {
    if (ExtensionApisInitialized) {
        // We are running under KD, just call the real entry point
        // We are assuming that parameter sizes on the stack are the
        // same for pointers and DWORDs
        Rport (hCurrentProcess,hCurrentThread,dwCurrentPc,PtrToUlong(lpExtensionApis),lpArgumentString);
    }
    else {
        INIT_API_N();
        Rport (hCurrentProcess,hCurrentThread,dwCurrentPc,0,lpArgumentString);
    }
}

DECLARE_API( State )

/*++

Routine Description:

    Dumps the SOCKET_INFORMATION structure of all sockets in the specified
    state.

Arguments:

    None.

Return Value:

    None.

--*/

{

    DWORD state = 0;

    //
    // Snag the state from the command line.
    //

    if( sscanf( args, "%lx", &state )<1 ) {

        dprintf (( "use: state state\n" ));
        dprintf (( "    valid states are:\n" ));
        dprintf (( "        0 - Open\n" ));
        dprintf (( "        1 - Bound\n" ));
        dprintf (( "        2 - BoundSpecific\n" ));
        dprintf (( "        3 - Connected\n" ));
        dprintf (( "        4 - Closing\n" ));
        dprintf (( "        0x10 - Listening\n" ));

    } else {

        EnumSockets(
            FindStateCallback,
            (LPVOID)state
            );

    }

}   // state

//
// Pure hack to make this work under ntsd as well.
// NTSD always loads lowercased entry point.
//
DECLARE_API_N( state ) {
    if (ExtensionApisInitialized) {
        // We are running under KD, just call the real entry point
        // We are assuming that parameter sizes on the stack are the
        // same for pointers and DWORDs
        State (hCurrentProcess,hCurrentThread,dwCurrentPc,PtrToUlong(lpExtensionApis),lpArgumentString);
    }
    else {
        INIT_API_N();
        State (hCurrentProcess,hCurrentThread,dwCurrentPc,0,lpArgumentString);
    }
}

//
// Private functions.
//

BOOL
DumpSocketCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    )
{

    DumpSocket(
        Socket,
        ActualAddress
        );

    return TRUE;

}   // DumpSocketCallback


BOOL
FindPortCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    )
{

    SOCKADDR_IN addr;
    ULONG result;
    DWORD port;

    if( Socket->AddressFamily != AF_INET ) {

        return TRUE;

    }

    if( !ReadMemory(
            (ULONG_PTR)Socket->LocalAddress,
            &addr,
            sizeof(addr),
            &result
            ) ) {

        dprintf ((
            "port: cannot read SOCKADDR @ %08lx\n",
            Socket->LocalAddress
            ));

        return TRUE;

    }

    port = (DWORD)NTOHS( addr.sin_port );

    if( addr.sin_family == AF_INET &&
        port == (DWORD)((DWORD_PTR)Context) ) {

        DumpSocket(
            Socket,
            ActualAddress
            );

    }

    return TRUE;

}   // FindPortCallback


BOOL
FindRportCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    )
{

    SOCKADDR_IN addr;
    ULONG result;
    DWORD port;

    if( Socket->AddressFamily != AF_INET ) {

        return TRUE;

    }

    if( !ReadMemory(
            (ULONG_PTR)Socket->RemoteAddress,
            &addr,
            sizeof(addr),
            &result
            ) ) {

        dprintf ((
            "port: cannot read SOCKADDR @ %08lx\n",
            Socket->RemoteAddress
            ));

        return TRUE;

    }

    port = (DWORD)NTOHS( addr.sin_port );

    if( addr.sin_family == AF_INET &&
        port == (DWORD)((DWORD_PTR)Context) ) {

        DumpSocket(
            Socket,
            ActualAddress
            );

    }

    return TRUE;

}   // FindRportCallback


BOOL
FindStateCallback(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress,
    LPVOID Context
    )
{

    if( Socket->State == (SOCKET_STATE)((DWORD_PTR)Context) ||
        ((INT)((DWORD_PTR)Context) == 0x10 && Socket->Listening)) {

        DumpSocket(
            Socket,
            ActualAddress
            );

    }

    return TRUE;

}   // FindStateCallback

