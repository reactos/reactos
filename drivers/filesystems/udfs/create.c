////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Create.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Create"/"Open" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

#define IsFileObjectReadOnly(FO) (!((FO)->WriteAccess | (FO)->DeleteAccess))

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_CREATE

#define         MEM_USOBJ_TAG                   "US_Obj"

_When_(RelatedTypeOfOpen != UnopenedFileObject, _At_(RelatedCcb, _In_))
_When_(RelatedTypeOfOpen == UnopenedFileObject, _At_(RelatedCcb, _In_opt_))
_When_(RelatedTypeOfOpen != UnopenedFileObject, _At_(RelatedFileName, _In_))
_When_(RelatedTypeOfOpen == UnopenedFileObject, _At_(RelatedFileName, _In_opt_))
NTSTATUS
UDFNormalizeFileNames(
    _Inout_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ BOOLEAN OpenByFileId,
    _In_ TYPE_OF_OPEN RelatedTypeOfOpen,
    PCCB RelatedCcb,
    PUNICODE_STRING RelatedFileName,
    _Inout_ PUNICODE_STRING FileName,
    _Inout_ PUNICODE_STRING RemainingName
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCompleteFcbOpen(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_STACK_LOCATION IrpSp,
    _In_ PVCB Vcb,
    _Inout_ PFCB *CurrentFcb,
    _In_ TYPE_OF_OPEN TypeOfOpen,
    _In_ ULONG UserCcbFlags,
    _In_ ULONG CreateDisposition,
    _In_ BOOLEAN VcbLocked,
    _In_ ULONG DesiredInformation
    );

NTSTATUS
UDFOpenObjectByFileId(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_STACK_LOCATION IrpSp,
    _In_ PVCB Vcb,
    _Inout_ PFCB *CurrentFcb
    );

NTSTATUS
UDFSupersedeOrOverwriteFile(
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN PUDF_FILE_INFO FileInfo,
    IN LONGLONG AllocationSize,
    IN ULONG FileAttributes,
    IN BOOLEAN Supersede
    )
{
    NTSTATUS RC;
    ULONG NewFileAttributes;

    /* Preserve the prior cached generation before truncating or replacing
     * its allocation. Cc must run before PagingIoResource is acquired. */
    RC = UDFDrainNewFileData(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(RC))
        return RC;

    UDFAcquirePagingIoExclusive(IrpContext, Fcb);

    _SEH2_TRY {

        if (!MmCanFileBeTruncated(&Fcb->FcbNonpaged->SegmentObject, &UdfData.UDFLargeZero)) {

            AdPrint(("    Can't truncate. File is mapped\n"));
            try_return(RC = STATUS_USER_MAPPED_FILE);
        }

        // Truncate file to zero
        RC = UDFResizeFile__(IrpContext, Vcb, FileInfo, 0);

        if (!NT_SUCCESS(RC)) {

            AdPrint(("    Error during resize operation\n"));
            try_return(RC);
        }

        // Set file sizes
        Fcb->Header.AllocationSize.QuadPart = UDFSysGetAllocSize(Vcb, AllocationSize);
        Fcb->Header.FileSize.QuadPart = 0;
        Fcb->Header.ValidDataLength.QuadPart = 0;
        Fcb->FcbState &= ~UDF_FCB_DELAY_CLOSE;

        MmPrint(("    CcSetFileSizes()\n"));
        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
        Fcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;

        // Set attributes
        NewFileAttributes = FileAttributes | FILE_ATTRIBUTE_ARCHIVE;
        if (!Supersede) {
            // For Overwrite, combine with current attributes from FCB cache
            NewFileAttributes |= Fcb->FileAttributes;
        }
        // Write back to DirIndex
        UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index),
                           FileInfo->Dloc->FileEntry, NewFileAttributes);
        // Update FCB cache
        Fcb->FileAttributes = NewFileAttributes;

        // Recreating the main file drops its named streams. For a regular file
        // (not itself a stream or stream directory) that has a stream directory,
        // delete the streams here. Safe under the Vcb-exclusive overwrite path.
        if (!UDFIsAStream(FileInfo) && !UDFIsAStreamDir(FileInfo) &&
            UDFHasAStreamDir(FileInfo)) {

            RC = UDFDeleteAllStreams(IrpContext, Vcb, FileInfo);
            if (!NT_SUCCESS(RC)) {
                AdPrint(("    Error dropping streams on overwrite\n"));
                try_return(RC);
            }
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        UDFReleasePagingIo(IrpContext, Fcb);
    } _SEH2_END;

    return RC;
} // end UDFSupersedeOrOverwriteFile()


/*************************************************************************
*
* Function: UDFOpenExistingFcb()
*
* Description:
*   Open an existing FCB. Checks access rights and share access,
*   creates the CCB via UDFCompleteFcbOpen, and handles post-open
*   actions (MmFlushImageSection, cache purge, supersede/overwrite).
*   Caller must validate file/directory type and create disposition
*   before calling this function.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFOpenExistingFcb(
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN PUDF_FILE_INFO NewFileInfo,
    IN OUT PFCB *PtrNewFcb,
    IN BOOLEAN IgnoreCase,
    IN PFILE_OBJECT RelatedFileObject OPTIONAL,
    IN PFCB RelatedFcb OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG CcbFlags = 0;
    TYPE_OF_OPEN TypeOfOpen;
    ULONG DesiredInformation;
    USHORT TmpFileAttributes;

    ULONG Options = IrpSp->Parameters.Create.Options;
    ULONG CreateDisposition = (Options >> 24) & 0x000000FF;
    USHORT FileAttributes = (USHORT)(IrpSp->Parameters.Create.FileAttributes & ~FILE_ATTRIBUTE_NORMAL);
    LONGLONG AllocationSize = IrpContext->Irp->Overlay.AllocationSize.QuadPart;
    ACCESS_MASK DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    USHORT ShareAccess = IrpSp->Parameters.Create.ShareAccess;
    PACCESS_STATE AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;
    BOOLEAN DeleteOnClose = BooleanFlagOn(Options, FILE_DELETE_ON_CLOSE);

    PAGED_CODE();
    ASSERT(*PtrNewFcb);

    // Check share access BEFORE creating CCB/incrementing counts.
    Status = UDFCheckAccessRights(FileObject, AccessState, *PtrNewFcb, NULL, DesiredAccess, ShareAccess);
    if (!NT_SUCCESS(Status)) {
        AdPrint(("    Access/Share access check failed\n"));
        return Status;
    }

    if (DeleteOnClose && ((*PtrNewFcb)->FcbState & UDF_FCB_READ_ONLY)) {
        AdPrint(("    Can't delete Read-Only file\n"));
        return STATUS_CANNOT_DELETE;
    }

    /* Report a held named stream before accepting FILE_DELETE_ON_CLOSE.
     * Otherwise cleanup can refuse the delete after this CREATE has already
     * reported success to its caller.  VCB is held shared here, while stream
     * namespace changes require it exclusive.  A live stream holds the
     * shared Dloc's SDirInfo; the cleanup preflight remains the final guard. */
    if (DeleteOnClose &&
        UDFHasAStreamDir((*PtrNewFcb)->FileInfo) &&
        UDFStreamDirectoryHasLiveHandles(
            (*PtrNewFcb)->FileInfo->Dloc->SDirInfo)) {
        return STATUS_CANNOT_DELETE;
    }

    // Determine type of open based on FCB type
    if (UDFIsADirectory((*PtrNewFcb)->FileInfo)) {
        TypeOfOpen = UserDirectoryOpen;
    } else {
        TypeOfOpen = UserFileOpen;
    }

    // Set CCB flags
    if (IgnoreCase) {
        SetFlag(CcbFlags, CCB_FLAG_IGNORE_CASE);
    }

    // Determine Information based on CreateDisposition
    switch (CreateDisposition) {
    case FILE_SUPERSEDE:
        DesiredInformation = FILE_SUPERSEDED;
        break;
    case FILE_CREATE:
        DesiredInformation = FILE_CREATED;
        break;
    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
        DesiredInformation = FILE_OVERWRITTEN;
        break;
    default:
        DesiredInformation = FILE_OPENED;
        break;
    }

    // Create CCB, set share access, increment open counts
    Status = UDFCompleteFcbOpen(IrpContext,
                                IrpSp,
                                Vcb,
                                PtrNewFcb,
                                TypeOfOpen,
                                CcbFlags,
                                CreateDisposition,
                                FALSE,               // VcbLocked
                                DesiredInformation);
    if (!NT_SUCCESS(Status)) return Status;

    (*PtrNewFcb)->Header.IsFastIoPossible = UDFIsFastIoPossible(*PtrNewFcb);

    if (!UDFIsADirectory(NewFileInfo)) {
        //  If the user wants 'write access' access to the file make sure there
        //  is not a process mapping this file as an image.  Any attempt to
        //  delete the file will be stopped in fileinfo.cpp
        //
        //  If the user wants to delete on close, we must check at this
        //  point though.
        if ((DesiredAccess & FILE_WRITE_DATA) || DeleteOnClose) {

            MmPrint(("    MmFlushImageSection();\n"));

            if (!MmFlushImageSection(&(*PtrNewFcb)->FcbNonpaged->SegmentObject, MmFlushForWrite)) {

                Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                              STATUS_SHARING_VIOLATION;
                AdPrint(("    File is mapped or deletion in progress\n"));
                return Status;
            }
        }

        if (FlagOn(Options, FILE_NO_INTERMEDIATE_BUFFERING) &&
           !((*PtrNewFcb)->CachedOpenHandleCount) &&
            ((*PtrNewFcb)->FcbNonpaged->SegmentObject.DataSectionObject) ) {
            //  If this is a non-cached open, and there are no open cached
            //  handles, but there is still a data section, attempt a flush
            //  and purge operation to avoid cache coherency overhead later.
            //  We ignore any I/O errors from the flush.
            MmPrint(("    CcFlushCache()\n"));
            CcFlushCache(&(*PtrNewFcb)->FcbNonpaged->SegmentObject, NULL, 0, NULL);
            MmPrint(("    CcPurgeCacheSection()\n"));
            CcPurgeCacheSection(&(*PtrNewFcb)->FcbNonpaged->SegmentObject, NULL, 0, FALSE);
        }
    }

    if (DeleteOnClose && UDFIsADirectory(NewFileInfo) && !UDFIsDirEmpty__(NewFileInfo)) {
        AdPrint(("    Directory in not empry\n"));
        return STATUS_DIRECTORY_NOT_EMPTY;
    }

    // Get attributes from FCB cache.
    TmpFileAttributes = (USHORT)(*PtrNewFcb)->FileAttributes;

    // Reject DeleteOnClose on readonly files
    if (DeleteOnClose &&
       (TmpFileAttributes & FILE_ATTRIBUTE_READONLY)) {
        AdPrint(("    Read-only file with DeleteOnClose not allowed\n"));
        return STATUS_CANNOT_DELETE;
    }

    // If a supersede or overwrite was requested, do so now ...
    if ((CreateDisposition == FILE_SUPERSEDE) ||
       (CreateDisposition == FILE_OVERWRITE) ||
       (CreateDisposition == FILE_OVERWRITE_IF)) {

        ASSERT(!UDFIsADirectory(NewFileInfo));

        if (CreateDisposition == FILE_SUPERSEDE) {
            BOOLEAN RestoreRO = FALSE;

            // NT wants us to allow Supersede on RO files
            if ((*PtrNewFcb)->FcbState & UDF_FCB_READ_ONLY) {
                RestoreRO = TRUE;
                (*PtrNewFcb)->FcbState &= ~UDF_FCB_READ_ONLY;
            }
            Status = UDFCheckAccessRights(NULL, NULL, *PtrNewFcb, NULL, DELETE, 0);
            if (RestoreRO) {
                (*PtrNewFcb)->FcbState |= UDF_FCB_READ_ONLY;
            }
            if (!NT_SUCCESS(Status)) {
                AdPrint(("    Can't supersede. DELETE permission required\n"));
                return Status;
            }
        } else {
            Status = UDFCheckAccessRights(NULL, NULL, *PtrNewFcb, NULL,
                        FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES, 0);
            if (!NT_SUCCESS(Status)) {
                AdPrint(("    Can't overwrite. Permission denied\n"));
                return Status;
            }
        }
        // Existing & requested System and Hidden bits must match
        if ( (TmpFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) &
            (FileAttributes ^ (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) ) {
            AdPrint(("    The Hidden and/or System bits do not match\n"));
            return STATUS_ACCESS_DENIED;
        }

        // Truncate file and set attributes
        Status = UDFSupersedeOrOverwriteFile(
            IrpContext,
            FileObject,
            Vcb,
            *PtrNewFcb,
            NewFileInfo,
            AllocationSize,
            FileAttributes,
            (BOOLEAN)(CreateDisposition == FILE_SUPERSEDE)
        );
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        // notify changes
        UDFNotifyReportChange( IrpContext, Vcb, NewFileInfo->Fcb,
                                   FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE,
                                   FILE_ACTION_MODIFIED,
                                   NULL, FileObject);

        // Update parent object
        if ((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) &&
           RelatedFcb &&
           RelatedFileObject &&
           (RelatedFcb->FileInfo == NewFileInfo->ParentFile)) {
                RelatedFileObject->Flags |= (FO_FILE_MODIFIED | FO_FILE_SIZE_CHANGED);
        }
    }

    // Newly created file — set attributes, sizes, and send notification
    if (CreateDisposition == FILE_CREATE) {

        // NT requires ARCHIVE on files
        if (!UDFIsADirectory(NewFileInfo))
            FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;

        UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(NewFileInfo), NewFileInfo->Index),
                           NewFileInfo->Dloc->FileEntry, FileAttributes);
        (*PtrNewFcb)->FileAttributes = FileAttributes;
        if (FileAttributes & FILE_ATTRIBUTE_READONLY)
            (*PtrNewFcb)->FcbState |= UDF_FCB_READ_ONLY;

        (*PtrNewFcb)->Header.FileSize.QuadPart =
        (*PtrNewFcb)->Header.ValidDataLength.QuadPart = 0;
        if (AllocationSize) {
            (*PtrNewFcb)->Header.AllocationSize.QuadPart = AllocationSize;
            MmPrint(("    CcIsFileCached()\n"));
            if (CcIsFileCached(FileObject)) {
                 MmPrint(("    CcSetFileSizes()\n"));
                 BrutePoint();
                 CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&(*PtrNewFcb)->Header.AllocationSize);
                 (*PtrNewFcb)->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
            }
        }

        if (UDFIsAStream(NewFileInfo)) {
            UDFNotifyReportChange( IrpContext, Vcb, *PtrNewFcb,
                                       FILE_NOTIFY_CHANGE_STREAM_NAME,
                                       FILE_ACTION_ADDED_STREAM,
                                       NULL, FileObject);
        } else {
            UDFNotifyReportChange( IrpContext, Vcb, *PtrNewFcb,
                                       UDFIsADirectory(NewFileInfo) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_ADDED,
                                       NULL, FileObject);
        }
    }

    // Update parent object
    if ((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_READ) &&
       RelatedFcb &&
       RelatedFileObject &&
       (RelatedFcb->FileInfo == NewFileInfo->ParentFile)) {
            RelatedFileObject->Flags |= FO_FILE_FAST_IO_READ;
    }

    return STATUS_SUCCESS;

} // end UDFOpenExistingFcb()


/*************************************************************************
*
* Function: UDFOpenObjectFromDirContext()
*
* Description:
*   Open a file or directory found via directory search (UDFFindDirEntry).
*   Creates FileInfo from disk, creates or finds FCB, acquires FCB lock,
*   creates LCB for intermediate path components.
*
*   When PerformUserOpen is TRUE, also completes the open for the user
*   (access check, CCB creation, supersede/overwrite handling).
*   When FALSE, only sets up internal structures for path traversal.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFOpenObjectFromDirContext(
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PDIR_ENUM_CONTEXT DirContext OPTIONAL,
    IN PUDF_FILE_INFO RelatedFileInfo,
    IN OUT PFCB *CurrentFcb,
    IN OUT PFCB *PreviousFcb,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN PerformUserOpen,
    IN ULONG CreateDisposition,
    IN ULONG RemainingNameLength,
    IN PFILE_OBJECT RelatedFileObject OPTIONAL,
    IN PFCB RelatedFcb OPTIONAL,
    IN PUDF_FILE_INFO ExistingFileInfo OPTIONAL,
    OUT PUDF_FILE_INFO *OutFileInfo,
    OUT PFCB *OutFcb
    )
{
    NTSTATUS Status;
    PUDF_FILE_INFO NewFileInfo = NULL;
    PFCB PtrNewFcb = NULL;

    PAGED_CODE();

    *OutFileInfo = NULL;
    *OutFcb = NULL;

    if (ExistingFileInfo) {
        // Use pre-created FileInfo (e.g., from UDFCreateFile__).
        // Skip disk read — FileInfo is already open with valid RefCount.
        NewFileInfo = ExistingFileInfo;
        Status = STATUS_SUCCESS;
    } else {
        // Step 1: Open FileInfo from directory context (read from disk or find cached)
        ASSERT(DirContext);
        Status = UDFOpenFileInfoFromDirContext(IrpContext, Vcb, DirContext, TRUE, &NewFileInfo);
        if (Status == STATUS_FILE_DELETED) {
            NewFileInfo = NULL;
            AdPrint(("    File deleted\n"));
            return STATUS_ACCESS_DENIED;
        }
        if (Status == STATUS_SHARING_PAUSED) {
            AdPrint(("    Dloc is being initialized\n"));
            BrutePoint();
            return STATUS_SHARING_VIOLATION;
        }
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    // Step 2: Get or create FCB (always without CCB first)
    if (!(PtrNewFcb = NewFileInfo->Fcb)) {
        // First open — create FCB (no CCB).
        // CCB is created later by UDFOpenExistingFcb when PerformUserOpen=TRUE.
        Status = UDFFirstOpenFile(IrpContext, Vcb,
                                  &PtrNewFcb, RelatedFileInfo, NewFileInfo);
        if (!NT_SUCCESS(Status)) {
            BrutePoint();
            AdPrint(("    Can't perform FirstOpen\n"));
            if (ExistingFileInfo) {
                // Caller provided pre-created FileInfo — return it for cleanup.
                // Caller needs it to undo disk creation (flush + unlink).
                *OutFileInfo = NewFileInfo;
                *OutFcb = PtrNewFcb;
            } else {
                UDFCloseFile__(IrpContext, Vcb, NewFileInfo);
                if (PtrNewFcb) {
                    if (NewFileInfo->Dloc &&
                        NewFileInfo->Dloc->CommonFcb == PtrNewFcb) {
                        NewFileInfo->Dloc->CommonFcb = NULL;
                    }
                    UDFDeleteFcb(IrpContext, PtrNewFcb);
                }
                NewFileInfo->Fcb = NULL;
                if (UDFCleanUpFile__(Vcb, NewFileInfo)) {
                    MyFreePool__(NewFileInfo);
                    NewFileInfo = NULL;
                }
            }
            return Status;
        }
    } else {
        // FCB already exists — validate it
        if (!(PtrNewFcb->FcbState & UDF_FCB_VALID)) {
            BrutePoint();
            AdPrint(("    Fcb not valid\n"));
            if (ExistingFileInfo) {
                *OutFileInfo = NewFileInfo;
                *OutFcb = PtrNewFcb;
            } else {
                UDFCloseFile__(IrpContext, Vcb, NewFileInfo);
                if (UDFCleanUpFile__(Vcb, NewFileInfo)) {
                    MyFreePool__(NewFileInfo);
                    NewFileInfo = NULL;
                }
            }
            return STATUS_ACCESS_DENIED;
        }
    }

    // Step 2b: For user open — complete the open (access check + CCB)
    if (PerformUserOpen) {
        Status = UDFOpenExistingFcb(IrpContext, IrpSp, Vcb,
                                    IrpSp->FileObject, NewFileInfo, &PtrNewFcb,
                                    IgnoreCase, RelatedFileObject, RelatedFcb);
        if (!NT_SUCCESS(Status)) {
            *OutFileInfo = NewFileInfo;
            *OutFcb = PtrNewFcb;
            return Status;
        }
    }

    // Step 3: Keep ParentFcb in sync with current traversal path
    PtrNewFcb->ParentFcb = RelatedFileInfo->Fcb;

    // Step 4: Acquire FCB lock
    {
        PFCB NewFcb = NewFileInfo->Fcb;
        if (NewFcb != *CurrentFcb) {
            UDF_CHECK_PAGING_IO_RESOURCE(NewFcb);
            if (!UDFAcquireFcbExclusive(IrpContext, NewFcb, TRUE)) {
                // Try-lock failed — release all locks, then re-acquire
                // child first, parent second. This avoids deadlock: all locks
                // are released before re-acquiring, so no circular wait.
                // Bump child ref to prevent teardown while unlocked.
                UDFLockVcb(IrpContext, Vcb);
                NewFcb->FcbReference += 1;
                UDFUnlockVcb(IrpContext, Vcb);

                if (*PreviousFcb && *PreviousFcb != *CurrentFcb) {
                    UDFReleaseFcb(IrpContext, *PreviousFcb);
                }
                UDFReleaseFcb(IrpContext, *CurrentFcb);

                UDFAcquireFcbExclusive(IrpContext, NewFcb, FALSE);
                UDF_CHECK_PAGING_IO_RESOURCE((*CurrentFcb));
                UDFAcquireFcbExclusive(IrpContext, *CurrentFcb, FALSE);

                UDFLockVcb(IrpContext, Vcb);
                NewFcb->FcbReference -= 1;
                UDFUnlockVcb(IrpContext, Vcb);

                *PreviousFcb = *CurrentFcb;
            } else {
                if (*PreviousFcb && *PreviousFcb != *CurrentFcb) {
                    UDFReleaseFcb(IrpContext, *PreviousFcb);
                }
                *PreviousFcb = *CurrentFcb;
            }
            *CurrentFcb = NewFcb;
        }
    }

    // Step 5: For intermediate directories — create LCB linking parent→child.
    // Parent lock is held (PreviousFcb == parent) in both normal and
    // try-lock failure paths — splay tree insert will succeed.
    if (RemainingNameLength && *PreviousFcb && RelatedFileInfo && RelatedFileInfo->Fcb) {
        UDFAcquirePrefix(IrpContext,
                         RelatedFileInfo->Fcb,
                         PtrNewFcb,
                         NULL, NULL, NULL,
                         NewFileInfo ? NewFileInfo->Index : 0);

        // Release parent lock — now protected by LCB reference.
        if (*PreviousFcb && *PreviousFcb != *CurrentFcb) {
            UDFReleaseFcb(IrpContext, *PreviousFcb);
            *PreviousFcb = *CurrentFcb;
        }
    }

    *OutFileInfo = NewFileInfo;
    *OutFcb = PtrNewFcb;
    return Status;

} // end UDFOpenObjectFromDirContext()


/*************************************************************************
*
* Function: UDFOpenObjectByFileId()
*
* Description:
*   Open a file or directory by its 64-bit FileId.
*
*   The FileId is extracted from FileObject->FileName.Buffer and validated.
*   We look up or create an FCB using the FileId, verify file/directory
*   compatibility with CreateOptions, acquire the FCB, check access,
*   and complete the open.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFOpenObjectByFileId(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_STACK_LOCATION IrpSp,
    _In_ PVCB Vcb,
    _Inout_ PFCB *CurrentFcb
    )
{
    NTSTATUS Status = STATUS_ACCESS_DENIED;

    BOOLEAN UnlockFcbTable = FALSE;
    BOOLEAN FcbExisted = FALSE;
    PUDF_FILE_INFO OpenFileInfoReference = NULL;

    NODE_TYPE_CODE NodeTypeCode;
    TYPE_OF_OPEN TypeOfOpen;

    FILE_ID FileId;

    PFCB NextFcb = NULL;

    PAGED_CODE();

    ASSERT_VCB(Vcb);

    //
    // Extract the FileId from the FileObject.
    //
    // Our FileId format (UDFGetNTFileId):
    //   LowPart  = physicalLBA - PartitionRoot
    //   HighPart = (uint32)Vcb pointer
    //
    // For directories, UDFFirstOpenFile also sets FID_DIR_MASK (bit 31)
    // in HighPart, but on 32-bit systems this is a no-op since kernel
    // addresses already have bit 31 set.
    //
    // Directory enumeration reports the ICB address of each entry, setting
    // FID_DIR_MASK for directories.  Older callers may still supply an ID
    // without that bit, so the FCB lookup below accepts either form.
    //

    RtlCopyMemory(&FileId, IrpSp->FileObject->FileName.Buffer, sizeof(FILE_ID));

    //
    // Use a try-finally to facilitate cleanup.
    //

    _SEH2_TRY {

        //
        // Acquire the FcbTable and look for an existing FCB.
        //
        // The stored FileId for directories may have FID_DIR_MASK set
        // while the incoming FileId from enumeration does not. Try both
        // variants to find the existing FCB.
        //

        UDFLockFcbTable(IrpContext, Vcb);
        UnlockFcbTable = TRUE;

        NextFcb = UDFLookupFcbTable(IrpContext, Vcb, FileId);

        if (!NextFcb) {

            //
            // Try with FID_DIR_MASK toggled — handles the case where the
            // stored FCB FileId has the directory bit set but the incoming
            // FileId from enumeration does not (or vice versa).
            //

            FILE_ID AltFileId = FileId;
            AltFileId.HighPart ^= FID_DIR_MASK;
            NextFcb = UDFLookupFcbTable(IrpContext, Vcb, AltFileId);
        }

        if (NextFcb) {

            //
            // Found an existing FCB. Determine type from the FCB itself.
            //

            FcbExisted = TRUE;

        } else {

            //
            // FCB not in table. We need to open the file from disk to
            // determine its type and create the FCB.
            //

            if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

                UDFRaiseStatus(IrpContext, STATUS_CANT_WAIT);
            }

            //
            // Convert FileId back to physical LBA.
            // UDFGetNTFileId stores: LowPart = physLBA - PartitionRoot
            //
            uint32 PhysLba = (uint32)FileId.LowPart + UDFPartStart(Vcb, -2);

            //
            // Find which partition contains this LBA and convert to
            // partition-relative logical block number.
            //
            uint32 PartRef = UDFGetRefPartNumByPhysLba(Vcb, PhysLba);
            if (PartRef == (uint32)-1) {
                try_return(Status = STATUS_INVALID_PARAMETER);
            }

            lb_addr FileLoc;
            FileLoc.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartRef, PhysLba);
            FileLoc.partitionReferenceNum = (uint16)PartRef;

            if (FileLoc.logicalBlockNum == (ULONG)-1) {
                try_return(Status = STATUS_INVALID_PARAMETER);
            }

            //
            // Allocate and open the UDF_FILE_INFO directly from disk.
            //
            PUDF_FILE_INFO FileInfo = (PUDF_FILE_INFO)
                MyAllocatePoolTag__(UDF_FILE_INFO_MT, sizeof(UDF_FILE_INFO), MEM_FINF_TAG);

            if (!FileInfo) {
                try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            Status = UDFOpenRootFile__(IrpContext, Vcb, &FileLoc, FileInfo);

            if (!NT_SUCCESS(Status)) {

                UDFCleanUpFile__(Vcb, FileInfo);
                MyFreePool__(FileInfo);
                try_return(Status = STATUS_INVALID_PARAMETER);
            }

            //
            // Now we know the file type from the on-disk data.
            // Determine NodeTypeCode and build the FileId that matches
            // what UDFFirstOpenFile would store (with FID_DIR_MASK for dirs).
            //
            BOOLEAN IsDirectory = UDFIsADirectory(FileInfo);
            FILE_ID StoredFileId = FileId;

            if (IsDirectory) {
                NodeTypeCode = UDF_NODE_TYPE_INDEX;
                SetFlag(StoredFileId.HighPart, FID_DIR_MASK);
            } else {
                NodeTypeCode = UDF_NODE_TYPE_DATA;
                // Don't modify HighPart for files — bit 31 may be part of the
                // Vcb pointer (kernel address), not a flag we control.
                // UDFFirstOpenFile leaves HighPart unmodified for files.
            }

            //
            // Create the FCB with the correct type and FileId.
            //
            NextFcb = UDFCreateFcb(IrpContext, StoredFileId, NodeTypeCode, &FcbExisted);

            if (!NextFcb) {

                UDFCloseFile__(IrpContext, Vcb, FileInfo);
                UDFCleanUpFile__(Vcb, FileInfo);
                MyFreePool__(FileInfo);
                try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            if (FcbExisted) {

                //
                // Another thread created this FCB between our lookup and
                // UDFCreateFcb. Close the FileInfo we opened — the existing
                // FCB already has its own.
                //

                UDFCloseFile__(IrpContext, Vcb, FileInfo);
                UDFCleanUpFile__(Vcb, FileInfo);
                MyFreePool__(FileInfo);

            } else {

                //
                // Link FCB to FileInfo and initialize it.
                //

                NextFcb->FileInfo = FileInfo;
                FileInfo->Fcb = NextFcb;

                if (Vcb->RootIndexFcb) {
                    NextFcb->ParentFcb = Vcb->RootIndexFcb;
                }

                if (FileInfo->Dloc) {
                    FileInfo->Dloc->CommonFcb = NextFcb;
                }

                //
                // Initialize FCB fields from the file info.
                // VcbMutex is needed by UDFInitializeFCB (ASSERT_LOCKED_VCB).
                //

                UDFLockVcb(IrpContext, Vcb);

                Status = UDFInitializeFCB(NextFcb, Vcb,
                             IsDirectory ? UDF_FCB_DIRECTORY : 0);

                if (!NT_SUCCESS(Status)) {
                    UDFUnlockVcb(IrpContext, Vcb);
                    NextFcb->FileInfo = NULL;
                    FileInfo->Fcb = NULL;
                    UDFCloseFile__(IrpContext, Vcb, FileInfo);
                    UDFCleanUpFile__(Vcb, FileInfo);
                    MyFreePool__(FileInfo);
                    try_return(Status);
                }

                /* Match named-open initialization: inline data must use the
                 * extent walker, including Cache Manager paging requests. */
                if (!IsDirectory &&
                    ((((PFILE_ENTRY)FileInfo->Dloc->FileEntry)->icbTag.flags &
                      ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB)) {
                    NextFcb->FcbState |= UDF_FCB_EMBEDDED_DATA;
                }

                // Set file times
                UDFGetFileXTime(FileInfo,
                    &NextFcb->CreationTime.QuadPart,
                    &NextFcb->LastAccessTime.QuadPart,
                    &NextFcb->ChangeTime.QuadPart,
                    &NextFcb->LastWriteTime.QuadPart);

                // Set file sizes
                NextFcb->Header.AllocationSize.QuadPart =
                    UDFSysGetAllocSize(Vcb, FileInfo->Dloc->DataLoc.Length);
                NextFcb->Header.FileSize.QuadPart =
                NextFcb->Header.ValidDataLength.QuadPart = FileInfo->Dloc->DataLoc.Length;

                // Set file attributes
                if (IsDirectory) {
                    NextFcb->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                }

                if (!UDFIsAStreamDir(FileInfo)) {
                    PDIR_INDEX_HDR hDirIndex = UDFGetDirIndexByFileInfo(FileInfo);
                    DIR_INDEX_ITEM AttributesIndex;
                    PDIR_INDEX_ITEM DirNdx;
                    RtlZeroMemory(&AttributesIndex, sizeof(AttributesIndex));
                    DirNdx = hDirIndex ? UDFDirIndex(hDirIndex, FileInfo->Index) :
                                        &AttributesIndex;
                    NextFcb->FileAttributes = UDFAttributesToNT(
                        DirNdx, FileInfo->Dloc->FileEntry);
                    if (NextFcb->FileAttributes & FILE_ATTRIBUTE_READONLY) {
                        NextFcb->FcbState |= UDF_FCB_READ_ONLY;
                    }
                }

                /* The fresh open owns this FI reference until a CCB accepts
                 * it. A cached valid FCB is not an owner of a failed open. */
                OpenFileInfoReference = FileInfo;

                // Insert into table and mark valid
                UDFInsertFcbIntoTable(IrpContext, NextFcb);
                NextFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                NextFcb->FcbState |= UDF_FCB_VALID;

                UDFUnlockVcb(IrpContext, Vcb);
            }
        }

        //
        // Determine TypeOfOpen from the actual FCB type.
        //

        if (NextFcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_INDEX) {
            TypeOfOpen = UserDirectoryOpen;
        } else {
            TypeOfOpen = UserFileOpen;
        }

        //
        // Check that the type of the file is compatible with the
        // desired type of file to open.
        //

        if (FlagOn(NextFcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {

            if (FlagOn(IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE)) {

                try_return(Status = STATUS_FILE_IS_A_DIRECTORY);
            }

        } else if (FlagOn(IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

            try_return(Status = STATUS_NOT_A_DIRECTORY);
        }

        //
        // We now know the FCB and currently hold the FcbTable lock.
        // Try to acquire this FCB without waiting. Otherwise we
        // need to reference it, drop the lock, acquire the FCB,
        // relock and then dereference the FCB.
        //

        if (!UDFAcquireFcbExclusive(IrpContext, NextFcb, TRUE)) {

            UDFLockVcb(IrpContext, Vcb);
            NextFcb->FcbReference += 1;
            UDFUnlockVcb(IrpContext, Vcb);

            UDFUnlockFcbTable(IrpContext, Vcb);
            UnlockFcbTable = FALSE;

            UDFAcquireFcbExclusive(IrpContext, NextFcb, FALSE);

            UDFLockVcb(IrpContext, Vcb);
            NextFcb->FcbReference -= 1;
            UDFUnlockVcb(IrpContext, Vcb);

        } else {

            UDFUnlockFcbTable(IrpContext, Vcb);
            UnlockFcbTable = FALSE;
        }

        //
        // Move to this FCB.
        //

        *CurrentFcb = NextFcb;

        //
        // Check the requested access on this FCB.
        //

        if (!UDFIllegalFcbAccess(Vcb,
                                 IrpSp->Parameters.Create.SecurityContext->DesiredAccess)) {

            //
            // UDFOpenRootFile__ acquired the reference for a newly built
            // FCB. An FCB found in the table still needs one FileInfo
            // reference for this handle: cleanup closes it once per open.
            //

            if (FcbExisted) {
                if (!NextFcb->FileInfo) {
                    try_return(Status = STATUS_FILE_CORRUPT_ERROR);
                }
                UDFReferenceFile__(NextFcb->FileInfo);
                OpenFileInfoReference = NextFcb->FileInfo;
            }

            Status = UDFCompleteFcbOpen(IrpContext,
                                         IrpSp,
                                         Vcb,
                                         CurrentFcb,
                                         TypeOfOpen,
                                         CCB_FLAG_OPEN_BY_ID,
                                         FILE_OPEN,
                                         FALSE,        // VcbLocked
                                         FILE_OPENED);
            if (NT_SUCCESS(Status)) {
                // The successful CCB transfers this reference to cleanup.
                OpenFileInfoReference = NULL;
            }
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if (OpenFileInfoReference) {
            UDFCloseFile__(IrpContext, Vcb, OpenFileInfoReference);
        }

        if (UnlockFcbTable) {

            UDFUnlockFcbTable(IrpContext, Vcb);
        }

        //
        // Destroy the new FCB if it was not fully initialized.
        // Check UDF_FCB_VALID — if it's not set, the FCB was never inserted
        // into the table and never fully initialized.
        //

        if (NextFcb && !FcbExisted && !FlagOn(NextFcb->FcbState, UDF_FCB_VALID)) {

            UDFDeleteFcb(IrpContext, NextFcb);
        }

    } _SEH2_END;

    return Status;
} // end UDFOpenObjectByFileId()


/*************************************************************************
*
* Function: UDFNormalizeStreamSuffix()
*
* Description:
*   Validate and strip the stream type suffix that UDFDissectName leaves in
*   RemainingName after extracting a stream component into StreamName.
*
*   On entry StreamName holds the stream name (e.g. "stream1", possibly empty
*   for the default data stream) and RemainingName holds the trailing suffix
*   (e.g. ":$DATA") or is empty when no suffix was present. UDF only supports
*   the default $DATA stream type, so the suffix — if any — must be exactly
*   ":$DATA"; on a match RemainingName is consumed (Length set to 0).
*
* Raises: STATUS_OBJECT_NAME_INVALID for an unknown suffix, or for an empty
*   stream name with no suffix (e.g. "file:").
*
*************************************************************************/
static
VOID
UDFNormalizeStreamSuffix(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING StreamName,
    IN OUT PUNICODE_STRING RemainingName
    )
{
    static const UNICODE_STRING DataSuffix = RTL_CONSTANT_STRING(L":$DATA");

    if (RemainingName->Length == 0) {
        // No suffix follows the stream name. An empty stream name with no
        // suffix ("file:") is not a valid object.
        if (StreamName->Length == 0) {
            UDFRaiseStatus(IrpContext, STATUS_OBJECT_NAME_INVALID);
        }
        return;
    }

    // A suffix is present — it must be exactly ":$DATA" (case-insensitive).
    // Anything else is an unsupported stream type.
    if (RtlEqualUnicodeString(RemainingName, &DataSuffix, TRUE)) {
        RemainingName->Length = 0;
        return;
    }

    UDFRaiseStatus(IrpContext, STATUS_OBJECT_NAME_INVALID);
}

/*************************************************************************
*
* Function: UDFCommonCreate()
*
* Description:
*   The actual work is performed here. This routine may be invoked in one'
*   of the two possible contexts:
*   (a) in the context of a system worker thread
*   (b) in the context of the original caller
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFCommonCreate(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = NULL;
    PFILE_OBJECT RelatedFileObject = NULL;
    LONGLONG AllocationSize;     // if we create a new file
    ULONG Options;
    ULONG CreateDisposition;
    USHORT FileAttributes;
    USHORT ShareAccess;
    ACCESS_MASK DesiredAccess;
    PACCESS_STATE AccessState;

    PVCB Vcb = NULL;
    // Hold two locks during tree traversal (child + parent)
    // CurrentFcb = current node lock, PreviousFcb = parent node lock (released after operations)
    PFCB CurrentFcb = NULL;
    PFCB PreviousFcb = NULL;

    BOOLEAN DeleteOnClose;
    BOOLEAN OpenByFileId;
    BOOLEAN DirectoryFile;
    BOOLEAN NonDirectoryFile;

    // Is this open for a target directory (used in rename operations)?
    BOOLEAN OpenTargetDirectory;
    // Should we ignore case when attempting to locate the object?
    BOOLEAN IgnoreCase;

    PCCB RelatedCcb = NULL;
    PFCB NextFcb = NULL;
    PFCB PtrNewFcb = NULL;
    PLCB OpenLcb = NULL;            // LCB from prefix match (for direct open)

    PUNICODE_STRING FileName;
    PUNICODE_STRING RelatedFileName = NULL;

    //BOOLEAN VolumeOpen = FALSE;

    TYPE_OF_OPEN RelatedTypeOfOpen = UnopenedFileObject;

    UNICODE_STRING FinalName;           // 'cdf' - current path component
    UNICODE_STRING RemainingName;       // 'fff\rrrr.tre:s' - remaining path to parse
    UNICODE_STRING StreamName;          // bare stream name when creating a new stream

    PUDF_FILE_INFO RelatedFileInfo;
    PUDF_FILE_INFO OldRelatedFileInfo = NULL;
    PUDF_FILE_INFO NewFileInfo = NULL;
    PUDF_FILE_INFO LastGoodFileInfo = NULL;
    BOOLEAN VolumeOpen = FALSE;

    BOOLEAN StreamOpen = FALSE;
    ULONG SNameIndex = 0;

    BOOLEAN NewFileCreated = FALSE;
    DIR_ENUM_CONTEXT DirContext;

    PAGED_CODE();

    ASSERT(IrpContext);
    ASSERT(Irp);

    // If we were called with our file system device object instead of a
    // volume device object, just complete this request with STATUS_SUCCESS.

    if (IrpContext->Vcb == NULL) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
        return STATUS_SUCCESS;
    }

    Options             = IrpSp->Parameters.Create.Options;
    OpenTargetDirectory = BooleanFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);
    DirectoryFile       = BooleanFlagOn(Options, FILE_DIRECTORY_FILE);
    NonDirectoryFile    = BooleanFlagOn(Options, FILE_NON_DIRECTORY_FILE);
    OpenByFileId = BooleanFlagOn(IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID);
    IgnoreCase = !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE );
    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    DeleteOnClose = BooleanFlagOn(IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE);
    FileAttributes = IrpSp->Parameters.Create.FileAttributes & ~FILE_ATTRIBUTE_NORMAL;
    AllocationSize = Irp->Overlay.AllocationSize.QuadPart;
    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;
    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess = IrpSp->Parameters.Create.ShareAccess;

    Vcb = IrpContext->Vcb;

    ASSERT_VCB(Vcb);

    Irp->IoStatus.Information = 0;

    // Check if the volume is read - only or write - protected and if the operation
    // requires write access

    if (FlagOn(Vcb->VcbState, VCB_STATE_MEDIA_WRITE_PROTECT | VCB_STATE_VOLUME_READ_ONLY)) {

        if (CreateDisposition == FILE_OVERWRITE_IF ||
            CreateDisposition == FILE_SUPERSEDE ||
            CreateDisposition == FILE_CREATE ||
            CreateDisposition == FILE_OVERWRITE ||
            OpenTargetDirectory ||
            FlagOn(Options, FILE_DELETE_ON_CLOSE)) {

            if (FlagOn(Vcb->VcbState, VCB_STATE_MEDIA_WRITE_PROTECT)) {

                IoSetHardErrorOrVerifyDevice(IrpContext->Irp, Vcb->Vpb->RealDevice);
                IrpContext->ExceptionStatus = STATUS_MEDIA_WRITE_PROTECTED;
                ExRaiseStatus(STATUS_MEDIA_WRITE_PROTECTED);
            }

            Status = STATUS_ACCESS_DENIED; 
            if (FlagOn(Vcb->VcbState, VCB_STATE_MOUNTED_DIRTY)) {

                Status = STATUS_VOLUME_DIRTY;
            }

            UDFCompleteRequest(IrpContext, Irp, Status);
            return Status;
        }
    }

    // Check for invalid combination of DELETE_ON_CLOSE with other flags

    if (FlagOn(Options, FILE_DELETE_ON_CLOSE) &&
        (OpenTargetDirectory || FlagOn(Options, FILE_OPEN_BY_FILE_ID))) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    // UDFS does not support Extended Attributes

    if (IrpSp->Parameters.Create.EaLength || Irp->AssociatedIrp.SystemBuffer) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_EAS_NOT_SUPPORTED);
        return STATUS_EAS_NOT_SUPPORTED;
    }

    //  UDFS does not support paging files

    if (FlagOn(IrpSp->Flags, SL_OPEN_PAGING_FILE)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }

#if (NTDDI_VERSION >= NTDDI_WIN7)

    // UDFS does not support FILE_OPEN_REQUIRING_OPLOCK

    if (FlagOn(Options, FILE_OPEN_REQUIRING_OPLOCK)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }
#endif

    // Reference our input parameters to make things easier

    FileObject = IrpSp->FileObject;
    RelatedFileObject = NULL;

    FileName = &FileObject->FileName;

    // Set up the file object's Vpb pointer in case anything happens.
    // This will allow us to get a reasonable pop-up.

    if ((FileObject->RelatedFileObject != NULL) && !OpenByFileId) {

        RelatedFileObject = FileObject->RelatedFileObject;
        FileObject->Vpb = RelatedFileObject->Vpb;

        RelatedTypeOfOpen = UDFDecodeFileObject(RelatedFileObject, &NextFcb, &RelatedCcb);

        ASSERT_CCB(RelatedCcb);
        ASSERT_FCB(NextFcb);

        // Fail the request if this is not a user file object.

        if (RelatedTypeOfOpen < UserVolumeOpen) {
            UDFCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }

        // Remember the name in the related file object.

        RelatedFileName = &RelatedFileObject->FileName;
    }

   // If we haven't initialized the names then make sure the strings are valid.
   // If this an OpenByFileId then verify the file id buffer.
   //
   // After this routine returns we know that the full name is in the
   // FileName buffer and the buffer will hold the upcased portion
   // of the name yet to parse immediately after the full name in the
   // buffer.  Any trailing backslash has been removed and the flag
   // in the IrpContext will indicate whether we removed the
   // backslash.

    Status = UDFNormalizeFileNames(IrpContext,
                               Vcb,
                               OpenByFileId,
                               RelatedTypeOfOpen,
                               RelatedCcb,
                               RelatedFileName,
                               FileName,
                               &RemainingName);

    // Return the error code if not successful.

    if (!NT_SUCCESS(Status)) {

        UDFCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    // We want to acquire the Vcb.  Exclusively for a volume open, shared otherwise.
    // The file name is empty for a volume open.

    if ((FileName->Length == 0) &&
        (RelatedTypeOfOpen <= UserVolumeOpen) &&
        !OpenByFileId) {

        VolumeOpen = TRUE;
        UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

    } else if (CreateDisposition == FILE_SUPERSEDE ||
               CreateDisposition == FILE_OVERWRITE ||
               CreateDisposition == FILE_OVERWRITE_IF) {

        // Overwriting/superseding an existing file may drop its stream directory,
        // which removes prefix LCBs and adjusts VCB-global reference counts that
        // are not individually locked — take the Vcb exclusively. These
        // dispositions are rare, so the reduced concurrency is acceptable.
        UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

    } else {

        UDFAcquireVcbShared(IrpContext, Vcb, FALSE);
    }

    // Use a try-finally to facilitate cleanup.

    _SEH2_TRY {

        // A forced dismount can leave an old Vcb referenced by an in-flight
        // open.  Complete that open with the invalid-volume status while the
        // Vcb resource is held; raising here can escape the FSD dispatch on
        // Windows x64 and turn the stale open into a system bugcheck.
        if (Vcb->VcbCondition == VcbInvalid ||
            Vcb->VcbCondition == VcbDismountInProgress) {
            try_return(Status = FlagOn(Vcb->VcbState, VCB_STATE_DISMOUNT_IN_PROGRESS) ?
                       STATUS_VOLUME_DISMOUNTED : STATUS_FILE_INVALID);
        }

        // Verify that the Vcb is not in an unusable condition.  This routine
        // will raise if not usable.

        UDFVerifyVcb(IrpContext, Vcb);

        // Check if dismount is in progress
        if (FlagOn(Vcb->VcbState, VCB_STATE_DISMOUNT_IN_PROGRESS)) {
            try_return(Status = STATUS_ACCESS_DENIED);
        }

        // If the Vcb is locked then we cannot open another file

        if (FlagOn(Vcb->VcbState, VCB_STATE_LOCKED)) {

            try_return(Status = STATUS_ACCESS_DENIED);
        }

        ASSERT(Vcb->VcbCondition == VcbMounted);

        // If we are opening this volume Dasd then process this immediately
        // and exit.

        // ****************
        // If a Volume open is requested, satisfy it now
        // ****************
        if (VolumeOpen) {

            // The only create disposition we allow is OPEN.

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return(Status = STATUS_ACCESS_DENIED);
            }

            // If a volume open is requested, perform checks to ensure that
            // invalid options have not also been specified ...

            if (OpenTargetDirectory) {

                try_return(Status = STATUS_NOT_A_DIRECTORY);
            }

            // If they wanted to open a directory, surprise.

            if (DirectoryFile) {

                try_return(Status = STATUS_NOT_A_DIRECTORY);
            }

            // Cannot delete a volume

            if (DeleteOnClose) {

                try_return(Status = STATUS_CANNOT_DELETE);
            }

            if (FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY)) {

                if (FlagOn(DesiredAccess, FILE_WRITE_DATA | FILE_APPEND_DATA | DELETE | WRITE_DAC)) {

                    try_return(Status = STATUS_ACCESS_DENIED);
                }
            }

            // Cannot supersede, overwrite, or create a volume.
            if (CreateDisposition == FILE_SUPERSEDE ||
                CreateDisposition == FILE_OVERWRITE ||
                CreateDisposition == FILE_OVERWRITE_IF ||
                CreateDisposition == FILE_CREATE) {

                try_return(Status = STATUS_ACCESS_DENIED);
            }

            CurrentFcb = Vcb->VolumeDasdFcb;

            // Acquire the Fcb exclusively before completing the open

            UDFAcquireFcbExclusive(IrpContext, CurrentFcb, FALSE);

            Status = UDFCompleteFcbOpen(IrpContext,
                                        IrpSp,
                                        Vcb,
                                        &CurrentFcb,
                                        UserVolumeOpen,
                                        0,
                                        CreateDisposition,
                                        FALSE,         // VcbLocked
                                        FILE_OPENED);  // DesiredInformation

            try_return(Status);
        }

        if (UDFIllegalFcbAccess(Vcb, DesiredAccess)) {
            AdPrint(("    Illegal share access\n"));
            try_return(Status = STATUS_ACCESS_DENIED);
        }

        ASSERT(Vcb->VcbCondition == VcbMounted);

        //
        // If we are opening this file by FileId then process this immediately
        // and exit.
        //

        if (OpenByFileId) {

            //
            // The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return(Status = STATUS_ACCESS_DENIED);
            }

            try_return(Status = UDFOpenObjectByFileId(IrpContext,
                                                      IrpSp,
                                                      Vcb,
                                                      &CurrentFcb));
        }

        //
        // If the remaining name is empty, we already have our target:
        //  - No RelatedFileObject: root directory open
        //  - RelatedFileObject + empty FileName: reopen the related file
        // Acquire the target FCB, reference its FileInfo, and go
        // directly to the common open path.
        //
        if (RemainingName.Length == 0) {

            if (OpenTargetDirectory) {
                // Reject OpenTargetDirectory on stream objects —
                // streams have no parent directory context.
                if (RelatedFileObject && NextFcb && NextFcb->FileInfo &&
                    (UDFIsAStream(NextFcb->FileInfo) || UDFIsAStreamDir(NextFcb->FileInfo))) {
                    try_return(Status = STATUS_INVALID_PARAMETER);
                }
                // For normal files/dirs: fall through to open the current directory
            }

            if (RelatedFileObject) {
                // Reopen: target is the FCB from RelatedFileObject
                PtrNewFcb = NextFcb;
            } else {
                // Root directory open
                PtrNewFcb = Vcb->RootIndexFcb;
                if (DeleteOnClose) {
                    try_return(Status = STATUS_CANNOT_DELETE);
                }
            }

            LastGoodFileInfo = PtrNewFcb->FileInfo;

            // Validate file/directory type against caller's request
            if (PtrNewFcb->FcbState & UDF_FCB_DIRECTORY) {
                if (NonDirectoryFile ||
                    (CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {
                    try_return(Status = STATUS_FILE_IS_A_DIRECTORY);
                }
            } else {
                if (DirectoryFile) {
                    try_return(Status = STATUS_NOT_A_DIRECTORY);
                }
            }
            if (CreateDisposition == FILE_CREATE) {
                try_return(Status = STATUS_OBJECT_NAME_COLLISION);
            }

            // Acquire target FCB exclusively
            if (PtrNewFcb != CurrentFcb) {
                UDF_CHECK_PAGING_IO_RESOURCE(PtrNewFcb);
                UDFAcquireFcbExclusive(IrpContext, PtrNewFcb, FALSE);
                if (PreviousFcb && PreviousFcb != CurrentFcb) {
                    UDFReleaseFcb(IrpContext, PreviousFcb);
                }
                PreviousFcb = CurrentFcb;
                CurrentFcb = PtrNewFcb;
            }

            // Reference the FileInfo (balanced by UDFCloseFile__ on error
            // or by IRP_MJ_CLEANUP on success)
            NewFileInfo = PtrNewFcb->FileInfo;
            UDFReferenceFile__(NewFileInfo);

            try_return(Status = UDFOpenExistingFcb(IrpContext, IrpSp, Vcb,
                                                    FileObject, NewFileInfo,
                                                    &PtrNewFcb, IgnoreCase,
                                                    RelatedFileObject, NextFcb));
        }

        _SEH2_TRY {
            AdPrint(("    Opening file %wZ %8.8x\n",FileName, FileObject));
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            AdPrint(("    Exception when printing FN\n"));
        } _SEH2_END;

        {

        //AdPrint(("    Opening file %ws %8.8x\n",FileName->Buffer, PtrNewFileObject));

        if (FileName->Length > UDF_X_PATH_LEN*sizeof(WCHAR)) {
            try_return(Status = STATUS_OBJECT_NAME_INVALID);
        }

        // validate path specified
        // (sometimes we can see here very strange characters ;)
        if (!UDFIsNameValid(FileName, &StreamOpen, &SNameIndex)) {
            AdPrint(("    Absolute path is not valid\n"));
            try_return(Status = STATUS_OBJECT_NAME_INVALID);
        }
        if (StreamOpen && !UDFIsStreamsSupported(Vcb)) {
            try_return(Status = STATUS_OBJECT_NAME_INVALID);
        }

        // RemainingName was set by UDFNormalizeFileNames to point to the relative path
        // after RelatedFileName (or the full path if no RelatedFileObject)

        if (RelatedFileObject) {
            // Our "start directory" is the one identified by the related file object
            RelatedFileInfo = NextFcb->FileInfo;
        } else {
            // Start at the root of the file system
            RelatedFileInfo = Vcb->RootIndexFcb->FileInfo;
        }

        // RemainingName keeps the full path including any ":stream:$DATA"
        // suffix. The traversal loop parses the stream component in place:
        // UDFDissectName splits on ':' and the suffix is validated as each
        // stream component is reached.
        FinalName.MaximumLength = RemainingName.MaximumLength;

        Status = STATUS_SUCCESS;
        LastGoodFileInfo = RelatedFileInfo;
        // reference RelatedObject to prevent releasing parent structures
        // Acquire only CurrentFcb (= LastGoodFileInfo->Fcb)
        CurrentFcb = RelatedFileInfo->Fcb;
        UDF_CHECK_PAGING_IO_RESOURCE(CurrentFcb);
        UDFAcquireFcbExclusive(IrpContext, CurrentFcb, FALSE);

        // Parent references are now handled by LCB in UDFCompleteFcbOpen
        // when child files are opened through this directory.

        // ****************
        // Use UDFFindPathPrefix to find the deepest already-opened
        // directory in the path. This avoids redundant directory
        // traversal for already-cached paths.
        // ****************
        {
            BOOLEAN ShortNameMatch = FALSE;
            PFCB PrefixFcb = CurrentFcb;
            UNICODE_STRING SearchPath = RemainingName;
            PLCB PrefixLcb;

            PrefixLcb = UDFFindPathPrefix(
                IrpContext,
                CurrentFcb,
                IgnoreCase,
                &PrefixFcb,
                &SearchPath,
                &ShortNameMatch
            );

            if (PrefixLcb && PrefixFcb && PrefixFcb != CurrentFcb && PrefixFcb->FileInfo) {

                // UDFFindPathPrefix already released old CurrentFcb lock
                // and holds PrefixFcb locked exclusive. Update state.
                CurrentFcb = PrefixFcb;
                PreviousFcb = CurrentFcb;  // single lock model

                // Check for delete-in-progress on matched FCB
                if (PrefixLcb->Flags & UDF_LCB_FLAG_LINK_DELETED) {
                    try_return(Status = STATUS_DELETE_PENDING);
                }

                // Check if entire path was matched
                if (SearchPath.Length == 0 ||
                    (SearchPath.Length == sizeof(WCHAR) && SearchPath.Buffer[0] == L'\\')) {

                    if (OpenTargetDirectory) {
                        // File found via prefix search, but caller wants the parent directory.
                        // Switch from file FCB to parent directory FCB and set up state
                        // so the loop runs one iteration — UDFFindDirEntry finds the file,
                        // then the early OpenTargetDirectory check opens the parent.
                        PFCB ParentFcb = PrefixLcb->ParentFcb;

                        if (!ParentFcb || !ParentFcb->FileInfo) {
                            try_return(Status = STATUS_INVALID_PARAMETER);
                        }

                        // Switch lock from file to parent directory
                        if (ParentFcb != CurrentFcb) {
                            UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb);
                            UDFAcquireFcbExclusive(IrpContext, ParentFcb, FALSE);
                            UDFReleaseFcb(IrpContext, CurrentFcb);
                            CurrentFcb = ParentFcb;
                        }
                        PreviousFcb = CurrentFcb;

                        OldRelatedFileInfo = RelatedFileInfo;
                        RelatedFileInfo = ParentFcb->FileInfo;
                        LastGoodFileInfo = ParentFcb->FileInfo;

                        // Set RemainingName to file name so UDFDissectName extracts it
                        // into FinalName, then the early OpenTargetDirectory check
                        // (before UDFOpenFileInfoFromDirContext) opens the parent.
                        RemainingName = PrefixLcb->FileName;

                    } else {

                    // Entire path found in prefix - skip loop, open directly
                    OpenLcb = PrefixLcb;
                    PtrNewFcb = PrefixFcb;
                    LastGoodFileInfo = PrefixFcb->FileInfo;
                    OldRelatedFileInfo = RelatedFileInfo;

                    // Validate file/directory type against caller's request
                    if (PtrNewFcb->FcbState & UDF_FCB_DIRECTORY) {
                        if (NonDirectoryFile ||
                            (CreateDisposition == FILE_SUPERSEDE) ||
                            (CreateDisposition == FILE_OVERWRITE) ||
                            (CreateDisposition == FILE_OVERWRITE_IF)) {
                            try_return(Status = STATUS_FILE_IS_A_DIRECTORY);
                        }
                    } else {
                        if (DirectoryFile) {
                            try_return(Status = STATUS_NOT_A_DIRECTORY);
                        }
                    }
                    if (CreateDisposition == FILE_CREATE) {
                        try_return(Status = STATUS_OBJECT_NAME_COLLISION);
                    }

                    // Reference the FileInfo (will be balanced by cleanup)
                    NewFileInfo = PrefixFcb->FileInfo;
                    UDFReferenceFile__(NewFileInfo);

                    // Skip the search loop - open directly
                    try_return(Status = UDFOpenExistingFcb(IrpContext, IrpSp, Vcb,
                                                            FileObject, NewFileInfo,
                                                            &PtrNewFcb, IgnoreCase,
                                                            RelatedFileObject, NextFcb));

                    } // end if OpenTargetDirectory

                } else {
                    // Partial match - use as starting point for loop
                    OldRelatedFileInfo = RelatedFileInfo;
                    RelatedFileInfo = PrefixFcb->FileInfo;
                    LastGoodFileInfo = RelatedFileInfo;
                    RemainingName = SearchPath;
                }
            }
        }

        // Check if the starting directory FCB is in a valid state.
        //   NEEDS_VERIFICATION: FID/ICB mismatch after verify → STATUS_REPARSE
        //   NOT_FOUND_ON_MEDIA: object gone from disk → STATUS_OBJECT_PATH_NOT_FOUND
        //   DELETED: FCB deleted → STATUS_OBJECT_PATH_NOT_FOUND
        if (CurrentFcb->FcbState & UDF_FCB_NEEDS_VERIFICATION) {
            try_return(Status = STATUS_REPARSE);
        }
        if (CurrentFcb->FcbState & (UDF_FCB_NOT_FOUND_ON_MEDIA | UDF_FCB_DELETED)) {
            try_return(Status = STATUS_OBJECT_PATH_NOT_FOUND);
        }

        // Path traversal start point must be a directory — unless the
        // remaining path is a stream suffix (":stream"), in which case the
        // start point is the base file that owns the stream (e.g. the prefix
        // search matched the base file and left ":stream" to process).
        if (!(CurrentFcb->FcbState & UDF_FCB_DIRECTORY) &&
            !(RemainingName.Length && RemainingName.Buffer[0] == L':')) {
            try_return(Status = STATUS_OBJECT_PATH_NOT_FOUND);
        }

        // go into a loop parsing the supplied name

        //  Note that we may have to "open" intermediate directory objects
        //  while traversing the path. We should __try to reuse existing code
        //  whenever possible therefore we should consider using a common
        //  open routine regardless of whether the open is on behalf of the
        //  caller or an intermediate (internal) open performed by the driver.

        // ****************
        // now we'll parse path to desired file
        // ****************

        // Path traversal requires blocking I/O — if caller cannot wait,
        // raise so the request gets requeued to a worker thread.
        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
            UDFRaiseStatus(IrpContext, STATUS_CANT_WAIT);
        }

        BOOLEAN IsStreamComponent = FALSE;

        while (TRUE) {

            // Intermediate traversal references were released below. Do not
            // let a later name-validation failure close that borrowed FileInfo.
            NewFileInfo = NULL;
            PtrNewFcb = NULL;

            // Split off the next component from the name.

            UDFDissectName(IrpContext, &RemainingName, &FinalName, &IsStreamComponent);

            // Cannot open children within a named stream — streams are leaf objects.

            if (RelatedFileInfo && UDFIsAStream(RelatedFileInfo)) {

                try_return(Status = STATUS_INVALID_PARAMETER);
            }

            // For a stream component, the remaining tail is its type suffix.
            // Validate it (":$DATA" only) and consume it from RemainingName so
            // the stream name in FinalName is processed as the final component.

            if (IsStreamComponent) {

                UDFNormalizeStreamSuffix(IrpContext, &FinalName, &RemainingName);
            }

            if ( FinalName.Length &&
               (NT_SUCCESS(Status) || !StreamOpen)) {
                // ...wow! non-zero! try to open!
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Error opening path component\n"));
                    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                        Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    try_return(Status);
                }

                // Note: FcbReference may be 0 for intermediate directories during path traversal;
                // it will be incremented when CCB is created in UDFCompleteFcbOpen

                // Mark intermediate directory FCB as valid (internal open)
                if (RelatedFileInfo && RelatedFileInfo->ParentFile) {
                    RelatedFileInfo->Fcb->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                    RelatedFileInfo->Fcb->FcbState |= UDF_FCB_VALID;
                }
                // Validate path component: length and characters.
                // Full path was validated by UDFIsNameValid, but stream names
                // injected via RemainingName need per-component checking.
                if (FinalName.Length > UDF_X_NAME_LEN * sizeof(WCHAR)) {
                    AdPrint(("    Path component is too long\n"));
                    try_return(Status = STATUS_OBJECT_NAME_INVALID);
                }
                // Note: per-component character validation is not needed here.
                // UDFIsNameValid already validated the full path before the loop.
                // UDFIsNameValid rejects trailing '.' which would break "." and ".." components.
                // Acquire FCB lock for tree traversal
                // Skip if same FCB to avoid recursive acquire leading to lock leak
                {
                    PFCB NewFcb = RelatedFileInfo->Fcb;
                    if (NewFcb != CurrentFcb) {
                        UDF_CHECK_PAGING_IO_RESOURCE(NewFcb);
                        UDFAcquireFcbExclusive(IrpContext, NewFcb, FALSE);
                        if (PreviousFcb && PreviousFcb != CurrentFcb) {
                            UDFReleaseFcb(IrpContext, PreviousFcb);
                        }
                        PreviousFcb = CurrentFcb;
                        CurrentFcb = NewFcb;
                    }
                }

                // check traverse rights (uses RelatedFileInfo->Fcb which is now locked as PreviousFcb)
                Status = UDFCheckAccessRights(NULL, NULL, RelatedFileInfo->Fcb, RelatedCcb, FILE_TRAVERSE, 0);
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Traverse check failed\n"));
                } else if (!IsStreamComponent) {
                    // Parent must be a directory for non-stream path traversal.
                    if (!(CurrentFcb->FcbState & UDF_FCB_DIRECTORY)) {
                        Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    } else {
                    // standard open: first find, then open
                    Status = UDFFindDirEntry(Vcb, RelatedFileInfo, &FinalName, IgnoreCase, TRUE, &DirContext);
                    if (NT_SUCCESS(Status)) {
                        // Directories cannot exist inside a stream directory.
                        if (UDFIsAStreamDir(RelatedFileInfo) &&
                            (DirContext.DirNdx->FileCharacteristics & FILE_DIRECTORY)) {
                            Status = STATUS_OBJECT_PATH_NOT_FOUND;
                        } else
                        // Check if intermediate path component is a directory.
                        // A leading ':' in RemainingName is a stream suffix, not a
                        // subdirectory — a file with a stream is still valid here.
                        if (RemainingName.Length &&
                            RemainingName.Buffer[0] != L':' &&
                            !(DirContext.DirNdx->FileCharacteristics & FILE_DIRECTORY)) {
                            AdPrint(("    Not a directory\n"));
                            Status = STATUS_NOT_A_DIRECTORY;
                        } else {
                            // OpenTargetDirectory: file found via dir search,
                            // but caller wants the parent directory, not the file.
                            // Skip opening the file entirely — open parent with FILE_EXISTS.
                            if (OpenTargetDirectory && !RemainingName.Length) {
                                if (!UDFIsADirectory(LastGoodFileInfo)) {
                                    try_return(Status = STATUS_NOT_A_DIRECTORY);
                                }
                                NewFileInfo = LastGoodFileInfo;
                                PtrNewFcb = NewFileInfo->Fcb;
                                UDFReferenceFile__(NewFileInfo);
                                Status = UDFCheckAccessRights(FileObject, AccessState, PtrNewFcb, NULL, DesiredAccess, ShareAccess);
                                if (!NT_SUCCESS(Status)) {
                                    try_return(Status);
                                }
                                Status = UDFCompleteFcbOpen(IrpContext, IrpSp, Vcb, &PtrNewFcb, UserDirectoryOpen,
                                                       (IgnoreCase ? CCB_FLAG_IGNORE_CASE : 0), CreateDisposition, FALSE,
                                                       FILE_EXISTS);
                                LastGoodFileInfo = NewFileInfo;
                                try_return(Status);
                            }
                            if (!RemainingName.Length) {
                                // Metadata files are internal volume structures and
                                // must not be opened by the caller.
                                if (DirContext.DirNdx->FileCharacteristics & FILE_METADATA) {
                                    try_return(Status = STATUS_ACCESS_DENIED);
                                }
                                // Final path component found — break out of loop.
                                // Will be opened post-loop with PerformUserOpen=TRUE.
                                break;
                            }
                            // Intermediate path component (or the base file of a
                            // stream path, where RemainingName still holds ":stream")
                            // — open now without user open. Non-zero RemainingNameLength
                            // tells UDFOpenObjectFromDirContext to create an LCB.
                            Status = UDFOpenObjectFromDirContext(
                                IrpContext, IrpSp, Vcb, &DirContext,
                                RelatedFileInfo,
                                &CurrentFcb, &PreviousFcb,
                                IgnoreCase, FALSE, CreateDisposition,
                                RemainingName.Length,
                                NULL, NULL,
                                NULL,  // ExistingFileInfo
                                &NewFileInfo, &PtrNewFcb);
                            if (NT_SUCCESS(Status)) {
                                LastGoodFileInfo = NewFileInfo;
                                UDFDereferenceFile__(NewFileInfo);
                            }
                        }
                    }
#ifdef UDF_DBG
                    else if (Status == STATUS_NOT_A_DIRECTORY) {
                        AdPrint(("    Not a directory\n"));
                    }
#endif // UDF_DBG
                    } // end directory check
                } else {
                    // Stream component: open or create the Stream Directory,
                    // then open it through the unified path (FCB + lock + LCB).
                    // DissectName already consumed the ':' prefix, so FinalName
                    // is the stream name (e.g., "stream1" not ":stream1").

                    // OpenTargetDirectory (rename/move target lookup) is not a
                    // meaningful operation on a named stream — reject it.

                    if (OpenTargetDirectory) {

                        try_return(Status = STATUS_INVALID_PARAMETER);
                    }

                    PUDF_FILE_INFO StreamDirInfo = NULL;
                    Status = UDFOpenStreamDir__(IrpContext, Vcb, RelatedFileInfo, &StreamDirInfo);
                    if (Status == STATUS_NOT_FOUND) {
                        // Stream Dir doesn't exist, but caller wants it to be
                        // created. Lets try to help him...
                        if ((CreateDisposition == FILE_CREATE) ||
                           (CreateDisposition == FILE_OPEN_IF) ||
                           (CreateDisposition == FILE_OVERWRITE_IF)) {
                            Status = UDFCreateStreamDir__(IrpContext, Vcb, RelatedFileInfo, &StreamDirInfo);
                        }
                        if (!NT_SUCCESS(Status)) {
                            // Remap STATUS_NOT_FOUND for consistent error handling
                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }
                    }
                    if (NT_SUCCESS(Status)) {
                        // Re-inject stream name: next iteration will search
                        // for FinalName within the stream directory.
                        RemainingName = FinalName;
                        // Open stream dir through unified path — handles
                        // FCB creation, lock management, and LCB creation.
                        Status = UDFOpenObjectFromDirContext(
                            IrpContext, IrpSp, Vcb,
                            NULL,           // no DirContext
                            RelatedFileInfo,
                            &CurrentFcb, &PreviousFcb,
                            IgnoreCase, FALSE, CreateDisposition,
                            RemainingName.Length,
                            NULL, NULL,
                            StreamDirInfo,  // ExistingFileInfo
                            &NewFileInfo, &PtrNewFcb);
                        if (NT_SUCCESS(Status)) {
                            LastGoodFileInfo = NewFileInfo;
                            UDFDereferenceFile__(NewFileInfo);
                        } else {
                            // FCB setup failed — close stream dir and exit
                            UDFCloseFile__(IrpContext, Vcb, StreamDirInfo);
                            try_return(Status);
                        }
                    }
                }

                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Can't open file\n"));
                }

                // Delete-on-close flags on the parent directory should not block
                // creating new files inside it - only the specific deleted file
                // should return STATUS_DELETE_PENDING when opened.

                // OpenTargetDirectory + file not found: open parent directory.
                // LastGoodFileInfo still points to the parent (not updated on failure).
                if (OpenTargetDirectory && !RemainingName.Length &&
                    Status == STATUS_OBJECT_NAME_NOT_FOUND) {
                    if (!UDFIsADirectory(LastGoodFileInfo)) {
                        try_return(Status = STATUS_NOT_A_DIRECTORY);
                    }
                    NewFileInfo = LastGoodFileInfo;
                    PtrNewFcb = NewFileInfo->Fcb;
                    UDFReferenceFile__(NewFileInfo);
                    Status = UDFCheckAccessRights(FileObject, AccessState, PtrNewFcb, NULL, DesiredAccess, ShareAccess);
                    if (!NT_SUCCESS(Status)) {
                        try_return(Status);
                    }
                    Status = UDFCompleteFcbOpen(IrpContext, IrpSp, Vcb, &PtrNewFcb, UserDirectoryOpen,
                                           (IgnoreCase ? CCB_FLAG_IGNORE_CASE : 0), CreateDisposition, FALSE,
                                           FILE_DOES_NOT_EXIST);
                    LastGoodFileInfo = NewFileInfo;
                    try_return(Status);
                }

                // update last good state information...
                OldRelatedFileInfo = RelatedFileInfo;
                RelatedFileInfo = NewFileInfo;

                // Base file of a stream path doesn't exist. Stop here: FinalName
                // holds the base name and RemainingName still holds the ":stream"
                // suffix. Post-loop creation builds the file, its stream directory,
                // and the stream from that suffix.
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND && StreamOpen &&
                    RemainingName.Length && RemainingName.Buffer[0] == L':') {
                    break;
                }

                // If this was the final path component (RemainingName exhausted),
                // break out of loop for open/create finalization.
                // OpenTargetDirectory (file found) is handled earlier (before UDFOpenFileInfoFromDirContext).
                if (!RemainingName.Length) {
                    // File not found — create inline if disposition allows.
                    // Stream creation is handled separately post-loop.
                    if (Status == STATUS_OBJECT_NAME_NOT_FOUND && !StreamOpen) {
                        if ((CreateDisposition == FILE_OPEN) ||
                            (CreateDisposition == FILE_OVERWRITE)) {
                            AdPrint(("    File doesn't exist\n"));
                            try_return(Status);
                        }
                        if ((CreateDisposition != FILE_CREATE) && (CreateDisposition != FILE_OPEN_IF) &&
                            (CreateDisposition != FILE_OVERWRITE_IF) && (CreateDisposition != FILE_SUPERSEDE)) {
                            AdPrint(("    File doesn't exist (2)\n"));
                            try_return(Status);
                        }
                        if (Vcb->VcbState & VCB_STATE_MEDIA_WRITE_PROTECT) {
                            AdPrint(("    Media write protected\n"));
                            IoSetHardErrorOrVerifyDevice(Irp, Vcb->Vpb->RealDevice);
                            try_return(Status = STATUS_MEDIA_WRITE_PROTECTED);
                        }
                        if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {
                            AdPrint(("    Volume read only\n"));
                            try_return(Status = STATUS_ACCESS_DENIED);
                        }
                        if (DeleteOnClose &&
                           (FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_READONLY ))) {
                            AdPrint(("    Can't create r/o file marked for deletion\n"));
                            try_return(Status = FlagOn(Vcb->VcbState, VCB_STATE_MOUNTED_DIRTY)
                                                ? STATUS_VOLUME_DIRTY : STATUS_CANNOT_DELETE);
                        }
                        // Validate name: FinalName from UDFDissectName never contains backslashes
                        {
                            USHORT i;
                            USHORT CharCount = FinalName.Length / sizeof(WCHAR);
                            for(i = 0; i < CharCount; i++) {
                                if (FinalName.Buffer[i] == L'\\') {
                                    AdPrint(("    Target name should not contain (back)slashes\n"));
                                    try_return(Status = STATUS_OBJECT_NAME_INVALID);
                                }
                            }
                        }
                        if (DirectoryFile &&
                           ((IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_TEMPORARY) ||
                             FALSE)) {
                            AdPrint(("    Creation of _temporary_ directory not permited\n"));
                            try_return(Status = STATUS_INVALID_PARAMETER);
                        }
                        // Check access rights on parent directory
                        ASSERT(CurrentFcb);
                        Status = UDFCheckAccessRights(NULL, NULL, OldRelatedFileInfo->Fcb, RelatedCcb,
                                                      DirectoryFile ? FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE, 0);
                        if (!NT_SUCCESS(Status)) {
                            AdPrint(("    Creation of File/Dir not permitted\n"));
                            try_return(Status);
                        }
                        // Create the file on disk
                        Status = UDFCreateFile__(IrpContext, Vcb, IgnoreCase, &FinalName, 0, 0,
                                             UdfIsExtendedFESupported(Vcb),
                                             (CreateDisposition == FILE_CREATE), OldRelatedFileInfo, &NewFileInfo);
                        if (!NT_SUCCESS(Status)) {
                            UDFPrint(("UDF BM: UDFCreateFile__ FAILED Status=%x Dir=%d Name=%wZ\n",
                                Status, DirectoryFile, &FinalName));
                            try_return(Status);
                        }
                        // Update parent object
                        if ((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) &&
                           NextFcb &&
                           RelatedFileObject &&
                           (NextFcb->FileInfo == NewFileInfo->ParentFile)) {
                                RelatedFileObject->Flags |= (FO_FILE_MODIFIED | FO_FILE_SIZE_CHANGED);
                        }
                        if (DirectoryFile) {
                            Status = UDFRecordDirectory__(IrpContext, Vcb, NewFileInfo);
                            if (!NT_SUCCESS(Status)) {
                                AdPrint(("    Can't transform to directory\n"));
                                if ((Status != STATUS_FILE_IS_A_DIRECTORY) &&
                                   (Status != STATUS_NOT_A_DIRECTORY) &&
                                   (Status != STATUS_ACCESS_DENIED)) {
                                    UDFFlushFile__(IrpContext, Vcb, NewFileInfo, 0);
                                    UDFUnlinkFile__(IrpContext, Vcb, NewFileInfo, TRUE);
                                }
                                // UDFCloseFile__ is handled by the finally block.
                                try_return(Status);
                            }
                        }
                        NewFileCreated = TRUE;
                        CreateDisposition = FILE_CREATE;
                        Status = STATUS_SUCCESS;
                    }
                    break;
                }
                // ...and go to the next open cycle
            } else {
                // ************
                if (StreamOpen && (Status == STATUS_NOT_FOUND))
                    // handle SDir return code
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
                    // good path, but no such file
                    // break open loop and continue with Create
                    break;
                }
                if (!NT_SUCCESS(Status)) {
                    // Hard error or damaged data structures ...
#ifdef UDF_DBG
                    if ((Status != STATUS_OBJECT_PATH_NOT_FOUND) &&
                       (Status != STATUS_ACCESS_DENIED) &&
                       (Status != STATUS_NOT_A_DIRECTORY)) {
                        AdPrint(("    Hard error or damaged data structures\n"));
                    }
#endif // UDF_DBG
                    // ... and exit with error
                    try_return(Status);
                }
                // Note: With LCB model, FcbReference is not incremented during path traversal,
                // so no decrement is needed here (removed the old InterlockedDecrement).
                Status = STATUS_SUCCESS;
                ASSERT(!OpenTargetDirectory);
                // Restore PtrNewFcb/NewFileInfo from last good state.
                // The stream dir branch didn't open anything in this iteration,
                // but PtrNewFcb was reset to NULL at iteration start.
                // Restore from LastGoodFileInfo (the base file).
                PtrNewFcb = LastGoodFileInfo->Fcb;
                NewFileInfo = LastGoodFileInfo;
                UDFReferenceFile__(NewFileInfo);
                // break open loop and continue with Open
                // (Create will be skipped)
                break;
            }
        } // end of while(TRUE)

        // ****************
        // Newly created file — open via unified path
        // ****************
        if (NewFileCreated) {
            ASSERT(NT_SUCCESS(Status));
            ASSERT(NewFileInfo);
            ASSERT(!StreamOpen);

            // Open the newly created file with PerformUserOpen=TRUE.
            // Uses ExistingFileInfo to skip disk read (FileInfo already created by UDFCreateFile__).
            Status = UDFOpenObjectFromDirContext(
                IrpContext, IrpSp, Vcb, NULL,
                OldRelatedFileInfo,
                &CurrentFcb, &PreviousFcb,
                IgnoreCase, TRUE, CreateDisposition,
                0,  // RemainingNameLength = 0 (final component)
                RelatedFileObject, NextFcb,
                NewFileInfo,  // ExistingFileInfo — skip disk read
                &NewFileInfo, &PtrNewFcb);
            if (!NT_SUCCESS(Status)) {
                AdPrint(("    Can't open newly created file\n"));
                // Undo create — flush and unlink from disk.
                // UDFCloseFile__ is handled by the finally block (line ~2714).
                if (NewFileInfo &&
                    (Status != STATUS_FILE_IS_A_DIRECTORY) &&
                    (Status != STATUS_NOT_A_DIRECTORY) &&
                    (Status != STATUS_ACCESS_DENIED)) {
                    UDFFlushFile__(IrpContext, Vcb, NewFileInfo, 0);
                    UDFUnlinkFile__(IrpContext, Vcb, NewFileInfo, TRUE);
                }
                try_return(Status);
            }

            // FILE_CREATED, allocation size, attributes, and notification are
            // all handled inside UDFOpenExistingFcb via CreateDisposition == FILE_CREATE.

            LastGoodFileInfo = NewFileInfo;
            try_return(Status);
        }

        // ****************
        // should we CREATE a new stream ?
        // (Non-stream creation is handled inline above)
        // ****************
        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                Status == STATUS_OBJECT_PATH_NOT_FOUND) {
                if ((CreateDisposition == FILE_OPEN) ||
                    (CreateDisposition == FILE_OVERWRITE)) {
                    AdPrint(("    File doesn't exist\n"));
                    try_return(Status);
                }
            } else {
                //  Any other operation return STATUS_ACCESS_DENIED.
                AdPrint(("    Can't create due to unexpected error\n"));
                try_return(Status);
            }
            // Object was not found, create if requested
            if ((CreateDisposition != FILE_CREATE) && (CreateDisposition != FILE_OPEN_IF) &&
                 (CreateDisposition != FILE_OVERWRITE_IF) && (CreateDisposition != FILE_SUPERSEDE)) {
                AdPrint(("    File doesn't exist (2)\n"));
                try_return(Status);
            }
            // Check write protection
            if (Vcb->VcbState & VCB_STATE_MEDIA_WRITE_PROTECT) {
                AdPrint(("    Media write protected\n"));
                IoSetHardErrorOrVerifyDevice(Irp, Vcb->Vpb->RealDevice);
                try_return(Status = STATUS_MEDIA_WRITE_PROTECTED);
            }
            if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {
                AdPrint(("    Volume read only\n"));
                try_return(Status = STATUS_ACCESS_DENIED);
            }
            // Check r/o + delete on close
            if (DeleteOnClose &&
               (FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_READONLY ))) {

                AdPrint(("    Can't create r/o file marked for deletion\n"));
                try_return(Status = FlagOn(Vcb->VcbState, VCB_STATE_MOUNTED_DIRTY)
                                    ? STATUS_VOLUME_DIRTY : STATUS_CANNOT_DELETE);
            }

            // Create a new file/directory here ...
            // FinalName from UDFDissectName never contains backslashes
            {
                USHORT i;
                USHORT CharCount = FinalName.Length / sizeof(WCHAR);
                for(i = 0; i < CharCount; i++) {
                    if (FinalName.Buffer[i] == L'\\') {
                        AdPrint(("    Target name should not contain (back)slashes\n"));
                        try_return(Status = STATUS_OBJECT_NAME_INVALID);
                    }
                }
            }
            if (DirectoryFile &&
               ((IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_TEMPORARY) ||
                 StreamOpen || FALSE)) {
                AdPrint(("    Creation of _temporary_ directory not permited\n"));
                try_return(Status = STATUS_INVALID_PARAMETER);
            }
            // check access rights
            ASSERT(CurrentFcb);
            Status = UDFCheckAccessRights(NULL, NULL, OldRelatedFileInfo->Fcb, RelatedCcb, DirectoryFile ? FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE, 0);
            if (!NT_SUCCESS(Status)) {
                AdPrint(("    Creation of File/Dir not permitted\n"));
                try_return(Status);
            }
            // Note that a FCB structure will be allocated at this time
            // and so will a CCB structure.
            // Further, note that since the file is being created, no other
            // thread can have the file stream open at this time.
            RelatedFileInfo = OldRelatedFileInfo;

            Status = UDFCreateFile__(IrpContext, Vcb, IgnoreCase, &FinalName, 0, 0,
                                 UdfIsExtendedFESupported(Vcb),
                                 (CreateDisposition == FILE_CREATE), RelatedFileInfo, &NewFileInfo);
            if (!NT_SUCCESS(Status)) {
                AdPrint(("    Creation error\n"));
                try_return(Status);
            }
            // Update parent object
            if ((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) &&
               NextFcb &&
               RelatedFileObject &&
               (NextFcb->FileInfo == NewFileInfo->ParentFile)) {
                    RelatedFileObject->Flags |= (FO_FILE_MODIFIED | FO_FILE_SIZE_CHANGED);
            }

            if (DirectoryFile) {
                // user wants the directory to be created
                Status = UDFRecordDirectory__(IrpContext, Vcb, NewFileInfo);
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Can't transform to directory\n"));
                    // Undo create - flush and unlink from disk
                    if ((Status != STATUS_FILE_IS_A_DIRECTORY) &&
                       (Status != STATUS_NOT_A_DIRECTORY) &&
                       (Status != STATUS_ACCESS_DENIED)) {
                        UDFFlushFile__(IrpContext, Vcb, NewFileInfo, 0);
                        UDFUnlinkFile__(IrpContext, Vcb, NewFileInfo, TRUE);
                    }
                    // The common failure path releases this create reference.
                    try_return(Status);
                }
            }

            // Decide whether a separate stream object must be created.
            // When OldRelatedFileInfo is already a stream directory, the base file
            // and SDir existed and FinalName (the stream) was just created above.
            // Otherwise, for a stream path, the base file was created above and its
            // stream suffix still sits in RemainingName — extract the stream name
            // (validating/consuming ":$DATA"). An empty stream name ("file::$DATA")
            // is the default data stream, i.e. the base file itself — nothing more
            // to create.
            BOOLEAN CreateStreamObject = StreamOpen && !UDFIsAStreamDir(OldRelatedFileInfo) &&
                                         RemainingName.Length && RemainingName.Buffer[0] == L':';
            if (CreateStreamObject) {
                BOOLEAN IsStreamDummy;
                RtlZeroMemory(&StreamName, sizeof(StreamName));
                UDFDissectName(IrpContext, &RemainingName, &StreamName, &IsStreamDummy);
                UDFNormalizeStreamSuffix(IrpContext, &StreamName, &RemainingName);
                CreateStreamObject = (StreamName.Length != 0);
            }

            if (CreateStreamObject) {

                // PHASE 0: Open the base file through unified path (FCB + lock + LCB)
                PUDF_FILE_INFO BaseFileInfo = NewFileInfo;
                Status = UDFOpenObjectFromDirContext(
                    IrpContext, IrpSp, Vcb,
                    NULL, RelatedFileInfo,
                    &CurrentFcb, &PreviousFcb,
                    IgnoreCase, FALSE, CreateDisposition,
                    1,  // non-zero: intermediate — creates LCB
                    NULL, NULL,
                    BaseFileInfo,
                    &NewFileInfo, &PtrNewFcb);
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Can't open file for stream creation\n"));
                    BrutePoint();
                    try_return(Status);
                }
                LastGoodFileInfo = NewFileInfo;

                UDFNotifyReportChange(IrpContext, Vcb, NewFileInfo->Fcb,
                    UDFIsADirectory(NewFileInfo) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                    FILE_ACTION_ADDED,
                    NULL, FileObject);

                UDFDereferenceFile__(NewFileInfo);
            
                // PHASE 1: Create stream directory + open through unified path
                RelatedFileInfo = NewFileInfo;
                NewFileInfo = NULL;
                PUDF_FILE_INFO StreamDirInfo = NULL;
                Status = UDFCreateStreamDir__(IrpContext, Vcb, RelatedFileInfo, &StreamDirInfo);
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Can't create SDir\n"));
                    BrutePoint();
                    try_return(Status);
                }

                Status = UDFOpenObjectFromDirContext(
                    IrpContext, IrpSp, Vcb,
                    NULL, RelatedFileInfo,
                    &CurrentFcb, &PreviousFcb,
                    IgnoreCase, FALSE, CreateDisposition,
                    1,  // non-zero: intermediate — creates LCB
                    NULL, NULL,
                    StreamDirInfo,
                    &NewFileInfo, &PtrNewFcb);
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Can't open SDir\n"));
                    BrutePoint();
                    UDFCloseFile__(IrpContext, Vcb, StreamDirInfo);
                    try_return(Status);
                }
                LastGoodFileInfo = NewFileInfo;

                UDFDereferenceFile__(NewFileInfo);

                // PHASE 2: Create stream file in the stream directory.
                // StreamName already holds the bare stream name (no leading ':').
                RelatedFileInfo = NewFileInfo;
                NewFileInfo = NULL;
                Status = UDFCreateFile__(IrpContext, Vcb, IgnoreCase, &StreamName, 0, 0,
                         UdfIsExtendedFESupported(Vcb), (CreateDisposition == FILE_CREATE),
                         RelatedFileInfo, &NewFileInfo);
                if (!NT_SUCCESS(Status)) {
                    AdPrint(("    Can't create Stream\n"));
                    BrutePoint();
                    try_return(Status);
                }
            }

            // Open the created object through unified path (FCB + CCB + lock)
            {
                PUDF_FILE_INFO CreatedFileInfo = NewFileInfo;
                Status = UDFOpenObjectFromDirContext(
                    IrpContext, IrpSp, Vcb,
                    NULL, RelatedFileInfo,
                    &CurrentFcb, &PreviousFcb,
                    IgnoreCase, TRUE, CreateDisposition,
                    0,  // final component — no LCB
                    NULL, NULL,
                    CreatedFileInfo,
                    &NewFileInfo, &PtrNewFcb);
            }
            if (!NT_SUCCESS(Status)) {
                AdPrint(("    Can't open created file or stream\n"));
                BrutePoint();
                // Undo create — flush and unlink from disk
                if (NewFileInfo &&
                    (Status != STATUS_FILE_IS_A_DIRECTORY) &&
                    (Status != STATUS_NOT_A_DIRECTORY) &&
                    (Status != STATUS_ACCESS_DENIED)) {
                    UDFFlushFile__(IrpContext, Vcb, NewFileInfo, 0);
                    UDFUnlinkFile__(IrpContext, Vcb, NewFileInfo, TRUE);
                }
                // The common failure path releases this create reference.
                try_return(Status);
            }

            LastGoodFileInfo = NewFileInfo;
            try_return(Status);
        }

        } // end of path traversal block

        // Path traversal found existing file - open it.
        // Two paths:
        // (a) File path: PtrNewFcb is NULL, DirContext has the found entry — open via UDFOpenObjectFromDirContext(TRUE)
        // (b) Stream path: PtrNewFcb is set from loop — open via UDFOpenExistingFcb
        if (PtrNewFcb) {
            // Stream path — FCB already created in loop
            // Validate file/directory type against caller's request
            if (PtrNewFcb->FcbState & UDF_FCB_DIRECTORY) {
                if (NonDirectoryFile ||
                    (CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {
                    try_return(Status = STATUS_FILE_IS_A_DIRECTORY);
                }
            } else {
                if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH) || DirectoryFile) {
                    try_return(Status = STATUS_NOT_A_DIRECTORY);
                }
            }
            if (CreateDisposition == FILE_CREATE) {
                try_return(Status = STATUS_OBJECT_NAME_COLLISION);
            }

            try_return(Status = UDFOpenExistingFcb(IrpContext, IrpSp, Vcb,
                                                    FileObject, NewFileInfo,
                                                    &PtrNewFcb, IgnoreCase,
                                                    RelatedFileObject, NextFcb));
        }

        // File path — final component not yet opened, DirContext has the entry
        if (!DirContext.DirNdx) {
            BrutePoint();
            try_return(Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        {
            BOOLEAN IsDirectory = !!(DirContext.DirNdx->FileCharacteristics & FILE_DIRECTORY);
            // Trailing backslash or DirectoryFile: target must be a directory
            if ((FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH) || DirectoryFile) &&
                !IsDirectory) {
                try_return(Status = STATUS_NOT_A_DIRECTORY);
            }
            if (IsDirectory) {
                if (NonDirectoryFile ||
                    (CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {
                    try_return(Status = STATUS_FILE_IS_A_DIRECTORY);
                }
            }
        }
        // Metadata stream files (UDF 2.0) cannot be opened by name
        if (DirContext.DirNdx->FileCharacteristics & FILE_METADATA) {
            try_return(Status = STATUS_ACCESS_DENIED);
        }
        if (CreateDisposition == FILE_CREATE) {
            try_return(Status = STATUS_OBJECT_NAME_COLLISION);
        }

        // Open the final component with PerformUserOpen=TRUE (creates FileInfo + FCB + CCB)
        Status = UDFOpenObjectFromDirContext(
            IrpContext, IrpSp, Vcb, &DirContext,
            RelatedFileInfo,
            &CurrentFcb, &PreviousFcb,
            IgnoreCase, TRUE, CreateDisposition,
            0,  // RemainingNameLength = 0 (final component)
            RelatedFileObject, NextFcb,
            NULL,  // ExistingFileInfo
            &NewFileInfo, &PtrNewFcb);
        try_return(Status);

try_exit:   NOTHING;

        //
        //  Set final file object flags on successful open.
        //

        if (NT_SUCCESS(Status) && PtrNewFcb) {

            if (DeleteOnClose) {
                ASSERT(!(PtrNewFcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
            }

            if (StreamOpen) {
                FileObject->Flags |= FO_STREAM_FILE;
            }

            PtrNewFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
            PtrNewFcb->FcbState |= UDF_FCB_VALID;

            AdPrint(("    FCB %x, FO %x, Flags %x\n", PtrNewFcb, FileObject, PtrNewFcb->FcbState));
        }

    } _SEH2_FINALLY {

        //
        //  The result of this open could be success, pending or some error
        //  condition.
        //

        if (_SEH2_AbnormalTermination()) {

            //
            //  In the error path we start by calling our teardown routine if we
            //  have a CurrentFcb.
            //

            if (CurrentFcb != NULL) {

                BOOLEAN RemovedFcb = FALSE;

                UDFTeardownStructures( IrpContext, CurrentFcb, FALSE, &RemovedFcb );

                if (RemovedFcb) {

                    if (PreviousFcb == CurrentFcb) {
                        PreviousFcb = NULL;
                    }
                    CurrentFcb = NULL;
                }
            }

            // No need to complete the request.

            IrpContext = NULL;
            Irp = NULL;

        // If we posted this request we need to show that there is no
        // reason to complete the request.

        } else if (Status == STATUS_PENDING) {

            IrpContext = NULL;
            Irp = NULL;

        } else if (!NT_SUCCESS(Status)) {

            // Failure path — clean up partial structures.

            AdPrint(("UDF: CREATE FAILED RC=%x Fcb=%p Name='%wZ' Disp=%x DesAccess=%x\n",
                     Status, PtrNewFcb,
                     &(IrpSp->FileObject->FileName),
                     (IrpSp->Parameters.Create.Options >> 24) & 0xFF,
                     IrpSp->Parameters.Create.SecurityContext ?
                         IrpSp->Parameters.Create.SecurityContext->DesiredAccess : 0));

            // Balance UDFReferenceFile__ from prefix match / path traversal.

            if (NewFileInfo) {
                UDFCloseFile__(IrpContext, Vcb, NewFileInfo);
            }

            // Mark last successfully opened directory as valid.

            if (LastGoodFileInfo && LastGoodFileInfo->Fcb) {
                LastGoodFileInfo->Fcb->FcbState |= UDF_FCB_VALID;
                LastGoodFileInfo->Fcb->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
            }


            // Teardown partial FCB structures.
            // Protect PreviousFcb from deletion via FcbReference bump —
            // TeardownStructures walks parent chain and may free parents.

            // CurrentFcb may be NULL on early failures (e.g. FILE_CREATE on existing root)
            // while LastGoodFileInfo->Fcb is valid from path traversal. This is expected.
            ASSERT(!LastGoodFileInfo || !LastGoodFileInfo->Fcb ||
                   !CurrentFcb || CurrentFcb == LastGoodFileInfo->Fcb);

            // Type/disposition checks for a related-file reopen can fail
            // before we acquire its FCB. Teardown requires that FCB exclusive;
            // the still-open related handle owns it in that early-failure case.
            if (Vcb && (PtrNewFcb != Vcb->RootIndexFcb) &&
                LastGoodFileInfo && CurrentFcb == LastGoodFileInfo->Fcb) {

                BOOLEAN ProtectedPreviousFcb = FALSE;
                if (PreviousFcb && PreviousFcb != CurrentFcb) {
                    UDFLockVcb(IrpContext, Vcb);
                    PreviousFcb->FcbReference++;
                    UDFUnlockVcb(IrpContext, Vcb);
                    ProtectedPreviousFcb = TRUE;
                }

                BOOLEAN RemovedFcb = FALSE;
                UDFTeardownStructures(IrpContext, LastGoodFileInfo->Fcb, FALSE, &RemovedFcb);

                if (RemovedFcb) {
                    if (PreviousFcb == CurrentFcb) {
                        PreviousFcb = NULL;
                    }
                    CurrentFcb = NULL;
                }

                if (ProtectedPreviousFcb) {
                    UDFLockVcb(IrpContext, Vcb);
                    PreviousFcb->FcbReference--;
                    UDFUnlockVcb(IrpContext, Vcb);
                }

            } else {
                // The target is root, or validation failed before its FCB
                // was acquired. Neither case owns a target FCB for teardown.
                ASSERT(!LastGoodFileInfo || !CurrentFcb ||
                       (PtrNewFcb == Vcb->RootIndexFcb));
            }
        }

        // Release the Fcb locks.

        if (PreviousFcb && PreviousFcb != CurrentFcb) {

            UDFReleaseFcb(IrpContext, PreviousFcb);
        }

        if (CurrentFcb != NULL) {

            UDFReleaseFcb(IrpContext, CurrentFcb);
        }

        // Release the Vcb.

        UDFReleaseVcb(IrpContext, Vcb);

        // Call our completion routine.  It will handle the case where either
        // the Irp and/or IrpContext are gone.

        UDFCompleteRequest( IrpContext, Irp, Status );

    } _SEH2_END;

    return(Status);
} // end UDFCommonCreate()

/*************************************************************************
*
* Function: UDFFirstOpenFile()
*
* Description:
*   Perform first Open/Create initialization.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFFirstOpenFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
   OUT PFCB* PtrNewFcb,
    IN PUDF_FILE_INFO RelatedFileInfo,
    IN PUDF_FILE_INFO NewFileInfo
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PDIR_INDEX_HDR      hDirIndex;
    PDIR_INDEX_ITEM     DirIndex;
    FILE_ID FileId;
    NODE_TYPE_CODE NodeTypeCode;
    BOOLEAN FcbExisted;

    ASSERT(RelatedFileInfo);

    AdPrint(("UDFFirstOpenFile\n"));

    FileId = UDFGetNTFileId(Vcb, NewFileInfo);

    if (UDFIsADirectory(NewFileInfo)) {

        NodeTypeCode = UDF_NODE_TYPE_INDEX;
        SetFlag(FileId.HighPart, FID_DIR_MASK);

    } else {

        NodeTypeCode = UDF_NODE_TYPE_DATA;
    }

    // Acquire FcbTableMutex for atomic create + init.
    // This prevents another thread from finding a half-initialized FCB
    // in FcbTable (or via DirNdx->FileInfo->Fcb / Dloc->CommonFcb).
    // Lock ordering: FcbTableMutex (outer) before VcbMutex (inner).
    UDFLockFcbTable(IrpContext, Vcb);

    if (!((*PtrNewFcb) = UDFCreateFcb(IrpContext, FileId, NodeTypeCode, &FcbExisted))) {

        AdPrint(("Can't allocate FCB\n"));
        UDFUnlockFcbTable(IrpContext, Vcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (FcbExisted) {

        // Make link between Fcb and FileInfo
        (*PtrNewFcb)->FileInfo = NewFileInfo;
        NewFileInfo->Fcb = (*PtrNewFcb);
        (*PtrNewFcb)->ParentFcb = RelatedFileInfo->Fcb;

        // On 32-bit, FID_DIR_MASK is a no-op (bit 31 already set in
        // kernel addresses), so a file and a directory at the same LBA
        // share the same FileId.  When an LBA is reused (e.g., deleted
        // file's block allocated for a new stream directory), the stale
        // FCB in FcbTable may have wrong type flags.  Update them.
        if (UDFIsADirectory(NewFileInfo)) {
            (*PtrNewFcb)->FcbState |= UDF_FCB_DIRECTORY;
            (*PtrNewFcb)->Header.NodeTypeCode = UDF_NODE_TYPE_INDEX;
        } else {
            (*PtrNewFcb)->FcbState &= ~UDF_FCB_DIRECTORY;
            (*PtrNewFcb)->Header.NodeTypeCode = UDF_NODE_TYPE_DATA;
        }
        // Clear stale flags from previous incarnation
        (*PtrNewFcb)->FcbState &= ~(UDF_FCB_DELETED | UDF_FCB_NOT_FOUND_ON_MEDIA);
        // Update Dloc linkage for the new file
        NewFileInfo->Dloc->CommonFcb = (*PtrNewFcb);

        UDFUnlockFcbTable(IrpContext, Vcb);
        return STATUS_SUCCESS;

    }

    // Make link between Fcb and FileInfo
    (*PtrNewFcb)->FileInfo = NewFileInfo;
    NewFileInfo->Fcb = (*PtrNewFcb);
    (*PtrNewFcb)->ParentFcb = RelatedFileInfo->Fcb;

    // FcbTable is the single source of truth for FCB lookup.
    // If UDFCreateFcb didn't find an existing FCB (FcbExisted=FALSE),
    // this is a new FCB regardless of what CommonFcb says.
    // CommonFcb may contain a stale pointer from a previously failed open.
    NewFileInfo->Dloc->CommonFcb = (*PtrNewFcb);

    PFCB NewFcb = *PtrNewFcb;
    // Initialize times and sizes from on-disk FileEntry
    UDFGetFileXTime((*PtrNewFcb)->FileInfo,
        &(NewFcb->CreationTime.QuadPart),
        &(NewFcb->LastAccessTime.QuadPart),
        &(NewFcb->ChangeTime.QuadPart),
        &(NewFcb->LastWriteTime.QuadPart) );

    // Set the allocation size for the object
    NewFcb->Header.AllocationSize.QuadPart =
        UDFSysGetAllocSize(Vcb, NewFileInfo->Dloc->DataLoc.Length);
    NewFcb->Header.FileSize.QuadPart =
    NewFcb->Header.ValidDataLength.QuadPart = NewFileInfo->Dloc->DataLoc.Length;

    // VcbMutex (inner lock) for refcount operations in UDFInitializeFCB
    UDFLockVcb(IrpContext, Vcb);

    // FileObject=NULL: CCB is created later by UDFOpenExistingFcb
    RC = UDFInitializeFCB(*PtrNewFcb, Vcb,
                 UDFIsADirectory(NewFileInfo) ? UDF_FCB_DIRECTORY : 0);

    if (!NT_SUCCESS(RC)) {

        UDFUnlockVcb(IrpContext, Vcb);
        UDFUnlockFcbTable(IrpContext, Vcb);
        return RC;
    }
    // Set embedded data flag if file data is stored in ICB
    if (!UDFIsADirectory(NewFileInfo) &&
        (((PFILE_ENTRY)(NewFileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {
        (*PtrNewFcb)->FcbState |= UDF_FCB_EMBEDDED_DATA;
    }
    // set Read-only attribute
    if (!UDFIsAStreamDir(NewFileInfo)) {
        hDirIndex = UDFGetDirIndexByFileInfo(NewFileInfo);
#ifdef UDF_DBG
        if (!hDirIndex) {
            BrutePoint();
        } else {
#endif // UDF_DBG
            (*PtrNewFcb)->FileAttributes = UDFAttributesToNT(DirIndex = UDFDirIndex(hDirIndex, NewFileInfo->Index),NULL);
            if ((*PtrNewFcb)->FileAttributes & FILE_ATTRIBUTE_READONLY) {
                (*PtrNewFcb)->FcbState |= UDF_FCB_READ_ONLY;
            }
            // Names are now stored in LCB, not appended to FCBName
#ifdef UDF_DBG
        }
#endif // UDF_DBG
    } else if (RelatedFileInfo->ParentFile) {
        hDirIndex = UDFGetDirIndexByFileInfo(RelatedFileInfo);
        (*PtrNewFcb)->FileAttributes = UDFAttributesToNT(DirIndex = UDFDirIndex(hDirIndex, RelatedFileInfo->Index),NULL);
        if ((*PtrNewFcb)->FileAttributes & FILE_ATTRIBUTE_READONLY) {
            (*PtrNewFcb)->FcbState |= UDF_FCB_READ_ONLY;
        }
        // Names are now stored in LCB, not appended to FCBName
//    } else {
//        BrutePoint();
    }

    // Insert FCB into table and mark valid.
    UDFInsertFcbIntoTable(IrpContext, *PtrNewFcb);
    (*PtrNewFcb)->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
    (*PtrNewFcb)->FcbState |= UDF_FCB_VALID;

    UDFUnlockVcb(IrpContext, Vcb);
    UDFUnlockFcbTable(IrpContext, Vcb);

    return RC;
} // end UDFFirstOpenFile()

/*************************************************************************
*
* Function: UDFOpenFile()
*
* Description:
*   Open a file/dir for the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCompleteFcbOpen(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_STACK_LOCATION IrpSp,
    _In_ PVCB Vcb,
    _Inout_ PFCB *CurrentFcb,
    _In_ TYPE_OF_OPEN TypeOfOpen,
    _In_ ULONG UserCcbFlags,
    _In_ ULONG CreateDisposition,
    _In_ BOOLEAN VcbLocked,
    _In_ ULONG DesiredInformation
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN LockVolume = FALSE;
    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN DeleteCcbOnUnwind = FALSE;
    PFCB Fcb = *CurrentFcb;
    PCCB Ccb = NULL;
    ULONG Information = DesiredInformation;
    USHORT FileAttributes = IrpSp->Parameters.Create.FileAttributes;
    PIO_SECURITY_CONTEXT SecurityContext = IrpSp->Parameters.Create.SecurityContext;
    ACCESS_MASK AddedAccess = 0;
    ACCESS_MASK DesiredAccess = SecurityContext->DesiredAccess;
    BOOLEAN DeleteOnClose = BooleanFlagOn(IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE);

    ASSERT_FCB(Fcb);

    // Expand maximum allowed to something sensible for share access checking

    if (DesiredAccess == MAXIMUM_ALLOWED) {

        if (FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY)) {

            DesiredAccess = FILE_ALL_ACCESS & ~((TypeOfOpen != UserVolumeOpen ?
                                                 (FILE_WRITE_ATTRIBUTES           |
                                                  FILE_WRITE_DATA                 |
                                                  FILE_WRITE_EA                   |
                                                  FILE_ADD_FILE                   |                     
                                                  FILE_ADD_SUBDIRECTORY           |
                                                  FILE_APPEND_DATA) : 0)          |
                                                FILE_DELETE_CHILD                 |
                                                DELETE                            |
                                                WRITE_DAC);
        } else {

            DesiredAccess = FILE_ALL_ACCESS & ~(WRITE_DAC | FILE_WRITE_EA);
        }

        SecurityContext->DesiredAccess = DesiredAccess;
    }

    if (CreateDisposition == FILE_SUPERSEDE) {

        SetFlag(AddedAccess, ~(DesiredAccess) & DELETE);
        SetFlag(DesiredAccess, DELETE);
        SecurityContext->DesiredAccess = DesiredAccess;

    } else if ((CreateDisposition == FILE_OVERWRITE) ||
               (CreateDisposition == FILE_OVERWRITE_IF)) {

        SetFlag(AddedAccess, ~DesiredAccess & (FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES));
        SetFlag(DesiredAccess, FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES);
        SecurityContext->DesiredAccess = DesiredAccess;
    }

    // If this a volume open and the user wants to lock the volume then
    // purge and lock the volume.

    if ((TypeOfOpen == UserVolumeOpen) &&
        !FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ)) {

        // If there are open handles then fail this immediately.

        if (Vcb->VcbCleanup != 0) {

            return STATUS_SHARING_VIOLATION;
        }

        // If we can't wait then force this to be posted.

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

            UDFRaiseStatus(IrpContext, STATUS_CANT_WAIT);
        }

        LockVolume = TRUE;

        // Flush the volume and make sure all of the user references
        // are gone.

        Status = UDFFlushVolume(IrpContext, Vcb, 0);

        if (!NT_SUCCESS(Status)) {

            return Status;
        }

        // Now force all of the delayed close operations to go away.

        UDFFspClose(Vcb);

        if (Vcb->VcbUserReference > Vcb->VcbResidualUserReference) {

            return STATUS_SHARING_VIOLATION;
        }
    }

    if (CreateDisposition == FILE_CREATE) {

        if (FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {

            Fcb->FcbState |= FCB_STATE_TEMPORARY;
            Fcb->FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
        }
    }
    else {

        if ((Fcb->FileAttributes & FILE_ATTRIBUTE_READONLY) &&
            (Fcb->Header.NodeTypeCode != UDF_NODE_TYPE_INDEX) &&
            (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {

            return STATUS_ACCESS_DENIED;
        }

        if (DeleteOnClose && (Fcb->FileAttributes & FILE_ATTRIBUTE_READONLY)) {

            return STATUS_CANNOT_DELETE;
        }
    }

    if (!(Ccb = UDFCreateCcb())) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeleteCcbOnUnwind = TRUE;

    _SEH2_TRY {

        if (AddedAccess) {

            ClearFlag(DesiredAccess, AddedAccess);
            SecurityContext->DesiredAccess = DesiredAccess;
        }

        // If write access or delete-on-close requested, verify that the file image
        // can be flushed. Pre-increment reference counts so the FCB/VCB stay alive
        // during the check; if the check fails, the finally block will undo them.

        if ((DesiredAccess & FILE_WRITE_DATA) || DeleteOnClose) {

            if (!VcbLocked) {
                UDFLockVcb(IrpContext, Fcb->Vcb);
            }

            Fcb->FcbReference++;
            Fcb->Vcb->VcbReference++;

            if (!VcbLocked) {
                UDFUnlockVcb(IrpContext, Fcb->Vcb);
            }

            VcbAcquired = TRUE;

            if (!MmFlushImageSection(&Fcb->FcbNonpaged->SegmentObject, MmFlushForWrite)) {

                Status = DeleteOnClose ? STATUS_CANNOT_DELETE : STATUS_SHARING_VIOLATION;
                try_return(Status);
            }
        }

        // Check share access before updating

        if (Fcb->FcbCleanup != 0) {

            Status = IoCheckShareAccess(DesiredAccess,
                                        IrpSp->Parameters.Create.ShareAccess,
                                        IrpSp->FileObject,
                                        &Fcb->ShareAccess,
                                        FALSE);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }
        }

        // Update the share access.

        if (Fcb->FcbCleanup == 0) {

            IoSetShareAccess(DesiredAccess,
                             IrpSp->Parameters.Create.ShareAccess,
                             IrpSp->FileObject,
                             &Fcb->ShareAccess);

        } else {

            IoUpdateShareAccess(IrpSp->FileObject, &Fcb->ShareAccess);
        }

        // initialize the CCB
        Ccb->Fcb = Fcb;
        // initialize the CCB to point to the file object
        Ccb->FileObject = IrpSp->FileObject;

        // Set CCB flags from caller + DeleteOnClose from IrpSp
        Ccb->Flags = UserCcbFlags;
        if (DeleteOnClose) {
            Ccb->Flags |= UDF_CCB_DELETE_ON_CLOSE;
        }

        // A relative open (RelatedFileObject supplied) leaves FileObject->FileName
        // holding only the final path component. Record it so change notifications
        // build the absolute target name from the LCB chain instead of trusting
        // the relative FileName.

        if (IrpSp->FileObject->RelatedFileObject != NULL) {

            Ccb->Flags |= CCB_FLAG_HAS_RELATED_CCB;
        }

        // Acquire or create LCB to link parent directory FCB to this file FCB.
        // Skip for root directory (no parent).
        // LCB keeps parent FCB alive while children are open, preventing
        // premature teardown of in-memory directory index.
        // LCB is also inserted into splay trees for O(log n) name lookup.
        if (Fcb->ParentFcb) {

            if (Fcb->FileInfo) {
                UNICODE_STRING FileName;
                UNICODE_STRING CaseFileName;
                UNICODE_STRING ShortName;
                NTSTATUS NameStatus;

                // Extract file name from FileInfo
                RtlZeroMemory(&FileName, sizeof(UNICODE_STRING));
                RtlZeroMemory(&CaseFileName, sizeof(UNICODE_STRING));
                RtlZeroMemory(&ShortName, sizeof(UNICODE_STRING));

                NameStatus = UDFGetFileNameFromFileInfo(Fcb->FileInfo, &FileName);

                if (NT_SUCCESS(NameStatus) && FileName.Buffer != NULL) {
                    // Create uppercase version for case-insensitive comparisons
                    UDFUpcaseString(&CaseFileName, &FileName);

                    // 8.3 alias for DOS-style lookups through the prefix cache
                    PDIR_INDEX_ITEM NameNdx = NULL;
                    if (Fcb->FileInfo->ParentFile)
                        NameNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo),
                                              Fcb->FileInfo->Index);
                    if (NameNdx ? !(NameNdx->FI_Flags & UDF_FI_FLAG_DOS)
                                : !UDFCanNameBeA8dot3(&FileName)) {
                        UDFGenerateShortName(Fcb->Vcb, &FileName, NameNdx, &ShortName);
                    }

                    Ccb->Lcb = UDFAcquirePrefix(IrpContext,
                                                Fcb->ParentFcb,
                                                Fcb,
                                                &FileName,
                                                &CaseFileName,
                                                ShortName.Buffer ? &ShortName : NULL,
                                                Fcb->FileInfo->Index);

                    if (FileName.Buffer) {
                        MyFreePool__(FileName.Buffer);
                    }
                    if (CaseFileName.Buffer) {
                        ExFreePool(CaseFileName.Buffer);
                    }
                    if (ShortName.Buffer) {
                        ExFreePool(ShortName.Buffer);
                    }
                } else {
                    Ccb->Lcb = UDFAcquirePrefix(IrpContext,
                                                Fcb->ParentFcb,
                                                Fcb,
                                                NULL, NULL, NULL,
                                                Fcb->FileInfo->Index);
                }
            } else {
                Ccb->Lcb = UDFAcquirePrefix(IrpContext,
                                            Fcb->ParentFcb,
                                            Fcb,
                                            NULL, NULL, NULL,
                                            0);
            }
        }

        // Set the file object type.
        UDFSetFileObject(IrpSp->FileObject, TypeOfOpen, Fcb, Ccb);

        UDFLockFcb(IrpContext, Fcb);

        if (TypeOfOpen == UserFileOpen) {

            Fcb->Header.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

            if (FlagOn(IrpSp->Parameters.Create.Options, FILE_SEQUENTIAL_ONLY) &&
               !FlagOn(Fcb->Vcb->CompatFlags, UDF_VCB_IC_IGNORE_SEQUENTIAL_IO)) {
                IrpSp->FileObject->Flags |= FO_SEQUENTIAL_ONLY;

                if (Fcb->Vcb->TargetDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
                    IrpSp->FileObject->Flags &= ~FO_WRITE_THROUGH;
                }

                if (Fcb->FileInfo) {
                    UDFSetFileAllocMode__(Fcb->FileInfo, EXTENT_FLAG_ALLOC_SEQUENTIAL);
                }
            }

            if (FlagOn(IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING)) {
                IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
            } else {
                IrpSp->FileObject->Flags |= FO_CACHE_SUPPORTED;
                InterlockedIncrement((PLONG)&Fcb->CachedOpenHandleCount);
            }

        } else if (TypeOfOpen == UserVolumeOpen) {

            Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
            IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;

        } else {

            Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
        }

        UDFUnlockFcb(IrpContext, Fcb);

        // Point to the section object pointer in the non-paged Fcb.
        IrpSp->FileObject->SectionObjectPointer = &Fcb->FcbNonpaged->SegmentObject;

        Fcb->FcbState &= ~UDF_FCB_DELAY_CLOSE;

        // Increment all reference counts here when CCB is successfully created.
        // If VcbLocked is TRUE, caller already holds VcbMutex - use simple ++.
        // If VcbLocked is FALSE, acquire VcbMutex for atomicity.
        //
        if (!VcbLocked) {
            UDFLockVcb(IrpContext, Fcb->Vcb);
        }

        // Cleanup counts:
        Fcb->FcbCleanup++;
        Fcb->Vcb->VcbCleanup++;

        // Reference counts: skip FcbReference/VcbReference if already
        // pre-incremented for MmFlushImageSection check above.
        if (!VcbAcquired) {
            Fcb->FcbReference++;
            Fcb->Vcb->VcbReference++;
        }

        // User reference counts: always increment.
        Fcb->FcbUserReference++;
        Fcb->Vcb->VcbUserReference++;

        // Increment LCB reference count.
        // This reference will be decremented in cleanup.cpp.
        if (Ccb->Lcb) {
            Ccb->Lcb->Reference++;
        }

        // All reference increments succeeded — clear VcbAcquired so the
        // finally block does not undo the pre-incremented counts.
        VcbAcquired = FALSE;

        // CCB is now owned by the file object — don't delete on unwind.
        DeleteCcbOnUnwind = FALSE;

        if (!VcbLocked) {
            UDFUnlockVcb(IrpContext, Fcb->Vcb);
        }

        // For SL_OPEN_TARGET_DIRECTORY: save full path length in MaximumLength,
        // then trim Length to the parent directory path (up to last '\').
        // Rename/link handlers extract filename from the MaximumLength - Length gap.

        if (FlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY)) {

            PFILE_OBJECT FileObject = IrpSp->FileObject;

            FileObject->FileName.MaximumLength = FileObject->FileName.Length;

            // Root path "\" — nothing to trim
            if (FileObject->FileName.Length == sizeof(WCHAR) &&
                FileObject->FileName.Buffer[0] == L'\\') {
                // Length stays as root
            } else if (FileObject->FileName.Length >= sizeof(WCHAR)) {

                LONG i;
                USHORT CharCount = FileObject->FileName.Length / sizeof(WCHAR);

                // Scan backwards from second-to-last char to find last '\'
                for (i = (LONG)CharCount - 2; i >= 0; i--) {

                    if (FileObject->FileName.Buffer[i] == L'\\') {
                        break;
                    }
                }

                if (i < 0) {
                    // No backslash found
                    FileObject->FileName.Length = 0;
                } else if (i == 0) {
                    // Backslash at root position: keep "\"
                    FileObject->FileName.Length = sizeof(WCHAR);
                } else {
                    // Normal case: trim to position before backslash
                    FileObject->FileName.Length = (USHORT)(i * sizeof(WCHAR));
                }
            }
        }

        // Set the information field in the Irp to indicate the action taken.
        IrpContext->Irp->IoStatus.Information = Information;

try_exit:   NOTHING;
    } _SEH2_FINALLY {

        // Undo pre-incremented reference counts if we failed between
        // the early increment (MmFlushImageSection path) and the final
        // increment block where VcbAcquired is cleared.
        if (VcbAcquired) {

            if (!VcbLocked) {
                UDFLockVcb(IrpContext, Fcb->Vcb);
            }

            Fcb->FcbReference--;
            Fcb->Vcb->VcbReference--;

            if (!VcbLocked) {
                UDFUnlockVcb(IrpContext, Fcb->Vcb);
            }
        }

        if (DeleteCcbOnUnwind) {
            Ccb->Lcb = NULL;
            UDFDeleteCcb(Ccb);
        }

    } _SEH2_END;

    return Status;
} // end UDFCompleteFcbOpen()

_When_(RelatedTypeOfOpen != UnopenedFileObject, _At_(RelatedCcb, _In_))
_When_(RelatedTypeOfOpen == UnopenedFileObject, _At_(RelatedCcb, _In_opt_))
_When_(RelatedTypeOfOpen != UnopenedFileObject, _At_(RelatedFileName, _In_))
_When_(RelatedTypeOfOpen == UnopenedFileObject, _At_(RelatedFileName, _In_opt_))
NTSTATUS
UDFNormalizeFileNames(
    _Inout_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ BOOLEAN OpenByFileId,
    _In_ TYPE_OF_OPEN RelatedTypeOfOpen,
    PCCB RelatedCcb,
    PUNICODE_STRING RelatedFileName,
    _Inout_ PUNICODE_STRING FileName,
    _Inout_ PUNICODE_STRING RemainingName
    )

/*++

Routine Description:

    This routine is called to store the full name and upcased name into the
    filename buffer.  We only upcase the portion yet to parse.  We also
    check for a trailing backslash and lead-in double backslashes.  This
    routine also verifies the mode of the related open against the name
    currently in the filename.

Arguments:

    Vcb - Vcb for this volume.

    OpenByFileId - Indicates if the filename should be a 64 bit FileId.

    IgnoreCase - Indicates if this open is a case-insensitive operation.

    RelatedTypeOfOpen - Indicates the type of the related file object.

    RelatedCcb - Ccb for the related open.  Ignored if no relative open.

    RelatedFileName - FileName buffer for related open.  Ignored if no
        relative open.

    FileName - FileName to update in this routine.  The name should
        either be a 64-bit FileId or a Unicode string.

    RemainingName - Name with the remaining portion of the name.  This
        will begin after the related name and any separator.  For a
        non-relative open we also step over the initial separator.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the names are OK, appropriate error code
        otherwise.

--*/

{
    ULONG RemainingNameLength = 0;
    ULONG RelatedNameLength = 0;
    ULONG SeparatorLength = 0;
    BOOLEAN HasColon = FALSE;

    ULONG BufferLength;

    UNICODE_STRING NewFileName;

    PAGED_CODE();

    // If this is the first pass then we need to build the full name and
    // check for name compatibility.

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FULL_NAME)) {

        //  Deal with the regular file name case first.

        if (!OpenByFileId) {

            //  This is here because the Win32 layer can't avoid sending me double
            //  beginning backslashes.

            if ((FileName->Length > sizeof( WCHAR )) &&
                (FileName->Buffer[1] == L'\\') &&
                (FileName->Buffer[0] == L'\\')) {

                //
                //  If there are still two beginning backslashes, the name is bogus.
                //

                if ((FileName->Length > 2 * sizeof( WCHAR )) &&
                    (FileName->Buffer[2] == L'\\')) {

                    return STATUS_OBJECT_NAME_INVALID;
                }

                //
                //  Slide the name down in the buffer.
                //

                FileName->Length -= sizeof( WCHAR );

                RtlMoveMemory( FileName->Buffer,
                               FileName->Buffer + 1,
                               FileName->Length );
            }

            //
            //  Check for a trailing backslash.  Don't strip off if only character
            //  in the full name or for relative opens where this is illegal.
            //

            if (((FileName->Length > sizeof( WCHAR)) ||
                 ((FileName->Length == sizeof( WCHAR )) && (RelatedTypeOfOpen == UserDirectoryOpen))) &&
                (FileName->Buffer[ (FileName->Length/2) - 1 ] == L'\\')) {

                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH);
                FileName->Length -= sizeof( WCHAR );
            }

            //
            //  Remember the length we need for this portion of the name.
            //

            RemainingNameLength = FileName->Length;

            //
            //  If this is a related file object then we verify the compatibility
            //  of the name in the file object with the relative file object.
            //

            if (RelatedTypeOfOpen != UnopenedFileObject) {

                //
                //  If the filename length was zero then it must be legal.
                //  If there are characters then check with the related
                //  type of open.
                //

                if (FileName->Length != 0) {

                    //
                    //  The name length must always be zero for a volume open.
                    //

                    if (RelatedTypeOfOpen <= UserVolumeOpen) {

                        return STATUS_INVALID_PARAMETER;

                    // The remaining name cannot begin with a backslash.

                    } else if (FileName->Buffer[0] == L'\\') {

                        return STATUS_INVALID_PARAMETER;

                        // If the related file is a user file then there
                        // is no file with this path.

                    } else if (RelatedTypeOfOpen == UserFileOpen &&
                               Vcb->UdfRevision < 0x200) {

                        return STATUS_OBJECT_PATH_NOT_FOUND;

                    } else if (FileName->Buffer[0] == L':') {

                        HasColon = TRUE;
                    }
                }

                // Remember the length of the related name when building
                // the full name.  We leave the RelatedNameLength and
                // SeparatorLength at zero if the relative file is opened
                // by Id.

                if (!FlagOn(RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID)) {

                    // Add a separator if the name length is non-zero
                    // unless the relative Fcb is at the root.

                    if ((FileName->Length != 0) &&
                        (RelatedCcb->Fcb != Vcb->RootIndexFcb)) {

                        if (!HasColon) {
                            SeparatorLength = sizeof(WCHAR);
                        }
                    }

                    RelatedNameLength = RelatedFileName->Length;
                }

            //  The full name is already in the filename.  It must either
            //  be length 0 or begin with a backslash.

            } else if (FileName->Length != 0) {

                if (FileName->Buffer[0] != L'\\') {

                    return STATUS_INVALID_PARAMETER;
                }

                //
                //  We will want to trim the leading backslash from the
                //  remaining name we return.
                //

                RemainingNameLength -= sizeof(WCHAR);
                SeparatorLength = sizeof(WCHAR);
            }

            //  Now see if the buffer is large enough to hold the full name.

            BufferLength = RelatedNameLength + SeparatorLength + RemainingNameLength;

            //  Check for an overflow of the maximum filename size.

            if (BufferLength > MAXUSHORT) {

                return STATUS_INVALID_PARAMETER;
            }

            //  Now see if we need to allocate a new buffer.

            if (FileName->MaximumLength < BufferLength) {

                NewFileName.Buffer = (PWCH)FsRtlAllocatePoolWithTag(PagedPool,
                                                                    BufferLength,
                                                                    TAG_FILE_NAME);

                NewFileName.MaximumLength = (USHORT) BufferLength;

            } else {

                NewFileName.Buffer = FileName->Buffer;
                NewFileName.MaximumLength = FileName->MaximumLength;
            }

            //  If there is a related name then we need to slide the remaining bytes up and
            //  insert the related name.  Otherwise the name is in the correct position
            //  already.

            if (RelatedNameLength != 0) {

                //
                //  Store the remaining name in its correct position.
                //

                if (RemainingNameLength != 0) {

                    RtlMoveMemory(Add2Ptr( NewFileName.Buffer, RelatedNameLength + SeparatorLength, PVOID),
                                  FileName->Buffer,
                                  RemainingNameLength);
                }

                RtlCopyMemory( NewFileName.Buffer,
                               RelatedFileName->Buffer,
                               RelatedNameLength );

                //
                //  Add the separator if needed.
                //

                if (SeparatorLength != 0) {

                    WCHAR separatorChar = (RelatedTypeOfOpen == UserDirectoryOpen) ? L'\\' : L':';
                    *(Add2Ptr(NewFileName.Buffer, RelatedNameLength, PWCHAR)) = separatorChar;
                }

                // Update the filename value we got from the user.

                if (NewFileName.Buffer != FileName->Buffer) {

                    if (FileName->Buffer != NULL) {

                        UDFFreePool((PVOID*)&FileName->Buffer);
                    }

                    FileName->Buffer = NewFileName.Buffer;
                    FileName->MaximumLength = NewFileName.MaximumLength;
                }

                //
                //  Copy the name length to the user's filename.
                //

                FileName->Length = (USHORT)(RelatedNameLength + SeparatorLength + RemainingNameLength);
            }

            // Now update the remaining name to parse.

            RemainingName->MaximumLength =
            RemainingName->Length = (USHORT)RemainingNameLength;

            RemainingName->Buffer = Add2Ptr(FileName->Buffer,
                                            RelatedNameLength + SeparatorLength,
                                            PWCHAR);

            //  For the open by file Id case we verify the name really contains
            //  a 64 bit value.

        } else {

            //
            //  Check for validity of the buffer.
            //

            if (FileName->Length != sizeof( FILE_ID )) {

                return STATUS_INVALID_PARAMETER;
            }
        }

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FULL_NAME);

        // If we are in the retry path then the full name is already in the
        // file object name.  If this is a case-sensitive operation then
        // we need to upcase the name from the end of any related file name already stored
        // there.

    } else {

        // Assume there is no relative name.

        *RemainingName = *FileName;

        // Nothing to do if the name length is zero.

        if (RemainingName->Length != 0) {

            //  If there is a relative name then we need to walk past it.

            if (RelatedTypeOfOpen != UnopenedFileObject) {

                // Nothing to walk past if the RelatedCcb is opened by FileId.

                if (!FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    //  Related file name is a proper prefix of the full name.
                    //  We step over the related name and if we are then
                    //  pointing at a separator character we step over that.

                    RemainingName->Buffer = Add2Ptr(RemainingName->Buffer,
                                                    RelatedFileName->Length,
                                                    PWCHAR);

                    RemainingName->Length -= RelatedFileName->Length;
                }
            }

            // If we are pointing at a separator character then step past that.

            if (RemainingName->Length != 0) {

                if (*(RemainingName->Buffer) == L'\\') {

                    RemainingName->Buffer = Add2Ptr(RemainingName->Buffer,
                                                    sizeof(WCHAR),
                                                    PWCHAR);

                    RemainingName->Length -= sizeof(WCHAR);
                }
            }
        }
    }

#pragma prefast(push)
#pragma prefast(suppress:26030, "RemainingName->FileName.Buffer = FileName.Buffer + (RelatedNameLength + SeparatorLength); FileName.MaximumLength < (RelatedNameLength + SeparatorLength + RemainingNameLength).")
    return STATUS_SUCCESS;
#pragma prefast(pop)
}
