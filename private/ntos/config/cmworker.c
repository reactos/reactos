/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmworker.c

Abstract:

    This module contains support for the worker thread of the registry.
    The worker thread (actually an executive worker thread is used) is
    required for operations that must take place in the context of the
    system process.  (particularly file I/O)

Author:

    John Vert (jvert) 21-Oct-1992

Revision History:

--*/

#include    "cmp.h"

extern  LIST_ENTRY  CmpHiveListHead;

VOID
CmpInitializeHiveList(
    VOID
    );

//
// ----- LAZY FLUSH CONTROL -----
//
// LAZY_FLUSH_INTERVAL_IN_SECONDS controls how many seconds will elapse
// between when the hive is marked dirty and when the lazy flush worker
// thread is queued to write the data to disk.
//
#define LAZY_FLUSH_INTERVAL_IN_SECONDS  5

//
// LAZY_FLUSH_TIMEOUT_IN_SECONDS controls how long the lazy flush worker
// thread will wait for the registry lock before giving up and queueing
// the lazy flush timer again.
//
#define LAZY_FLUSH_TIMEOUT_IN_SECONDS 1

#define SECOND_MULT 10*1000*1000        // 10->mic, 1000->mil, 1000->second

PKPROCESS   CmpSystemProcess;
KTIMER      CmpLazyFlushTimer;
KDPC        CmpLazyFlushDpc;
WORK_QUEUE_ITEM CmpLazyWorkItem;

BOOLEAN CmpLazyFlushPending = FALSE;

extern BOOLEAN CmpNoWrite;
extern BOOLEAN CmpWasSetupBoot;
extern BOOLEAN HvShutdownComplete;
extern BOOLEAN CmpProfileLoaded;

//
// Indicate whether the "disk full" popup has been triggered yet or not.
//
extern BOOLEAN CmpDiskFullWorkerPopupDisplayed;

//
// set to true if disk full when trying to save the changes made between system hive loading and registry initalization
//
extern BOOLEAN CmpCannotWriteConfiguration;

#if DBG
PKTHREAD    CmpCallerThread = NULL;
#endif

//
// Local function prototypes
//

VOID
CmpLazyFlushWorker(
    IN PVOID Parameter
    );

VOID
CmpLazyFlushDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
CmpDiskFullWarningWorker(
    IN PVOID WorkItem
    );

VOID
CmpDiskFullWarning(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpWorker)
#pragma alloc_text(PAGE,CmpLazyFlush)
#pragma alloc_text(PAGE,CmpLazyFlushWorker)
#pragma alloc_text(PAGE,CmpDiskFullWarningWorker)
#pragma alloc_text(PAGE,CmpDiskFullWarning)
#endif



VOID
CmpWorker(
    IN PREGISTRY_COMMAND CommandArea
    )
/*++

Routine Description:

    Actually execute the command specified in CommandArea, and
    report its completion.

Arguments:

    Parameter - supplies a pointer to the REGISTRY_COMMAND structure to
                be executed.

Return Value:

--*/
{
    NTSTATUS Status;
    PCMHIVE CmHive;
    IO_STATUS_BLOCK IoStatusBlock;
    PUNICODE_STRING FileName;
    ULONG i;
    HANDLE Handle;
    PFILE_RENAME_INFORMATION RenameInfo;
    PLIST_ENTRY p;
    BOOLEAN result;
    HANDLE NullHandle;

    PAGED_CODE();

    switch (CommandArea->Command) {

        case REG_CMD_INIT:
            //
            // Initialize lazy flush timer and DPC
            //
            KeInitializeDpc(&CmpLazyFlushDpc,
                            CmpLazyFlushDpcRoutine,
                            NULL);

            KeInitializeTimer(&CmpLazyFlushTimer);

            ExInitializeWorkItem(&CmpLazyWorkItem, CmpLazyFlushWorker, NULL);

            CmpNoWrite = FALSE;

            CmpWasSetupBoot = CommandArea->SetupBoot;
            if (CommandArea->SetupBoot == FALSE) {
                CmpInitializeHiveList();
            }

            //
            // flush dirty data to disk
            //
            CmpDoFlushAll();
            break;

        case REG_CMD_FLUSH_KEY:
            CommandArea->Status =
                CmFlushKey(CommandArea->Hive, CommandArea->Cell);
            break;

        case REG_CMD_REFRESH_HIVE:
            //
            // Refresh hive to match last flushed version
            //
            HvRefreshHive(CommandArea->Hive);
            break;

        case REG_CMD_FILE_SET_SIZE:
            CommandArea->Status = CmpDoFileSetSize(
                                    CommandArea->Hive,
                                    CommandArea->FileType,
                                    CommandArea->FileSize
                                    );
            break;

        case REG_CMD_HIVE_OPEN:

            //
            // Open the file.
            //
            FileName = CommandArea->FileAttributes->ObjectName;

            CommandArea->Status = CmpInitHiveFromFile(FileName,
                                                     0,
                                                     &CommandArea->CmHive,
                                                     &CommandArea->Allocate,
                                                     &CommandArea->RegistryLockAquired);
            //
            // NT Servers will return STATUS_ACCESS_DENIED. Netware 3.1x
            // servers could return any of the other error codes if the GUEST
            // account is disabled.
            //
            if (((CommandArea->Status == STATUS_ACCESS_DENIED) ||
                 (CommandArea->Status == STATUS_NO_SUCH_USER) ||
                 (CommandArea->Status == STATUS_WRONG_PASSWORD) ||
                 (CommandArea->Status == STATUS_ACCOUNT_EXPIRED) ||
                 (CommandArea->Status == STATUS_ACCOUNT_DISABLED) ||
                 (CommandArea->Status == STATUS_ACCOUNT_RESTRICTION)) &&
                (CommandArea->ImpersonationContext != NULL)) {
                //
                // Impersonate the caller and try it again.  This
                // lets us open hives on a remote machine.
                //
                Status = SeImpersonateClientEx(
                                CommandArea->ImpersonationContext,
                                NULL);

                if ( NT_SUCCESS( Status ) ) {

                    CommandArea->Status = CmpInitHiveFromFile(FileName,
                                                             0,
                                                             &CommandArea->CmHive,
                                                             &CommandArea->Allocate,
                                                             &CommandArea->RegistryLockAquired);
                    NullHandle = NULL;

                    PsRevertToSelf();
                }
            }

            break;

        case REG_CMD_HIVE_CLOSE:

            //
            // Close the files associated with this hive.
            //
            CmHive = CommandArea->CmHive;

            for (i=0; i<HFILE_TYPE_MAX; i++) {
                if (CmHive->FileHandles[i] != NULL) {
                    ZwClose(CmHive->FileHandles[i]);
                    CmHive->FileHandles[i] = NULL;
                }
            }
            CommandArea->Status = STATUS_SUCCESS;
            break;

        case REG_CMD_HIVE_READ:

            //
            // Used by special case of savekey, just do a read
            //
            result = CmpFileRead(
                        (PHHIVE)CommandArea->CmHive,
                        CommandArea->FileType,
                        CommandArea->Offset,
                        CommandArea->Buffer,
                        CommandArea->FileSize           // read length
                        );
            if (result) {
                CommandArea->Status = STATUS_SUCCESS;
            } else {
                CommandArea->Status = STATUS_REGISTRY_IO_FAILED;
            }
            break;

        case REG_CMD_SHUTDOWN:

            //
            // shut down the registry
            //
            CmpDoFlushAll();

            //
            // close all the hive files
            //
            p=CmpHiveListHead.Flink;
            while (p!=&CmpHiveListHead) {
                CmHive = CONTAINING_RECORD(p, CMHIVE, HiveList);
                for (i=0; i<HFILE_TYPE_MAX; i++) {
                    if (CmHive->FileHandles[i] != NULL) {
                        ZwClose(CmHive->FileHandles[i]);
                        CmHive->FileHandles[i] = NULL;
                    }
                }
                p=p->Flink;
            }

            break;

        case REG_CMD_RENAME_HIVE:
            //
            // Rename a CmHive's primary handle
            //
            Handle = CommandArea->CmHive->FileHandles[HFILE_TYPE_PRIMARY];
            if (CommandArea->OldName != NULL) {
                ASSERT_PASSIVE_LEVEL();
                Status = ZwQueryObject(Handle,
                                       ObjectNameInformation,
                                       CommandArea->OldName,
                                       CommandArea->NameInfoLength,
                                       &CommandArea->NameInfoLength);
                if (!NT_SUCCESS(Status)) {
                    CommandArea->Status = Status;
                    break;
                }
            }

            RenameInfo = ExAllocatePool(PagedPool,
                                        sizeof(FILE_RENAME_INFORMATION) +
                                        CommandArea->NewName->Length);
            if (RenameInfo == NULL) {
                CommandArea->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            RenameInfo->ReplaceIfExists = FALSE;
            RenameInfo->RootDirectory = NULL;
            RenameInfo->FileNameLength = CommandArea->NewName->Length;
            RtlMoveMemory(RenameInfo->FileName,
                          CommandArea->NewName->Buffer,
                          CommandArea->NewName->Length);

            Status = ZwSetInformationFile(Handle,
                                          &IoStatusBlock,
                                          (PVOID)RenameInfo,
                                          sizeof(FILE_RENAME_INFORMATION) +
                                          CommandArea->NewName->Length,
                                          FileRenameInformation);
            ExFreePool(RenameInfo);
            CommandArea->Status = Status;
            break;

        case REG_CMD_ADD_HIVE_LIST:
            //
            // Add a hive to the hive file list
            //
            Status = CmpAddToHiveFileList(CommandArea->CmHive);
            CommandArea->Status = Status;
            break;

        case REG_CMD_REMOVE_HIVE_LIST:
            //
            // Remove a hive from the hive file list
            //
            CmpRemoveFromHiveFileList(CommandArea->CmHive);
            CommandArea->Status = STATUS_SUCCESS;
            break;

        default:
            KeBugCheckEx(REGISTRY_ERROR,6,1,0,0);

    } // switch

    return;
}


VOID
CmpLazyFlush(
    VOID
    )

/*++

Routine Description:

    This routine resets the registry timer to go off at a specified interval
    in the future (LAZY_FLUSH_INTERVAL_IN_SECONDS).

Arguments:

    None

Return Value:

    None.

--*/

{
    LARGE_INTEGER DueTime;

    PAGED_CODE();
    CMLOG(CML_FLOW, CMS_IO) {
        KdPrint(("CmpLazyFlush: setting lazy flush timer\n"));
    }
    if (!CmpNoWrite) {

        DueTime.QuadPart = Int32x32To64(LAZY_FLUSH_INTERVAL_IN_SECONDS,
                                        - SECOND_MULT);

        //
        // Indicate relative time
        //

        KeSetTimer(&CmpLazyFlushTimer,
                   DueTime,
                   &CmpLazyFlushDpc);

    }


}


VOID
CmpLazyFlushDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the DPC routine triggered by the lazy flush timer.  All it does
    is queue a work item to an executive worker thread.  The work item will
    do the actual lazy flush to disk.

Arguments:

    Dpc - Supplies a pointer to the DPC object.

    DeferredContext - not used

    SystemArgument1 - not used

    SystemArgument2 - not used

Return Value:

    None.

--*/

{
    CMLOG(CML_FLOW, CMS_IO) {
        KdPrint(("CmpLazyFlushDpc: queuing lazy flush work item\n"));
    }

    if (!CmpLazyFlushPending) {
        CmpLazyFlushPending = TRUE;
        ExQueueWorkItem(&CmpLazyWorkItem, DelayedWorkQueue);
    }

}


VOID
CmpLazyFlushWorker(
    IN PVOID Parameter
    )

/*++

Routine Description:

    Worker routine called to do a lazy flush.  Called by an executive worker
    thread in the system process.  

Arguments:

    Parameter - not used.

Return Value:

    None.

--*/

{
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    CMLOG(CML_FLOW, CMS_IO) {
        KdPrint(("CmpLazyFlushWorker: flushing hives\n"));
    }

    CmpLockRegistry();
    if (!HvShutdownComplete) {
        Result = CmpDoFlushAll();
    }
    CmpLazyFlushPending = FALSE;
    CmpUnlockRegistry();

    if( CmpCannotWriteConfiguration ) {
        //
        // Disk full; system hive haven't been save at initialization
        //
        if(Result) {
            //
            // All hives were saved; No need for disk full warning anymore
            //
            CmpCannotWriteConfiguration = FALSE;
        } else {
            //
            // Issue another hard error (if not already displayed) and postpone a lazy flush operation
            //
            CmpDiskFullWarning();
            CmpLazyFlush();
        }
    }
}

VOID
CmpDiskFullWarningWorker(
    IN PVOID WorkItem
    )

/*++

Routine Description:

    Displays hard error popup that indicates the disk is full.

Arguments:

    WorkItem - Supplies pointer to the work item. This routine will
               free the work item.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG Response;

    ExFreePool(WorkItem);

    Status = ExRaiseHardError(STATUS_DISK_FULL,
                              0,
                              0,
                              NULL,
                              OptionOk,
                              &Response);
}



VOID
CmpDiskFullWarning(
    VOID
    )
/*++

Routine Description:

    Raises a hard error of type STATUS_DISK_FULL if wasn't already raised

Arguments:

    None

Return Value:

    None

--*/
{
    PWORK_QUEUE_ITEM WorkItem;

    if( (!CmpDiskFullWorkerPopupDisplayed) && (CmpCannotWriteConfiguration) && (ExReadyForErrors) && (CmpProfileLoaded) ) {

        //
        // Queue work item to display popup
        //
        WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
        if (WorkItem != NULL) {

            CmpDiskFullWorkerPopupDisplayed = TRUE;
            ExInitializeWorkItem(WorkItem,
                                 CmpDiskFullWarningWorker,
                                 WorkItem);
            ExQueueWorkItem(WorkItem, DelayedWorkQueue);
        }
    }
}
