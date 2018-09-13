/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    addr.c

Abstract:

    Implements the addr command.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#include "msafdext.h"


//
// Public functions.
//

DECLARE_API( Addr )

/*++

Routine Description:

    Dumps the specified SOCKADDR structure.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG_PTR   address = 0;
    ULONG       result;
    SOCKADDR    addr;
    CHAR        expr[256];

    //
    // Snag the address from the command line.
    //

    if ((sscanf( args, "%s", expr )!=1) ||
            (address = GetExpression (expr))==0 ) {

        dprintf ((
            "use: addr address\n"
            ));

    } else {

        if( !ReadMemory(
                address,
                &addr,
                sizeof(addr),
                &result
                ) ) {

            dprintf ((
                "addr: cannot read SOCKADDR @ %08lx\n",
                address
                ));

            return;

        }

        DumpSockaddr(
            "",
            &addr,
            address
            );

    }

}   // addr

//
// Pure hack to make this work under ntsd as well.
// NTSD always loads lowercased entry point.
//
DECLARE_API_N( addr ) {
    if (ExtensionApisInitialized) {
        // We are running under KD, just call the real entry point
        // We are assuming that parameter sizes on the stack are the
        // same for pointers and DWORDs
        Addr (hCurrentProcess,hCurrentThread,dwCurrentPc,PtrToUlong(lpExtensionApis),lpArgumentString);
    }
    else {
        INIT_API_N();
        Addr (hCurrentProcess,hCurrentThread,dwCurrentPc,0,lpArgumentString);
    }
}
