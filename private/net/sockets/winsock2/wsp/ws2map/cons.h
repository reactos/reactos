/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    cons.h

Abstract:

    This module contains global constant definitions for the Winsock 2
    to Winsock 1.1 Mapper Service Provider.

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#ifndef _CONS_H_
#define _CONS_H_


//
// Filler for empty if/while/except/whatever blocks.
//

#ifndef NOTHING
#define NOTHING
#endif


//
// The registry key for the "GUID-to-DLL-path" mapping.
//

#define SOCK_HOOKER_GUID_MAPPER_KEY "System\\CurrentControlSet\\Services\\Ws2Map\\Parameters\\Guid Mapper"


//
// Macro for converting a pointer to a WSAOVERLAPPED structure to the
// corresponding SOCK_IO_STATUS pointer.
//

#define SOCK_OVERLAPPED_TO_IO_STATUS(over)              \
            ( (PSOCK_IO_STATUS)&(over)->Internal )


#endif // _CONS_H_

