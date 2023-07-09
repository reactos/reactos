/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dlltask.c

Abstract:

    This module implements Csr DLL tasking routines

Author:

    Mark Lucovsky (markl) 13-Nov-1990

Revision History:

--*/

#include "csrdll.h"

NTSTATUS
CsrNewThread(
    VOID
    )

/*++

Routine Description:

    This function is called by each new thread (except the first thread in
    a process.) It's function is to call the subsystem to notify it that
    a new thread is starting.

Arguments:

    None.

Return Value:

    Status Code from either client or server

--*/

{
    return NtRegisterThreadTerminatePort( CsrPortHandle );
}

NTSTATUS
CsrIdentifyAlertableThread( VOID )
{
    NTSTATUS Status;
    CSR_API_MSG m;
    PCSR_IDENTIFY_ALERTABLE_MSG a = &m.u.IndentifyAlertable;

    a->ClientId = NtCurrentTeb()->ClientId;

    Status = CsrClientCallServer(
                &m,
                NULL,
                CSR_MAKE_API_NUMBER( CSRSRV_SERVERDLL_INDEX,
                                     CsrpIdentifyAlertable
                                   ),
                sizeof( *a )
                );

    return Status;
}


NTSTATUS
CsrSetPriorityClass(
    IN HANDLE ProcessHandle,
    IN OUT PULONG PriorityClass
    )
{
    NTSTATUS Status;
    CSR_API_MSG m;
    PCSR_SETPRIORITY_CLASS_MSG a = &m.u.PriorityClass;

    a->ProcessHandle = ProcessHandle;
    a->PriorityClass = *PriorityClass;

    Status = CsrClientCallServer(
                &m,
                NULL,
                CSR_MAKE_API_NUMBER( CSRSRV_SERVERDLL_INDEX,
                                     CsrpSetPriorityClass
                                   ),
                sizeof( *a )
                );

    if ( *PriorityClass == 0 ) {
        *PriorityClass = a->PriorityClass;
        }

    return Status;

}
