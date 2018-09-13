/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmwraper.c

Abstract:

    Provides wrapper routines to support ntos\config routines called from
    user-mode.

Author:

    John Vert (jvert) 26-Mar-1992

Revision History:

--*/
#include "edithive.h"
#include "nturtl.h"
#include "stdlib.h"
#include "stdio.h"

extern  ULONG   UsedStorage;

CCHAR KiFindFirstSetRight[256] = {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};


ULONG   MmSizeOfPagedPoolInBytes = 0xffffffff;  // stub out

ULONG
DbgPrint (
    IN PCH Format,
    ...
    )
{
    va_list arglist;
    UCHAR Buffer[512];
    STRING Output;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    Output.Length = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    Output.Buffer = Buffer;
    printf("%s", Buffer);
    return 0;
}


//
// Structure that describes the mapping of generic access rights to object
// specific access rights for registry key and keyroot objects.
//

GENERIC_MAPPING CmpKeyMapping = {
    KEY_READ,
    KEY_WRITE,
    KEY_EXECUTE,
    KEY_ALL_ACCESS
};
BOOLEAN CmpNoWrite = FALSE;
ULONG CmLogLevel=0;
ULONG CmLogSelect=0;
PCMHIVE CmpMasterHive = NULL;
LIST_ENTRY CmpHiveListHead;            // List of CMHIVEs

NTSTATUS
MyCmpInitHiveFromFile(
    IN PUNICODE_STRING FileName,
    OUT PCMHIVE *CmHive,
    OUT PBOOLEAN Allocate
    );


VOID
CmpLazyFlush(
    VOID
    )
{
}


VOID
CmpFreeSecurityDescriptor(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )
{
    return;
}

VOID
CmpReportNotify(
    UNICODE_STRING          Name,
    PHHIVE                  Hive,
    HCELL_INDEX             Cell,
    ULONG                   Filter
    )
{
}

VOID
CmpLockRegistry(VOID)
{
    return;
}

BOOLEAN
CmpTryLockRegistryExclusive(
    IN BOOLEAN CanWait
    )
{
    return TRUE;
}

VOID
CmpUnlockRegistry(
    )
{
}

BOOLEAN
CmpTestRegistryLock()
{
    return TRUE;
}

BOOLEAN
CmpTestRegistryLockExclusive()
{
    return TRUE;
}
LONG
KeReleaseMutex (
    IN PKMUTEX Mutex,
    IN BOOLEAN Wait
    )
{
    return(0);
}
NTSTATUS
KeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    return(STATUS_SUCCESS);
}

BOOLEAN
CmpValidateHiveSecurityDescriptors(
    IN PHHIVE Hive
    )
{
    PCM_KEY_NODE RootNode;
    PCM_KEY_SECURITY SecurityCell;
    HCELL_INDEX ListAnchor;
    HCELL_INDEX NextCell;
    HCELL_INDEX LastCell;
    BOOLEAN ValidHive = TRUE;

    CMLOG(CML_FLOW, CMS_SEC) {
        KdPrint(("CmpValidateHiveSecurityDescriptor: Hive = %lx\n",(ULONG)Hive));
    }
    RootNode = (PCM_KEY_NODE) HvGetCell(Hive, Hive->BaseBlock->RootCell);
    ListAnchor = NextCell = RootNode->u1.s1.Security;

    do {
        SecurityCell = (PCM_KEY_SECURITY) HvGetCell(Hive, NextCell);
        if (NextCell != ListAnchor) {
            //
            // Check to make sure that our Blink points to where we just
            // came from.
            //
            if (SecurityCell->Blink != LastCell) {
                CMLOG(CML_MAJOR, CMS_SEC) {
                    KdPrint(("  Invalid Blink (%ld) on security cell %ld\n",SecurityCell->Blink, NextCell));
                    KdPrint(("  should point to %ld\n", LastCell));
                }
                ValidHive = FALSE;
            }
        }
        CMLOG(CML_MINOR, CMS_SEC) {
            KdPrint(("CmpValidSD:  SD shared by %d nodes\n",SecurityCell->ReferenceCount));
        }
//        SetUsed(Hive, NextCell);
        LastCell = NextCell;
        NextCell = SecurityCell->Flink;
    } while ( NextCell != ListAnchor );
    return(TRUE);
}

VOID
KeBugCheck(
    IN ULONG BugCheckCode
    )
{
    printf("BugCheck: code = %08lx\n", BugCheckCode);
    exit(1);
}

VOID
KeBugCheckEx(
    IN ULONG BugCheckCode,
    IN ULONG Arg1,
    IN ULONG Arg2,
    IN ULONG Arg3,
    IN ULONG Arg4
    )
{
    printf("BugCheck: code = %08lx\n", BugCheckCode);
    printf("Args =%08lx %08lx %08lx %08lx\n", Arg1, Arg2, Arg3, Arg4);
    exit(1);
}


VOID
KeQuerySystemTime(
    OUT PLARGE_INTEGER SystemTime
    )
{
    NtQuerySystemTime(SystemTime);
}

#ifdef POOL_TAGGING
PVOID
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID   Address = NULL;
    ULONG   Size;
    NTSTATUS    status;

    Size = ROUND_UP(NumberOfBytes, HBLOCK_SIZE);
    status = NtAllocateVirtualMemory(
                NtCurrentProcess(),
                &Address,
                0,
                &Size,
                MEM_COMMIT,
                PAGE_READWRITE
                );
    if (!NT_SUCCESS(status)) {
        return NULL;
    }
    return Address;
}
#else

PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )
{
    PVOID   Address = NULL;
    ULONG   Size;
    NTSTATUS    status;

    Size = ROUND_UP(NumberOfBytes, HBLOCK_SIZE);
    status = NtAllocateVirtualMemory(
                NtCurrentProcess(),
                &Address,
                0,
                &Size,
                MEM_COMMIT,
                PAGE_READWRITE
                );
    if (!NT_SUCCESS(status)) {
        return NULL;
    }
    return Address;
}
#endif

VOID
ExFreePool(
    IN PVOID P
    )
{
    ULONG   size;
    size = HBLOCK_SIZE;

    // if it was really more than 1 page, well, too bad
    NtFreeVirtualMemory(
        NtCurrentProcess(),
        &P,
        &size,
        MEM_DECOMMIT
        );
    return;
}


NTSTATUS
CmpWorkerCommand(
    IN OUT PREGISTRY_COMMAND Command
    )

/*++

Routine Description:

    This routine just encapsulates all the necessary synchronization for
    sending a command to the worker thread.

Arguments:

    Command - Supplies a pointer to an initialized REGISTRY_COMMAND structure
            which will be copied into the global communication structure.

Return Value:

    NTSTATUS = Command.Status

--*/

{
    PCMHIVE CmHive;
    PUNICODE_STRING FileName;
    ULONG i;

    switch (Command->Command) {

        case REG_CMD_FLUSH_KEY:
            return CmFlushKey(Command->Hive, Command->Cell);
            break;

        case REG_CMD_FILE_SET_SIZE:
            return CmpDoFileSetSize(
                      Command->Hive,
                      Command->FileType,
                      Command->FileSize
                      );
            break;

        case REG_CMD_HIVE_OPEN:

            //
            // Open the file.
            //
            FileName = Command->FileAttributes->ObjectName;

            return MyCmpInitHiveFromFile(FileName,
                                         &Command->CmHive,
                                         &Command->Allocate);

            break;

        case REG_CMD_HIVE_CLOSE:

            //
            // Close the files associated with this hive.
            //
            CmHive = Command->CmHive;

            for (i=0; i<HFILE_TYPE_MAX; i++) {
                if (CmHive->FileHandles[i] != NULL) {
                    NtClose(CmHive->FileHandles[i]);
                }
            }
            return STATUS_SUCCESS;
            break;

        case REG_CMD_SHUTDOWN:

            //
            // shut down the registry
            //
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS
MyCmpInitHiveFromFile(
    IN PUNICODE_STRING FileName,
    OUT PCMHIVE *CmHive,
    OUT PBOOLEAN Allocate
    )

/*++

Routine Description:

    This routine opens a file and log, allocates a CMHIVE, and initializes
    it.

Arguments:

    FileName - Supplies name of file to be loaded.

    CmHive   - Returns pointer to initialized hive (if successful)

    Allocate - Returns whether the hive was allocated or existing.

Return Value:

    NTSTATUS

--*/

{
    PCMHIVE NewHive;
    ULONG Disposition;
    ULONG SecondaryDisposition;
    HANDLE PrimaryHandle;
    HANDLE LogHandle;
    NTSTATUS Status;
    ULONG FileType;
    ULONG Operation;

    BOOLEAN Success;

    *CmHive = NULL;

    Status = CmpOpenHiveFiles(FileName,
                              L".log",
                              &PrimaryHandle,
                              &LogHandle,
                              &Disposition,
                              &SecondaryDisposition,
                              TRUE,
                              NULL
                              );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    if (LogHandle == NULL) {
        FileType = HFILE_TYPE_PRIMARY;
    } else {
        FileType = HFILE_TYPE_LOG;
    }

    if (Disposition == FILE_CREATED) {
        Operation = HINIT_CREATE;
        *Allocate = TRUE;
    } else {
        Operation = HINIT_FILE;
        *Allocate = FALSE;
    }

    Success = CmpInitializeHive(&NewHive,
                                Operation,
                                FALSE,
                                FileType,
                                NULL,
                                PrimaryHandle,
                                NULL,
                                LogHandle,
                                NULL,
                                NULL);
    if (!Success) {
        NtClose(PrimaryHandle);
        if (LogHandle != NULL) {
            NtClose(LogHandle);
        }
        return(STATUS_REGISTRY_CORRUPT);
    } else {
        *CmHive = NewHive;
        return(STATUS_SUCCESS);
    }
}

NTSTATUS
CmpLinkHiveToMaster(
    PUNICODE_STRING LinkName,
    HANDLE RootDirectory,
    PCMHIVE CmHive,
    BOOLEAN Allocate,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    return( STATUS_SUCCESS );
}


BOOLEAN
CmpFileSetSize(
    PHHIVE      Hive,
    ULONG       FileType,
    ULONG       FileSize
    )
/*++

Routine Description:

    This routine sets the size of a file.  It must not return until
    the size is guaranteed, therefore, it does a flush.

    It is environment specific.

    This routine will force execution to the correct thread context.

Arguments:

    Hive - Hive we are doing I/O for

    FileType - which supporting file to use

    FileSize - 32 bit value to set the file's size to

Return Value:

    FALSE if failure
    TRUE if success

--*/
{
    NTSTATUS    status;

    status = CmpDoFileSetSize(Hive, FileType, FileSize);
    if (!NT_SUCCESS(status)) {
        CMLOG(CML_MAJOR, CMS_IO_ERROR) {
            KdPrint(("CmpFileSetSize:\n\t"));
            KdPrint(("Failure: status = %08lx ", status));
        }
        return FALSE;
    }

    return TRUE;
}
