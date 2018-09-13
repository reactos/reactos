/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wow64dbg.c

Abstract:

    This module contains the WOW64 versions of the code for the debugger client.
    See dlluistb.c in ntos\dll for more function comments. 

Author:

    Michael Zoran (mzoran) 1-DEC-1998

Environment:

    User Mode only

Revision History:

--*/

#include "dbgdllp.h"
#include "csrdll.h"
#include "ldrp.h"
#include "ntwow64.h"

NTSTATUS
DbgUiConnectToDbg( VOID ) {
    return NtDbgUiConnectToDbg();
}

NTSTATUS
DbgUiWaitStateChange (
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange,
    IN PLARGE_INTEGER Timeout OPTIONAL
    ) 
{
    return NtDbgUiWaitStateChange(StateChange, Timeout);
}

NTSTATUS
DbgUiContinue (
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
    )
{
    return NtDbgUiContinue(AppClientId, ContinueStatus);
}
