/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    enumsock.c

Abstract:

    Enumerates all sockets in the target process.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#include "msafdext.h"
#include "newctx.h"

//
// Public functions.
//

VOID
EnumSockets(
    PENUM_SOCKETS_CALLBACK Callback,
    LPVOID Context
    )

/*++

Routine Description:

    Enumerates all sockets in the target process, invoking the
    specified callback for each socket.

Arguments:

    Callback - Points to the callback to invoke for each socket.

    Context - An uninterpreted context value passed to the callback
        routine.

Return Value:

    None.

--*/

{

    ULONG    result;
    DWORD i,j;
    ULONG_PTR tablePtr;
    ULONG_PTR tableAddr;
    ULONG    mask;
    CTX_LOOKUP_TABLE table;
    ULONG    numBuckets;
    ULONG_PTR sockInfoAddr;
    PSOCKET_INFORMATION sockInfo;
    SOCKET_INFORMATION localSocket;

    tablePtr = GetExpression( "msafd!SockContextTable" );

    if( tablePtr == 0 ) {

        dprintf (( "cannot find msafd!SockContextTable\n" ));
        return;

    }

    if( !ReadMemory(
            tablePtr,
            &tableAddr,
            sizeof(tableAddr),
            &result
            ) ) {

        dprintf ((
            "EnumSockets: cannot read msafd!SockContextTable @ %08lx\n",
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

        dprintf ((
            "EnumSockets: cannot read context table mask @ %08lx\n",
            tableAddr
            ));
        return;

    }

    tableAddr += sizeof(mask);
    for (i=0; i<=mask; i++, tableAddr+=sizeof(table)) {
        if( CheckControlC() ) {

            return;

        }
        if( !ReadMemory(
                tableAddr,
                &table,
                sizeof(table),
                &result
                ) ) {

            dprintf ((
                "EnumSockets: cannot read lookup table @ %08lx\n",
                tableAddr
                ));
            return;
        }
        if (table.HashTable!=NULL) {
            if( CheckControlC() ) {

                return;

            }
            if( !ReadMemory(
                    (ULONG_PTR)table.HashTable,
                    &numBuckets,
                    sizeof(numBuckets),
                    &result
                    ) ) {

                dprintf ((
                    "EnumSockets: cannot read hash table @ %08lx\n",
                    table.HashTable
                    ));
                return;
            }
            
            sockInfoAddr = (ULONG_PTR)table.HashTable+sizeof(numBuckets);
            for (j=0; j<numBuckets; j++, sockInfoAddr+=sizeof(sockInfo)) {
                if( CheckControlC() ) {

                    return;

                }
                if( !ReadMemory(
                        sockInfoAddr,
                        &sockInfo,
                        sizeof(sockInfo),
                        &result
                        ) ) {

                    dprintf ((
                        "EnumSockets: cannot read hash table entry @ %08lx\n",
                        sockInfoAddr
                        ));
                    return;
                }
                if (sockInfo==NULL)
                    continue;

                if( !ReadMemory(
                        (ULONG_PTR)sockInfo,
                        &localSocket,
                        sizeof(localSocket),
                        &result
                        ) ) {

                    dprintf ((
                        "EnumSockets: cannot read SOCKET_INFORMATION @ %08lx\n",
                        sockInfo
                        ));

                    return;

                }
                if( !(Callback)( &localSocket, (ULONG_PTR)sockInfo, Context ) ) {

                    return;

                }
            }
        }
    }

}   // EnumSockets

