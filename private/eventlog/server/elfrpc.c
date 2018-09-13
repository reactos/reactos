/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ELFRPC.C

Abstract:

    This file contains the routines that handle the RPC calls to the
    Eventlog service via the Elf APIs.

Author:

    Rajen Shah  (rajens)    16-Jul-1991

Revision History:

    15-Feb-1995     MarkBl
        Unlink the ElfHandle *prior* to unlinking the module. Otherwise,
        if another thread happens to coincidentally be in the routine,
        FindModuleStrucFromAtom, it's not going to get a hit for the
        module atom.

    18-May-1994     Danl
        IELF_HANDLE_rundown:  If the eventlog has been shutdown, then
        we want to skip the code in this routine because most of the
        resources will have been free'd.

    31-Jan-1994     Danl
        IELF_HANDLE_rundown: Notifiee structure was being free'd and then
        referenced when it's handle was removed from the list.  Now this
        is fixed so it advances to the next Notifiee in the list BEFORE the
        buffer is free'd.

--*/

//
// INCLUDES
//

#include <eventp.h>



extern      DWORD  ElState;


VOID
IELF_HANDLE_rundown(
    IELF_HANDLE    ElfHandle
    )

/*++

Routine Description:

    This routine is called by the server RPC runtime to run down a
    Context Handle and to free any allocated data.  It also does all
    the work for ElfrCloseEL.

    It has to undo whatever was done in ElfrOpenEventLog in terms of
    allocating memory.

Arguments:

    None.

Return Value:

--*/

{
    PLOGMODULE pModule;
    NTSTATUS Status;
    PNOTIFIEE Notifiee;
    PNOTIFIEE NotifieeToDelete;

    //
    // Generate an Audit if neccessary
    //
    ElfpCloseAudit(L"EventLog",ElfHandle);

    //
    // If the eventlog service is stopped or in the process of
    // stopping, then we just want to ignore this rundown and return.
    //
    // Note, we don't bother calling GetElState() because that uses
    // a critical section which may not exist anymore is the
    // eventlog service has been shutdown.
    //
    // The eventlog isn't designed to be shutdown (except when the
    // system is shutdown), so it isn't real good at cleaning up
    // its resources.
    //
    if (ElState == STOPPING || ElState == STOPPED) {
        return;
    }

    ElfDbgPrint(( "[ELF] Run down context handle - 0x%lx\n", ElfHandle ));

    if (ElfHandle->Signature != ELF_CONTEXTHANDLE_SIGN) {
        ElfDbgPrint (("[ELF] Invalid context handle in rundown routine.\n"));
        return;
    }

    pModule = FindModuleStrucFromAtom(ElfHandle->Atom);

    //
    // This shouldn't ever happen.  It means that a handle got created,
    // and it's module went away without the handle getting closed.
    //

    if (!pModule) {
        ElfDbgPrint(("[ELF] - Could not find module for Atom %d on close\n",
            ElfHandle->Atom));
        return;
    }

    UnlinkContextHandle (ElfHandle);    // Unlink it from the linked list

    //
    // If this handle was for a backup module, then we need to
    // close the file and clean up the data structures.  The standard logs
    // (application, system and security) are never freed.
    //

    if (ElfHandle->Flags & ELF_LOG_HANDLE_BACKUP_LOG) {

        Status = ElfpCloseLogFile (pModule->LogFile, ELF_LOG_CLOSE_BACKUP);

        UnlinkLogModule(pModule);
        DeleteAtom(pModule->ModuleAtom);
        ElfpFreeBuffer(pModule->ModuleName);

        // ElfpCloseLogFile decrements the RefCount.

        if (pModule->LogFile->RefCount == 0) {
            UnlinkLogFile(pModule->LogFile);
            RtlDeleteResource ( &pModule->LogFile->Resource );
            RtlDeleteSecurityObject(&pModule->LogFile->Sd);
            ElfpFreeBuffer (pModule->LogFile->LogFileName);
            ElfpFreeBuffer (pModule->LogFile->LogModuleName);
            ElfpFreeBuffer (pModule->LogFile);
        }
        ElfpFreeBuffer(pModule);

    }
    else {

        //
        // See if this handle had a ElfChangeNotify outstanding, and if so,
        // remove it from the list.  ElfChangeNotify can't be called with a
        // handle to a backup file.

        //
        // Get exclusive access to the log file. This will ensure no one
        // else is accessing the file.
        //

        RtlAcquireResourceExclusive (
                        &pModule->LogFile->Resource,
                        TRUE                    // Wait until available
                        );


        //
        // Walk the linked list and remove any entries for this handle
        //

        Notifiee = CONTAINING_RECORD (
                            pModule->LogFile->Notifiees.Flink,
                            struct _NOTIFIEE,
                            Next
                            );


        while (Notifiee->Next.Flink != pModule->LogFile->Notifiees.Flink) {

            //
            // If it's for this handle, remove it from the list
            //

            if (Notifiee->Handle == ElfHandle) {

                RemoveEntryList(&Notifiee->Next);
                NtClose(Notifiee->Event);
                NotifieeToDelete = Notifiee;

                Notifiee = CONTAINING_RECORD(
                            Notifiee->Next.Flink,
                            struct _NOTIFIEE,
                            Next);

                ElfpFreeBuffer(NotifieeToDelete);
            }
            else {

                Notifiee = CONTAINING_RECORD (
                                    Notifiee->Next.Flink,
                                    struct _NOTIFIEE,
                                    Next
                                    );
            }
        }

        //
        // Free the resource
        //

        RtlReleaseResource ( &pModule->LogFile->Resource );
    }

    ElfpFreeBuffer (ElfHandle);    // Free the context-handle structure

    return;
}
