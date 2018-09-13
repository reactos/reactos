
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pscid.c

Abstract:

    This module implements the Client ID related services.


Author:

    Mark Lucovsky (markl) 25-Apr-1989
    Jim Kelly (JimK) 2-August-1990

Revision History:

--*/

#include "psp.h"

NTSTATUS
PsLookupProcessThreadByCid(
    IN PCLIENT_ID Cid,
    OUT PEPROCESS *Process OPTIONAL,
    OUT PETHREAD *Thread
    )

/*++

Routine Description:

    This function accepts The Client ID of a thread, and returns a
    referenced pointer to the thread, and possibly a referenced pointer
    to the process.

Arguments:

    Cid - Specifies the Client ID of the thread.

    Process - If specified, returns a referenced pointer to the process
        specified in the Cid.

    Thread - Returns a referenced pointer to the thread specified in the
        Cid.

Return Value:

    STATUS_SUCCESS - A process and thread were located based on the contents
        of the Cid.

    STATUS_INVALID_CID - The specified Cid is invalid.

--*/

{

    PHANDLE_TABLE_ENTRY CidEntry;
    PETHREAD lThread;
    NTSTATUS Status;

    CidEntry = ExMapHandleToPointer(PspCidTable, Cid->UniqueThread);
    Status = STATUS_INVALID_CID;
    if (CidEntry != NULL) {
        lThread = (PETHREAD)CidEntry->Object;
        if ((lThread != (PETHREAD)PSP_INVALID_ID) &&
            (
             lThread->Tcb.Header.Type == ThreadObject &&
             lThread->Cid.UniqueProcess == Cid->UniqueProcess &&
             lThread->GrantedAccess
            ) ) {
            if (ARGUMENT_PRESENT(Process)) {
                *Process = THREAD_TO_PROCESS(lThread);
                ObReferenceObject(*Process);
            }

            ObReferenceObject(lThread);
            *Thread = lThread;
            Status = STATUS_SUCCESS;
        }

        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }

    return Status;
}


NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    )

/*++

Routine Description:

    This function accepts the process id of a process and returns a
    referenced pointer to the process.

Arguments:

    ProcessId - Specifies the Process ID of the process.

    Process - Returns a referenced pointer to the process specified by the
        process id.

Return Value:

    STATUS_SUCCESS - A process was located based on the contents of
        the process id.

    STATUS_INVALID_PARAMETER - The process was not found.

--*/

{

    PHANDLE_TABLE_ENTRY CidEntry;
    PEPROCESS lProcess;
    NTSTATUS Status;

    CidEntry = ExMapHandleToPointer(PspCidTable, ProcessId);
    Status = STATUS_INVALID_PARAMETER;
    if (CidEntry != NULL) {
        lProcess = (PEPROCESS)CidEntry->Object;
        if (lProcess != (PEPROCESS)PSP_INVALID_ID && lProcess->Pcb.Header.Type == ProcessObject && lProcess->GrantedAccess ) {
            ObReferenceObject(lProcess);
            *Process = lProcess;
            Status = STATUS_SUCCESS;
        }

        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }

    return Status;
}


NTSTATUS
PsLookupThreadByThreadId(
    IN HANDLE ThreadId,
    OUT PETHREAD *Thread
    )

/*++

Routine Description:

    This function accepts the thread id of a thread and returns a
    referenced pointer to the thread.

Arguments:

    ThreadId - Specifies the Thread ID of the thread.

    Thread - Returns a referenced pointer to the thread specified by the
        thread id.

Return Value:

    STATUS_SUCCESS - A thread was located based on the contents of
        the thread id.

    STATUS_INVALID_PARAMETER - The thread was not found.

--*/

{

    PHANDLE_TABLE_ENTRY CidEntry;
    PETHREAD lThread;
    NTSTATUS Status;

    CidEntry = ExMapHandleToPointer(PspCidTable, ThreadId);
    Status = STATUS_INVALID_PARAMETER;
    if (CidEntry != NULL) {
        lThread = (PETHREAD)CidEntry->Object;
        if (lThread != (PETHREAD)PSP_INVALID_ID && lThread->Tcb.Header.Type == ThreadObject && lThread->GrantedAccess ) {

            ObReferenceObject(lThread);
            *Thread = lThread;
            Status = STATUS_SUCCESS;
        }

        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }

    return Status;
}
