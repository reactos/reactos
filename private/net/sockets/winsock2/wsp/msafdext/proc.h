/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    Global procedure definitions for the MSAFD NTSD Debugger Extensions.

Author:

    Keith Moore (keithmo) 20-May-1996.

Environment:

    User Mode.

--*/


#ifndef _PROC_H_
#define _PROC_H_


//
//  Functions from SOCKUTIL.C
//

VOID
DumpSocket(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress
    );

VOID
DumpSockaddr(
    PSTR Prefix,
    PSOCKADDR Sockaddr,
    ULONG_PTR ActualAddress
    );


//
//  Functions from DBGUTIL.C.
//

PSTR
LongLongToString(
    LONGLONG Value
    );

PSTR
BooleanToString(
    BOOLEAN Value
    );


//
//  Functions from ENUMSOCK.C
//

VOID
EnumSockets(
    PENUM_SOCKETS_CALLBACK Callback,
    LPVOID Context
    );


BOOLEAN
IsCheckedMsafd (
    VOID
    );

BOOLEAN
IsCheckedWs2_32 (
    VOID
    );

#endif  // _PROC_H_

