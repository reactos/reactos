/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cminit.c

Abstract:

    This module contains init support for the CM level of the
    config manager/hive.

Author:

    Bryan M. Willman (bryanwi) 2-Apr-1992

Revision History:

--*/

#include    "cmp.h"

//
// Prototypes local to this module
//
NTSTATUS
CmpOpenFileWithExtremePrejudice(
    OUT PHANDLE Primary,
    IN POBJECT_ATTRIBUTES Obja,
    IN ULONG IoFlags,
    IN ULONG AttributeFlags
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpOpenHiveFiles)
#pragma alloc_text(PAGE,CmpInitializeHive)
#pragma alloc_text(PAGE,CmpDestroyHive)
#pragma alloc_text(PAGE,CmpOpenFileWithExtremePrejudice)
#endif

extern PCMHIVE CmpMasterHive;
extern LIST_ENTRY CmpHiveListHead;

NTSTATUS
CmpOpenHiveFiles(
    PUNICODE_STRING     BaseName,
    PWSTR               Extension OPTIONAL,
    PHANDLE             Primary,
    PHANDLE             Secondary,
    PULONG              PrimaryDisposition,
    PULONG              SecondaryDisposition,
    BOOLEAN             CreateAllowed,
    BOOLEAN             MarkAsSystemHive,
    OUT OPTIONAL PULONG ClusterSize
    )
/*++

Routine Description:

    Open/Create Primary, Alternate, and Log files for Hives.

    BaseName is some name like "\winnt\system32\config\system".
    Extension is ".alt" or ".log" or NULL.

    If extension is NULL skip secondary work.

    If extension is .alt or .log, open/create a secondary file
    (e.g. "\winnt\system32\config\system.alt")

    If extension is .log, open secondary for buffered I/O, else,
    open for non-buffered I/O.  Primary always uses non-buffered I/O.

    If primary is newly created, supersede secondary.  If secondary
    does not exist, simply create (other code will complain if Log
    is needed but does not exist.)

    WARNING:    If Secondary handle is NULL, you have no log
                or alternate!

Arguments:

    BaseName - unicode string of base hive file, must have space for
                extension if that is used.

    Extension - unicode type extension of secondary file, including
                the leading "."

    Primary - will get handle to primary file

    Secondary - will get handle to secondary, or NULL

    PrimaryDisposition - STATUS_SUCCESS or STATUS_CREATED, of primary file.

    SecondaryDisposition - STATUS_SUCCESS or STATUS_CREATED, of secondary file.

    CreateAllowed - if TRUE will create nonexistent primary, if FALSE will
                    fail if primary does not exist.  no effect on log

    MarkAsSystemHive - if TRUE will call into file system to mark this
                       as a critical system hive.

    ClusterSize - if not NULL, will compute and return the appropriate
        cluster size for the primary file.

Return Value:

    status - if status is success, Primay succeeded, check Secondary
             value to see if it succeeded.

--*/
{
    IO_STATUS_BLOCK     IoStatus;
    IO_STATUS_BLOCK     FsctlIoStatus;
    FILE_FS_SIZE_INFORMATION FsSizeInformation;
    ULONG Cluster;
    ULONG               CreateDisposition;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    NTSTATUS            status;
    UNICODE_STRING      ExtName;
    UNICODE_STRING      WorkName;
    PVOID               WorkBuffer;
    USHORT              NameSize;
    ULONG               IoFlags;
    ULONG               AttributeFlags;
    USHORT              CompressionState;
    HANDLE              hEvent;
    PKEVENT             pEvent;

    //
    // Allocate an event to use for our overlapped I/O
    //
    status = CmpCreateEvent(NotificationEvent, &hEvent, &pEvent);
    if (!NT_SUCCESS(status)) {
        return(status);
    }
    
    //
    // Allocate a buffer big enough to hold the full name
    //
    WorkName.Length = 0;
    WorkName.MaximumLength = 0;
    WorkName.Buffer = NULL;
    WorkBuffer = NULL;

    NameSize = BaseName->Length;
    if (ARGUMENT_PRESENT(Extension)) {
        NameSize += (wcslen(Extension)+1) * sizeof(WCHAR);
        WorkBuffer = ExAllocatePool(PagedPool, NameSize);
        WorkName.Buffer = WorkBuffer;
        if (WorkBuffer == NULL) {
            ObDereferenceObject(pEvent);
            ZwClose(hEvent);
            return STATUS_NO_MEMORY;
        }
        WorkName.MaximumLength = NameSize;
        RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)BaseName);
    } else {
        WorkName = *BaseName;
    }


    //
    // Open/Create the primary
    //
    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    if (CreateAllowed) {
        CreateDisposition = FILE_OPEN_IF;
    } else {
        CreateDisposition = FILE_OPEN;
    }

    ASSERT_PASSIVE_LEVEL();

    status = ZwCreateFile(
                Primary,
                FILE_READ_DATA | FILE_WRITE_DATA,
                &ObjectAttributes,
                &IoStatus,
                NULL,                               // alloc size = none
                FILE_ATTRIBUTE_NORMAL,
                0,                                  // share nothing
                CreateDisposition,
                FILE_NO_INTERMEDIATE_BUFFERING | 
                FILE_OPEN_FOR_BACKUP_INTENT |
                FILE_NO_COMPRESSION,
                NULL,                               // eabuffer
                0                                   // ealength
                );
    if (status == STATUS_ACCESS_DENIED) {

        //
        // This means some foolish person has put a read-only attribute
        // on one of the critical system hive files. Remove it so they
        // don't hurt themselves.
        //

        status = CmpOpenFileWithExtremePrejudice(Primary,
                                                 &ObjectAttributes,
                                                 FILE_NO_INTERMEDIATE_BUFFERING |
                                                 FILE_OPEN_FOR_BACKUP_INTENT |
                                                 FILE_NO_COMPRESSION,
                                                 FILE_ATTRIBUTE_NORMAL);
    }

    if ((MarkAsSystemHive) &&
        (NT_SUCCESS(status))) {

        ASSERT_PASSIVE_LEVEL();
        status = ZwFsControlFile(*Primary,
                                 hEvent,
                                 NULL,
                                 NULL,
                                 &FsctlIoStatus,
                                 FSCTL_MARK_AS_SYSTEM_HIVE,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(pEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = FsctlIoStatus.Status;
        }

        //
        //  STATUS_INVALID_DEVICE_REQUEST is OK.
        //

        if (status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;

        } else if (!NT_SUCCESS(status)) {
            ZwClose(*Primary);
        }
    }

    if (!NT_SUCCESS(status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT_ERROR) {
            KdPrint(("CMINIT: CmpOpenHiveFile: "));
            KdPrint(("\tPrimary Open/Create failed for:\n"));
            KdPrint(("\t%wZ\n", &WorkName));
            KdPrint(("\tstatus = %08lx\n", status));
        }
        if (WorkBuffer != NULL) {
            ExFreePool(WorkBuffer);
        }
        ObDereferenceObject(pEvent);
        ZwClose(hEvent);
        return status;
    }

    //
    // Make sure the file is uncompressed in order to prevent the filesystem
    // from failing our updates due to disk full conditions.
    //
    // Do not fail to open the file if this fails, we don't want to prevent
    // people from booting just because their disk is full. Although they
    // will not be able to update their registry, they will at lease be
    // able to delete some files.
    //
    CompressionState = 0;
    ASSERT_PASSIVE_LEVEL();
    status = ZwFsControlFile(*Primary,
                             hEvent,
                             NULL,
                             NULL,
                             &FsctlIoStatus,
                             FSCTL_SET_COMPRESSION,
                             &CompressionState,
                             sizeof(CompressionState),
                             NULL,
                             0);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(pEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    *PrimaryDisposition = (ULONG) IoStatus.Information;

    if (ARGUMENT_PRESENT(ClusterSize)) {

        ASSERT_PASSIVE_LEVEL();
        status = ZwQueryVolumeInformationFile(*Primary,
                                              &IoStatus,
                                              &FsSizeInformation,
                                              sizeof(FILE_FS_SIZE_INFORMATION),
                                              FileFsSizeInformation);
        if (!NT_SUCCESS(status)) {
            ObDereferenceObject(pEvent);
            ZwClose(hEvent);
            return(status);
        }
        if (FsSizeInformation.BytesPerSector > HBLOCK_SIZE) {
            CMLOG(CML_BUGCHECK, CMS_INIT_ERROR) {
                KdPrint(("CmpOpenHiveFiles: sectorsize %lx > HBLOCK_SIZE\n"));
            }
            ObDereferenceObject(pEvent);
            ZwClose(hEvent);
            return(STATUS_CANNOT_LOAD_REGISTRY_FILE);
        }

        Cluster = FsSizeInformation.BytesPerSector / HSECTOR_SIZE;
        *ClusterSize = (Cluster < 1) ? 1 : Cluster;

    }

    if ( ! ARGUMENT_PRESENT(Extension)) {
        if (WorkBuffer != NULL) {
            ExFreePool(WorkBuffer);
        }
        ObDereferenceObject(pEvent);
        ZwClose(hEvent);
        return STATUS_SUCCESS;
    }

    //
    // Open/Create the secondary
    //
    CreateDisposition = FILE_OPEN_IF;
    if (*PrimaryDisposition == FILE_CREATED) {
        CreateDisposition = FILE_SUPERSEDE;
    }

    RtlInitUnicodeString(&ExtName,Extension);
    status = RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)&ExtName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &WorkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    IoFlags = FILE_NO_COMPRESSION;
    if (_wcsnicmp(Extension, L".log", 4) != 0) {
        IoFlags |= FILE_NO_INTERMEDIATE_BUFFERING;
        AttributeFlags = FILE_ATTRIBUTE_NORMAL;
    } else {
        AttributeFlags = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN;
    }

    ASSERT_PASSIVE_LEVEL();
    status = ZwCreateFile(
                Secondary,
                FILE_READ_DATA | FILE_WRITE_DATA,
                &ObjectAttributes,
                &IoStatus,
                NULL,                               // alloc size = none
                AttributeFlags,
                0,                                  // share nothing
                CreateDisposition,
                IoFlags,
                NULL,                               // eabuffer
                0                                   // ealength
                );

    if (status == STATUS_ACCESS_DENIED) {

        //
        // This means some foolish person has put a read-only attribute
        // on one of the critical system hive files. Remove it so they
        // don't hurt themselves.
        //

        status = CmpOpenFileWithExtremePrejudice(Secondary,
                                                 &ObjectAttributes,
                                                 IoFlags,
                                                 AttributeFlags);
    }

    if ((MarkAsSystemHive) &&
        (NT_SUCCESS(status))) {

        ASSERT_PASSIVE_LEVEL();
        status = ZwFsControlFile(*Secondary,
                                 hEvent,
                                 NULL,
                                 NULL,
                                 &FsctlIoStatus,
                                 FSCTL_MARK_AS_SYSTEM_HIVE,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(pEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = FsctlIoStatus.Status;
        }
        //
        //  STATUS_INVALID_DEVICE_REQUEST is OK.
        //

        if (status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;

        } else if (!NT_SUCCESS(status)) {

            ZwClose(*Secondary);
        }
    }

    if (!NT_SUCCESS(status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT_ERROR) {
            KdPrint(("CMINIT: CmpOpenHiveFile: "));
            KdPrint(("\tSecondary Open/Create failed for:\n"));
            KdPrint(("\t%wZ\n", &WorkName));
            KdPrint(("\tstatus = %08lx\n", status));
        }
        *Secondary = NULL;
    }

    *SecondaryDisposition = (ULONG) IoStatus.Information;

    //
    // Make sure the file is uncompressed in order to prevent the filesystem
    // from failing our updates due to disk full conditions.
    //
    // Do not fail to open the file if this fails, we don't want to prevent
    // people from booting just because their disk is full. Although they
    // will not be able to update their registry, they will at lease be
    // able to delete some files.
    //
    CompressionState = 0;

    ASSERT_PASSIVE_LEVEL();
    status = ZwFsControlFile(*Secondary,
                             hEvent,
                             NULL,
                             NULL,
                             &FsctlIoStatus,
                             FSCTL_SET_COMPRESSION,
                             &CompressionState,
                             sizeof(CompressionState),
                             NULL,
                             0);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(pEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    if (WorkBuffer != NULL) {
        ExFreePool(WorkBuffer);
    }
    ObDereferenceObject(pEvent);
    ZwClose(hEvent);
    return STATUS_SUCCESS;
}


NTSTATUS
CmpInitializeHive(
    PCMHIVE         *CmHive,
    ULONG           OperationType,
    ULONG           HiveFlags,
    ULONG           FileType,
    PVOID           HiveData OPTIONAL,
    HANDLE          Primary,
    HANDLE          Alternate,
    HANDLE          Log,
    HANDLE          External,
    PUNICODE_STRING FileName OPTIONAL
    )
/*++

Routine Description:

    Initialize a hive.

Arguments:

    CmHive - pointer to a variable to receive a pointer to the CmHive structure

    OperationType - specifies whether to create a new hive from scratch,
            from a memory image, or by reading a file from disk.
            [HINIT_CREATE | HINIT_MEMORY | HINIT_FILE ]

    HiveFlags - HIVE_VOLATILE - Entire hive is to be volatile, regardless
                                   of the types of cells allocated
                HIVE_NO_LAZY_FLUSH - Data in this hive is never written
                                   to disk except by an explicit FlushKey

    FileType - HFILE_TYPE_*, HFILE_TYPE_LOG or HFILE_TYPE_ALTERNATE set
            up for logging or alternate support respectively.

    HiveData - if present, supplies a pointer to an in memory image of
            from which to init the hive.  Only useful when OperationType
            is set to HINIT_MEMORY.

    Primary - File handle for primary hive file (e.g. SYSTEM)

    Alternate - File handle for alternate hive file (e.g. SYSTEM.ALT)

    Log - File handle for log hive file (e.g. SOFTWARE.LOG)

    External - File handle for primary hive file  (e.g.  BACKUP.REG)

    FileName - some path like "...\system32\config\system", which will
                be written into the base block as an aid to debugging.
                may be NULL.

Return Value:

    NTSTATUS

--*/
{
    FILE_FS_SIZE_INFORMATION    FsSizeInformation;
    IO_STATUS_BLOCK             IoStatusBlock;
    ULONG                       Cluster;
    NTSTATUS                    Status;
    PCMHIVE                     cmhive2;
    ULONG                       rc;

    CMLOG(CML_MAJOR, CMS_INIT) {
        KdPrint(("CmpInitializeHive:\t\n"));
    }

    //
    // Reject illegal parms
    //
    if ( (Alternate && Log)  ||
         (External && (Primary || Alternate || Log)) ||
         (Alternate && !Primary) ||
         (Log && !Primary) ||
         ((HiveFlags & HIVE_VOLATILE) && (Alternate || Primary || External || Log)) ||
         ((OperationType == HINIT_MEMORY) && (!ARGUMENT_PRESENT(HiveData))) ||
         (Log && (FileType != HFILE_TYPE_LOG)) ||
         (Alternate && (FileType != HFILE_TYPE_ALTERNATE))
       )
    {
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // compute control
    //
    if (Primary) {

        ASSERT_PASSIVE_LEVEL();
        Status = ZwQueryVolumeInformationFile(
                    Primary,
                    &IoStatusBlock,
                    &FsSizeInformation,
                    sizeof(FILE_FS_SIZE_INFORMATION),
                    FileFsSizeInformation
                    );
        if (!NT_SUCCESS(Status)) {
            return (Status);
        }
        if (FsSizeInformation.BytesPerSector > HBLOCK_SIZE) {
            return (STATUS_REGISTRY_IO_FAILED);
        }
        Cluster = FsSizeInformation.BytesPerSector / HSECTOR_SIZE;
        Cluster = (Cluster < 1) ? 1 : Cluster;
    } else {
        Cluster = 1;
    }

    cmhive2 = CmpAllocate(sizeof(CMHIVE), FALSE);
    if (cmhive2 == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Allocate the mutex from NonPagedPool so it will not be swapped to the disk
    //
    cmhive2->HiveLock = (PFAST_MUTEX)ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), CM_POOL_TAG );
    if( cmhive2->HiveLock == NULL ) {
        CmpFree(cmhive2, sizeof(CMHIVE));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize the Cm hive control block
    //
    //
    ASSERT((HFILE_TYPE_EXTERNAL+1) == HFILE_TYPE_MAX);
    cmhive2->FileHandles[HFILE_TYPE_PRIMARY] = Primary;
    cmhive2->FileHandles[HFILE_TYPE_ALTERNATE] = Alternate;
    cmhive2->FileHandles[HFILE_TYPE_LOG] = Log;
    cmhive2->FileHandles[HFILE_TYPE_EXTERNAL] = External;

    cmhive2->NotifyList.Flink = NULL;
    cmhive2->NotifyList.Blink = NULL;

    ExInitializeFastMutex(cmhive2->HiveLock);

    //
    // Initialize the Hv hive control block
    //
    Status = HvInitializeHive(
                &(cmhive2->Hive),
                OperationType,
                HiveFlags,
                FileType,
                HiveData,
                CmpAllocate,
                CmpFree,
                CmpFileSetSize,
                CmpFileWrite,
                CmpFileRead,
                CmpFileFlush,
                Cluster,
                FileName
                );
    if (!NT_SUCCESS(Status)) {
        CMLOG(CML_MAJOR, CMS_INIT_ERROR) {
            KdPrint(("CmpInitializeHive: "));
            KdPrint(("HvInitializeHive failed, Status = %08lx\n", Status));
        }
        ASSERT( cmhive2->HiveLock );
        ExFreePool(cmhive2->HiveLock);
        CmpFree(cmhive2, sizeof(CMHIVE));
        return (Status);
    }
    if ( (OperationType == HINIT_FILE) ||
         (OperationType == HINIT_MEMORY) ||
         (OperationType == HINIT_MEMORY_INPLACE))
    {
        rc = CmCheckRegistry(cmhive2, TRUE);
        if (rc != 0) {
            CMLOG(CML_MAJOR, CMS_INIT_ERROR) {
                KdPrint(("CmpInitializeHive: "));
                KdPrint(("CmCheckRegistry failed, rc = %08lx\n",rc));
            }
            //
            // in theory we should do this for MEMORY and MEMORY_INPLACE
            // as well, but they're only used at init time.
            //
            if (OperationType == HINIT_FILE) {
                HvFreeHive((PHHIVE)cmhive2);
            }
            ASSERT( cmhive2->HiveLock );
            ExFreePool(cmhive2->HiveLock);
            CmpFree(cmhive2, sizeof(CMHIVE));
            return(STATUS_REGISTRY_CORRUPT);
        }
    }

    InsertHeadList(&CmpHiveListHead, &(cmhive2->HiveList));
    *CmHive = cmhive2;
    return (STATUS_SUCCESS);
}


BOOLEAN
CmpDestroyHive(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )

/*++

Routine Description:

    This routine tears down a cmhive.

Arguments:

    Hive - Supplies a pointer to the hive to be freed.

    Cell - Supplies index of the hive's root cell.

Return Value:

    TRUE if successful
    FALSE if some failure occurred

--*/

{
    PCELL_DATA CellData;
    HCELL_INDEX LinkCell;
    NTSTATUS Status;

    //
    // First find the link cell.
    //
    CellData = HvGetCell(Hive, Cell);
    LinkCell = CellData->u.KeyNode.Parent;

    //
    // Now delete the link cell.
    //
    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);
    Status = CmpFreeKeyByCell((PHHIVE)CmpMasterHive, LinkCell, TRUE);

    if (NT_SUCCESS(Status)) {
        //
        // Take the hive out of the hive list
        //
        RemoveEntryList(&( ((PCMHIVE)Hive)->HiveList));
        return(TRUE);
    } else {
        return(FALSE);
    }
}


NTSTATUS
CmpOpenFileWithExtremePrejudice(
    OUT PHANDLE Primary,
    IN POBJECT_ATTRIBUTES Obja,
    IN ULONG IoFlags,
    IN ULONG AttributeFlags
    )

/*++

Routine Description:

    This routine opens a hive file that some foolish person has put a
    read-only attribute on. It is used to prevent people from hurting
    themselves by making the critical system hive files read-only.

Arguments:

    Primary - Returns handle to file

    Obja - Supplies Object Attributes of file.

    IoFlags - Supplies flags to pass to ZwCreateFile

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;

    //
    // Get the current file attributes
    //
    ASSERT_PASSIVE_LEVEL();
    Status = ZwQueryAttributesFile(Obja, &FileInfo);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Clear the readonly bit.
    //
    FileInfo.FileAttributes &= ~FILE_ATTRIBUTE_READONLY;

    //
    // Open the file
    //
    Status = ZwOpenFile(&Handle,
                        FILE_WRITE_ATTRIBUTES,
                        Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Set the new attributes
    //
    Status = ZwSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FileInfo),
                                  FileBasicInformation);
    ZwClose(Handle);
    if (NT_SUCCESS(Status)) {
        //
        // Reopen the file with the access that we really need.
        //
        Status = ZwCreateFile(Primary,
                              FILE_READ_DATA | FILE_WRITE_DATA,
                              Obja,
                              &IoStatusBlock,
                              NULL,
                              AttributeFlags,
                              0,
                              FILE_OPEN,
                              IoFlags,
                              NULL,
                              0);
    }

    return(Status);

}
