/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    TERMINAT.C

Abstract:

    This file contains all the cleanup routines for the Eventlog service.
    These routines are called when the service is terminating.

Author:

    Rajen Shah  (rajens)    09-Aug-1991


Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>
#include <ntrpcp.h>




VOID
StopLPCThread ()

/*++

Routine Description:

    This routine stops the LPC thread and cleans up LPC-related resources.

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    ElfDbgPrint(( "[ELF] Stop the LPC thread\n" ));

    //
    // Close communication port handle
    //

    NtClose ( ElfCommunicationPortHandle );

    //
    // Close connection port handle
    //

    NtClose ( ElfConnectionPortHandle );

    //
    // Terminate the LPC thread.
    //

    if (!TerminateThread(LPCThreadHandle,NO_ERROR)) {
        ElfDbgPrint(("[ELF] LPC Thread termination failed %d\n",GetLastError()));
    }
    CloseHandle ( LPCThreadHandle );

    return;
}




VOID
FreeModuleAndLogFileStructs ( )

/*++

Routine Description:

    This routine walks the module and log file list and frees all the
    data structures.

Arguments:

    NONE

Return Value:

    NONE

Note:

    The file header and ditry bits must have been dealt with before
    this routine is called. Also, the file must have been unmapped and
    the handle closed.

--*/
{

    NTSTATUS Status;
    PLOGMODULE pModule;
    PLOGFILE pLogFile;

    ElfDbgPrint (("[ELF] Freeing module and log file structs\n"));

    //
    // First free all the modules
    //

    while (!IsListEmpty (&LogModuleHead) ) {

        pModule = (PLOGMODULE)
            CONTAINING_RECORD(LogModuleHead.Flink, LOGMODULE, ModuleList);

        UnlinkLogModule(pModule);   // Remove from linked list

        ElfpFreeBuffer (pModule);    // Free module memory

    }

    //
    // Now free all the logfiles
    //

    while (!IsListEmpty (&LogFilesHead) ) {

        pLogFile = (PLOGFILE)
            CONTAINING_RECORD(LogFilesHead.Flink, LOGFILE, FileList);

        Status = ElfpCloseLogFile ( pLogFile, ELF_LOG_CLOSE_NORMAL);

        UnlinkLogFile(pLogFile); // Unlink the structure
        RtlDeleteResource ( &pLogFile->Resource );
        ElfpFreeBuffer (pLogFile->LogFileName);
        ElfpFreeBuffer (pLogFile);
    }
}


VOID
ElfpCleanUp (
    ULONG EventFlags
    )

/*++

Routine Description:

    This routine cleans up before the service terminates. It cleans up
    based on the parameter passed in (which indicates what has been allocated
    and/or started.

Arguments:

    Bit-mask indicating what needs to be cleaned up.

Return Value:

    NONE

Note:
    It is expected that the RegistryMonitor has already
    been notified of Shutdown prior to calling this routine.

--*/
{
    DWORD   status = NO_ERROR;


    ElfDbgPrint (("[ELF] ElfpCleanUp.\n"));

    //
    // Notify the Service Controller for the first time that we are
    // about to stop the service.
    //
    // *** STATUS UPDATE ***
    ElfStatusUpdate(STOPPING);


    //
    // Stop the RPC Server
    //
    if (EventFlags & ELF_STARTED_RPC_SERVER) {
        ElfDbgPrint (("[ELF] Stopping the RPC Server.\n"));

        status = ElfGlobalData->StopRpcServer(eventlog_ServerIfHandle);
        if (status != NO_ERROR) {
            ElfDbgPrint (("[ELF] Stopping RpcServer Failed %d\n",status));
        }
    }

    //
    // Stop the LPC thread
    //
    if (EventFlags & ELF_STARTED_LPC_THREAD)
        StopLPCThread();

    //
    // Tell service controller that we are making progress
    //
    // *** STATUS UPDATE ***
    ElfStatusUpdate(STOPPING);

    //
    // Flush all the log files to disk.
    //
    ElfDbgPrint (("[ELF] Flushing Files.\n"));
    ElfpFlushFiles();

    //
    // Tell service controller that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    //
    // Clean up any resources that were allocated
    //
    FreeModuleAndLogFileStructs();

    //
    // Free up memory
    //

    if (LocalComputerName) {
        ElfpFreeBuffer(LocalComputerName);
    }

    //
    // If we queued up any events, flush them
    //

    ElfDbgPrint (("[ELF] Flushing QueuedEvents.\n"));
    FlushQueuedEvents();

    //
    // Tell service controller of that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    if (EventFlags & ELF_INIT_GLOBAL_RESOURCE)
    {
        RtlDeleteResource ( &GlobalElfResource );
    }
    if (EventFlags & ELF_INIT_CLUS_CRIT_SEC)
    {
        RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)&gClPropCritSec);
    }

    if (EventFlags & ELF_INIT_LOGHANDLE_CRIT_SEC)
        RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)&LogHandleCritSec);

    if (EventFlags & ELF_INIT_LOGFILE_CRIT_SEC)
        RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)&LogFileCritSec);

    if (EventFlags & ELF_INIT_QUEUED_EVENT_CRIT_SEC)
        RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);

    // *** STATUS UPDATE ***
    ElfDbgPrint(("[ELF] Leaving the Eventlog service\n"));
    ElfStatusUpdate(STOPPED);
    ElCleanupStatus();
    return;
}
