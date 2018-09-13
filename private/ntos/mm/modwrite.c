/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    modwrite.c

Abstract:

    This module contains the modified page writer for memory management.

Author:

    Lou Perazzoli (loup) 10-Jun-1989
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

typedef enum _MODIFIED_WRITER_OBJECT {
    NormalCase,
    MappedPagesNeedWriting,
    ModifiedWriterMaximumObject
} MODIFIED_WRITER_OBJECT;

typedef struct _MMWORK_CONTEXT {
    LARGE_INTEGER Size;
    NTSTATUS Status;
    KEVENT Event;
} MMWORK_CONTEXT, *PMMWORK_CONTEXT;

typedef struct _MM_WRITE_CLUSTER {
    ULONG Count;
    ULONG StartIndex;
    ULONG Cluster[2 * (MM_MAXIMUM_DISK_IO_SIZE / PAGE_SIZE) + 1];
} MM_WRITE_CLUSTER, *PMM_WRITE_CLUSTER;

ULONG MmWriteAllModifiedPages;
LOGICAL MiFirstPageFileCreatedAndReady = FALSE;

#define ONEMB_IN_PAGES  ((1024 * 1024) / PAGE_SIZE)

NTSTATUS
MiCheckForCrashDump (
    PFILE_OBJECT File,
    IN ULONG FileNumber
    );

VOID
MiCrashDumpWorker (
    IN PVOID Context
    );

VOID
MiClusterWritePages (
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex,
    IN PMM_WRITE_CLUSTER WriteCluster,
    IN ULONG Size
    );

VOID
MiExtendPagingFileMaximum (
    IN ULONG PageFileNumber,
    IN PRTL_BITMAP NewBitmap
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtCreatePagingFile)
#pragma alloc_text(PAGE,MmGetPageFileInformation)
#pragma alloc_text(PAGE,MiModifiedPageWriter)
#pragma alloc_text(PAGE,MiCheckForCrashDump)
#pragma alloc_text(PAGE,MmGetCrashDumpInformation)
#pragma alloc_text(PAGE,MiCrashDumpWorker)
#pragma alloc_text(PAGELK,MiFlushAllPages)
#endif


PSECTION MmCrashDumpSection;

extern POBJECT_TYPE IoFileObjectType;
extern HANDLE PspInitialSystemProcessHandle;

LIST_ENTRY MmMappedPageWriterList;

KEVENT MmMappedPageWriterEvent;

KEVENT MmMappedFileIoComplete;

ULONG MmSystemShutdown;

SIZE_T MmOverCommit2;

SIZE_T MmPageFileFullExtendPages;

ULONG MmPageFileFullExtendCount;

#define MI_PAGEFILE_FULL_CHARGE 100

SIZE_T MiPageFileFullCharge;

LOGICAL MmPageFileFullPopupShown = FALSE;

ULONG MmModNoWriteInsert;

BOOLEAN MmSystemPageFileLocated;

NTSTATUS
MiCheckPageFileMapping (
    IN PFILE_OBJECT File
    );

VOID
MiInsertPageFileInList (
    VOID
    );

VOID
MiGatherMappedPages (
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
    );

VOID
MiGatherPagefilePages (
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
    );

VOID
MiPageFileFull (
    VOID
    );

LOGICAL
MiCauseOverCommitPopup(
    IN SIZE_T NumberOfPages,
    IN ULONG Extension
    );

#if DBG
ULONG_PTR MmPagingFileDebug[8192];
#endif

extern PFN_NUMBER MmMoreThanEnoughFreePages;

#define MINIMUM_PAGE_FILE_SIZE ((ULONG)(256*PAGE_SIZE))

VOID
MiModifiedPageWriterWorker (
    VOID
    );

SIZE_T
MiAttemptPageFileExtension (
    IN ULONG PageFileNumber,
    IN SIZE_T ExtendSize,
    IN SIZE_T Maximum
    );

NTSTATUS
MiCheckPageFilePath (
    PFILE_OBJECT FileObject
    )
{
    PIRP irp;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    KEVENT event;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;

    PAGED_CODE();

    //
    // Reference the file object here so that no special checks need be made
    // in I/O completion to determine whether or not to dereference the file
    // object.
    //

    ObReferenceObject( FileObject );

    //
    // Initialize the local event.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );
    ASSERT( irp != NULL );

    if (irp == NULL) {

        //
        // Don't dereference the file object, our caller will take care of that.
        //

        return STATUS_NO_MEMORY;
    }

    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = &event;
    irp->Flags = IRP_SYNCHRONOUS_API;
    irp->UserIosb = &localIoStatus;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE) NULL;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    irpSp->FileObject = FileObject;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->AssociatedIrp.SystemBuffer = NULL;
    // irp->Flags = 0;

    irpSp->Parameters.UsageNotification.InPath = TRUE;
    irpSp->Parameters.UsageNotification.Type = DeviceUsageTypePaging;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //

    IoQueueThreadIrp( irp );

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver( deviceObject, irp );

    //
    // Wait for the local event and copy the final status information
    // back to the caller.
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = localIoStatus.Status;
    }

    return status;
}


VOID
MiReleaseModifiedWriter (
    VOID
    )

/*++

Routine Description:

    Nonpagable wrapper to signal the modified writer when the first pagefile
    creation has completely finished.

--*/
 
{
    KIRQL OldIrql;
    LOCK_PFN (OldIrql);
    MiFirstPageFileCreatedAndReady = TRUE;
    UNLOCK_PFN (OldIrql);
}


NTSTATUS
NtCreatePagingFile (
    IN PUNICODE_STRING PageFileName,
    IN PLARGE_INTEGER MinimumSize,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG Priority OPTIONAL
    )

/*++

Routine Description:

    This routine opens the specified file, attempts to write a page
    to the specified file, and creates the necessary structures to
    use the file as a paging file.

    If this file is the first paging file, the modified page writer
    is started.

    This system service requires the caller to have SeCreatePagefilePrivilege.

Arguments:

    PageFileName - Supplies the fully qualified file name.

    MinimumSize - Supplies the starting size of the paging file.
                  This value is rounded up to the host page size.

    MaximumSize - Supplies the maximum number of bytes to write to the file.
                  This value is rounded up to the host page size.

    Priority - Supplies the relative priority of this paging file.

Return Value:

    tbs

--*/

{
    PFILE_OBJECT File;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES PagingFileAttributes;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING CapturedName;
    PWSTR CapturedBuffer;
    LARGE_INTEGER CapturedMaximumSize;
    LARGE_INTEGER CapturedMinimumSize;
    LARGE_INTEGER SpecifiedSize;
    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN Attached = FALSE;
    BOOLEAN HasPrivilege;
    HANDLE SystemProcess;
    FILE_FS_DEVICE_INFORMATION FileDeviceInfo;
    ULONG ReturnedLength;
    ULONG FinalStatus;
    ULONG PageFileNumber;
    ULONG NewMaxSizeInPages;
    ULONG NewMinSizeInPages;
    PMMPAGING_FILE FoundExisting;
    PRTL_BITMAP NewBitmap;
    PRTL_BITMAP OldBitmap;
    PDEVICE_OBJECT deviceObject;
    MMPAGE_FILE_EXPANSION PageExtend;

    DBG_UNREFERENCED_PARAMETER (Priority);

    PAGED_CODE();

    CapturedBuffer = NULL;

    if (MmNumberOfPagingFiles == MAX_PAGE_FILES) {

        //
        // The maximum number of paging files is already in use.
        //

        Status = STATUS_TOO_MANY_PAGING_FILES;
        goto ErrorReturn0;
    }

    PreviousMode = KeGetPreviousMode();

    try {

        if (PreviousMode != KernelMode) {

            //
            // Make sure the caller has the proper privilege to make
            // this call.
            //

            HasPrivilege = SeSinglePrivilegeCheck (SeCreatePagefilePrivilege,
                                                   PreviousMode
                                                   );

            if (!HasPrivilege) {

                Status = STATUS_PRIVILEGE_NOT_HELD;
                goto ErrorReturn0;
            }

            //
            // Probe arguments.
            //

            ProbeForRead( PageFileName, sizeof(*PageFileName), sizeof(UCHAR));
            ProbeForRead( MaximumSize, sizeof(LARGE_INTEGER), 4);
            ProbeForRead( MinimumSize, sizeof(LARGE_INTEGER), 4);
        }

        //
        // Capture arguments.
        //

        CapturedMinimumSize = *MinimumSize;

#if defined (_WIN64) || defined (_X86PAE_)
        if (CapturedMinimumSize.QuadPart < MINIMUM_PAGE_FILE_SIZE) {
            Status = STATUS_INVALID_PARAMETER_2;
            goto ErrorReturn0;
        }

        SpecifiedSize.QuadPart = (ROUND_TO_PAGES (CapturedMinimumSize.QuadPart)) >> PAGE_SHIFT;

        if (SpecifiedSize.HighPart != 0) {
            Status = STATUS_INVALID_PARAMETER_2;
            goto ErrorReturn0;
        }
#else
        if ((CapturedMinimumSize.HighPart != 0) ||
            (CapturedMinimumSize.LowPart < MINIMUM_PAGE_FILE_SIZE)) {
            Status = STATUS_INVALID_PARAMETER_2;
            goto ErrorReturn0;
        }
#endif

        CapturedMaximumSize = *MaximumSize;

#if defined (_WIN64) || defined (_X86PAE_)
        SpecifiedSize.QuadPart = (ROUND_TO_PAGES (CapturedMaximumSize.QuadPart)) >> PAGE_SHIFT;

        if (SpecifiedSize.HighPart != 0) {
            Status = STATUS_INVALID_PARAMETER_3;
            goto ErrorReturn0;
        }
#else
        if (CapturedMaximumSize.HighPart != 0) {
            Status = STATUS_INVALID_PARAMETER_3;
            goto ErrorReturn0;
        }
#endif

        if (CapturedMinimumSize.QuadPart > CapturedMaximumSize.QuadPart) {
            Status = STATUS_INVALID_PARAMETER_3;
            goto ErrorReturn0;
        }

        CapturedName = *PageFileName;
        CapturedName.MaximumLength = CapturedName.Length;

        if ((CapturedName.Length == 0) ||
            (CapturedName.Length > MAXIMUM_FILENAME_LENGTH )) {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto ErrorReturn0;
        }

        if (PreviousMode != KernelMode) {
            ProbeForRead (CapturedName.Buffer,
                          CapturedName.Length,
                          sizeof( UCHAR ));
        }

        CapturedBuffer = ExAllocatePoolWithTag (PagedPool,
                                                (ULONG)CapturedName.Length,
                                                '  mM');

        if (CapturedBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorReturn0;
        }

        //
        // Copy the string to the allocated buffer.
        //

        RtlMoveMemory (CapturedBuffer,
                       CapturedName.Buffer,
                       CapturedName.Length);

        //
        // Point the buffer to the string that was just copied.
        //

        CapturedName.Buffer = CapturedBuffer;

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        if (CapturedBuffer != NULL) {
            ExFreePool (CapturedBuffer);
        }

        Status = GetExceptionCode();
        goto ErrorReturn0;
    }

    //
    // Open a paging file and get the size.
    //

    InitializeObjectAttributes( &PagingFileAttributes,
                                &CapturedName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

#if defined (_WIN64) || defined (_X86PAE_)
    EndOfFileInformation.EndOfFile.QuadPart =
                                ROUND_TO_PAGES (CapturedMinimumSize.QuadPart);
#else
    EndOfFileInformation.EndOfFile.HighPart = 0;
#endif
    EndOfFileInformation.EndOfFile.LowPart = (ULONG)
                                ROUND_TO_PAGES (CapturedMinimumSize.LowPart);

    Status = IoCreateFile( &FileHandle,
                           FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
                           &PagingFileAttributes,
                           &IoStatus,
                           &CapturedMinimumSize,
                           FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                           FILE_SHARE_WRITE,
                           FILE_SUPERSEDE,
                           FILE_NO_INTERMEDIATE_BUFFERING | FILE_NO_COMPRESSION,
                           (PVOID) NULL,
                           0L,
                           CreateFileTypeNone,
                           (PVOID) NULL,
                           IO_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING );

    if (!NT_SUCCESS(Status)) {

        //
        // Treat this as an extension of an existing pagefile maximum -
        // and try to open rather than create the paging file specified.
        //

        Status = IoCreateFile( &FileHandle,
                           FILE_WRITE_DATA | SYNCHRONIZE,
                           &PagingFileAttributes,
                           &IoStatus,
                           &CapturedMinimumSize,
                           FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_NO_INTERMEDIATE_BUFFERING | FILE_NO_COMPRESSION,
                           (PVOID) NULL,
                           0L,
                           CreateFileTypeNone,
                           (PVOID) NULL,
                           IO_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING );

        if (!NT_SUCCESS(Status)) {

#if DBG
            if (Status != STATUS_DISK_FULL) {
                DbgPrint("MM MODWRITE: unable to open paging file %wZ - status = %X \n", &CapturedName, Status);
            }
#endif

            goto ErrorReturn1;
        }

        Status = ObReferenceObjectByHandle ( FileHandle,
                                             FILE_READ_DATA | FILE_WRITE_DATA,
                                             IoFileObjectType,
                                             KernelMode,
                                             (PVOID *)&File,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            goto ErrorReturn2;
        }

        FoundExisting = NULL;

        ExAcquireFastMutex (&MmPageFileCreationLock);

        for (PageFileNumber = 0; PageFileNumber < MmNumberOfPagingFiles; PageFileNumber += 1) {
            if (MmPagingFile[PageFileNumber]->File->SectionObjectPointer == File->SectionObjectPointer) {
                FoundExisting = MmPagingFile[PageFileNumber];
                break;
            }
        }

        if (FoundExisting == NULL) {
            Status = STATUS_NOT_FOUND;
            goto ErrorReturn4;
        }

        //
        // Check for increases in the minimum or the maximum paging file sizes.
        // Decreasing either paging file size on the fly is not allowed.
        //

        NewMaxSizeInPages = (ULONG)(CapturedMaximumSize.QuadPart >> PAGE_SHIFT);
        NewMinSizeInPages = (ULONG)(CapturedMinimumSize.QuadPart >> PAGE_SHIFT);

        if (FoundExisting->MinimumSize > NewMinSizeInPages) {
            Status = STATUS_INVALID_PARAMETER_2;
            goto ErrorReturn4;
        }

        if (FoundExisting->MaximumSize > NewMaxSizeInPages) {
            Status = STATUS_INVALID_PARAMETER_3;
            goto ErrorReturn4;
        }

        if (NewMaxSizeInPages > FoundExisting->MaximumSize) {

            //
            // Make sure that the pagefile increase doesn't cause the commit
            // limit (in pages) to wrap.  Currently this can only happen on
            // PAE systems where 16 pagefiles of 16TB (==256TB) is greater
            // than the 32-bit commit variable (max is 16TB).
            //

            if (MmTotalCommitLimitMaximum + (NewMaxSizeInPages - FoundExisting->MaximumSize) <= MmTotalCommitLimitMaximum) {
                Status = STATUS_INVALID_PARAMETER_3;
                goto ErrorReturn4;
            }

            //
            // Handle the increase to the maximum paging file size.
            //

            MiCreateBitMap (&NewBitmap, NewMaxSizeInPages, NonPagedPool);
    
            if (NewBitmap == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn4;
            }
    
            OldBitmap = FoundExisting->Bitmap;
    
            MiExtendPagingFileMaximum (PageFileNumber, NewBitmap);
    
            MiRemoveBitMap (&OldBitmap);
    
            //
            // We may be low on commitment and/or may have put a temporary
            // stopgate on things.  Clear up the logjam now by forcing an
            // extension and immediately returning it.
            //

            if (MmTotalCommittedPages + 100 > MmTotalCommitLimit) {
                if (MiChargeCommitment (200, NULL) == TRUE) {
                    MiReturnCommitment (200);
                }
            }
        }
    
        if (NewMinSizeInPages > FoundExisting->MinimumSize) {

            //
            // Handle the increase to the minimum paging file size.
            //

            if (NewMinSizeInPages > FoundExisting->Size) {

                //
                // Queue a message to the segment dereferencing / pagefile
                // extending thread to see if the page file can be extended.
                //
        
                PageExtend.RequestedExpansionSize = NewMinSizeInPages - FoundExisting->Size;
                PageExtend.Segment = NULL;
                PageExtend.PageFileNumber = PageFileNumber;
                KeInitializeEvent (&PageExtend.Event, NotificationEvent, FALSE);

                MiIssuePageExtendRequest (&PageExtend);
            }

            //
            // The current size is now greater than the new desired minimum.
            // Ensure subsequent contractions obey this new minimum.
            //

            if (FoundExisting->Size >= NewMinSizeInPages) {
                ASSERT (FoundExisting->Size >= FoundExisting->MinimumSize);
                ASSERT (NewMinSizeInPages >= FoundExisting->MinimumSize);
                FoundExisting->MinimumSize = NewMinSizeInPages;
            }
            else {

                //
                // The pagefile could not be expanded to handle the new minimum.
                // No easy way to undo any maximum raising that may have been
                // done as the space may have already been used, so just set
                // Status so our caller knows it didn't all go perfectly.
                //

                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        goto ErrorReturn4;
    }

    if (!NT_SUCCESS(IoStatus.Status)) {
        KdPrint(("MM MODWRITE: unable to open paging file %wZ - iosb %lx\n", &CapturedName, IoStatus.Status));
        Status = IoStatus.Status;
        goto ErrorReturn1;
    }

    //
    // Make sure that the pagefile increase doesn't cause the commit
    // limit (in pages) to wrap.  Currently this can only happen on
    // PAE systems where 16 pagefiles of 16TB (==256TB) is greater
    // than the 32-bit commit variable (max is 16TB).
    //

    if (MmTotalCommitLimitMaximum + (CapturedMaximumSize.QuadPart >> PAGE_SHIFT)
        <= MmTotalCommitLimitMaximum) {
        Status = STATUS_INVALID_PARAMETER_3;
        goto ErrorReturn2;
    }

    Status = ZwSetInformationFile (FileHandle,
                                   &IoStatus,
                                   &EndOfFileInformation,
                                   sizeof(EndOfFileInformation),
                                   FileEndOfFileInformation);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("MM MODWRITE: unable to set length of paging file %wZ status = %X \n",
                 &CapturedName, Status));
        goto ErrorReturn2;
    }

    if (!NT_SUCCESS(IoStatus.Status)) {
        KdPrint(("MM MODWRITE: unable to set length of paging file %wZ - iosb %lx\n",
                &CapturedName, IoStatus.Status));
        Status = IoStatus.Status;
        goto ErrorReturn2;
    }

    Status = ObReferenceObjectByHandle ( FileHandle,
                                         FILE_READ_DATA | FILE_WRITE_DATA,
                                         IoFileObjectType,
                                         KernelMode,
                                         (PVOID *)&File,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("MM MODWRITE: Unable to reference paging file - %wZ\n",
                 &CapturedName));
        goto ErrorReturn2;
    }

    //
    // Get the address of the target device object and ensure
    // the specified file is of a suitable type.
    //

    deviceObject = IoGetRelatedDeviceObject (File);

    if ((deviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM) &&
        (deviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM) &&
        (deviceObject->DeviceType != FILE_DEVICE_DFS_VOLUME) &&
        (deviceObject->DeviceType != FILE_DEVICE_DFS_FILE_SYSTEM)) {
            KdPrint(("MM MODWRITE: Invalid paging file type - %x\n",
                     deviceObject->DeviceType));
            Status = STATUS_UNRECOGNIZED_VOLUME;
            goto ErrorReturn3;
    }

    //
    // Make sure the specified file is not currently being used
    // as a mapped data file.
    //

    Status = MiCheckPageFileMapping (File);
    if (!NT_SUCCESS(Status)) {
        goto ErrorReturn3;
    }

    //
    // Make sure the volume is not a floppy disk.
    //

    Status = IoQueryVolumeInformation ( File,
                                        FileFsDeviceInformation,
                                        sizeof(FILE_FS_DEVICE_INFORMATION),
                                        &FileDeviceInfo,
                                        &ReturnedLength
                                      );

    if (FILE_FLOPPY_DISKETTE & FileDeviceInfo.Characteristics) {
        Status = STATUS_FLOPPY_VOLUME;
        goto ErrorReturn3;
    }

    //
    // Check with all of the drivers along the path to the file to ensure
    // that they are willing to follow the rules required of them and to
    // give them a chance to lock down code and data that needs to be locked.
    // If any of the drivers along the path refuses to participate, fail the
    // pagefile creation.
    //
    // BUGBUG: Failing the pagefile creation is commented out until the
    // storage drivers have been modified to correctly handle this request.
    //

    Status = MiCheckPageFilePath (File);
    if (!NT_SUCCESS(Status)) {
        KdPrint(( "MiCheckPageFilePath(%wZ) FAILED: %x\n", &CapturedName, Status ));
        //goto ErrorReturn3;
    }

    //
    // Acquire the global page file creation mutex.
    //

    ExAcquireFastMutex (&MmPageFileCreationLock);

    MmPagingFile[MmNumberOfPagingFiles] = ExAllocatePoolWithTag (NonPagedPool,
                                                        sizeof(MMPAGING_FILE),
                                                        '  mM');
    if (MmPagingFile[MmNumberOfPagingFiles] == NULL) {

        //
        // Allocate pool failed.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    RtlZeroMemory (MmPagingFile[MmNumberOfPagingFiles], sizeof(MMPAGING_FILE));
    MmPagingFile[MmNumberOfPagingFiles]->File = File;
    MmPagingFile[MmNumberOfPagingFiles]->Size = (ULONG)(
                                                CapturedMinimumSize.QuadPart
                                                >> PAGE_SHIFT);

    MmPagingFile[MmNumberOfPagingFiles]->MinimumSize =
                                      MmPagingFile[MmNumberOfPagingFiles]->Size;
    MmPagingFile[MmNumberOfPagingFiles]->FreeSpace =
                                      MmPagingFile[MmNumberOfPagingFiles]->Size - 1;

    MmPagingFile[MmNumberOfPagingFiles]->MaximumSize = (PFN_NUMBER)(
                                                CapturedMaximumSize.QuadPart >>
                                                PAGE_SHIFT);

    MmPagingFile[MmNumberOfPagingFiles]->PageFileNumber = MmNumberOfPagingFiles;

    //
    // Adjust the commit page limit to reflect the new page file space.
    //

    MmPagingFile[MmNumberOfPagingFiles]->Entry[0] = ExAllocatePoolWithTag (NonPagedPool,
                                            sizeof(MMMOD_WRITER_MDL_ENTRY) +
                                            MmModifiedWriteClusterSize *
                                            sizeof(PFN_NUMBER),
                                            '  mM');

    if (MmPagingFile[MmNumberOfPagingFiles]->Entry[0] == NULL) {

        //
        // Allocate pool failed.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn5;
    }

    RtlZeroMemory (MmPagingFile[MmNumberOfPagingFiles]->Entry[0],
                   sizeof(MMMOD_WRITER_MDL_ENTRY));

    MmPagingFile[MmNumberOfPagingFiles]->Entry[0]->PagingListHead =
                                          &MmPagingFileHeader;
    MmPagingFile[MmNumberOfPagingFiles]->Entry[0]->PagingFile =
                                          MmPagingFile[MmNumberOfPagingFiles];

    MmPagingFile[MmNumberOfPagingFiles]->Entry[1] = ExAllocatePoolWithTag (NonPagedPool,
                                            sizeof(MMMOD_WRITER_MDL_ENTRY) +
                                            MmModifiedWriteClusterSize *
                                            sizeof(PFN_NUMBER),
                                            '  mM');

    if (MmPagingFile[MmNumberOfPagingFiles]->Entry[1] == NULL) {

        //
        // Allocate pool failed.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn6;
    }

    RtlZeroMemory (MmPagingFile[MmNumberOfPagingFiles]->Entry[1],
                   sizeof(MMMOD_WRITER_MDL_ENTRY));

    MmPagingFile[MmNumberOfPagingFiles]->Entry[1]->PagingListHead =
                                          &MmPagingFileHeader;
    MmPagingFile[MmNumberOfPagingFiles]->Entry[1]->PagingFile =
                                          MmPagingFile[MmNumberOfPagingFiles];

    MmPagingFile[MmNumberOfPagingFiles]->PageFileName = CapturedName;

    MiCreateBitMap (&MmPagingFile[MmNumberOfPagingFiles]->Bitmap,
                    MmPagingFile[MmNumberOfPagingFiles]->MaximumSize,
                    NonPagedPool);

    if (MmPagingFile[MmNumberOfPagingFiles]->Bitmap == NULL) {

        //
        // Allocate pool failed.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn7;
    }

    RtlSetAllBits (MmPagingFile[MmNumberOfPagingFiles]->Bitmap);

    //
    // Set the first bit as 0 is an invalid page location, clear the
    // following bits.
    //

    RtlClearBits (MmPagingFile[MmNumberOfPagingFiles]->Bitmap,
                  1,
                  (ULONG)(MmPagingFile[MmNumberOfPagingFiles]->Size - 1));

    PageFileNumber = MmNumberOfPagingFiles;
    MiInsertPageFileInList ();

    FinalStatus = MiCheckForCrashDump (File, PageFileNumber);

    if (PageFileNumber == 0) {

        //
        // The first paging file has been created and reservation of any
        // crashdump pages has completed, signal the modified
        // page writer.
        //

        MiReleaseModifiedWriter ();
    }

    ExReleaseFastMutex (&MmPageFileCreationLock);

    //
    // Note that the file handle is not closed to prevent the
    // paging file from being deleted or opened again.  (Actually,
    // the file handle is duped to the system process so process
    // termination will not close the handle).
    //

    Status = ObOpenObjectByPointer(
                PsInitialSystemProcess,
                0,
                NULL,
                0,
                PsProcessType,
                KernelMode,
                &SystemProcess
                );

    if ( !NT_SUCCESS(Status)) {
        ZwClose (FileHandle);
        return FinalStatus;
    }

    Status = ZwDuplicateObject(
                NtCurrentProcess(),
                FileHandle,
                SystemProcess,
                NULL,
                0,
                0,
                DUPLICATE_SAME_ATTRIBUTES | DUPLICATE_SAME_ACCESS
                );

    ASSERT(NT_SUCCESS(Status));

    if (!MmSystemPageFileLocated) {
        MmSystemPageFileLocated = IoPageFileCreated(FileHandle);
    }

    ZwClose (SystemProcess);
    ZwClose (FileHandle);

    return FinalStatus;

    //
    // Error returns:
    //

ErrorReturn7:
    ExFreePool (MmPagingFile[MmNumberOfPagingFiles]->Entry[0]);

ErrorReturn6:
    ExFreePool (MmPagingFile[MmNumberOfPagingFiles]->Entry[1]);

ErrorReturn5:
    ExFreePool (MmPagingFile[MmNumberOfPagingFiles]);

ErrorReturn4:
    ExReleaseFastMutex (&MmPageFileCreationLock);

ErrorReturn3:
    ObDereferenceObject (File);

ErrorReturn2:
    ZwClose (FileHandle);

ErrorReturn1:
    ExFreePool (CapturedBuffer);

ErrorReturn0:
    return Status;
}


VOID
MiExtendPagingFileMaximum (
    IN ULONG PageFileNumber,
    IN PRTL_BITMAP NewBitmap
    )

/*++

Routine Description:

    This routine switches from the old bitmap to the new (larger) bitmap.

Arguments:

    PageFileNumber - Supplies the paging file number to be extended.

    NewBitmap - Supplies the new bitmap to use.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL, MmPageFileCreationLock held.

--*/

{
    KIRQL OldIrql;
    PRTL_BITMAP OldBitmap;

    OldBitmap = MmPagingFile[PageFileNumber]->Bitmap;

    RtlSetAllBits (NewBitmap);

    LOCK_PFN (OldIrql);

    //
    // Copy the bits from the existing map.
    //

    RtlCopyMemory (NewBitmap->Buffer,
                   OldBitmap->Buffer,
                   ((OldBitmap->SizeOfBitMap + 31) / 32) * sizeof (ULONG));

    MmTotalCommitLimitMaximum += (NewBitmap->SizeOfBitMap - OldBitmap->SizeOfBitMap);

    MmPagingFile[PageFileNumber]->MaximumSize = NewBitmap->SizeOfBitMap;

    MmPagingFile[PageFileNumber]->Bitmap = NewBitmap;

    //
    // If any MDLs are waiting for space, get them up now.
    //

    if (!IsListEmpty (&MmFreePagingSpaceLow)) {
        MiUpdateModifiedWriterMdls (PageFileNumber);
    }

    UNLOCK_PFN (OldIrql);
}


NTSTATUS
MiCheckForCrashDump (
    PFILE_OBJECT File,
    IN ULONG FileNumber
    )

/*++

Routine Description:

    This routine checks the first page of the paging file to
    determine if a crash dump exists.  If a crash dump is found
    a section is created which maps the crash dump.  A handle
    to the section is created via NtQuerySystemInformation specifying
    SystemCrashDumpInformation.

Arguments:

    File - Supplies a pointer to the file object for the paging file.

    FileNumber - Supplies the index into the paging file array.

Return Value:

    Returns STATUS_CRASH_DUMP if a crash dump exists, success otherwise.

--*/

{
    PMDL Mdl;
    LARGE_INTEGER Offset = {0,0};
    LARGE_INTEGER DumpSpaceUsed;
    PFN_NUMBER DumpSpaceUsedInPages;
    PULONG Block;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PFN_NUMBER j;
    PPFN_NUMBER Page;
    NTSTATUS FinalStatus;
    PMMPTE PointerPte;
    PMMPFN Pfn;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    WORK_QUEUE_ITEM WorkItem;
    MMWORK_CONTEXT Context;

    FinalStatus = STATUS_SUCCESS;

    Mdl = (PMDL)&MdlHack[0];
    MmCreateMdl( Mdl, NULL, PAGE_SIZE);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    Page = (PPFN_NUMBER)(Mdl + 1);
    *Page = MiGetPageForHeader ();
    Block = MmGetSystemAddressForMdlSafe (Mdl, HighPagePriority);

    if (Block == NULL) {
        MiRemoveImageHeaderPage(*Page);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent (&Context.Event, NotificationEvent, FALSE);

    Status = IoPageRead (File,
                         Mdl,
                         &Offset,
                         &Context.Event,
                         &IoStatus);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Context.Event,
                               WrPageIn,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    }

    KeClearEvent (&Context.Event);

    DumpSpaceUsed.LowPart = Block[DH_REQUIRED_DUMP_SPACE];
    DumpSpaceUsed.HighPart = Block[DH_REQUIRED_DUMP_SPACE + 1];

    DumpSpaceUsedInPages = (PFN_NUMBER)(DumpSpaceUsed.QuadPart >> PAGE_SHIFT);

    if ((Block[0] == 'EGAP') &&
        (Block[1] == 'PMUD') &&
        (DumpSpaceUsedInPages <= MmPagingFile[FileNumber]->Size)) {

        //
        // A crash dump already exists, don't let pager use
        // it and build named section for it.
        //

        Context.Size.QuadPart = DumpSpaceUsed.QuadPart;

        ExInitializeWorkItem(&WorkItem,
                             MiCrashDumpWorker,
                             (PVOID)&Context);

        ExQueueWorkItem( &WorkItem, DelayedWorkQueue );

        KeWaitForSingleObject( &Context.Event,
                               WrPageIn,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);

        KeClearEvent (&Context.Event);

        if (!NT_SUCCESS(Context.Status)) {
            goto Failed;
        }

        //
        // Make the section point to the paging file.
        //

        PointerPte = MmCrashDumpSection->Segment->PrototypePte;
        MI_WRITE_INVALID_PTE (PointerPte, MmCrashDumpSection->Segment->SegmentPteTemplate);

        Pfn = MI_PFN_ELEMENT (*Page);
#if PFN_CONSISTENCY
        MiSetModified (Pfn, 1);
#else
        Pfn->u3.e1.Modified = 1;
#endif

        PointerPte += 1;

        for (j = 1; j < DumpSpaceUsedInPages; j += 1) {

            MI_SET_PAGING_FILE_INFO (*PointerPte,
                                     MmCrashDumpSection->Segment->SegmentPteTemplate,
                                     FileNumber,
                                     j);

#if DBG
            if ((j < 8192) && (FileNumber == 0)) {
                ASSERT ((MmPagingFileDebug[j] & 1) == 0);
                MmPagingFileDebug[j] = (((ULONG_PTR)PointerPte << 3) | 1);
            }
#endif //DBG
            PointerPte += 1;
        }

        //
        // Change the original PTE contents to refer to
        // the paging file offset where this was written.
        //

        RtlSetBits (MmPagingFile[FileNumber]->Bitmap,
                    1,
                    (ULONG)DumpSpaceUsedInPages - 1);

        MmPagingFile[FileNumber]->FreeSpace -= DumpSpaceUsedInPages;
        MmPagingFile[FileNumber]->CurrentUsage += DumpSpaceUsedInPages;
        FinalStatus = STATUS_CRASH_DUMP;

Failed:

        //
        // Indicate that no crash dump is in file so if system is
        // rebooted the page file is available.
        //

        Block[1] = 'EGAP';
    } else {

        //
        // Set new pattern into file.
        //

        RtlFillMemoryUlong (Block,
                            PAGE_SIZE,
                            'EGAP');

#if !defined(_WIN64)

        //
        // This bit of code does not work on Win64.  Worse, it generates
        // alignment faults.  So, until it (and the other components that
        // deal with crashdump blocks) is fixed, it is disabled for Win64.
        //

        *(PULONG_PTR)(&Block[4]) = PsInitialSystemProcess->Pcb.DirectoryTableBase[0];
        *(PULONG *)(&Block[5]) = (PULONG)MmPfnDatabase;
        *(PLIST_ENTRY *)(&Block[6]) = (PLIST_ENTRY)&PsLoadedModuleList;
        *(PLIST_ENTRY *)(&Block[7]) = (PLIST_ENTRY)&PsActiveProcessHead;
#endif
        Block[8] =
#ifdef _X86_
                    IMAGE_FILE_MACHINE_I386;
#endif //_X86_

#ifdef _ALPHA_
                    IMAGE_FILE_MACHINE_ALPHA;
#endif //_ALPHA_

#ifdef _IA64_
                    IMAGE_FILE_MACHINE_IA64;
#endif //_IA64_

        ExAcquireFastMutex (&MmDynamicMemoryMutex);

        RtlCopyMemory (&Block[DH_PHYSICAL_MEMORY_BLOCK],
                       MmPhysicalMemoryBlock,
                       (sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
                        (sizeof(PHYSICAL_MEMORY_RUN) *
                        (MmPhysicalMemoryBlock->NumberOfRuns - 1))));

        ExReleaseFastMutex (&MmDynamicMemoryMutex);
    }

    Status = IoSynchronousPageWrite (
                           File,
                           Mdl,
                           &Offset,
                           &Context.Event,
                           &IoStatus );

    KeWaitForSingleObject (&Context.Event,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
    if (FinalStatus == STATUS_CRASH_DUMP) {

        //
        // Set the first page to point to the page that was just operated
        // upon.
        //

        MiUpdateImageHeaderPage (MmCrashDumpSection->Segment->PrototypePte,
                                 *Page,
                                 MmCrashDumpSection->Segment->ControlArea);
    } else {
        MiRemoveImageHeaderPage(*Page);
    }
    return FinalStatus;
}


VOID
MiCrashDumpWorker (
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called in the context of a delayed worker thread.
    Its function is to create a section which will be used to map the
    crash dump in the paging file.

Arguments:

    Context - Supplies the context record which contains the section's
              size, an event to set at completion and a status value
              to be returned.

Return Value:

    None.

--*/

{
    PMMWORK_CONTEXT Work;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    Work = (PMMWORK_CONTEXT)Context;

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0,
                                NULL,
                                NULL );


    Work->Status = MmCreateSection ( &MmCrashDumpSection,
                               SECTION_MAP_READ,
                               &ObjectAttributes,
                               &Work->Size,
                               PAGE_READONLY,
                               SEC_COMMIT,
                               NULL,
                               NULL );

    KeSetEvent (&Work->Event, 0, FALSE);
    return;
}


NTSTATUS
MmGetCrashDumpInformation (
    IN PSYSTEM_CRASH_DUMP_INFORMATION CrashInfo
    )

/*++

Routine Description:

    This function checks to see if a crash dump section exists and
    if so creates a handle to the section and returns that value
    in the CrashDumpInformation structure.  Once the handle to the
    section has been created, no other references can be made
    to the crash dump section, and when that handle is closed, the
    crash dump section is deleted and the paging file space is
    available for reuse.

Arguments:

    CrashInfo - Supplies a pointer to the crash dump information
                structure.

Return Value:

    Status of the operation.  A handle value of zero indicates no
    crash dump was located.

--*/

{
    NTSTATUS Status;
    HANDLE Handle;

    PAGED_CODE();

    if (MmCrashDumpSection == NULL) {
        Handle = 0;
        Status = STATUS_SUCCESS;
    } else {
        Status = ObInsertObject (MmCrashDumpSection,
                                 NULL,
                                 SECTION_MAP_READ,
                                 0,
                                 (PVOID *)NULL,
                                 &Handle);
        if (NT_SUCCESS(Status)) {

            //
            // One shot operation.
            //

            MmCrashDumpSection = NULL;
        }
    }

    CrashInfo->CrashDumpSection = Handle;
    return Status;
}


NTSTATUS
MmGetCrashDumpStateInformation (
    IN PSYSTEM_CRASH_STATE_INFORMATION CrashInfo
    )

/*++

Routine Description:

    This function checks to see if a crash dump section exists and
    returns a BOOLEAN value in the CrashStateInformation structure
    based on the outcome.

Arguments:

    CrashInfo - Supplies a pointer to the crash dump state information
                structure.

Return Value:

    Status of the operation.  A BOOLEAN value of FALSE indicates no
    crash dump was located.

--*/

{
    PAGED_CODE();

    CrashInfo->ValidCrashDump = (MmCrashDumpSection != NULL);
    return STATUS_SUCCESS;
}


SIZE_T
MiAttemptPageFileExtension (
    IN ULONG PageFileNumber,
    IN SIZE_T ExtendSize,
    IN SIZE_T Maximum
    )

/*++

Routine Description:

    This routine attempts to extend the specified page file by
    ExtendSize.

Arguments:

    PageFileNumber - Supplies the page file number to attempt to extend.

    ExtendSize - Supplies the number of pages to extend the file by.

    Maximum - Supplies TRUE if the page file should be extended
              by the maximum size possible, but not to exceed
              ExtendSize.

Return Value:

    Returns the size of the extension.  Zero if the page file cannot
    be extended.

--*/

{

    NTSTATUS status;
    FILE_FS_SIZE_INFORMATION FileInfo;
    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;
    KIRQL OldIrql;
    ULONG AllocSize;
    PFN_NUMBER AdditionalAllocation;
    ULONG ReturnedLength;
    PFN_NUMBER PagesAvailable;
    SIZE_T SizeToExtend;
    LARGE_INTEGER BytesAvailable;

    //
    // Check to see if this page file is at the maximum.
    //

    if (MmPagingFile[PageFileNumber]->Size ==
                                    MmPagingFile[PageFileNumber]->MaximumSize) {
        return 0;
    }

    //
    // Find out how much free space is on this volume.
    //

    status = IoQueryVolumeInformation ( MmPagingFile[PageFileNumber]->File,
                                        FileFsSizeInformation,
                                        sizeof(FileInfo),
                                        &FileInfo,
                                        &ReturnedLength
                                      );

    if (!NT_SUCCESS (status)) {

        //
        // The volume query did not succeed - return 0 indicating
        // the paging file was not extended.
        //

        return 0;
    }

    //
    // Always attempt to extend by at least megabyte.
    //

    SizeToExtend = ExtendSize;
    if (ExtendSize < MmPageFileExtension) {
        SizeToExtend = MmPageFileExtension;
    }

    //
    // Don't go over the maximum size for the paging file.
    //

    if ((SizeToExtend + MmPagingFile[PageFileNumber]->Size) >
                                       MmPagingFile[PageFileNumber]->MaximumSize) {
        SizeToExtend = (MmPagingFile[PageFileNumber]->MaximumSize -
                                MmPagingFile[PageFileNumber]->Size);
    }

    if ((Maximum == FALSE) && (SizeToExtend < ExtendSize)) {

        //
        // Can't meet the requirement.
        //

        return 0;
    }
    //
    // See if there is enough space on the volume for the extension.
    //

    AllocSize = FileInfo.SectorsPerAllocationUnit * FileInfo.BytesPerSector;

    BytesAvailable = RtlExtendedIntegerMultiply (
                        FileInfo.AvailableAllocationUnits,
                        AllocSize);

    if ((UINT64)BytesAvailable.QuadPart > (UINT64)MmMinimumFreeDiskSpace) {

        BytesAvailable.QuadPart = BytesAvailable.QuadPart -
                                    (LONGLONG)MmMinimumFreeDiskSpace;

        if ((UINT64)BytesAvailable.QuadPart > (UINT64)(SizeToExtend << PAGE_SHIFT)) {
            BytesAvailable.QuadPart = (LONGLONG)(SizeToExtend << PAGE_SHIFT);
        }

        PagesAvailable = (PFN_NUMBER)(BytesAvailable.QuadPart >> PAGE_SHIFT);

        if ((Maximum == FALSE) && (PagesAvailable < ExtendSize)) {

            //
            // Can't satisfy this requirement.
            //

            return 0;
        }

    } else {

        //
        // Not enough space is available.
        //

        return 0;
    }

#if defined (_WIN64) || defined (_X86PAE_)
    EndOfFileInformation.EndOfFile.QuadPart =
              ((ULONG64)MmPagingFile[PageFileNumber]->Size + PagesAvailable) * PAGE_SIZE;
#else
    EndOfFileInformation.EndOfFile.LowPart =
              (MmPagingFile[PageFileNumber]->Size + PagesAvailable) * PAGE_SIZE;

    //
    // Set high part to zero as paging files are limited to 4GB.
    //

    EndOfFileInformation.EndOfFile.HighPart = 0;
#endif

    //
    // Attempt to extend the file by setting the end-of-file position.
    //

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    status = IoSetInformation (MmPagingFile[PageFileNumber]->File,
                               FileEndOfFileInformation,
                               sizeof(FILE_END_OF_FILE_INFORMATION),
                               &EndOfFileInformation
                              );

    if (status != STATUS_SUCCESS) {
        KdPrint(("MM MODWRITE: page file extension failed %lx %lx\n",status));
        return 0;
    }

    //
    // Clear bits within the paging file bitmap to allow the extension
    // to take effect.
    //

    LOCK_PFN (OldIrql);

    ASSERT (RtlCheckBit (MmPagingFile[PageFileNumber]->Bitmap,
                         MmPagingFile[PageFileNumber]->Size) == 1);

    AdditionalAllocation = PagesAvailable;

    RtlClearBits (MmPagingFile[PageFileNumber]->Bitmap,
                  (ULONG)MmPagingFile[PageFileNumber]->Size,
                  (ULONG)AdditionalAllocation );

    MmPagingFile[PageFileNumber]->Size += AdditionalAllocation;
    MmPagingFile[PageFileNumber]->FreeSpace += AdditionalAllocation;

    MiUpdateModifiedWriterMdls (PageFileNumber);

    UNLOCK_PFN (OldIrql);

    return AdditionalAllocation;
}

SIZE_T
MiExtendPagingFiles (
    IN PMMPAGE_FILE_EXPANSION PageExpand
    )

/*++

Routine Description:

    This routine attempts to extend the paging files to provide
    ExtendSize bytes.

    Note - Page file expansion and page file reduction are synchronized
           because a single thread is responsible for performing the
           operation.  Hence, while expansion is occurring, a reduction
           request will be queued to the thread.

Arguments:

    DesiredQuota - Supplies the quota in pages desired.

    PageFileNumber - Supplies the page file number to extend.
                     MI_EXTEND_ANY_PAGFILE indicates to extend any page file.

Return Value:

    Returns the size of the extension.  Zero if the page file(s) cannot
    be extended.

--*/

{
    SIZE_T DesiredQuota;
    ULONG PageFileNumber;
    SIZE_T ExtendedSize;
    SIZE_T ExtendSize;
    ULONG i;
    KIRQL OldIrql;
    LOGICAL LockHeld;
    LOGICAL RealExpansion;

    RealExpansion = TRUE;
    LockHeld = FALSE;
    ExtendedSize = 0;
    DesiredQuota = PageExpand->RequestedExpansionSize;
    PageFileNumber = PageExpand->PageFileNumber;

    PageExpand->ActualExpansion = 0;

    ASSERT (PageFileNumber < MmNumberOfPagingFiles || PageFileNumber == MI_EXTEND_ANY_PAGEFILE);

    if (MmNumberOfPagingFiles == 0) {
        goto alldone;
    }

    if (PageFileNumber < MmNumberOfPagingFiles) {
        i = PageFileNumber;
        ExtendedSize = MmPagingFile[i]->MaximumSize - MmPagingFile[i]->Size;
        if (ExtendedSize < DesiredQuota) {
            ExtendedSize = 0;
        }
        else {
            ExtendedSize = MiAttemptPageFileExtension (i, DesiredQuota, FALSE);
        }
        goto alldone;
    }

    LockHeld = TRUE;
    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

    //
    // Check to see if ample space already exists now that we have
    // the spinlock.
    //

    ExtendSize = DesiredQuota + MmTotalCommittedPages;

    if (MmTotalCommitLimit >= ExtendSize) {
        ExtendedSize = 1;
        RealExpansion = FALSE;
        goto alldone;
    }

    //
    // Calculate the additional pages needed.
    //

    ExtendSize -= MmTotalCommitLimit;

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    LockHeld = FALSE;

    //
    // Make sure ample space exists within the paging files.
    //

    i = 0;

    do {
        ExtendedSize += MmPagingFile[i]->MaximumSize - MmPagingFile[i]->Size;
        i += 1;
    } while (i < MmNumberOfPagingFiles);

    if (ExtendedSize < ExtendSize) {
        ExtendedSize = 0;
        goto alldone;
    }

    //
    // Attempt to extend only one of the paging files.
    //

    i = 0;
    do {
        ExtendedSize = MiAttemptPageFileExtension (i, ExtendSize, FALSE);
        if (ExtendedSize != 0) {
            goto alldone;
        }
        i += 1;
    } while (i < MmNumberOfPagingFiles);

    ASSERT (ExtendedSize == 0);

    if (MmNumberOfPagingFiles == 1) {

        //
        // If the attempt didn't succeed for one (not enough disk space free) -
        // don't try to set it to the maximum size.
        //

        goto alldone;
    }

    //
    // Attempt to extend all paging files.
    //

    i = 0;
    do {
        ASSERT (ExtendSize > ExtendedSize);
        ExtendedSize += MiAttemptPageFileExtension (i,
                                                    ExtendSize - ExtendedSize,
                                                    TRUE);
        if (ExtendedSize >= ExtendSize) {
            goto alldone;
        }
        i += 1;
    } while (i < MmNumberOfPagingFiles);

    //
    // Not enough space is available.
    //

    ExtendedSize = 0;

alldone:

    if (LockHeld == FALSE) {
        ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
    }

    if ((ExtendedSize != 0) && (RealExpansion == TRUE)) {
        MmTotalCommitLimit += ExtendedSize;
    }
    
    //
    // If commit allotments have been temporarily blocked then unblock now.
    //

    if (MmPageFileFullExtendPages) {
        ASSERT (MmTotalCommittedPages >= MmPageFileFullExtendPages);
        MmTotalCommittedPages -= MmPageFileFullExtendPages;
        MmPageFileFullExtendPages = 0;
    }

    PageExpand->InProgress = FALSE;
    PageExpand->ActualExpansion = ExtendedSize;

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

    return ExtendedSize;
}

VOID
MiContractPagingFiles (
    VOID
    )

/*++

Routine Description:

    This routine checks to see if ample space is no longer committed
    and if so, does enough free space exist in any paging file.  IF
    the answer to both these is affirmative, a reduction in the
    paging file size(s) is attempted.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN Reduce;
    PMMPAGE_FILE_EXPANSION PageReduce;
    KIRQL OldIrql;
    ULONG i;

    Reduce = FALSE;

    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

    if ((MmTotalCommitLimit - MmMinimumPageFileReduction) >
                                                       MmTotalCommittedPages) {

        for (i = 0;i < MmNumberOfPagingFiles; i += 1) {
            if (MmPagingFile[i]->Size != MmPagingFile[i]->MinimumSize) {
                if (MmPagingFile[i]->FreeSpace > MmMinimumPageFileReduction) {
                    Reduce = TRUE;
                    break;
                }
            }
        }

        ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

        if (!Reduce) {
            return;
        }

        PageReduce = ExAllocatePoolWithTag (NonPagedPool,
                                            sizeof(MMPAGE_FILE_EXPANSION),
                                            '  mM');

        if (PageReduce == NULL) {
            return;
        }

        PageReduce->Segment = NULL;
        PageReduce->RequestedExpansionSize = 0xFFFFFFFF;

        ExAcquireSpinLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);
        InsertTailList ( &MmDereferenceSegmentHeader.ListHead,
                         &PageReduce->DereferenceList);
        ExReleaseSpinLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

        KeReleaseSemaphore (&MmDereferenceSegmentHeader.Semaphore, 0L, 1L, FALSE);
        return;
    }

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    return;
}

VOID
MiAttemptPageFileReduction (
    VOID
    )

/*++

Routine Description:

    This routine attempts to reduce the size of the paging files to
    their minimum levels.

    Note - Page file expansion and page file reduction are synchronized
           because a single thread is responsible for performing the
           operation.  Hence, while expansion is occurring, a reduction
           request will be queued to the thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN Reduce;
    KIRQL OldIrql;
    ULONG i;
    PFN_NUMBER StartReduction;
    PFN_NUMBER ReductionSize;
    PFN_NUMBER TryBit;
    PFN_NUMBER TryReduction;
    SIZE_T MaxReduce;
    FILE_ALLOCATION_INFORMATION FileAllocationInfo;
    NTSTATUS status;

    Reduce = FALSE;

    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

    //
    // Make sure the commit limit is greater than the number of committed
    // pages by twice the minimum page file reduction.  Keep the
    // difference between the two at least minimum page file reduction.
    //

    if ((MmTotalCommittedPages + (2 * MmMinimumPageFileReduction)) <
                                                     MmTotalCommitLimit) {

        MaxReduce = MmTotalCommitLimit -
                        (MmMinimumPageFileReduction + MmTotalCommittedPages);
        ASSERT ((LONG)MaxReduce >= 0);

        i = 0;
        do {

            if (MaxReduce < MmMinimumPageFileReduction) {

                //
                // Don't reduce any more paging files.
                //

                break;
            }

            if (MmPagingFile[i]->MinimumSize != MmPagingFile[i]->Size) {

                if (MmPagingFile[i]->FreeSpace > MmMinimumPageFileReduction) {

                    //
                    // Attempt to reduce this paging file.
                    //

                    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

                    //
                    // Lock the PFN database and check to see if ample pages
                    // are free at the end of the paging file.
                    //

                    TryBit = MmPagingFile[i]->Size - MmMinimumPageFileReduction;
                    TryReduction = MmMinimumPageFileReduction;

                    if (TryBit <= MmPagingFile[i]->MinimumSize) {
                        TryBit = MmPagingFile[i]->MinimumSize;
                        TryReduction = MmPagingFile[i]->Size -
                                                   MmPagingFile[i]->MinimumSize;
                    }

                    StartReduction = 0;
                    ReductionSize = 0;

                    LOCK_PFN (OldIrql);

                    while (TRUE) {

                        //
                        // Try to reduce.
                        //

                        if ((ReductionSize + TryReduction) > MaxReduce) {

                            //
                            // The reduction attempt would remove more
                            // than MaxReduce pages.
                            //

                            break;
                        }

                        if (RtlAreBitsClear (MmPagingFile[i]->Bitmap,
                                             (ULONG)TryBit,
                                             (ULONG)TryReduction)) {

                            //
                            // Can reduce it by TryReduction, see if it can
                            // be made smaller.
                            //

                            StartReduction = TryBit;
                            ReductionSize += TryReduction;

                            if (StartReduction == MmPagingFile[i]->MinimumSize) {
                                break;
                            }

                            TryBit = StartReduction - MmMinimumPageFileReduction;

                            if (TryBit <= MmPagingFile[i]->MinimumSize) {
                                TryReduction -=
                                        MmPagingFile[i]->MinimumSize - TryBit;
                                TryBit = MmPagingFile[i]->MinimumSize;
                            } else {
                                TryReduction = MmMinimumPageFileReduction;
                            }
                        } else {

                            //
                            // Reduction has failed.
                            //

                            break;
                        }
                    } //end while

                    //
                    // Make sure there are no outstanding writes to
                    // pages within the start reduction range.
                    //

                    if (StartReduction != 0) {

                        //
                        // There is an outstanding write past where the
                        // new end of the paging file should be.  This
                        // is a very rare condition, so just punt shrinking
                        // the file.
                        //

                        if ((MmPagingFile[i]->Entry[0]->LastPageToWrite >
                                                              StartReduction) ||
                            (MmPagingFile[i]->Entry[1]->LastPageToWrite >
                                                              StartReduction)) {
                            StartReduction = 0;
                        }
                    }

                    //
                    // Are there any pages to remove?
                    //

                    if (StartReduction != 0) {

                        //
                        // Reduce the paging file's size and free space.
                        //

                        ASSERT (ReductionSize == (MmPagingFile[i]->Size - StartReduction));

                        MmPagingFile[i]->Size = StartReduction;
                        MmPagingFile[i]->FreeSpace -= ReductionSize;
                        MaxReduce -= ReductionSize;
                        ASSERT ((LONG)MaxReduce >= 0);

                        RtlSetBits (MmPagingFile[i]->Bitmap,
                                    (ULONG)StartReduction,
                                    (ULONG)ReductionSize );

                        //
                        // Release the PFN lock now that the size info
                        // has been updated.
                        //

                        UNLOCK_PFN (OldIrql);

                        //
                        // Change the commit limit to reflect the returned
                        // page file space.
                        //

                        ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

                        //
                        // Now that the commit lock is again held, recheck
                        // the commit to ensure it is still safe to contract
                        // the paging files.
                        //

                        if ((MmTotalCommittedPages + (2 * MmMinimumPageFileReduction)) >=
                                                                         MmTotalCommitLimit) {

                            ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
                            LOCK_PFN (OldIrql);

                            MmPagingFile[i]->Size = StartReduction + ReductionSize;
                            MmPagingFile[i]->FreeSpace += ReductionSize;
                            MaxReduce += ReductionSize;
                            ASSERT ((LONG)MaxReduce >= 0);
    
                            RtlClearBits (MmPagingFile[i]->Bitmap,
                                          (ULONG)StartReduction,
                                          (ULONG)ReductionSize );

                            UNLOCK_PFN (OldIrql);

                            ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
                            break;
                        }

                        MmTotalCommitLimit -= ReductionSize;

                        ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

#if defined (_WIN64) || defined (_X86PAE_)
                        FileAllocationInfo.AllocationSize.QuadPart =
                                                   ((ULONG64)StartReduction << PAGE_SHIFT);

#else
                        FileAllocationInfo.AllocationSize.LowPart =
                                                   StartReduction * PAGE_SIZE;

                        //
                        // Set high part to zero, paging files are
                        // limited to 4gb.
                        //

                        FileAllocationInfo.AllocationSize.HighPart = 0;
#endif

                        //
                        // Reduce the allocated size of the paging file
                        // thereby actually freeing the space and
                        // setting a new end of file.
                        //


                        ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
                        status = IoSetInformation (
                                        MmPagingFile[i]->File,
                                        FileAllocationInformation,
                                        sizeof(FILE_ALLOCATION_INFORMATION),
                                        &FileAllocationInfo
                                       );
#if DBG

                        //
                        // Ignore errors on truncating the paging file
                        // as we can always have less space in the bitmap
                        // than the pagefile holds.
                        //

                        if (status != STATUS_SUCCESS) {
                            DbgPrint ("MM: pagefile truncate status %lx\n",
                                    status);
                        }
#endif
                    } else {
                        UNLOCK_PFN (OldIrql);
                    }

                    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
                }
            }
            i += 1;
        } while (i < MmNumberOfPagingFiles);
    }

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    return;
}

VOID
MiWriteComplete (
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus,
    IN ULONG Reserved
    )

/*++

Routine Description:

    This routine is the APC write completion procedure.  It is invoked
    at APC_LEVEL when a page write operation is completed.

Arguments:

    Context - Supplies a pointer to the MOD_WRITER_MDL_ENTRY which was
              used for this I/O.

    IoStatus - Supplies a pointer to the IO_STATUS_BLOCK which was used
               for this I/O.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL.

--*/

{

    PMMMOD_WRITER_MDL_ENTRY WriterEntry;
    PMMMOD_WRITER_MDL_ENTRY NextWriterEntry;
    PPFN_NUMBER Page;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    LONG ByteCount;
    NTSTATUS status;
    PCONTROL_AREA ControlArea;
    ULONG FailAllIo;
    PFILE_OBJECT FileObject;
    PERESOURCE FileResource;

    UNREFERENCED_PARAMETER (Reserved);

    FailAllIo = FALSE;

#if DBG
    if (MmDebug & MM_DBG_MOD_WRITE) {
        DbgPrint("MM MODWRITE: modified page write completed\n");
    }
#endif

    //
    // A page write has completed, at this time the pages are not
    // on any lists, write-in-progress is set in the PFN database,
    // and the reference count was incremented.
    //

    WriterEntry = (PMMMOD_WRITER_MDL_ENTRY)Context;
    ByteCount = (LONG)WriterEntry->Mdl.ByteCount;
    Page = &WriterEntry->Page[0];

    if (WriterEntry->Mdl.MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
        MmUnmapLockedPages (WriterEntry->Mdl.MappedSystemVa,
                            &WriterEntry->Mdl);
    }

    //
    // Get the PFN lock so the PFN database can be manipulated.
    //

    status = IoStatus->Status;
    ControlArea = WriterEntry->ControlArea;

    LOCK_PFN (OldIrql);

    //
    // Indicate that the write is complete.
    //

    WriterEntry->LastPageToWrite = 0;


    while (ByteCount > 0) {

        Pfn1 = MI_PFN_ELEMENT (*Page);
        ASSERT (Pfn1->u3.e1.WriteInProgress == 1);
#if DBG

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {

            ULONG Offset;
            Offset = GET_PAGING_FILE_OFFSET(Pfn1->OriginalPte);
            if ((Offset < 8192) &&
                    (GET_PAGING_FILE_NUMBER(Pfn1->OriginalPte) == 0)) {
                ASSERT ((MmPagingFileDebug[Offset] & 1) != 0);
                if (!MI_IS_PFN_DELETED(Pfn1)) {
                    if ((GET_PAGING_FILE_NUMBER (Pfn1->OriginalPte)) == 0) {
                        if ((MmPagingFileDebug[Offset] & ~0x1f) !=
                                   ((ULONG_PTR)Pfn1->PteAddress << 3)) {
                            if (Pfn1->PteAddress != MiGetPteAddress(PDE_BASE)) {

                                //
                                // Make sure this isn't a PTE that was forked
                                // during the I/O.
                                //

                                if ((Pfn1->PteAddress < (PMMPTE)PDE_TOP) ||
                                    ((Pfn1->OriginalPte.u.Soft.Protection &
                                            MM_COPY_ON_WRITE_MASK) ==
                                                MM_PROTECTION_WRITE_MASK)) {
                                    DbgPrint("MMWRITE: Mismatch Pfn1 %p Offset %lx info %p\n",
                                             Pfn1,
                                             Offset,
                                             MmPagingFileDebug[Offset]);

                                    DbgBreakPoint();

                                } else {
                                    MmPagingFileDebug[Offset] &= 0x1f;
                                    MmPagingFileDebug[Offset] |=
                                        ((ULONG_PTR)Pfn1->PteAddress << 3);
                                }
                            }

                        }
                    }
                }
            }
        }
#endif //DBG

        Pfn1->u3.e1.WriteInProgress = 0;

        if (NT_ERROR(status)) {

            //
            // If the file object is over the network, assume that this
            // I/O operation can never complete and mark the pages as
            // clean and indicate in the control area all I/O should fail.
            // Note that the modified bit in the PFN database is not set.
            //

            if (((status != STATUS_FILE_LOCK_CONFLICT) &&
                (ControlArea != NULL) &&
                (ControlArea->u.Flags.Networked == 1))
                            ||
                (status == STATUS_FILE_INVALID)) {

                if (ControlArea->u.Flags.FailAllIo == 0) {
                    ControlArea->u.Flags.FailAllIo = 1;
                    FailAllIo = TRUE;

                    KdPrint(("MM MODWRITE: failing all io, controlarea %lx status %lx\n",
                          ControlArea, status));
                }
            } else {

                //
                // The modified write operation failed, SET the modified bit
                // for each page which was written and free the page file
                // space.
                //

#if DBG
                if ((status != STATUS_FILE_LOCK_CONFLICT) &&
                   ((MmDebug & MM_DBG_PRINTS_MODWRITES) == 0)) {
                    KdPrint(("MM MODWRITE: modified page write iosb failed - status 0x%lx\n",
                            status));
                }
#endif

                Pfn1->u3.e1.Modified = 1;
            }
        }

        if ((Pfn1->u3.e1.Modified == 1) &&
            (Pfn1->OriginalPte.u.Soft.Prototype == 0)) {

            //
            // This page was modified since the write was done,
            // release the page file space.
            //

            MiReleasePageFileSpace (Pfn1->OriginalPte);
            Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
        }

        MI_REMOVE_LOCKED_PAGE_CHARGE (Pfn1, 15);
        MiDecrementReferenceCount (*Page);
#if DBG
        *Page = 0xF0FFFFFF;
#endif //DBG

        Page += 1;
        ByteCount -= (LONG)PAGE_SIZE;
    }

    //
    // Check to which list to insert this entry into depending on
    // the amount of free space left in the paging file.
    //

    FileObject = WriterEntry->File;
    FileResource = WriterEntry->FileResource;

    if ((WriterEntry->PagingFile != NULL) &&
        (WriterEntry->PagingFile->FreeSpace < MM_USABLE_PAGES_FREE)) {

        if (MmNumberOfActiveMdlEntries == 1) {

            //
            // If we put this entry on the list, there will be
            // no more paging.  Locate all entries which are non
            // zero and pull them from the list.
            //

            InsertTailList (&MmFreePagingSpaceLow, &WriterEntry->Links);
            WriterEntry->CurrentList = &MmFreePagingSpaceLow;

            MmNumberOfActiveMdlEntries -= 1;

            //
            // Try to pull entries off the list.
            //

            WriterEntry = (PMMMOD_WRITER_MDL_ENTRY)MmFreePagingSpaceLow.Flink;

            while ((PLIST_ENTRY)WriterEntry != &MmFreePagingSpaceLow) {

                NextWriterEntry =
                            (PMMMOD_WRITER_MDL_ENTRY)WriterEntry->Links.Flink;

                if (WriterEntry->PagingFile->FreeSpace != 0) {

                    RemoveEntryList (&WriterEntry->Links);

                    //
                    // Insert this into the active list.
                    //

                    if (IsListEmpty (&WriterEntry->PagingListHead->ListHead)) {
                        KeSetEvent (&WriterEntry->PagingListHead->Event,
                                    0,
                                    FALSE);
                    }

                    InsertTailList (&WriterEntry->PagingListHead->ListHead,
                                    &WriterEntry->Links);
                    WriterEntry->CurrentList = &MmPagingFileHeader.ListHead;
                    MmNumberOfActiveMdlEntries += 1;
                }

                WriterEntry = NextWriterEntry;
            }

        } else {

            InsertTailList (&MmFreePagingSpaceLow, &WriterEntry->Links);
            WriterEntry->CurrentList = &MmFreePagingSpaceLow;
            MmNumberOfActiveMdlEntries -= 1;
        }
    } else {

        //
        // Ample space exists, put this on the active list.
        //

        if (IsListEmpty (&WriterEntry->PagingListHead->ListHead)) {
            KeSetEvent (&WriterEntry->PagingListHead->Event, 0, FALSE);
        }

        InsertTailList (&WriterEntry->PagingListHead->ListHead,
                        &WriterEntry->Links);
    }

    ASSERT (((ULONG_PTR)WriterEntry->Links.Flink & 1) == 0);

    UNLOCK_PFN (OldIrql);

    if (FileResource != NULL) {
        FsRtlReleaseFileForModWrite (FileObject, FileResource);
    }

    if (FailAllIo) {

        if (ControlArea->FilePointer->FileName.Length &&
            ControlArea->FilePointer->FileName.MaximumLength &&
            ControlArea->FilePointer->FileName.Buffer) {

            IoRaiseInformationalHardError(
                STATUS_LOST_WRITEBEHIND_DATA,
                &ControlArea->FilePointer->FileName,
                NULL
                );
        }
    }

    if (ControlArea != NULL) {

        LOCK_PFN (OldIrql);

        //
        // A write to a mapped file just completed, check to see if
        // there are any waiters on the completion of this i/o.
        //

        ControlArea->ModifiedWriteCount -= 1;
        ASSERT ((SHORT)ControlArea->ModifiedWriteCount >= 0);
        if (ControlArea->u.Flags.SetMappedFileIoComplete != 0) {
            KePulseEvent (&MmMappedFileIoComplete,
                          0,
                          FALSE);
        }

        ControlArea->NumberOfPfnReferences -= 1;

        if (ControlArea->NumberOfPfnReferences == 0) {

            //
            // This routine return with the PFN lock released!.
            //

            MiCheckControlArea (ControlArea, NULL, OldIrql);
        } else {
            UNLOCK_PFN (OldIrql);
        }
    }

    if (NT_ERROR(status)) {

        //
        // Wait for a short time so other processing can continue.
        //

        KeDelayExecutionThread (KernelMode, FALSE, &Mm30Milliseconds);
    }

    return;
}

LOGICAL
MiCancelWriteOfMappedPfn (
    IN PFN_NUMBER PageToStop
    )

/*++

Routine Description:

    This routine attempts to stop a pending mapped page writer write for the
    specified PFN.  Note that if the write can be stopped, any other pages
    that may be clustered with the write are also stopped.

Arguments:

    PageToStop - Supplies the frame number that the caller wants to stop.

Return Value:

    TRUE if the write was stopped, FALSE if not.

Environment:

    Kernel mode, PFN lock held.  The PFN lock is released and reacquired if
    the write was stopped.

    N.B.  No other locks may be held as IRQL is lowered to APC_LEVEL here.

--*/

{
    ULONG i;
    ULONG PageCount;
    KIRQL OldIrql;
    PPFN_NUMBER Page;
    PLIST_ENTRY NextEntry;
    PMDL MemoryDescriptorList;
    PMMMOD_WRITER_MDL_ENTRY ModWriterEntry;

    //
    // Walk the MmMappedPageWriterList looking for an MDL which contains
    // the argument page.  If found, remove it and cancel the write.
    //

    NextEntry = MmMappedPageWriterList.Flink;
    while (NextEntry != &MmMappedPageWriterList) {

        ModWriterEntry = CONTAINING_RECORD(NextEntry,
                                           MMMOD_WRITER_MDL_ENTRY,
                                           Links);

        MemoryDescriptorList = &ModWriterEntry->Mdl;
        PageCount = (MemoryDescriptorList->ByteCount >> PAGE_SHIFT);
        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

        for (i = 0; i < PageCount; i += 1) {
            if (*Page == PageToStop) {
                RemoveEntryList (NextEntry);
                goto CancelWrite;
            }
            Page += 1;
        }

        NextEntry = NextEntry->Flink;
    }

    return FALSE;
    
CancelWrite:

    UNLOCK_PFN (APC_LEVEL);

    //
    // File lock conflict to indicate an error has occurred,
    // but that future I/Os should be allowed.  Keep APCs disabled and
    // call the write completion routine.
    //

    ModWriterEntry->u.IoStatus.Status = STATUS_FILE_LOCK_CONFLICT;
    ModWriterEntry->u.IoStatus.Information = 0;

    MiWriteComplete ((PVOID)ModWriterEntry,
                     &ModWriterEntry->u.IoStatus,
                     0 );

    LOCK_PFN (OldIrql);

    return TRUE;
}

VOID
MiModifiedPageWriter (
    IN PVOID StartContext
    )

/*++

Routine Description:

    Implements the NT modified page writer thread.  When the modified
    page threshold is reached, or memory becomes overcommitted the
    modified page writer event is set, and this thread becomes active.

Arguments:

    StartContext - not used.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG i;

    PAGED_CODE();

    StartContext;  //avoid compiler warning.

    //
    // Initialize listheads as empty.
    //

    MmSystemShutdown = 0;
    KeInitializeEvent (&MmPagingFileHeader.Event, NotificationEvent, FALSE);
    KeInitializeEvent (&MmMappedFileHeader.Event, NotificationEvent, FALSE);

    InitializeListHead(&MmPagingFileHeader.ListHead);
    InitializeListHead(&MmMappedFileHeader.ListHead);
    InitializeListHead(&MmFreePagingSpaceLow);

    for (i = 0; i < MM_MAPPED_FILE_MDLS; i += 1) {
        MmMappedFileMdl[i] = ExAllocatePoolWithTag (NonPagedPoolMustSucceed,
                                             sizeof(MMMOD_WRITER_MDL_ENTRY) +
                                                MmModifiedWriteClusterSize *
                                                    sizeof(PFN_NUMBER),
                                                '  mM');

        MmMappedFileMdl[i]->PagingFile = NULL;
        MmMappedFileMdl[i]->PagingListHead = &MmMappedFileHeader;

        InsertTailList (&MmMappedFileHeader.ListHead,
                        &MmMappedFileMdl[i]->Links);
    }

    //
    // Make this a real time thread.
    //

    (VOID) KeSetPriorityThread (&PsGetCurrentThread()->Tcb,
                                LOW_REALTIME_PRIORITY + 1);

    //
    // Start a secondary thread for writing mapped file pages.  This
    // is required as the writing of mapped file pages could cause
    // page faults resulting in requests for free pages.  But there
    // could be no free pages - hence a dead lock.  Rather than deadlock
    // the whole system waiting on the modified page writer, creating
    // a secondary thread allows that thread to block without affecting
    // on going page file writes.
    //

    KeInitializeEvent (&MmMappedPageWriterEvent, NotificationEvent, FALSE);
    InitializeListHead(&MmMappedPageWriterList);
    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

    PsCreateSystemThread (&ThreadHandle,
                          THREAD_ALL_ACCESS,
                          &ObjectAttributes,
                          0L,
                          NULL,
                          MiMappedPageWriter,
                          NULL );
    ZwClose (ThreadHandle);
    MiModifiedPageWriterWorker();

    //
    // Shutdown in progress, wait forever.
    //

    {
        LARGE_INTEGER Forever;

        //
        // System has shutdown, go into LONG wait.
        //

        Forever.LowPart = 0;
        Forever.HighPart = 0xF000000;
        KeDelayExecutionThread (KernelMode, FALSE, &Forever);
    }

    return;
}


VOID
MiModifiedPageWriterTimerDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed whenever modified mapped pages are waiting to
    be written.  Its job is to signal the Modified Page Writer to write
    these out.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Optional deferred context;  not used.

    SystemArgument1 - Optional argument 1;  not used.

    SystemArgument2 - Optional argument 2;  not used.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (DeferredContext);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    LOCK_PFN2 (OldIrql);

    MiTimerPending = TRUE;
    KeSetEvent (&MiMappedPagesTooOldEvent, 0, FALSE);

    UNLOCK_PFN2 (OldIrql);
}


VOID
MiModifiedPageWriterWorker (
    VOID
    )

/*++

Routine Description:

    Implements the NT modified page writer thread.  When the modified
    page threshold is reached, or memory becomes overcommitted the
    modified page writer event is set, and this thread becomes active.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    ULONG NextColor;
    ULONG i;
    static KWAIT_BLOCK WaitBlockArray[ModifiedWriterMaximumObject];
    PVOID WaitObjects[ModifiedWriterMaximumObject];
    NTSTATUS WakeupStatus;

    //
    // Wait for the modified page writer event or the mapped pages event.
    //

    WaitObjects[NormalCase] = (PVOID)&MmModifiedPageWriterEvent;
    WaitObjects[MappedPagesNeedWriting] = (PVOID)&MiMappedPagesTooOldEvent;

    for (;;) {

        WakeupStatus = KeWaitForMultipleObjects(ModifiedWriterMaximumObject,
                                          &WaitObjects[0],
                                          WaitAny,
                                          WrFreePage,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          &WaitBlockArray[0]);

        //
        // Switch on the wait status.
        //

        switch (WakeupStatus) {

        case NormalCase:
                break;

        case MappedPagesNeedWriting:

                //
                // Our mapped pages DPC went off, only deal with those pages.
                // Write all the mapped pages (ONLY), then clear the flag
                // and come back to the top.
                //

                break;

        default:
                break;

        }

        //
        // Indicate that the hint values have not been reset in
        // the paging files.
        //

        if (MmNumberOfPagingFiles != 0) {
            i = 0;
            do {
                MmPagingFile[i]->HintSetToZero = FALSE;
                i += 1;
            } while (i < MmNumberOfPagingFiles);
        }

        NextColor = 0;

        LOCK_PFN (OldIrql);

        for (;;) {

            //
            // Modified page writer was signalled.
            //

            if ((MmAvailablePages < MmFreeGoal) &&
                (MmModNoWriteInsert)) {

                //
                // Remove pages from the modified no write list
                // that are waiting for the cache manager to flush them.
                //

                i = 0;
                while ((MmModifiedNoWritePageListHead.Total != 0) &&
                      (i < 32)) {
                    PSUBSECTION Subsection;
                    PCONTROL_AREA ControlArea;

                    PageFrameIndex = MmModifiedNoWritePageListHead.Flink;
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                    ControlArea = Subsection->ControlArea;
                    if (ControlArea->u.Flags.NoModifiedWriting) {
                        MmModNoWriteInsert = FALSE;
                        break;
                    }
                    MiUnlinkPageFromList (Pfn1);
                    MiInsertPageInList (&MmModifiedPageListHead,
                                        PageFrameIndex);
                    i += 1;
                }
            }

            if (MmModifiedPageListHead.Total == 0) {

                //
                // No more pages, clear the event(s) and wait again...
                // Note we can clear both events regardless of why we woke up
                // since no modified pages of any type exist.
                //

                if (MiTimerPending == TRUE) {
                    MiTimerPending = FALSE;
                    KeClearEvent (&MiMappedPagesTooOldEvent);
                }

                UNLOCK_PFN (OldIrql);

                KeClearEvent (&MmModifiedPageWriterEvent);

                break;
            }

            //
            // If we didn't wake up explicitly to deal with mapped pages,
            // then determine which type of pages are the most popular:
            // page file backed pages, or mapped file backed pages.
            //

            if (WakeupStatus == MappedPagesNeedWriting) {
                PageFrameIndex = MmModifiedPageListHead.Flink;
                if (PageFrameIndex == MM_EMPTY_LIST) {

                    //
                    // No more modified mapped pages (there may still be
                    // modified pagefile-destined pages), so clear only the
                    // mapped pages event and check for directions at the top
                    // again.
                    //

                    MiTimerPending = FALSE;
                    KeClearEvent (&MiMappedPagesTooOldEvent);

                    UNLOCK_PFN (OldIrql);

                    break;
                }
            }
            else if (MmTotalPagesForPagingFile >=
                (MmModifiedPageListHead.Total - MmTotalPagesForPagingFile)) {

                //
                // More pages are destined for the paging file.
                //

                MI_GET_MODIFIED_PAGE_ANY_COLOR (PageFrameIndex, NextColor);

            } else {

                //
                // More pages are destined for mapped files.
                //

                PageFrameIndex = MmModifiedPageListHead.Flink;
            }

            //
            // Check to see what type of page (section file backed or page
            // file backed) and write out that page and more if possible.
            //

            //
            // Check to see if this page is destined for a paging file or
            // a mapped file.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {
                if (IsListEmpty (&MmMappedFileHeader.ListHead)) {

                    //
                    // Make sure page is destined for paging file as there
                    // are no MDLs for mapped writes free.
                    //

                    if (WakeupStatus != MappedPagesNeedWriting) {

                        MI_GET_MODIFIED_PAGE_ANY_COLOR (PageFrameIndex, NextColor);

                        //
                        // No pages are destined for the paging file, get the
                        // first page destined for a mapped file.
                        //

                        if (PageFrameIndex == MM_EMPTY_LIST) {

                            //
                            // Select the first page from the list anyway.
                            //

                            PageFrameIndex = MmModifiedPageListHead.Flink;
                        }

                        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    }
                }
            } else if ((IsListEmpty(&MmPagingFileHeader.ListHead)) ||
                       (MiFirstPageFileCreatedAndReady == FALSE)) {

                //
                // Try for a dirty section-backed page as no paging file MDLs
                // are available.
                //

                if (MmModifiedPageListHead.Flink != MM_EMPTY_LIST) {
                    ASSERT (MmTotalPagesForPagingFile != MmModifiedPageListHead.Total);
                    PageFrameIndex = MmModifiedPageListHead.Flink;
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                }
                else {
                    ASSERT (MmTotalPagesForPagingFile == MmModifiedPageListHead.Total);
                    if ((MiFirstPageFileCreatedAndReady == FALSE) &&
                        (MmNumberOfPagingFiles != 0)) {

                        //
                        // The first paging has been created but the reservation
                        // checking for crashdumps has not finished yet.  Delay
                        // a bit as this will finish shortly and then restart.
                        //

                        UNLOCK_PFN (OldIrql);
                        KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);
                        LOCK_PFN (OldIrql);
                        continue;
                    }
                }
            }

            if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {

                if (IsListEmpty(&MmMappedFileHeader.ListHead)) {

                    if (WakeupStatus == MappedPagesNeedWriting) {

                        //
                        // Since we woke up only to take care of mapped pages,
                        // don't wait for an MDL below because drivers may take
                        // an inordinate amount of time processing the
                        // outstanding ones.  We might have to wait too long,
                        // resulting in the system running out of pages.
                        //

                        if (MiTimerPending == TRUE) {

                            //
                            // This should be normal case - the reason we must
                            // first check timer pending above is for the rare
                            // case - when this thread first ran for normal
                            // modified page processing and took
                            // care of all the pages including the mapped ones.
                            // Then this thread woke up again for the mapped
                            // reason and here we are.
                            //

                            MiTimerPending = FALSE;
                            KeClearEvent (&MiMappedPagesTooOldEvent);
                        }

                        MiTimerPending = TRUE;

                        (VOID) KeSetTimerEx( &MiModifiedPageWriterTimer, MiModifiedPageLife, 0, &MiModifiedPageWriterTimerDpc );
                        UNLOCK_PFN (OldIrql);
                        break;
                    }

                    //
                    // Reset the event indicating no mapped files in
                    // the list, drop the PFN lock and wait for an
                    // I/O operation to complete with a one second
                    // timeout.
                    //

                    KeClearEvent (&MmMappedFileHeader.Event);

                    UNLOCK_PFN (OldIrql);
                    KeWaitForSingleObject( &MmMappedFileHeader.Event,
                                           WrPageOut,
                                           KernelMode,
                                           FALSE,
                                           &Mm30Milliseconds);
                    LOCK_PFN (OldIrql);

                    //
                    // Don't go on as the old PageFrameIndex at the
                    // top of the ModifiedList may have changed states.
                    //

                    continue;
                }

                MiGatherMappedPages (Pfn1, PageFrameIndex);

            } else {

                MiGatherPagefilePages (Pfn1, PageFrameIndex);
            }

            if (MmSystemShutdown) {

                //
                // Shutdown has returned.  Stop the modified page writer.
                //

                UNLOCK_PFN (OldIrql);
                return;
            }

            if (WakeupStatus != MappedPagesNeedWriting && !MmWriteAllModifiedPages) {
                if (((MmAvailablePages > MmFreeGoal) &&
                        (MmModifiedPageListHead.Total < MmFreeGoal))
                         ||
                    (MmAvailablePages > MmMoreThanEnoughFreePages)) {

                    //
                    // There are ample pages, clear the event and wait again...
                    //

                    UNLOCK_PFN (OldIrql);

                    KeClearEvent (&MmModifiedPageWriterEvent);
                    break;
                }
            }
        } // end for

    } // end for
}

VOID
MiGatherMappedPages (
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine processes the specified modified page by examining
    the prototype PTE for that page and the adjacent prototype PTEs
    building a cluster of modified pages destined for a mapped file.
    Once the cluster is built, it is sent to the mapped writer thread
    to be processed.

Arguments:

    Pfn1 - Supplies a pointer to the PFN element for the corresponding
           page.

    PageFrameIndex - Supplies the physical page frame to write.

Return Value:

    None.

Environment:

    PFN lock held.

--*/

{
    PMMPFN Pfn2;
    PMMMOD_WRITER_MDL_ENTRY ModWriterEntry;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    PPFN_NUMBER Page;
    PMMPTE LastPte;
    PMMPTE BasePte;
    PMMPTE NextPte;
    PMMPTE PointerPte;
    PMMPTE StartingPte;
    MMPTE PteContents;
    KIRQL OldIrql = 0;
    KIRQL OldIrql2;

    //
    // This page is destined for a mapped file, check to see if
    // there are any physically adjacent pages are also in the
    // modified page list and write them out at the same time.
    //

    Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
    ControlArea = Subsection->ControlArea;

    if (ControlArea->u.Flags.NoModifiedWriting) {

        //
        // This page should not be written out, add it to the
        // tail of the modified NO WRITE list and get the next page.
        //

        MiUnlinkPageFromList (Pfn1);
        MiInsertPageInList (MmPageLocationList[ModifiedNoWritePageList],
                            PageFrameIndex);
        return;
    }

    if (ControlArea->u.Flags.Image) {

#if 0
        //
        // Assert that there are no dangling shared global pages
        // for an image section that is not being used.
        //
        // This assert can be re-enabled when the segment dereference
        // thread list re-insertion is fixed.  Note the recovery code is
        // fine, so disabling the assert is benign.
        //

        ASSERT ((ControlArea->NumberOfMappedViews != 0) ||
                (ControlArea->NumberOfSectionReferences != 0) ||
                (ControlArea->u.Flags.FloppyMedia != 0));
#endif

        //
        // This is an image section, writes are not
        // allowed to an image section.
        //

        //
        // Change page contents to look like it's a demand zero
        // page and put it back into the modified list.
        //

        //
        // Decrement the count for PfnReferences to the
        // segment as paging file pages are not counted as
        // "image" references.
        //

        ControlArea->NumberOfPfnReferences -= 1;
        ASSERT ((LONG)ControlArea->NumberOfPfnReferences >= 0);
        MiUnlinkPageFromList (Pfn1);

        Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
        Pfn1->OriginalPte.u.Soft.Prototype = 0;
        Pfn1->OriginalPte.u.Soft.Transition = 0;

        //
        // Insert the page at the tail of the list and get
        // color update performed.
        //

        MiInsertPageInList (MmPageLocationList[ModifiedPageList],
                            PageFrameIndex);
        return;
    }

    if ((ControlArea->u.Flags.HadUserReference == 0) &&
        (MmAvailablePages > (MmFreeGoal + 40)) &&
        (MmEnoughMemoryForWrite())) {

        //
        // This page was modified via the cache manager.  Don't
        // write it out at this time as there are ample pages.
        //

        MiUnlinkPageFromList (Pfn1);
        MiInsertFrontModifiedNoWrite (PageFrameIndex);
        MmModNoWriteInsert = TRUE;
        return;
    }

    //
    // Look at backwards at previous prototype PTEs to see if
    // this can be clustered into a larger write operation.
    //

    PointerPte = Pfn1->PteAddress;
    NextPte = PointerPte - (MmModifiedWriteClusterSize - 1);

    //
    // Make sure NextPte is in the same page.
    //

    if (NextPte < (PMMPTE)PAGE_ALIGN (PointerPte)) {
        NextPte = (PMMPTE)PAGE_ALIGN (PointerPte);
    }

    //
    // Make sure NextPte is within the subsection.
    //

    if (NextPte < Subsection->SubsectionBase) {
        NextPte = Subsection->SubsectionBase;
    }

    //
    // If the prototype PTEs are not currently mapped,
    // map them via hyperspace.  BasePte refers to the
    // prototype PTEs for nonfaulting references.
    //

    OldIrql2 = 99;
    if (MmIsAddressValid (PointerPte)) {
        BasePte = PointerPte;
    } else {
        BasePte = MiMapPageInHyperSpace (Pfn1->PteFrame, &OldIrql2);
        BasePte = (PMMPTE)((PCHAR)BasePte +
                            BYTE_OFFSET (PointerPte));
    }

    ASSERT (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (BasePte) == PageFrameIndex);

    PointerPte -= 1;
    BasePte -= 1;

    //
    // Don't go before the start of the subsection nor cross
    // a page boundary.
    //

    while (PointerPte >= NextPte) {

        PteContents = *BasePte;

        //
        // If the page is not in transition, exit loop.
        //

        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Transition == 0) ||
            (PteContents.u.Soft.Prototype == 1)) {

            break;
        }

        Pfn2 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

        //
        // Make sure page is modified and on the modified list.
        //

        if ((Pfn2->u3.e1.Modified == 0 ) ||
            (Pfn2->u3.e2.ReferenceCount != 0)) {
            break;
        }
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
        PointerPte -= 1;
        BasePte -= 1;
    }

    StartingPte = PointerPte + 1;
    BasePte = BasePte + 1;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (StartingPte == Pfn1->PteAddress);
    MiUnlinkPageFromList (Pfn1);

    //
    // Get an entry from the list and fill it in.
    //

    ModWriterEntry = (PMMMOD_WRITER_MDL_ENTRY)RemoveHeadList (
                                    &MmMappedFileHeader.ListHead);

    ModWriterEntry->File = ControlArea->FilePointer;
    ModWriterEntry->ControlArea = ControlArea;

    //
    // Calculate the offset to read into the file.
    //  offset = base + ((thispte - basepte) << PAGE_SHIFT)
    //

    ModWriterEntry->WriteOffset.QuadPart = MiStartingOffset (Subsection,
                                                              Pfn1->PteAddress);

    MmInitializeMdl(&ModWriterEntry->Mdl,
                    (PVOID)ULongToPtr(Pfn1->u3.e1.PageColor << PAGE_SHIFT),
                    PAGE_SIZE);

    ModWriterEntry->Mdl.MdlFlags |= MDL_PAGES_LOCKED;

    ModWriterEntry->Mdl.Size = (CSHORT)(sizeof(MDL) +
                      (sizeof(PFN_NUMBER) * MmModifiedWriteClusterSize));

    Page = &ModWriterEntry->Page[0];

    //
    // Up the reference count for the physical page as there
    // is I/O in progress.
    //

    MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 14);
    Pfn1->u3.e2.ReferenceCount += 1;

    //
    // Clear the modified bit for the page and set the write
    // in progress bit.
    //

    Pfn1->u3.e1.Modified = 0;
    Pfn1->u3.e1.WriteInProgress = 1;

    //
    // Put this physical page into the MDL.
    //

    *Page = PageFrameIndex;

    //
    // See if any adjacent pages are also modified and in
    // the transition state and if so, write them out at
    // the same time.
    //


    //
    // Look at the previous PTE, ensuring a page boundary is
    // not crossed.
    //

    LastPte = StartingPte + MmModifiedWriteClusterSize;

    //
    // If BasePte is not in the same page as LastPte,
    // set last pte to be the last PTE in this page.
    //

    if (StartingPte < (PMMPTE)PAGE_ALIGN(LastPte)) {
        LastPte = ((PMMPTE)PAGE_ALIGN(LastPte)) - 1;
    }

    //
    // Make sure LastPte is within the subsection.
    //

    if (LastPte > &Subsection->SubsectionBase[
                                Subsection->PtesInSubsection]) {
        LastPte = &Subsection->SubsectionBase[
                                Subsection->PtesInSubsection];
    }

    //
    // Look forwards.
    //

    NextPte = BasePte + 1;
    PointerPte = StartingPte + 1;

    //
    // Loop until an MDL is filled, the end of a subsection
    // is reached, or a page boundary is reached.
    // Note, PointerPte points to the PTE. NextPte points
    // to where it is mapped in hyperspace (if required).
    //

    while (PointerPte < LastPte) {

        PteContents = *NextPte;

        //
        // If the page is not in transition, exit loop.
        //

        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Transition == 0) ||
            (PteContents.u.Soft.Prototype == 1)) {

            break;
        }

        Pfn2 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

        if ((Pfn2->u3.e1.Modified == 0 ) ||
            (Pfn2->u3.e2.ReferenceCount != 0)) {

            //
            // Page is not dirty or not on the modified list,
            // end clustering operation.
            //

            break;
        }
        Page += 1;

        //
        // Add physical page to MDL.
        //

        *Page = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
        ASSERT (PointerPte == Pfn2->PteAddress);
        MiUnlinkPageFromList (Pfn2);

        //
        // Up the reference count for the physical page as there
        // is I/O in progress.
        //

        MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn2, 14);
        Pfn2->u3.e2.ReferenceCount += 1;

        //
        // Clear the modified bit for the page and set the
        // write in progress bit.
        //

        Pfn2->u3.e1.Modified = 0;
        Pfn2->u3.e1.WriteInProgress = 1;

        ModWriterEntry->Mdl.ByteCount += PAGE_SIZE;

        NextPte += 1;
        PointerPte += 1;

    } //end while

    if (OldIrql2 != 99) {
        MiUnmapPageInHyperSpace (OldIrql2);
    }

    ASSERT (BYTES_TO_PAGES (ModWriterEntry->Mdl.ByteCount) <= MmModifiedWriteClusterSize);

    ModWriterEntry->u.LastByte.QuadPart = ModWriterEntry->WriteOffset.QuadPart +
                        ModWriterEntry->Mdl.ByteCount;

    ASSERT (Subsection->ControlArea->u.Flags.Image == 0);

#if DBG
    if ((ULONG)ModWriterEntry->Mdl.ByteCount >
                                ((1+MmModifiedWriteClusterSize)*PAGE_SIZE)) {
        DbgPrint("Mdl %p, MDL End Offset %lx %lx Subsection %p\n",
            ModWriterEntry->Mdl,
            ModWriterEntry->u.LastByte.LowPart,
            ModWriterEntry->u.LastByte.HighPart,
            Subsection);
        DbgBreakPoint();
    }
#endif //DBG

    MmInfoCounters.MappedWriteIoCount += 1;
    MmInfoCounters.MappedPagesWriteCount +=
                                (ModWriterEntry->Mdl.ByteCount >> PAGE_SHIFT);

    //
    // Increment the count of modified page writes outstanding
    // in the control area.
    //

    ControlArea->ModifiedWriteCount += 1;

    //
    // Increment the number of PFN references.  This allows the file
    // system to purge (i.e. call MmPurgeSection) modified writes.
    //

    ControlArea->NumberOfPfnReferences += 1;

    ModWriterEntry->FileResource = NULL;

    if (ControlArea->u.Flags.BeingPurged == 1) {
        UNLOCK_PFN (OldIrql);
        ModWriterEntry->u.IoStatus.Status = STATUS_FILE_LOCK_CONFLICT;
        ModWriterEntry->u.IoStatus.Information = 0;
        KeRaiseIrql (APC_LEVEL, &OldIrql);
        MiWriteComplete ((PVOID)ModWriterEntry,
                         &ModWriterEntry->u.IoStatus,
                         0 );
        KeLowerIrql (OldIrql);
        LOCK_PFN (OldIrql);
        return;
    }

    //
    // Send the entry for the MappedPageWriter.
    //

    InsertTailList (&MmMappedPageWriterList,
                    &ModWriterEntry->Links);

    KeSetEvent (&MmMappedPageWriterEvent, 0, FALSE);


#if 0

    UNLOCK_PFN (OldIrql);

    ModWriterEntry->FileResource = NULL;

    if (ModWriterEntry->ControlArea->u.Flags.FailAllIo == 1) {
        Status = STATUS_UNSUCCESSFUL;

    } else if (FsRtlAcquireFileForModWrite (ModWriterEntry->File,
                                            &ModWriterEntry->u.LastByte,
                                            &ModWriterEntry->FileResource)) {

        //
        // Issue the write request.
        //

        Status = IoAsynchronousPageWrite (
                               ModWriterEntry->File,
                               &ModWriterEntry->Mdl,
                               &ModWriterEntry->WriteOffset,
                               MiWriteComplete,
                               (PVOID)ModWriterEntry,
                               &ModWriterEntry->IoStatus,
                               &ModWriterEntry->Irp );
    } else {

        //
        // Unable to get the file system resources, set error status
        // to lock conflict (ignored by MiWriteComplete) so the APC
        // routine is explicitly called.
        //

        Status = STATUS_FILE_LOCK_CONFLICT;
    }

    if (NT_ERROR(Status)) {

        //
        // An error has occurred, disable APCs and
        // call the write completion routine.
        //

        ModWriterEntry->IoStatus.Status = Status;
        ModWriterEntry->IoStatus.Information = 0;
        KeRaiseIrql (APC_LEVEL, &OldIrql);
        MiWriteComplete ((PVOID)ModWriterEntry,
                         &ModWriterEntry->IoStatus,
                         0 );
        KeLowerIrql (OldIrql);
    }

    LOCK_PFN (OldIrql);
#endif //0
    return;
}

VOID
MiGatherPagefilePages (
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine processes the specified modified page by getting
    that page and gather any other pages on the modified list destined
    for the paging file in a large write cluster.  This cluster is
    then written to the paging file.

Arguments:

    Pfn1 - Supplies a pointer to the PFN element for the corresponding page.

    PageFrameIndex - Supplies the physical page frame to write.

Return Value:

    None.

Environment:

    PFN lock held.

--*/

{
    PFILE_OBJECT File;
    PMMMOD_WRITER_MDL_ENTRY ModWriterEntry;
    PMMPAGING_FILE CurrentPagingFile;
    NTSTATUS Status;
    PPFN_NUMBER Page;
    ULONG StartBit;
    LARGE_INTEGER StartingOffset;
    PFN_NUMBER ClusterSize;
    PFN_NUMBER ThisCluster;
    MMPTE LongPte;
    KIRQL OldIrql;
    ULONG NextColor;
    LOGICAL PageFileFull;
    //MM_WRITE_CLUSTER WriteCluster;

    OldIrql = 0;

    if (IsListEmpty(&MmPagingFileHeader.ListHead)) {

        //
        // Reset the event indicating no paging files MDLs in
        // the list, drop the PFN lock and wait for an
        // I/O operation to complete.
        //

        KeClearEvent (&MmPagingFileHeader.Event);
        UNLOCK_PFN (OldIrql);
        KeWaitForSingleObject( &MmPagingFileHeader.Event,
                               WrPageOut,
                               KernelMode,
                               FALSE,
                               &Mm30Milliseconds);
        LOCK_PFN (OldIrql);

        //
        // Don't go on as the old PageFrameIndex at the
        // top of the ModifiedList may have changed states.
        //

        return;
    }

    //
    // Page is destined for the paging file.
    // Find the paging file with the most free space and get a cluster.
    //

    NextColor = Pfn1->u3.e1.PageColor;

    ModWriterEntry = (PMMMOD_WRITER_MDL_ENTRY)RemoveHeadList (
                                    &MmPagingFileHeader.ListHead);
#if DBG
    ModWriterEntry->Links.Flink = MM_IO_IN_PROGRESS;
#endif
    CurrentPagingFile = ModWriterEntry->PagingFile;

    File = ModWriterEntry->PagingFile->File;
    ThisCluster = MmModifiedWriteClusterSize;

    PageFileFull = FALSE;

    do {
        //
        // Attempt to cluster MmModifiedWriteClusterSize pages
        // together.  Reduce by one half until we succeed or
        // can't find a single page free in the paging file.
        //

        if (((CurrentPagingFile->Hint + MmModifiedWriteClusterSize) >
                                CurrentPagingFile->MinimumSize)
             &&
            (CurrentPagingFile->HintSetToZero == FALSE)) {

            CurrentPagingFile->HintSetToZero = TRUE;
            CurrentPagingFile->Hint = 0;
        }

        StartBit = RtlFindClearBitsAndSet (CurrentPagingFile->Bitmap,
                                           (ULONG)ThisCluster,
                                           (ULONG)CurrentPagingFile->Hint);

        if (StartBit != 0xFFFFFFFF) {
            break;
        }
        if (CurrentPagingFile->Hint != 0) {

            //
            // Start looking from front of the file.
            //

            CurrentPagingFile->Hint = 0;
        } else {
            ThisCluster = ThisCluster >> 1;
            PageFileFull = TRUE;
        }

    } while (ThisCluster != 0);

    if (StartBit == 0xFFFFFFFF) {

        //
        // Paging file must be full.
        //

        KdPrint(("MM MODWRITE: page file full\n"));
        ASSERT(CurrentPagingFile->FreeSpace == 0);

        //
        // Move this entry to the not enough space list,
        // and try again.
        //

        InsertTailList (&MmFreePagingSpaceLow,
                        &ModWriterEntry->Links);
        ModWriterEntry->CurrentList = &MmFreePagingSpaceLow;
        MmNumberOfActiveMdlEntries -= 1;
        MiPageFileFull ();
        return;
    }

    CurrentPagingFile->FreeSpace -= ThisCluster;
    CurrentPagingFile->CurrentUsage += ThisCluster;
    if (CurrentPagingFile->FreeSpace < 32) {
        PageFileFull = TRUE;
    }

    StartingOffset.QuadPart = (UINT64)StartBit << PAGE_SHIFT;

    MmInitializeMdl(&ModWriterEntry->Mdl,
                    (PVOID)ULongToPtr(Pfn1->u3.e1.PageColor << PAGE_SHIFT),
                    PAGE_SIZE);

    ModWriterEntry->Mdl.MdlFlags |= MDL_PAGES_LOCKED;

    ModWriterEntry->Mdl.Size = (CSHORT)(sizeof(MDL) +
                    sizeof(PFN_NUMBER) * MmModifiedWriteClusterSize);

    Page = &ModWriterEntry->Page[0];

    ClusterSize = 0;

    //
    // Search through the modified page list looking for other
    // pages destined for the paging file and build a cluster.
    //

    while (ClusterSize != ThisCluster) {

        //
        // Is this page destined for a paging file?
        //

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {

#if 0  //********* commented out

            MiClusterWritePages (Pfn1,
                                 PageFrameIndex,
                                 &WriteCluster,
                                 ThisCluster - ClusterSize);
            do {

                PageFrameIndex = WriteCluster.Cluster[WriteCluster.StartIndex];
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
#endif //0
                *Page = PageFrameIndex;

                //
                // Remove the page from the modified list. Note that
                // write-in-progress marks the state.
                //

                //
                // Unlink the page so the same page won't be found
                // on the modified page list by color.
                //

                MiUnlinkPageFromList (Pfn1);
                NextColor = MI_GET_NEXT_COLOR(NextColor);

                MI_GET_MODIFIED_PAGE_BY_COLOR (PageFrameIndex,
                                               NextColor);

                //
                // Up the reference count for the physical page as there
                // is I/O in progress.
                //

                MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 16);
                Pfn1->u3.e2.ReferenceCount += 1;

                //
                // Clear the modified bit for the page and set the
                // write in progress bit.
                //

                Pfn1->u3.e1.Modified = 0;
                Pfn1->u3.e1.WriteInProgress = 1;
                ASSERT (Pfn1->OriginalPte.u.Soft.PageFileHigh == 0);

                MI_SET_PAGING_FILE_INFO (LongPte,
                                         Pfn1->OriginalPte,
                                         CurrentPagingFile->PageFileNumber,
                                         StartBit);

#if DBG
                if ((StartBit < 8192) &&
                    (CurrentPagingFile->PageFileNumber == 0)) {
                    ASSERT ((MmPagingFileDebug[StartBit] & 1) == 0);
                    MmPagingFileDebug[StartBit] =
                        (((ULONG_PTR)Pfn1->PteAddress << 3) |
                            ((ClusterSize & 0xf) << 1) | 1);
                }
#endif //DBG

                //
                // Change the original PTE contents to refer to
                // the paging file offset where this was written.
                //

                Pfn1->OriginalPte = LongPte;

                ClusterSize += 1;
                Page += 1;
                StartBit += 1;
#if 0 // COMMENTED OUT
                WriteCluster.Count -= 1;
                WriteCluster.StartIndex += 1;

            } while (WriteCluster.Count != 0);
#endif //0
        } else {

            //
            // This page was not destined for a paging file,
            // get another page.
            //
            // Get a page of the same color as the one which
            // was not usable.
            //

            MI_GET_MODIFIED_PAGE_BY_COLOR (PageFrameIndex,
                                           NextColor);
        }

        if (PageFrameIndex == MM_EMPTY_LIST) {
            break;
        }

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    } //end while

    if (ClusterSize != ThisCluster) {

        //
        // A complete cluster could not be located, free the
        // excess page file space that was reserved and adjust
        // the size of the packet.
        //

        RtlClearBits (CurrentPagingFile->Bitmap,
                      StartBit,
                      (ULONG)(ThisCluster - ClusterSize));

        CurrentPagingFile->FreeSpace += ThisCluster - ClusterSize;
        CurrentPagingFile->CurrentUsage -= ThisCluster - ClusterSize;

        //
        // If their are no pages to write, don't issue a write
        // request and restart the scan loop.
        //

        if (ClusterSize == 0) {

            //
            // No pages to write.  Inset the entry back in the
            // list.
            //

            if (IsListEmpty (&ModWriterEntry->PagingListHead->ListHead)) {
                KeSetEvent (&ModWriterEntry->PagingListHead->Event,
                            0,
                            FALSE);
            }

            InsertTailList (&ModWriterEntry->PagingListHead->ListHead,
                            &ModWriterEntry->Links);

            return;
        }
    }

    if (CurrentPagingFile->PeakUsage <
                                CurrentPagingFile->CurrentUsage) {
        CurrentPagingFile->PeakUsage =
                                CurrentPagingFile->CurrentUsage;
    }

    ModWriterEntry->Mdl.ByteCount = (ULONG)(ClusterSize * PAGE_SIZE);
    ModWriterEntry->LastPageToWrite = StartBit - 1;

    MmInfoCounters.DirtyWriteIoCount += 1;
    MmInfoCounters.DirtyPagesWriteCount += (ULONG)ClusterSize;

    //
    // For now release the PFN lock and wait for the write to complete.
    //

    UNLOCK_PFN (OldIrql);

#if DBG
    if (MmDebug & MM_DBG_MOD_WRITE) {
        DbgPrint("MM MODWRITE: modified page write begun @ %08lx by %08lx\n",
                StartingOffset.LowPart, ModWriterEntry->Mdl.ByteCount);
    }
#endif

    //
    // Issue the write request.
    //

    Status = IoAsynchronousPageWrite ( File,
                           &ModWriterEntry->Mdl,
                           &StartingOffset,
                           MiWriteComplete,
                           (PVOID)ModWriterEntry,
                           &ModWriterEntry->u.IoStatus,
                           &ModWriterEntry->Irp );

    if (NT_ERROR(Status)) {
        KdPrint(("MM MODWRITE: modified page write failed %lx\n", Status));

        //
        // An error has occurred, disable APCs and
        // call the write completion routine.
        //

        ModWriterEntry->u.IoStatus.Status = Status;
        ModWriterEntry->u.IoStatus.Information = 0;
        KeRaiseIrql (APC_LEVEL, &OldIrql);
        MiWriteComplete ((PVOID)ModWriterEntry,
                         &ModWriterEntry->u.IoStatus,
                         0 );
        KeLowerIrql (OldIrql);
    }

    LOCK_PFN (OldIrql);

    if (PageFileFull == TRUE) {
        MiPageFileFull ();
    }

    return;
}


#if 0 // COMMENTED OUT **************************************************
ULONG ClusterCounts[20];
ULONG ClusterSizes[20];
VOID
MiClusterWritePages (
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex,
    IN PMM_WRITE_CLUSTER WriteCluster,
    IN ULONG Size
    )

{
    PMMPTE PointerClusterPte;
    PMMPTE OriginalPte;
    PMMPTE StopPte;
    PMMPTE ThisPage;
    PMMPTE BasePage;
    ULONG Start;
    PMMPFN Pfn2;
    KIRQL OldIrql = 99;

    Start = MM_MAXIMUM_DISK_IO_SIZE / PAGE_SIZE;
    WriteCluster->Cluster[Start] = PageFrameIndex;
    WriteCluster->Count = 1;
    ClusterSizes[Size] += 1;
    if (Size == 1) {
        WriteCluster->StartIndex = Start;
        return;
    }

    //
    // The page points to a page table page which may not be
    // for the current process.  Map the page into hyperspace
    // reference it through hyperspace.
    //

    PointerClusterPte = Pfn1->PteAddress;
    BasePage = (PMMPTE)((ULONG_PTR)PointerClusterPte & ~(PAGE_SIZE - 1));
    ThisPage = BasePage;

    if ((PointerClusterPte < (PMMPTE)PDE_TOP) ||
        (!MmIsAddressValid (PointerClusterPte))) {

        //
        // Map page into hyperspace as it is either a page table
        // page or nonresident paged pool.
        //

        PointerClusterPte = (PMMPTE)((PCHAR)MiMapPageInHyperSpace (
                                        Pfn1->PteFrame, &OldIrql)
                                        +
                                BYTE_OFFSET (PointerClusterPte));
        ThisPage = (PMMPTE)((ULONG_PTR)PointerClusterPte & ~(PAGE_SIZE - 1));
    }

    OriginalPte = PointerClusterPte;
    ASSERT (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerClusterPte) == PageFrameIndex);

    //
    // Check backwards and forwards for other pages from this process
    // destined for the paging file.
    //

    StopPte = PointerClusterPte - (Size - 1);
    if (StopPte < ThisPage) {
        StopPte = ThisPage;
    }

    while (PointerClusterPte > StopPte) {
        PointerClusterPte -= 1;

        //
        // Look for the pointer at start of segment, quit as this is NOT
        // a prototype PTE.  Normal PTEs will not match this.
        //

        if (BasePage != (PMMPTE)
                        (ULONG_PTR)(PointerClusterPte->u.Long & ~(PAGE_SIZE - 1))) {

            if ((PointerClusterPte->u.Hard.Valid == 0) &&
                (PointerClusterPte->u.Soft.Prototype == 0) &&
                (PointerClusterPte->u.Soft.Transition == 1))  {

                //
                // PTE is in transition state, see if it is modified.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerClusterPte);
                Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
                ASSERT (Pfn2->OriginalPte.u.Soft.Prototype == 0);
                if ((Pfn2->u3.e1.Modified != 0 ) &&
                    (Pfn2->u3.e2.ReferenceCount == 0)) {

                    Start -= 1;
                    WriteCluster->Count += 1;
                    WriteCluster->Cluster[Start] = PageFrameIndex;
                }
            }
        }
        break;
    }

    WriteCluster->StartIndex = Start;
    PointerClusterPte = OriginalPte + 1;
    Start = MM_MAXIMUM_DISK_IO_SIZE / PAGE_SIZE;

    //
    // Remove pages looking forward from PointerClusterPte until
    // a cluster is filled or a PTE is not on the modified list.
    //

    ThisPage = (PMMPTE)((PCHAR)ThisPage + PAGE_SIZE);

    while ((WriteCluster->Count < Size) &&
           (PointerClusterPte < ThisPage)) {

        if ((PointerClusterPte->u.Hard.Valid == 0) &&
            (PointerClusterPte->u.Soft.Prototype == 0) &&
            (PointerClusterPte->u.Soft.Transition == 1))  {

            //
            // PTE is in transition state, see if it is modified.
            //

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerClusterPte);
            Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
            ASSERT (Pfn2->OriginalPte.u.Soft.Prototype == 0);
            if ((Pfn2->u3.e1.Modified != 0 ) &&
                (Pfn2->u3.e2.ReferenceCount == 0)) {

                Start += 1;
                WriteCluster->Count += 1;
                WriteCluster->Cluster[Start] = PageFrameIndex;
                PointerClusterPte += 1;
                continue;
            }
        }
        break;
    }

    if (OldIrql != 99) {
        MiUnmapPageInHyperSpace (OldIrql);
    }
    ClusterCounts[WriteCluster->Count] += 1;
    return;
}
#endif // COMMENTED OUT **************************************************


VOID
MiMappedPageWriter (
    IN PVOID StartContext
    )

/*++

Routine Description:

    Implements the NT secondary modified page writer thread.
    Requests for writes to mapped files are sent to this thread.
    This is required as the writing of mapped file pages could cause
    page faults resulting in requests for free pages.  But there
    could be no free pages - hence a dead lock.  Rather than deadlock
    the whole system waiting on the modified page writer, creating
    a secondary thread allows that thread to block without affecting
    on going page file writes.

Arguments:

    StartContext - not used.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMMOD_WRITER_MDL_ENTRY ModWriterEntry;
    KIRQL OldIrql = 0;
    NTSTATUS Status;
    KEVENT TempEvent;

    UNREFERENCED_PARAMETER (StartContext);

    //
    // Make this a real time thread.
    //

    (VOID) KeSetPriorityThread (&PsGetCurrentThread()->Tcb,
                                LOW_REALTIME_PRIORITY + 1);

    //
    // Let the file system know that we are getting resources.
    //

    FsRtlSetTopLevelIrpForModWriter();

    KeInitializeEvent (&TempEvent, NotificationEvent, FALSE);

    while (TRUE) {
        KeWaitForSingleObject (&MmMappedPageWriterEvent,
                               WrVirtualMemory,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);

        LOCK_PFN (OldIrql);
        if (IsListEmpty (&MmMappedPageWriterList)) {
            KeClearEvent (&MmMappedPageWriterEvent);
            UNLOCK_PFN (OldIrql);
        } else {

            ModWriterEntry = (PMMMOD_WRITER_MDL_ENTRY)RemoveHeadList (
                                                &MmMappedPageWriterList);

            UNLOCK_PFN (OldIrql);


            if (ModWriterEntry->ControlArea->u.Flags.FailAllIo == 1) {
                Status = STATUS_UNSUCCESSFUL;

            } else if (FsRtlAcquireFileForModWrite (ModWriterEntry->File,
                                                    &ModWriterEntry->u.LastByte,
                                                    &ModWriterEntry->FileResource)) {

                //
                // Issue the write request.
                //

                Status = IoAsynchronousPageWrite (
                                       ModWriterEntry->File,
                                       &ModWriterEntry->Mdl,
                                       &ModWriterEntry->WriteOffset,
                                       MiWriteComplete,
                                       (PVOID)ModWriterEntry,
                                       &ModWriterEntry->u.IoStatus,
                                       &ModWriterEntry->Irp );
            } else {

                //
                // Unable to get the file system resources, set error status
                // to lock conflict (ignored by MiWriteComplete) so the APC
                // routine is explicitly called.
                //

                Status = STATUS_FILE_LOCK_CONFLICT;
            }

            if (NT_ERROR(Status)) {

                //
                // An error has occurred, disable APC's and
                // call the write completion routine.
                //

                ModWriterEntry->u.IoStatus.Status = Status;
                ModWriterEntry->u.IoStatus.Information = 0;
                KeRaiseIrql (APC_LEVEL, &OldIrql);
                MiWriteComplete ((PVOID)ModWriterEntry,
                                 &ModWriterEntry->u.IoStatus,
                                 0 );
                KeLowerIrql (OldIrql);
            }
#if 0
    //TEMPORARY code to use synchronous I/O here.

            //
            // Issue the write request.
            //

            Status = IoSynchronousPageWrite (
                                   ModWriterEntry->File,
                                   &ModWriterEntry->Mdl,
                                   &ModWriterEntry->WriteOffset,
                                   &TempEvent,
                                   &ModWriterEntry->u.IoStatus );

            if (NT_ERROR(Status)) {
                ModWriterEntry->u.IoStatus.Status = Status;
                ModWriterEntry->u.IoStatus.Information = 0;
            }

            if (NT_ERROR(ModWriterEntry->u.IoStatus.Status)) {
                KdPrint(("MM MODWRITE: modified page write failed %lx\n", Status));
            }

            //
            // Call the write completion routine.
            //

            KeRaiseIrql (APC_LEVEL, &OldIrql);
            MiWriteComplete ((PVOID)ModWriterEntry,
                             &ModWriterEntry->IoStatus,
                             0 );
            KeLowerIrql (OldIrql);
#endif //0

        }

    }
}

BOOLEAN
MmDisableModifiedWriteOfSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
    )

/*++

Routine Description:

    This function disables page writing by the modified page writer for
    the section which is mapped by the specified file object pointer.

    This should only be used for files which CANNOT be mapped by user
    programs, e.g., volume files, directory files, etc.

Arguments:

    SectionObjectPointer - Supplies a pointer to the section objects


Return Value:

    Returns TRUE if the operation was a success, FALSE if either
    the there is no section or the section already has a view.

--*/

{
    PCONTROL_AREA ControlArea;
    KIRQL OldIrql;
    BOOLEAN state = 1;

    LOCK_PFN (OldIrql);

    ControlArea = ((PCONTROL_AREA)(SectionObjectPointer->DataSectionObject));

    if (ControlArea != NULL) {
        if (ControlArea->NumberOfMappedViews == 0) {

            //
            // There are no views to this section, indicate no modified
            // page writing is allowed.
            //

            ControlArea->u.Flags.NoModifiedWriting = 1;
        } else {

            //
            // Return the current modified page writing state.
            //

            state = (BOOLEAN)ControlArea->u.Flags.NoModifiedWriting;
        }
    } else {

        //
        // This file no longer has an associated segment.
        //

        state = 0;
    }

    UNLOCK_PFN (OldIrql);
    return state;
}


#define ROUND_UP(VALUE,ROUND) ((ULONG)(((ULONG)VALUE + \
                               ((ULONG)ROUND - 1L)) & (~((ULONG)ROUND - 1L))))
NTSTATUS
MmGetPageFileInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine returns information about the currently active paging
    files.

Arguments:

    SystemInformation - Returns the paging file information.

    SystemInformationLength - Supplies the length of the SystemInformation
                              buffer.

    Length - Returns the length of the paging file information placed in the
             buffer.

Return Value:

    Returns the status of the operation.

--*/

{
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    ULONG NextEntryOffset = 0;
    ULONG TotalSize = 0;
    ULONG i;
    UNICODE_STRING UserBufferPageFileName;

    PAGED_CODE();

    *Length = 0;
    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)SystemInformation;

    PageFileInfo->TotalSize = 0;

    for (i = 0; i < MmNumberOfPagingFiles; i += 1) {
        PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(
                                (PUCHAR)PageFileInfo + NextEntryOffset);
        NextEntryOffset = sizeof(SYSTEM_PAGEFILE_INFORMATION);
        TotalSize += sizeof(SYSTEM_PAGEFILE_INFORMATION);

        if (TotalSize > SystemInformationLength) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        PageFileInfo->TotalSize = (ULONG)MmPagingFile[i]->Size;
        PageFileInfo->TotalInUse = (ULONG)MmPagingFile[i]->CurrentUsage;
        PageFileInfo->PeakUsage = (ULONG)MmPagingFile[i]->PeakUsage;

        //
        // The PageFileName portion of the UserBuffer must be saved locally
        // to protect against a malicious thread changing the contents.  This
        // is because we will reference the contents ourselves when the actual
        // string is copied out carefully below.
        //

        UserBufferPageFileName.Length = MmPagingFile[i]->PageFileName.Length;
        UserBufferPageFileName.MaximumLength = MmPagingFile[i]->PageFileName.Length + sizeof(WCHAR);
        UserBufferPageFileName.Buffer = (PWCHAR)(PageFileInfo + 1);

        PageFileInfo->PageFileName = UserBufferPageFileName;

        TotalSize += ROUND_UP (UserBufferPageFileName.MaximumLength,
                               sizeof(ULONG));
        NextEntryOffset += ROUND_UP (UserBufferPageFileName.MaximumLength,
                                     sizeof(ULONG));

        if (TotalSize > SystemInformationLength) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Carefully reference the user buffer here.
        //

        RtlMoveMemory(UserBufferPageFileName.Buffer,
                      MmPagingFile[i]->PageFileName.Buffer,
                      MmPagingFile[i]->PageFileName.Length);
        UserBufferPageFileName.Buffer[
                    MmPagingFile[i]->PageFileName.Length/sizeof(WCHAR)] = UNICODE_NULL;
        PageFileInfo->NextEntryOffset = NextEntryOffset;
    }
    PageFileInfo->NextEntryOffset = 0;
    *Length = TotalSize;
    return STATUS_SUCCESS;
}


NTSTATUS
MiCheckPageFileMapping (
    IN PFILE_OBJECT File
    )

/*++

Routine Description:

    Non-pagable routine to check to see if a given file has
    no sections and therefore is eligible to become a paging file.

Arguments:

    File - Supplies a pointer to the file object.

Return Value:

    Returns STATUS_SUCCESS if the file can be used as a paging file.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    if (File->SectionObjectPointer == NULL) {
        UNLOCK_PFN (OldIrql);
        return STATUS_SUCCESS;
    }

    if ((File->SectionObjectPointer->DataSectionObject != NULL) ||
        (File->SectionObjectPointer->ImageSectionObject != NULL)) {

        UNLOCK_PFN (OldIrql);
        return STATUS_INCOMPATIBLE_FILE_MAP;
    }
    UNLOCK_PFN (OldIrql);
    return STATUS_SUCCESS;

}


VOID
MiInsertPageFileInList (
    VOID
    )

/*++

Routine Description:

    Non-pagable routine to add a page file into the list
    of system wide page files.

Arguments:

    None, implicitly found through page file structures.

Return Value:

    None.  Operation cannot fail.

--*/

{
    KIRQL OldIrql;
    ULONG Count;

    LOCK_PFN (OldIrql);

    MmNumberOfPagingFiles += 1;
    Count = MmNumberOfPagingFiles;

    if (IsListEmpty (&MmPagingFileHeader.ListHead)) {
        KeSetEvent (&MmPagingFileHeader.Event, 0, FALSE);
    }

    InsertTailList (&MmPagingFileHeader.ListHead,
                    &MmPagingFile[MmNumberOfPagingFiles - 1]->Entry[0]->Links);

    MmPagingFile[MmNumberOfPagingFiles - 1]->Entry[0]->CurrentList =
                                                &MmPagingFileHeader.ListHead;

    InsertTailList (&MmPagingFileHeader.ListHead,
                    &MmPagingFile[MmNumberOfPagingFiles - 1]->Entry[1]->Links);

    MmPagingFile[MmNumberOfPagingFiles - 1]->Entry[1]->CurrentList =
                                                &MmPagingFileHeader.ListHead;

    MmNumberOfActiveMdlEntries += 2;

    UNLOCK_PFN (OldIrql);


    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

    if (Count == 1) {

        //
        // We have just created the first paging file.  Start the
        // modified page writer.
        //

        MmTotalCommitLimit =
                    MmPagingFile[MmNumberOfPagingFiles - 1]->FreeSpace + MmOverCommit;

        MmTotalCommitLimitMaximum =
                    MmPagingFile[MmNumberOfPagingFiles - 1]->MaximumSize + MmOverCommit;

        //
        // Keep commit limit above 20mb so we can boot with a small paging
        // file and clean things up.
        //

        if (MmTotalCommitLimit < 5500) {
            MmOverCommit2 = 5500 - MmTotalCommitLimit;
            MmTotalCommitLimit = 5500;
        }

        if (MmTotalCommitLimitMaximum < 5500) {
            MmTotalCommitLimitMaximum = MmTotalCommitLimit;
        }

    } else {

        //
        // Balance overcommitment in the case an extension was granted.
        //

        if (MmOverCommit2 > MmPagingFile[MmNumberOfPagingFiles - 1]->FreeSpace) {
            MmOverCommit2 -= MmPagingFile[MmNumberOfPagingFiles - 1]->FreeSpace;
        } else {
            MmTotalCommitLimit +=
              MmPagingFile[MmNumberOfPagingFiles - 1]->FreeSpace - MmOverCommit2;
            MmTotalCommitLimitMaximum +=
              MmPagingFile[MmNumberOfPagingFiles - 1]->MaximumSize - MmOverCommit2;
            MmOverCommit2 = 0;
        }
    }

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    return;
}

VOID
MiPageFileFull (
    )

/*++

Routine Description:

    This routine is called when no space can be found in a paging file.
    It looks through all the paging files to see if ample space is
    available and if not, tries to expand the paging files.

    If more than 90% of all paging files is used, the commitment limit
    is set to the total and then 100 pages are added.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;
    PFN_NUMBER Total;
    PFN_NUMBER Free;
    KIRQL OldIrql;
    SIZE_T SizeToExpand;

    MM_PFN_LOCK_ASSERT();

    Total = 0;
    Free = 0;
    OldIrql = 0;

    i = 0;
    do {
        Total += MmPagingFile[i]->Size;
        Free += MmPagingFile[i]->FreeSpace;
        i += 1;
    } while (i < MmNumberOfPagingFiles);

    //
    // Check to see if more than 90% of the total space has been used.
    //

    if (((Total >> 5) + (Total >> 4)) >= Free) {

        //
        // Try to expand the paging files.
        //

        UNLOCK_PFN (OldIrql);

        //
        // Check commit limits and set the limit to what is now used.
        // If all the pagefiles are already at their maximums, then don't
        // make things worse by setting commit to the maximum - this gives
        // systems with lots of memory a longer lease on life when they have
        // small pagefiles.
        //

        SizeToExpand = 0;
        i = 0;

        ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

        do {
            SizeToExpand += MmPagingFile[i]->MaximumSize - MmPagingFile[i]->Size;
            i += 1;
        } while (i < MmNumberOfPagingFiles);
    
        if (SizeToExpand == 0) {
            ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

            //
            // Display a popup once.
            //

            if (MmPageFileFullPopupShown == FALSE) {
                MmPageFileFullPopupShown = TRUE;
                MiCauseOverCommitPopup (1, 0);
            }

            LOCK_PFN (OldIrql);
            return;
        }

        if (MmTotalCommittedPages <= MmTotalCommitLimit + 50) {

            //
            // The total commit limit is less than the number of committed
            // pages + 50. Reset commit limit.
            //

            if (MmTotalCommittedPages < MmTotalCommitLimit) {

                if (MmPageFileFullExtendPages) {
                    ASSERT (MmTotalCommittedPages >= MmPageFileFullExtendPages);
                    MmTotalCommittedPages -= MmPageFileFullExtendPages;
                    MmPageFileFullExtendPages = 0;
                }

                MmPageFileFullExtendPages = MmTotalCommitLimit - MmTotalCommittedPages;
                MmPageFileFullExtendCount += 1;

                MmTotalCommittedPages = MmTotalCommitLimit;
            }

            //
            // Charge 100 pages against the commitment.
            //

            MiPageFileFullCharge += MI_PAGEFILE_FULL_CHARGE;

            ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

            MiChargeCommitmentCantExpand (MI_PAGEFILE_FULL_CHARGE, TRUE);

            MM_TRACK_COMMIT (MM_DBG_COMMIT_PAGEFILE_FULL, 100);

            //
            // Display a popup once.
            //

            if (MmPageFileFullPopupShown == FALSE) {
                MmPageFileFullPopupShown = TRUE;
                MiCauseOverCommitPopup (1, 0);
            }

            //
            // Delay a bit before returning the commitment so the segment
            // dereference thread gets a chance to actually grow the pagefile.
            //

            if (MiPageFileFullCharge >= 5 * MI_PAGEFILE_FULL_CHARGE) {
                ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
                SizeToExpand = MiPageFileFullCharge;
                MiPageFileFullCharge = 0;
                ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
                MiReturnCommitment (SizeToExpand);
            }

        }
        else {

            //
            // Commit limit is lower than the number of committed pages.
            //

            ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
        }

        LOCK_PFN (OldIrql);
    }
    return;
}

VOID
MiFlushAllPages (
    VOID
    )

/*++

Routine Description:

    Forces a write of all modified pages.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  APC_LEVEL or less.

--*/

{
    ULONG j = 0xff;

    MmWriteAllModifiedPages = TRUE;
    KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);

    do {
        KeDelayExecutionThread (KernelMode, FALSE, &Mm30Milliseconds);
        j -= 1;
    } while ((MmModifiedPageListHead.Total > 50) && (j > 0));

    MmWriteAllModifiedPages = FALSE;
    return;
}


LOGICAL
MiIssuePageExtendRequest (
    IN PMMPAGE_FILE_EXPANSION PageExtend
    )

/*++

Routine Description:

    Queue a message to the segment dereferencing / pagefile extending
    thread to see if the page file can be extended.  Extension is done
    in the context of a system thread due to mutexes which the current
    thread may be holding.
    
Arguments:

    PageExtend - Supplies a pointer to the page file extension request.

Return Value:

    TRUE indicates the request completed.  FALSE indicates the request timed
    out and was removed.

Environment:

    Kernel mode.  No locks held.  APC level or less.

--*/

{
    KIRQL OldIrql;
    NTSTATUS status;
    PLIST_ENTRY NextEntry;

    ExAcquireFastLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);
    InsertTailList ( &MmDereferenceSegmentHeader.ListHead,
                     &PageExtend->DereferenceList);
    ExReleaseFastLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

    KeReleaseSemaphore (&MmDereferenceSegmentHeader.Semaphore,
                        0L,
                        1L,
                        TRUE);

    //
    // Wait for the thread to extend the paging file.
    //

    status = KeWaitForSingleObject (&PageExtend->Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    (PageExtend->RequestedExpansionSize < 10) ?
                                        &MmOneSecond : &MmTwentySeconds);

    if (status == STATUS_TIMEOUT) {

        //
        // The wait has timed out, if this request has not
        // been processed, remove it from the list and check
        // to see if we should allow this request to succeed.
        // This prevents a deadlock between the file system
        // trying to allocate memory in the FSP and the
        // segment dereferencing thread trying to close a
        // file object, and waiting in the file system.
        //

        KdPrint(("MiIssuePageExtendRequest: wait timed out, page-extend= %lx, quota = %lx\n",
                   PageExtend, PageExtend->RequestedExpansionSize));

        ExAcquireFastLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);

        NextEntry = MmDereferenceSegmentHeader.ListHead.Flink;

        while (NextEntry != &MmDereferenceSegmentHeader.ListHead) {

            //
            // Check to see if this is the entry we are waiting for.
            //

            if (NextEntry == &PageExtend->DereferenceList) {

                //
                // Remove this entry.
                //

                RemoveEntryList (&PageExtend->DereferenceList);
                ExReleaseFastLock (&MmDereferenceSegmentHeader.Lock, OldIrql);
                return FALSE;
            }
            NextEntry = NextEntry->Flink;
        }

        ExReleaseFastLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

        //
        // Entry is being processed, wait for completion.
        //

        KdPrint (("MiIssuePageExtendRequest: rewaiting...\n"));

        KeWaitForSingleObject (&PageExtend->Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);
    }

    return TRUE;
}


VOID
MiIssuePageExtendRequestNoWait (
    IN PFN_NUMBER SizeInPages
    )

/*++

Routine Description:

    Queue a message to the segment dereferencing / pagefile extending
    thread to see if the page file can be extended.  Extension is done
    in the context of a system thread due to mutexes which the current
    thread may be holding.
    
Arguments:

    SizeInPages - Supplies the size in pages to increase the page file(s) by.
                  This is rounded up to a 1MB multiple by this routine.

Return Value:

    TRUE indicates the request completed.  FALSE indicates the request timed
    out and was removed.

Environment:

    Kernel mode.  No locks held.  APC level or less.

    Note this routine must be very careful to not use any paged
    pool as the only reason it is being called is because pool is depleted.

--*/

{
    KIRQL OldIrql;
    NTSTATUS status;
    PLIST_ENTRY NextEntry;

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    ExAcquireFastLock (&MmChargeCommitmentLock, &OldIrql);

    if (MmAttemptForCantExtend.InProgress != FALSE) {

        //
        // An expansion request is already in progress, assume
        // it will help enough (another can always be issued later) and
        // that it will succeed.
        //

        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
        return;
    }

    MmAttemptForCantExtend.InProgress = TRUE;
    ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);

    SizeInPages = (SizeInPages + ONEMB_IN_PAGES - 1) & ~(ONEMB_IN_PAGES - 1);

    MmAttemptForCantExtend.RequestedExpansionSize = SizeInPages;

    ExAcquireFastLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);

    InsertTailList ( &MmDereferenceSegmentHeader.ListHead,
                     &MmAttemptForCantExtend.DereferenceList);

    ExReleaseFastLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

    KeReleaseSemaphore (&MmDereferenceSegmentHeader.Semaphore,
                        0L,
                        1L,
                        FALSE);

    return;
}
