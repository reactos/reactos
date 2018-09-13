/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    help.c

Abstract:

    Implements the help command.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#include "msafdext.h"


//
// Public functions.
//

DECLARE_API( Help )
{


    dprintf (( "?              - Displays this list\n" ));
    dprintf (( "Help           - Displays this list\n" ));
    dprintf (( "Sock [Handle]  - Dumps sockets in the current process\n" ));
    dprintf (( "Mssock [Handle]- Dumps MS base provider sockets\n" ));
    dprintf (( "Addr address   - Dumps a sockaddr structure\n" ));
    dprintf (( "Port port      - Finds MS base provider sockets bound to a port\n" ));
    dprintf (( "Rport port     - Finds MS base provider sockets connected to a port\n" ));
    dprintf (( "State state    - Finds MS base provider sockets in a specific state\n" ));
    dprintf (( "    valid states are:\n" ));
    dprintf (( "        0 - Open\n" ));
    dprintf (( "        1 - Bound\n" ));
    dprintf (( "        2 - BoundSpecific\n" ));
    dprintf (( "        3 - Connected\n" ));
    dprintf (( "        4 - Closing\n" ));
    dprintf (( "        0x10 - Listening\n" ));
#ifdef _AFD_SAN_SWITCH_
	dprintf (( "Sansock [-b] [Handle]- Dumps SAN sockets\n" ));
	dprintf (( "Sanaddr [-b] address - Dumps SAN socket at specified address\n" ));
	dprintf (( "      -b   - use brief display\n" ));
#endif // _AFD_SAN_SWITCH_
    dprintf (( "Make sure msafd.dll and ws2_32.dll symbols are loaded\n"));
    dprintf (( "for all commands to work properly\n"));

}   // help

//
// Pure hack to make this work under ntsd as well.
// NTSD always loads lowercased entry point.
//
DECLARE_API_N( help )
{
    if (ExtensionApisInitialized) {
        // We are running under KD, just call the real entry point
        // We are assuming that parameter sizes on the stack are the
        // same for pointers and DWORDs
        Help (hCurrentProcess,hCurrentThread,dwCurrentPc,PtrToUlong(lpExtensionApis),lpArgumentString);
    }
    else {
        INIT_API_N();
        dprintf (( "?              - Displays this list\n" ));
        dprintf (( "help           - Displays this list\n" ));
        dprintf (( "sock [Handle]  - Dumps sockets\n" ));
        dprintf (( "mssock [Handle]- Dumps MS base provider sockets\n" ));
        dprintf (( "addr address   - Dumps a sockaddr structure\n" ));
        dprintf (( "port port      - Finds MS base provider sockets bound to a port\n" ));
        dprintf (( "rport port     - Finds MS base provider sockets connected to a port\n" ));
        dprintf (( "state state    - Finds MS base provider sockets in a specific state\n" ));
        dprintf (( "    valid states are:\n" ));
        dprintf (( "        0 - Open\n" ));
        dprintf (( "        1 - Bound\n" ));
        dprintf (( "        2 - BoundSpecific\n" ));
        dprintf (( "        3 - Connected\n" ));
        dprintf (( "        4 - Closing\n" ));
        dprintf (( "        0x10 - Listening\n" ));
#ifdef _AFD_SAN_SWITCH_
		dprintf (( "sansock [-b] [Handle]- Dumps SAN sockets\n" ));
		dprintf (( "sanaddr [-b] address - Dumps SAN socket at specified address\n" ));
		dprintf (( "      -b   - use brief display\n" ));
#endif // _AFD_SAN_SWITCH_
    }
}
