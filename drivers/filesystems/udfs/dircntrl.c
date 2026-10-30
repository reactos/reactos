////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: DirCntrl.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "directory control" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_DIR_CONTROL

/*
// Local support routine(s):
*/

#define UDF_FNM_FLAG_CAN_BE_8D3    0x01
#define UDF_FNM_FLAG_IGNORE_CASE   0x02
#define UDF_FNM_FLAG_CONTAINS_WC   0x04

NTSTATUS
UDFFindNextMatch(
    IN PVCB            Vcb,
    IN PDIR_INDEX_HDR  hDirIndex,
    IN PLONG           CurrentNumber,      // Must be modified
    IN PUNICODE_STRING SearchPattern,
    IN ULONG           CcbFlags,
    IN PHASH_ENTRY     hashes,
   OUT PDIR_INDEX_ITEM* _DirNdx);

NTSTATUS
NTAPI
UDFQueryDirectory(
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp,
    PFILE_OBJECT FileObject,
    PFCB Fcb,
    PCCB Ccb
    );

NTSTATUS
NTAPI
UDFNotifyChangeDirectory(
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp,
    PFILE_OBJECT FileObject,
    PFCB Fcb,
    PCCB Ccb
    );

/*************************************************************************
*
* Function: UDFCommonDirControl()
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
NTAPI
UDFCommonDirControl(
   PIRP_CONTEXT IrpContext,
   PIRP              Irp
   )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp;
    PFILE_OBJECT            FileObject = NULL;
    PFCB                    Fcb = NULL;
    PCCB                    Ccb = NULL;
    PVCB                    Vcb = NULL;

    PAGED_CODE();

    TmPrint(("UDFCommonDirControl: \n"));

    // Decode the user file object and fail this request if it is not
    // a user directory.

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IrpSp->FileObject;

    if (UDFDecodeFileObject(FileObject, &Fcb, &Ccb) != UserDirectoryOpen) {

        UDFCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    // Validate the sent-in FCB
    if ((Fcb == Fcb->Vcb->VolumeDasdFcb) ||
        !(Fcb->FcbState & UDF_FCB_DIRECTORY)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    UDFFlushTryBreak(Vcb);

    // Get some of the parameters supplied to us
    switch (IrpSp->MinorFunction) {
    case IRP_MN_QUERY_DIRECTORY:

        RC = UDFQueryDirectory(IrpContext, Irp, IrpSp, FileObject, Fcb, Ccb);
        break;
    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        RC = UDFNotifyChangeDirectory(IrpContext, Irp, IrpSp, FileObject, Fcb, Ccb);
        break;
    default:

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return(RC);
} // end UDFCommonDirControl()


/*************************************************************************
*
* Function: UDFInitializeEnumeration()
*
* Description:
*   Initialize the state for a directory enumeration.  This sets up the
*   search pattern in the CCB and determines the starting position.
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFInitializeEnumeration(
    IN PIRP_CONTEXT         IrpContext,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PVCB                 Vcb,
    IN PFCB                 Fcb,
    IN PCCB                 Ccb,
    OUT PUNICODE_STRING     *PtrSearchPattern,
    OUT PHASH_ENTRY         *CurHashes,
    OUT PLONG               NextMatch,
    OUT PBOOLEAN            ReturnNextEntry,
    OUT PBOOLEAN            ReturnSingleEntry,
    OUT PBOOLEAN            InitialQuery
    )
{
    PUNICODE_STRING FileName;
    UNICODE_STRING  SearchExpression;
    HASH_ENTRY      SearchHashes;
    ULONG           CcbFlags;

    *CurHashes = NULL;
    *InitialQuery = FALSE;
    *ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);

    //
    // If this is the initial query then build a search expression from the
    // input file name.
    //

    if (!FlagOn(Ccb->Flags, UDF_CCB_ENUM_INITIALIZED)) {

        FileName = (PUNICODE_STRING)(IrpSp->Parameters.QueryDirectory.FileName);

        CcbFlags = 0;

        // Strip trailing null from the pattern if present.
        if (FileName && FileName->Buffer && FileName->Length >= sizeof(WCHAR) &&
            !FileName->Buffer[FileName->Length / sizeof(WCHAR) - 1]) {
            FileName->Length -= sizeof(WCHAR);
        }

        //
        // If the filename is not specified or is a match-all mask then we
        // will match all names.
        //

        if (!FileName || !FileName->Buffer || FileName->Length == 0 ||
            UDFIsMatchAllMask(FileName, NULL)) {

            SetFlag(CcbFlags, UDF_CCB_MATCH_ALL);

            SearchExpression.Length =
            SearchExpression.MaximumLength = 0;
            SearchExpression.Buffer = NULL;

        } else {

            //
            // Allocate buffer for the search expression.
            // Upcase if this is a case-insensitive search.
            //

            SearchExpression.Buffer = (PWCHAR)FsRtlAllocatePoolWithTag(
                PagedPool, FileName->MaximumLength, TAG_SEARCH_EXPR);
            SearchExpression.MaximumLength = FileName->MaximumLength;

            if (FlagOn(Ccb->Flags, CCB_FLAG_IGNORE_CASE)) {
                NTSTATUS Status = RtlUpcaseUnicodeString(&SearchExpression, FileName, FALSE);
                if (!NT_SUCCESS(Status)) {
                    MyFreePool__(SearchExpression.Buffer);
                    return Status;
                }
            } else {
                SearchExpression.Length = FileName->Length;
                RtlCopyMemory(SearchExpression.Buffer,
                              FileName->Buffer, FileName->MaximumLength);
            }

            //
            // Check for wildcards and build hash for exact-match patterns.
            //

            if (FsRtlDoesNameContainWildCards(&SearchExpression)) {
                SetFlag(CcbFlags, UDF_CCB_WILDCARD_PRESENT);
            } else {
                UDFBuildHashEntry(Vcb, &SearchExpression,
                                  &SearchHashes, HASH_POSIX | HASH_ULFN);
            }

            if (UDFCanNameBeA8dot3(&SearchExpression)) {
                SetFlag(CcbFlags, UDF_CCB_CAN_BE_8_DOT_3);
            }
        }

        //
        // Now lock the Fcb in order to update the CCB with the initial
        // enumeration values.
        //

        UDFLockFcb(IrpContext, Fcb);

        //
        // Check again that this is the initial search.
        //

        if (!FlagOn(Ccb->Flags, UDF_CCB_ENUM_INITIALIZED)) {

            Ccb->CurrentIndex = 0;
            Ccb->SearchExpression = SearchExpression;

            if (!FlagOn(CcbFlags, UDF_CCB_WILDCARD_PRESENT) && SearchExpression.Buffer) {
                Ccb->hashes = SearchHashes;
            }

            SetFlag(Ccb->Flags, CcbFlags | UDF_CCB_ENUM_INITIALIZED);
            *InitialQuery = TRUE;

        } else {

            //
            // Another thread initialized — free our local buffer.
            //

            if (SearchExpression.Buffer) {
                MyFreePool__(SearchExpression.Buffer);
            }
        }

    //
    // Otherwise lock the Fcb so we can read the current enumeration values.
    //

    } else {

        UDFLockFcb(IrpContext, Fcb);
    }

    //
    // Set up the search pattern pointer for the caller.
    //

    if (FlagOn(Ccb->Flags, UDF_CCB_MATCH_ALL)) {
        *PtrSearchPattern = NULL;
    } else {
        *PtrSearchPattern = &Ccb->SearchExpression;
        if (!FlagOn(Ccb->Flags, UDF_CCB_WILDCARD_PRESENT)) {
            *CurHashes = &Ccb->hashes;
        }
    }

    //
    // Determine the starting position.
    //

    if (FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED)) {
        *NextMatch = IrpSp->Parameters.QueryDirectory.FileIndex;
        *ReturnNextEntry = FALSE;
    } else if (FlagOn(IrpSp->Flags, SL_RESTART_SCAN)) {
        *NextMatch = 0;
        *ReturnNextEntry = FALSE;
    } else {
        *NextMatch = Ccb->CurrentIndex;
        *ReturnNextEntry = BooleanFlagOn(Ccb->Flags, UDF_CCB_ENUM_RETURN_NEXT);
    }

    //
    // Unlock the Fcb.
    //

    UDFUnlockFcb(IrpContext, Fcb);

    return STATUS_SUCCESS;
} // end UDFInitializeEnumeration()


/*************************************************************************
*
* Function: UDFQueryDirectory()
*
* Description:
*   Query directory request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFQueryDirectory(
    PIRP_CONTEXT IrpContext,
    PIRP                        Irp,
    PIO_STACK_LOCATION          IrpSp,
    PFILE_OBJECT                FileObject,
    PFCB                        Fcb,
    PCCB                        Ccb
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Information = 0;

    ULONG LastEntry = 0;
    ULONG NextEntry = 0;

    PVCB Vcb = NULL;
    PUNICODE_STRING             PtrSearchPattern;
    FILE_INFORMATION_CLASS      FileInformationClass = IrpSp->Parameters.QueryDirectory.FileInformationClass;
    BOOLEAN                     ReturnSingleEntry = FALSE;
    PUCHAR                      UserBuffer = NULL;
    BOOLEAN                     FirstTimeQuery = FALSE;
    LONG                        NextMatch = 0;
    ULONG                       BaseLength;
    ULONG                       FileNameBytes;
    BOOLEAN                     ReturnNextEntry = FALSE;
    PUDF_FILE_INFO              DirFileInfo = NULL;
    PDIR_INDEX_HDR              hDirIndex = NULL;
    PFILE_BOTH_DIR_INFORMATION  DirInformation = NULL;      // Returned from udf_info module
    PFILE_BOTH_DIR_INFORMATION  BothDirInformation = NULL;  // Pointer in callers buffer
    PFILE_NAMES_INFORMATION     NamesInfo;
    PFILE_ID_BOTH_DIR_INFORMATION IdBothDirInfo = NULL;
    PFILE_ID_FULL_DIR_INFORMATION IdFullDirInfo = NULL;
    ULONG                       BytesRemainingInBuffer;
    PHASH_ENTRY                 cur_hashes = NULL;
    PDIR_INDEX_ITEM             DirNdx;
    FILE_ID                     EntryFileId;

    Vcb = Fcb->Vcb;
    ASSERT_VCB(Vcb);

    // Check if we support this search mode.  Also remember the size of the base part of
    // each of these structures.

    switch (FileInformationClass) {

    case FileDirectoryInformation:
        BaseLength = FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName[0]);
        break;
    case FileFullDirectoryInformation:
        BaseLength = FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName[0]);
        break;
    case FileNamesInformation:
        BaseLength = FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName[0]);
        break;
    case FileBothDirectoryInformation:
        BaseLength = FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName[0]);
        break;
    case FileIdBothDirectoryInformation:
        BaseLength = FIELD_OFFSET(FILE_ID_BOTH_DIR_INFORMATION, FileName[0]);
        break;
    case FileIdFullDirectoryInformation:
        BaseLength = FIELD_OFFSET(FILE_ID_FULL_DIR_INFORMATION, FileName[0]);
        break;
    default:

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_INFO_CLASS);
        return STATUS_INVALID_INFO_CLASS;
    }

    // Get the user buffer.

    UserBuffer = (PUCHAR)UDFMapUserBuffer(Irp);

    // Acquire the directory.

    UDFAcquireFcbShared(IrpContext, Fcb, FALSE);

    _SEH2_TRY
    {
        // Verify the Fcb is still good.

        UDFVerifyFcbOperation(IrpContext, Fcb, Ccb);

        DirFileInfo = Fcb->FileInfo;

        // Start by getting the initial state for the enumeration.  This will set up the Ccb with
        // the initial search parameters and let us know the starting offset in the directory
        // to search.

        Status = UDFInitializeEnumeration(IrpContext,
                                          IrpSp,
                                          Vcb,
                                          Fcb,
                                          Ccb,
                                          &PtrSearchPattern,
                                          &cur_hashes,
                                          &NextMatch,
                                          &ReturnNextEntry,
                                          &ReturnSingleEntry,
                                          &FirstTimeQuery);

        if (!NT_SUCCESS(Status)) {

            try_return(Status);
        }

        // This is an additional verifying

        if (!UDFIsADirectory(DirFileInfo)) {

            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        hDirIndex = DirFileInfo->Dloc->DirIndex;

        if (!hDirIndex) {

            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        Status = STATUS_SUCCESS;

        // Allocate buffer enough to save both DirInformation and FileName
        DirInformation = (PFILE_BOTH_DIR_INFORMATION)MyAllocatePool__(NonPagedPool,
                            sizeof(FILE_BOTH_DIR_INFORMATION)+((ULONG)UDF_NAME_LEN*sizeof(WCHAR)) );

        if (!DirInformation) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        NextEntry = 0;
        BytesRemainingInBuffer = IrpSp->Parameters.QueryDirectory.Length;
        RtlZeroMemory(UserBuffer, BytesRemainingInBuffer);

        if ((!FirstTimeQuery) && !UDFDirIndex(hDirIndex, (uint_di)NextMatch) ) {
            try_return( Status = STATUS_NO_MORE_FILES);
        }

        // One final note though:
        // If we do not find a directory entry OR while searching we reach the
        // end of the directory, then the return code should be set as follows:

        // (a) If any files have been returned (i.e. ReturnSingleEntry was FALSE
        //       and we did find at least one match), then return STATUS_SUCCESS
        // (b) If no entry is being returned then:
        //       (i) If this is the first query i.e. FirstTimeQuery is TRUE
        //            then return STATUS_NO_SUCH_FILE
        //       (ii) Otherwise, return STATUS_NO_MORE_FILES

        while(TRUE) {

            // If the user had requested only a single match and we have
            // returned that, then we stop at this point.
            if ((NextEntry != 0) && ReturnSingleEntry) {
                try_return(Status);
            }

            // Advance past the previous match if we returned it.

            if (ReturnNextEntry) {
                NextMatch++;
            }

            if ((NextMatch < 2)
                && (Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY)) {
                    NextMatch = 2;
            }

            // We call UDFFindNextMatch to look down the next matching dirent.

            Status = UDFFindNextMatch(Vcb, hDirIndex,&NextMatch,PtrSearchPattern, Ccb->Flags, cur_hashes, &DirNdx);

            // If we didn't receive next match, then we are at the end of the
            // directory.  If we have returned any files, we exit with
            // success, otherwise we return STATUS_NO_MORE_FILES.
            if (!NT_SUCCESS(Status)) {
                Status = (NextEntry != 0) ? STATUS_SUCCESS :
                                      (FirstTimeQuery ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES);
                try_return(Status);
            }

            Status = UDFFileDirInfoToNT(IrpContext, Vcb, DirNdx, DirInformation);

            if (!NT_SUCCESS(Status)) {

                // If we already have entries in the buffer, return them and
                // raise the error on the next call.  Otherwise propagate now.
                if (NextEntry != 0) {
                    ReturnNextEntry = FALSE;
                    Status = STATUS_SUCCESS;
                    try_return(Status);
                }
                try_return(Status);
            }

            if (FileInformationClass == FileIdBothDirectoryInformation ||
                FileInformationClass == FileIdFullDirectoryInformation) {
                uint32 PhysLba = UDFPartLbaToPhys(Vcb, &DirNdx->FileEntryLoc);
                uint32 PartitionRoot = UDFPartStart(Vcb, -2);

                if (PhysLba == LBA_OUT_OF_EXTENT || PhysLba < PartitionRoot) {
                    if (NextEntry != 0) {
                        ReturnNextEntry = FALSE;
                        try_return(Status = STATUS_SUCCESS);
                    }
                    try_return(Status = STATUS_FILE_CORRUPT_ERROR);
                }

                /* The cached DirNdx->FileInfo may be absent.  The FID's ICB
                 * address still identifies this entry's FE, including after
                 * a cold remount.  Match UDFFirstOpenFile's directory bit. */
                EntryFileId = UDFMakeLargeInteger(
                    ((LONGLONG)(PhysLba - PartitionRoot)) +
                    ((LONGLONG)((UINT_PTR)Vcb) << 32));
                if (DirInformation->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    SetFlag(EntryFileId.HighPart, FID_DIR_MASK);
            }
            DirInformation->FileIndex = NextMatch;
            FileNameBytes = DirInformation->FileNameLength;

            // If the slot for the next entry would be beyond the length of the
            // user's buffer, just exit (we know we've returned at least one entry
            // already). This can happen when we quad-align the pointer past the end.

            if (NextEntry > IrpSp->Parameters.QueryDirectory.Length) {

                ReturnNextEntry = FALSE;
                try_return(Status = STATUS_SUCCESS);
            }

            // Compute the number of bytes remaining in the buffer. Round this
            // down to a WCHAR boundary so we can copy full characters.

            BytesRemainingInBuffer = IrpSp->Parameters.QueryDirectory.Length - NextEntry;
            ClearFlag(BytesRemainingInBuffer, 1);

            // If this won't fit and we have returned a previous entry then just
            // return STATUS_SUCCESS.

            if ((BaseLength + FileNameBytes) > BytesRemainingInBuffer) {

                // If we already found an entry then just exit.

                if (NextEntry != 0) {

                    ReturnNextEntry = FALSE;
                    try_return(Status = STATUS_SUCCESS);
                }

                // Reduce the FileNameBytes to just fit in the buffer.

                FileNameBytes = BytesRemainingInBuffer - BaseLength;
                ReturnSingleEntry = TRUE;
                Status = STATUS_BUFFER_OVERFLOW;
            }
            //  Protect access to the user buffer with an exception handler.
            //  Since (at our request) IO doesn't buffer these requests, we have
            //  to guard against a user messing with the page protection and other
            //  such trickery.

            _SEH2_TRY
            {
                //  Now we have an entry to return to our caller.
                //  We'll case on the type of information requested and fill up
                //  the user buffer if everything fits.
                switch (FileInformationClass) {

                case FileBothDirectoryInformation:
                case FileFullDirectoryInformation:
                case FileIdBothDirectoryInformation:
                case FileIdFullDirectoryInformation:
                case FileDirectoryInformation:

                    BothDirInformation = (PFILE_BOTH_DIR_INFORMATION)(UserBuffer + NextEntry);
                    RtlCopyMemory(BothDirInformation,DirInformation,BaseLength);
                    BothDirInformation->FileIndex = NextMatch;
                    BothDirInformation->FileNameLength = FileNameBytes;
                    break;

                case FileNamesInformation:

                    NamesInfo = (PFILE_NAMES_INFORMATION)(UserBuffer + NextEntry);
                    NamesInfo->FileIndex = NextMatch;
                    NamesInfo->FileNameLength = FileNameBytes;
                    break;

                default:
                    break;
                }

                switch (FileInformationClass) {

                case FileIdBothDirectoryInformation:
                    IdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)(UserBuffer + NextEntry);
                    IdBothDirInfo->FileId = EntryFileId;
                    break;

                case FileIdFullDirectoryInformation:
                    IdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)(UserBuffer + NextEntry);
                    IdFullDirInfo->FileId = EntryFileId;
                    break;

                default:
                    break;
                }

                if (FileNameBytes) {
                    //  This is a Unicode name, we can copy the bytes directly.
                    RtlCopyMemory( (PVOID)(UserBuffer + NextEntry + BaseLength),
                                   DirInformation->FileName, FileNameBytes );
                }

                Information = NextEntry + BaseLength + FileNameBytes;

                //  ((..._INFORMATION)(PointerToPreviousEntryInBuffer))->NextEntryOffset = NextEntry - LastEntry;
                *((PULONG)(UserBuffer+LastEntry)) = NextEntry - LastEntry;
                //  Set up our variables for the next dirent.
                FirstTimeQuery = FALSE;

                LastEntry    = NextEntry;
                ReturnNextEntry = TRUE;
                NextEntry = UDFQuadAlign(Information);
            }
            _SEH2_EXCEPT(!FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                          EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                //  We had a problem filling in the user's buffer, so stop
                //  and fail this request.
                Information = 0;
                try_return(Status = _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }

try_exit:   NOTHING;


    } _SEH2_FINALLY {

        if (!_SEH2_AbnormalTermination() && !NT_ERROR(Status)) {

            // Update the CCB to show the current state of the enumeration.

            UDFLockFcb(IrpContext, Fcb);

            Ccb->CurrentIndex = NextMatch;

            ClearFlag(Ccb->Flags, UDF_CCB_ENUM_RETURN_NEXT);

            if (ReturnNextEntry) {

                SetFlag(Ccb->Flags, UDF_CCB_ENUM_RETURN_NEXT);
            }

            UDFUnlockFcb(IrpContext, Fcb);
        }

        if (DirInformation) MyFreePool__(DirInformation);

        UDFReleaseFcb(IrpContext, Fcb);
    } _SEH2_END;

    Irp->IoStatus.Information = Information;

    UDFCompleteRequest(IrpContext, Irp, Status);

    return Status;
} // end UDFQueryDirectory()

/*
  Return: STATUS_NO_SUCH_FILE if no more files found
*/
NTSTATUS
UDFFindNextMatch(
    IN PVCB Vcb,
    IN PDIR_INDEX_HDR  hDirIndex,
    IN PLONG           CurrentNumber,      // Must be modified in case, when we found next match
    IN PUNICODE_STRING SearchPattern,
    IN ULONG           CcbFlags,
    IN PHASH_ENTRY     hashes,
   OUT PDIR_INDEX_ITEM* _DirNdx
    )
{
    LONG    EntryNumber = (*CurrentNumber);
    PDIR_INDEX_ITEM DirNdx;
    for(;(DirNdx = UDFDirIndex(hDirIndex, EntryNumber));EntryNumber++) {

        if (!DirNdx->FName.Buffer ||
            UDFIsDeleted(DirNdx)) {

            continue;
        }

        if (hashes &&
           (DirNdx->hashes.hLfn != hashes->hLfn) &&
           (DirNdx->hashes.hPosix != hashes->hPosix) &&
           (!FlagOn(CcbFlags, UDF_CCB_CAN_BE_8_DOT_3) || ((DirNdx->hashes.hDos != hashes->hLfn) && (DirNdx->hashes.hDos != hashes->hPosix))) ) {

            continue;
        }

        if (UDFIsNameInExpression(Vcb,
                                  &DirNdx->FName,
                                  SearchPattern,
                                  NULL,
                                  BooleanFlagOn(CcbFlags, CCB_FLAG_IGNORE_CASE),
                                  BooleanFlagOn(CcbFlags, UDF_CCB_WILDCARD_PRESENT),
                                  BooleanFlagOn(CcbFlags, UDF_CCB_CAN_BE_8_DOT_3) && !(DirNdx->FI_Flags & UDF_FI_FLAG_DOS),
                                  EntryNumber < 2, DirNdx) && !(DirNdx->FI_Flags & UDF_FI_FLAG_FI_INTERNAL)) {

            break;
        }
    }

    if (DirNdx) {
        // Modify CurrentNumber to appropriate value
        *CurrentNumber = EntryNumber;
        *_DirNdx = DirNdx;
        return STATUS_SUCCESS;
    } else {
        // Do not modify CurrentNumber because we have not found next match entry
        return STATUS_NO_MORE_FILES;
    }
} // end UDFFindNextMatch()

/*************************************************************************
*
* Function: UDFNotifyChangeDirectory()
*
* Description:
*   Handle the notify request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFNotifyChangeDirectory(
    PIRP_CONTEXT IrpContext,
    PIRP                        Irp,
    PIO_STACK_LOCATION          IrpSp,
    PFILE_OBJECT                FileObject,
    PFCB                        Fcb,
    PCCB                        Ccb
    )
{
    // Always set the wait bit in the IrpContext so the initial wait can't fail.

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    // Acquire the Vcb shared.

    UDFAcquireVcbShared(IrpContext, IrpContext->Vcb, FALSE);

    _SEH2_TRY {

        // Verify the Vcb.

        UDFVerifyVcb(IrpContext, IrpContext->Vcb);

        // Call the Fsrtl package to process the request.  We cast the
        // unicode strings to ansi strings as the dir notify package
        // only deals with memory matching.

        FsRtlNotifyFullChangeDirectory(IrpContext->Vcb->NotifySync,
                                       &IrpContext->Vcb->NextNotifyIRP,
                                       (PVOID)Ccb,
                                       (PSTRING)&FileObject->FileName,
                                       BooleanFlagOn(IrpSp->Flags, SL_WATCH_TREE),
                                       FALSE,
                                       IrpSp->Parameters.NotifyDirectory.CompletionFilter,
                                       Irp,
                                       NULL,
                                       NULL);

    } _SEH2_FINALLY {

        // Release the Vcb.

        UDFReleaseVcb(IrpContext, IrpContext->Vcb);

    } _SEH2_END;

    //  Cleanup the IrpContext.
    UDFCompleteRequest(IrpContext, NULL, STATUS_SUCCESS);

    return STATUS_PENDING;
} // end UDFNotifyChangeDirectory()

VOID
UDFNotifyReportChange(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PFCB Fcb,
    ULONG Filter,
    ULONG Action,
    PLCB Lcb,
    PFILE_OBJECT FileObject
    )
{
    USHORT TargetNameOffset = 0;
    UNICODE_STRING FullPath;
    BOOLEAN PathAllocated = FALSE;
    WCHAR RootChar;

    FullPath.Buffer = NULL;
    FullPath.Length = 0;
    FullPath.MaximumLength = 0;

    //
    // Acquire FcbResource shared for the duration of the notify.
    // Protects LCB chain and name buffers from concurrent teardown.
    //
    UDFAcquireFcbShared(IrpContext, Fcb, FALSE);

    _SEH2_TRY {

        // If no LCB provided, find first valid (non-deleted) one
        if (!Lcb) {
            if (!IsListEmpty(&Fcb->ParentLcbQueue)) {
                PLIST_ENTRY ListEntry = Fcb->ParentLcbQueue.Flink;
                while (ListEntry != &Fcb->ParentLcbQueue) {
                    PLCB CandidateLcb = CONTAINING_RECORD(ListEntry, struct LCB, ChildFcbLinks);
                    if (!(CandidateLcb->Flags & UDF_LCB_FLAG_LINK_DELETED)) {
                        Lcb = CandidateLcb;
                        break;
                    }
                    ListEntry = ListEntry->Flink;
                }
            }
        }

        //
        // Build the full target name in the same namespace the registered
        // notify watches were keyed under. Prefer FileObject->FileName (the
        // absolute path from the volume root, stable and independent of LCB
        // chain state) unless the open cannot be trusted to carry one:
        //   - a relative open holds only the final component, no parent path
        //     (recorded as CCB_FLAG_HAS_RELATED_CCB on the CCB), and
        //   - a link with a generated short name is ambiguous.
        // For those, build the absolute path from the LCB chain instead.
        //
        // FsContext2 carries the type-of-open in its low bits; UDFDecodeFileObject
        // strips them and returns the real CCB (NULL for an unopened object).
        //
        PCCB Ccb = NULL;
        if (FileObject != NULL) {
            PFCB DecodedFcb = NULL;
            UDFDecodeFileObject(FileObject, &DecodedFcb, &Ccb);
        }
        BOOLEAN RelativeOpen = (Ccb != NULL &&
                                Ccb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_CCB &&
                                FlagOn(Ccb->Flags, CCB_FLAG_HAS_RELATED_CCB));
        BOOLEAN ShortNameLink = (Lcb != NULL && FlagOn(Lcb->Flags, UDF_LCB_FLAG_SHORT_NAME_CREATED));

        if (FileObject && FileObject->FileName.Buffer &&
            FileObject->FileName.Length > 0 &&
            !RelativeOpen && !ShortNameLink) {
            FullPath = FileObject->FileName;
        } else if (Lcb) {
            NTSTATUS Status = UDFBuildFullPathFromLcb(NULL, Lcb, &FullPath, FALSE);
            if (NT_SUCCESS(Status)) {
                PathAllocated = TRUE;
            }
        }

        // Fallback to root
        if (!FullPath.Buffer || FullPath.Length == 0) {
            RootChar = L'\\';
            FullPath.Buffer = &RootChar;
            FullPath.Length = sizeof(WCHAR);
            FullPath.MaximumLength = sizeof(WCHAR);
            PathAllocated = FALSE;
            Lcb = NULL;
        }

        // TargetNameOffset: byte offset within FullPath to the final name component
        if (Lcb && Lcb->ExactCaseLinkName.Length > 0 &&
            FullPath.Length > Lcb->ExactCaseLinkName.Length) {
            TargetNameOffset = FullPath.Length - Lcb->ExactCaseLinkName.Length;
        }

        FsRtlNotifyFullReportChange(Vcb->NotifySync,
                                    &Vcb->NextNotifyIRP,
                                    (PSTRING)&FullPath,
                                    TargetNameOffset,
                                    NULL,
                                    NULL,
                                    Filter,
                                    Action,
                                    NULL);

    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    } _SEH2_END;

    UDFReleaseFcb(IrpContext, Fcb);

    if (PathAllocated && FullPath.Buffer) {
        ExFreePool(FullPath.Buffer);
    }
}
