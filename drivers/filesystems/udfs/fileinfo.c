////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Fileinfo.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "set/query file information" dispatch
*   entry points.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_INFORMATION

#define         MEM_USREN_TAG                   "US_Ren"
#define         MEM_USREN2_TAG                  "US_Ren2"
#define         MEM_USFIDC_TAG                  "US_FIDC"
#define         MEM_USHL_TAG                    "US_HL"

/*************************************************************************
*
* Function: UDFCommonQueryInfo()
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
UDFCommonQueryInfo(
    PIRP_CONTEXT IrpContext,
    PIRP             Irp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    LONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;
    BOOLEAN ReleaseFcb = FALSE;

    PAGED_CODE();

    // Reference our input parameters to make things easier

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    // Decode the file object

    TypeOfOpen = UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    // An unopened or stream file object has no user CCB.  Reject it before
    // the debug assertions dereference the decoded pointers.
    if ((TypeOfOpen != UserDirectoryOpen && TypeOfOpen != UserFileOpen) ||
        Fcb == NULL || Ccb == NULL) {
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);

    _SEH2_TRY {

        // We only support query on file and directory handles.

        switch (TypeOfOpen) {

        case UserDirectoryOpen:
        case UserFileOpen:

            if (FileInformationClass == FileStreamInformation) {
                UDFAcquireFcbExclusive(IrpContext, Fcb, FALSE);
            } else {
                UDFAcquireFcbShared(IrpContext, Fcb, FALSE);
            }
            ReleaseFcb = TRUE;

            // Make sure the Fcb is in a usable condition.  This will raise
            // an error condition if the volume is unusable

            UDFVerifyFcbOperation(IrpContext, Fcb, Ccb);

            // Do whatever the caller asked us to do
            switch (FileInformationClass) {
            case FileBasicInformation:
                Status = UDFGetBasicInformation(IrpSp->FileObject, Fcb, (PFILE_BASIC_INFORMATION)Buffer, &Length);
                break;
            case FileStandardInformation:
                Status = UDFGetStandardInformation(Fcb, (PFILE_STANDARD_INFORMATION)Buffer, &Length);
                break;
            case FileNetworkOpenInformation:
                Status = UDFGetNetworkInformation(Fcb, (PFILE_NETWORK_OPEN_INFORMATION)Buffer, &Length);
                break;
            case FileInternalInformation:
                Status = UDFGetInternalInformation(IrpContext, Fcb, (PFILE_INTERNAL_INFORMATION)Buffer, &Length);
                break;
            case FileEaInformation:
                Status = UDFGetEaInformation(IrpContext, Fcb, (PFILE_EA_INFORMATION)Buffer, &Length);
                break;
            case FileNameInformation:

                // We don't allow this operation on a file opened by file Id.

                if (FlagOn(Ccb->Flags, CCB_FLAG_OPEN_BY_ID)) {

                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                Status = UDFGetFullNameInformation(IrpSp->FileObject, (PFILE_NAME_INFORMATION)Buffer, &Length);
                break;
            case FileAlternateNameInformation:
                Status = UDFGetAltNameInformation(Fcb, (PFILE_NAME_INFORMATION)Buffer, &Length);
                break;
            //TODO: impl
    //            case FileCompressionInformation:
    //                // Status = UDFGetCompressionInformation(...);
    //                break;
            case FilePositionInformation:
                Status = UDFGetPositionInformation(IrpSp->FileObject, (PFILE_POSITION_INFORMATION)Buffer, &Length);
                break;
            case FileStreamInformation:
                Status = UDFGetFileStreamInformation(IrpContext, Fcb, (PFILE_STREAM_INFORMATION)Buffer, (PULONG)&Length);
                break;
            case FileAllInformation:
                {
                    // We don't allow this operation on a file opened by file Id.

                    if (FlagOn(Ccb->Flags, CCB_FLAG_OPEN_BY_ID)) {

                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    PFILE_ALL_INFORMATION PtrAllInfo = (PFILE_ALL_INFORMATION)Buffer;

                    Length -= (sizeof(FILE_MODE_INFORMATION) +
                               sizeof(FILE_ACCESS_INFORMATION) +
                               sizeof(FILE_ALIGNMENT_INFORMATION));

                    // Get the remaining stuff.
                    if (!NT_SUCCESS(Status = UDFGetBasicInformation(IrpSp->FileObject, Fcb, &(PtrAllInfo->BasicInformation), &Length)) ||
                        !NT_SUCCESS(Status = UDFGetStandardInformation(Fcb, &(PtrAllInfo->StandardInformation), &Length)) ||
                        !NT_SUCCESS(Status = UDFGetInternalInformation(IrpContext, Fcb, &(PtrAllInfo->InternalInformation), &Length)) ||
                        !NT_SUCCESS(Status = UDFGetEaInformation(IrpContext, Fcb, &(PtrAllInfo->EaInformation), &Length)) ||
                        !NT_SUCCESS(Status = UDFGetPositionInformation(IrpSp->FileObject, &(PtrAllInfo->PositionInformation), &Length)) ||
                        !NT_SUCCESS(Status = UDFGetFullNameInformation(IrpSp->FileObject, &(PtrAllInfo->NameInformation), &Length))
                        )
                        break;
                }

                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                try_return(Status);
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
        }

        // Set the information field to the number of bytes actually filled in
        // and then complete the request

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if (ReleaseFcb) {

            UDFReleaseFcb(IrpContext, Fcb);
        }

    } _SEH2_END;// end of "__finally" processing

    // Complete the request if we didn't raise.

    UDFCompleteRequest(IrpContext, Irp, Status);

    return Status;
} // end UDFCommonQueryInfo()

/*************************************************************************
*
* Function: UDFCommonSetInfo()
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
UDFCommonSetInfo(
    PIRP_CONTEXT IrpContext,
    PIRP             Irp
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp = NULL;
    PFILE_OBJECT            FileObject = NULL;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB                    Fcb = NULL;
    PCCB                    Ccb = NULL;
    PVCB                    Vcb = NULL;
    BOOLEAN                 MainResourceAcquired = FALSE;
    PVOID                   Buffer = NULL;
    FILE_INFORMATION_CLASS  FunctionalityRequested;
    BOOLEAN                 CanWait = FALSE;
    BOOLEAN                 PostRequest = FALSE;
    BOOLEAN                 VcbAcquired = FALSE;

    TmPrint(("UDFCommonSetInfo: irp %x\n", Irp));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FileObject = IrpSp->FileObject;

    // Decode the file object

    TypeOfOpen = UDFDecodeFileObject(FileObject, &Fcb, &Ccb);

    if ((TypeOfOpen != UserFileOpen && TypeOfOpen != UserDirectoryOpen) ||
        Fcb == NULL || Ccb == NULL) {
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    _SEH2_TRY {

        // Case on the type of open we're dealing with

        switch (TypeOfOpen) {

        case UserVolumeOpen:

            // We cannot query the user volume open.

            try_return(Status = STATUS_INVALID_PARAMETER);
            break;
        case UserFileOpen:

            break;

        case UserDirectoryOpen:

            break;

        default:

            try_return(Status = STATUS_INVALID_PARAMETER);
        }


        CanWait = (IrpContext->Flags & IRP_CONTEXT_FLAG_WAIT) ? TRUE : FALSE;

        // The NT I/O Manager always allocates and supplies a system
        // buffer for query and set file information calls.
        // Copying information to/from the user buffer and the system
        // buffer is performed by the I/O Manager and the FSD need not worry about it.
        Buffer = Irp->AssociatedIrp.SystemBuffer;

        // Now, obtain some parameters.
        FunctionalityRequested = IrpSp->Parameters.SetFile.FileInformationClass;
        if ((Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) &&
            (FunctionalityRequested != FilePositionInformation)) {
            try_return(Status = STATUS_ACCESS_DENIED);
        }

        if (FunctionalityRequested == FileRenameInformation ||
            FunctionalityRequested == FileLinkInformation) {

            UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

            VcbAcquired = TRUE;
        }

        //
        // Acquire FcbResource exclusive for all operations except
        // rename/link (those acquire resources in their own functions).
        //
        if ((FunctionalityRequested != FileRenameInformation) &&
            (FunctionalityRequested != FileLinkInformation)) {

            UDFAcquireFcbExclusive(IrpContext, Fcb, FALSE);
            MainResourceAcquired = TRUE;
        }

        /* Drain an owned cached generation before namespace or allocation
         * changes. Rename/link have not acquired their ordered parent/child
         * locks yet; EOF/allocation have not acquired PagingIoResource yet.
         * Basic timestamps/attributes retain ordinary cached semantics. */
        if (FunctionalityRequested == FileRenameInformation ||
            FunctionalityRequested == FileLinkInformation ||
            FunctionalityRequested == FileAllocationInformation ||
            FunctionalityRequested == FileEndOfFileInformation) {
            Status = UDFDrainNewFileData(IrpContext, Vcb, Fcb->FileInfo);
            if (!NT_SUCCESS(Status))
                try_return(Status);
        }

        // Do whatever the caller asked us to do
        switch (FunctionalityRequested) {
        case FileBasicInformation:
            Status = UDFSetBasicInformation(IrpContext, Fcb, Ccb, FileObject, (PFILE_BASIC_INFORMATION)Buffer);
            break;
        case FilePositionInformation: {
            // Check if no intermediate buffering has been specified.
            // If it was specified, do not allow non-aligned set file
            // position requests to succeed.
            PFILE_POSITION_INFORMATION       PtrFileInfoBuffer;

            PtrFileInfoBuffer = (PFILE_POSITION_INFORMATION)Buffer;

            if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
                if (PtrFileInfoBuffer->CurrentByteOffset.LowPart & IrpSp->DeviceObject->AlignmentRequirement) {
                    // Invalid alignment.
                    try_return(Status = STATUS_INVALID_PARAMETER);
                }
            }

            FileObject->CurrentByteOffset = PtrFileInfoBuffer->CurrentByteOffset;
            break;
        }
        case FileDispositionInformation:
            Status = UDFSetDispositionInfo(IrpContext, FileObject, Fcb, Ccb, (PFILE_DISPOSITION_INFORMATION)Buffer);
            break;
        case FileRenameInformation:
            if (!CanWait) {
                PostRequest = TRUE;
                try_return(Status = STATUS_PENDING);
            }
            Status = UDFSetRenameInfo(IrpContext, Fcb, Ccb, FileObject, (PFILE_RENAME_INFORMATION)Buffer);
            if (Status == STATUS_PENDING) {
                PostRequest = TRUE;
                try_return(Status);
            }
            break;
#ifdef UDF_ALLOW_HARD_LINKS
        case FileLinkInformation:
            if (!CanWait) {
                PostRequest = TRUE;
                try_return(Status = STATUS_PENDING);
            }
            Status = UDFHardLink(IrpContext, IrpSp, Fcb, Ccb, FileObject, (PFILE_LINK_INFORMATION)Buffer);
            break;
#endif //UDF_ALLOW_HARD_LINKS
        case FileAllocationInformation:
            Status = UDFSetAllocationInfo(Fcb, Ccb, Vcb, FileObject, IrpContext, Irp, (PFILE_ALLOCATION_INFORMATION)Buffer);
            break;
        case FileEndOfFileInformation:
            Status = UDFSetEndOfFileInfo(IrpContext, IrpSp, Fcb, Ccb, Vcb, FileObject, Irp, (PFILE_END_OF_FILE_INFORMATION)Buffer);
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            try_return(Status);
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if (MainResourceAcquired) {
            UDFReleaseFcb(IrpContext, Fcb);
            MainResourceAcquired = FALSE;
        }

        if (VcbAcquired) {

            UDFReleaseVcb(IrpContext, Vcb);
            VcbAcquired = FALSE;
        }

        // Post IRP if required
        if (PostRequest) {

            // Since, the I/O Manager gave us a system buffer, we do not
            // need to "lock" anything.

            // Perform the post operation which will mark the IRP pending
            // and will return STATUS_PENDING back to us
            Status = UDFFsdPostRequest(IrpContext, Irp);

        } else {

            if (!_SEH2_AbnormalTermination()) {

                if (NT_SUCCESS(Status)) {

                    if (FunctionalityRequested == FileDispositionInformation) {
                        UDFFspClose(Vcb);
                    }
                }

                UDFCompleteRequest(IrpContext, Irp, Status);
            }
        }
    } _SEH2_END;// end of "__finally" processing

    return Status;
} // end UDFCommonSetInfo()

/*
    Return some time-stamps and file attributes to the caller.
 */
NTSTATUS
UDFGetBasicInformation(
    IN PFILE_OBJECT                FileObject,
    IN PFCB                        Fcb,
    IN PFILE_BASIC_INFORMATION     PtrBuffer,
 IN OUT LONG*                      PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;
    PDIR_INDEX_ITEM     DirNdx;

    AdPrint(("UDFGetBasicInformation: \n"));

    _SEH2_TRY {

        if (*PtrReturnedLength < (LONG)sizeof(FILE_BASIC_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_BASIC_INFORMATION));

        // Get information from the FCB and update TimesCache in DirIndex
        FileInfo = Fcb->FileInfo;

        if (!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! GetBasicInfo to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }

        DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);

        PtrBuffer->CreationTime = Fcb->CreationTime;
        DirNdx->CreationTime = PtrBuffer->CreationTime.QuadPart;

        PtrBuffer->LastAccessTime = Fcb->LastAccessTime;
        DirNdx->LastAccessTime = PtrBuffer->LastAccessTime.QuadPart;

        PtrBuffer->LastWriteTime = Fcb->LastWriteTime;
        DirNdx->LastWriteTime = PtrBuffer->LastWriteTime.QuadPart;

        PtrBuffer->ChangeTime = Fcb->ChangeTime;
        DirNdx->ChangeTime = PtrBuffer->ChangeTime.QuadPart;

        // Now fill in the attributes.
        if (Fcb->FcbState & UDF_FCB_DIRECTORY) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
#ifdef UDF_DBG
            if (!FileInfo->Dloc->DirIndex) AdPrint(("*****!!!!! Directory has no DirIndex !!!!!*****\n"));
#endif
        }
        // Similarly, fill in attributes indicating a hidden file, system
        // file, compressed file, temporary file, etc. if the FSD supports
        // such file attribute values.
        PtrBuffer->FileAttributes |= UDFAttributesToNT(DirNdx,NULL);
        if (FileObject->Flags & FO_TEMPORARY_FILE) {
            PtrBuffer->FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
        } else {
            PtrBuffer->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;
        }
        if (!PtrBuffer->FileAttributes) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if (NT_SUCCESS(RC)) {
            // Return the amount of information filled in.
            (*PtrReturnedLength) -= sizeof(FILE_BASIC_INFORMATION);
        }
    } _SEH2_END;
    return(RC);
} // end UDFGetBasicInformation()


/*
    Return file sizes to the caller.
 */
NTSTATUS
UDFGetStandardInformation(
    IN PFCB                        Fcb,
    IN PFILE_STANDARD_INFORMATION  PtrBuffer,
 IN OUT LONG*                      PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;
//    PVCB Vcb;

    AdPrint(("UDFGetStandardInformation: \n"));

    _SEH2_TRY {

        if (*PtrReturnedLength < (LONG)sizeof(FILE_STANDARD_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_STANDARD_INFORMATION));

        FileInfo = Fcb->FileInfo;

        if (!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! GetStandardInfo to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
//        Vcb = Fcb->Vcb;
        PtrBuffer->NumberOfLinks = UDFGetFileNameLinkCount(FileInfo);
        PtrBuffer->DeletePending = (Fcb->FcbState & UDF_FCB_DELETE_ON_CLOSE) ? TRUE : FALSE;

        //  Case on whether this is a file or a directory, and extract
        //  the information and fill in the fcb/dcb specific parts
        //  of the output buffer
        if (UDFIsADirectory(Fcb->FileInfo)) {
            PtrBuffer->Directory = TRUE;
        } else {
            if (Fcb->Header.AllocationSize.LowPart == 0xffffffff) {
                Fcb->Header.AllocationSize.QuadPart =
                    UDFSysGetAllocSize(Fcb->Vcb, UDFGetFileSize(FileInfo));
            }
            PtrBuffer->AllocationSize = Fcb->Header.AllocationSize;
            PtrBuffer->EndOfFile = Fcb->Header.FileSize;

            PtrBuffer->Directory = FALSE;
        }

        try_exit: NOTHING;
    } _SEH2_FINALLY {
        if (NT_SUCCESS(RC)) {
            // Return the amount of information filled in.
            *PtrReturnedLength -= sizeof(FILE_STANDARD_INFORMATION);
        }
    } _SEH2_END;
    return(RC);
} // end UDFGetStandardInformation()

/*
    Return some time-stamps and file attributes to the caller.
 */
NTSTATUS
UDFGetNetworkInformation(
    IN PFCB                           Fcb,
    IN PFILE_NETWORK_OPEN_INFORMATION PtrBuffer,
 IN OUT PLONG                         PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;

    AdPrint(("UDFGetNetworkInformation: \n"));

    _SEH2_TRY {

        if (*PtrReturnedLength < (LONG)sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_NETWORK_OPEN_INFORMATION));

        // Get information from the FCB.
        PtrBuffer->CreationTime = Fcb->CreationTime;
        PtrBuffer->LastAccessTime = Fcb->LastAccessTime;
        PtrBuffer->LastWriteTime = Fcb->LastWriteTime;
        PtrBuffer->ChangeTime = Fcb->ChangeTime;

        FileInfo = Fcb->FileInfo;

        if (!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! UDFGetNetworkInformation to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        // Now fill in the attributes.
        if (Fcb->FcbState & UDF_FCB_DIRECTORY) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
#ifdef UDF_DBG
            if (!FileInfo->Dloc->DirIndex) AdPrint(("*****!!!!! Directory has no DirIndex !!!!!*****\n"));
#endif
        } else {
            if (Fcb->Header.AllocationSize.LowPart == 0xffffffff) {
                Fcb->Header.AllocationSize.QuadPart =
                    UDFSysGetAllocSize(Fcb->Vcb, UDFGetFileSize(FileInfo));
            }
            PtrBuffer->AllocationSize = Fcb->Header.AllocationSize;
            PtrBuffer->EndOfFile = Fcb->Header.FileSize;
        }
        // Similarly, fill in attributes indicating a hidden file, system
        // file, compressed file, temporary file, etc. if the FSD supports
        // such file attribute values.
        PtrBuffer->FileAttributes |= UDFAttributesToNT(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index),NULL);
        if (!PtrBuffer->FileAttributes) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (NT_SUCCESS(RC)) {
            // Return the amount of information filled in.
            (*PtrReturnedLength) -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
        }
    } _SEH2_END;
    return(RC);
} // end UDFGetNetworkInformation()


/*
    Return some time-stamps and file attributes to the caller.
 */
NTSTATUS
UDFGetInternalInformation(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _Out_ PFILE_INTERNAL_INFORMATION Buffer,
    _Inout_ PLONG Length
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(IrpContext);

    AdPrint(("UDFGetInternalInformation\n"));

    if (*Length < (LONG)sizeof(FILE_INTERNAL_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    // Index number is the file Id number in the Fcb.

    Buffer->IndexNumber = Fcb->FileId;

    *Length -= sizeof(FILE_INTERNAL_INFORMATION);

    return Status;
} // end UDFGetInternalInformation()

/*
    Return zero-filled EAs to the caller.
 */
NTSTATUS
UDFGetEaInformation(
    PIRP_CONTEXT IrpContext,
    IN PFCB                 Fcb,
    IN PFILE_EA_INFORMATION PtrBuffer,
 IN OUT PLONG               PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;

    AdPrint(("UDFGetEaInformation\n"));

    _SEH2_TRY {

        if (*PtrReturnedLength < (LONG)sizeof(FILE_EA_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        PtrBuffer->EaSize = 0;

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (NT_SUCCESS(RC)) {
            // Return the amount of information filled in.
            *PtrReturnedLength -= sizeof(FILE_EA_INFORMATION);
        }
    } _SEH2_END;
    return(RC);
} // end UDFGetEaInformation()

/*
    Return file's long name to the caller.
 */
NTSTATUS
UDFGetFullNameInformation(
    IN PFILE_OBJECT                FileObject,
    IN PFILE_NAME_INFORMATION      PtrBuffer,
 IN OUT PLONG                      PtrReturnedLength
    )
{
    ULONG BytesToCopy;

    AdPrint(("UDFGetFullNameInformation\n"));

    /* If buffer can't hold at least the file name length, bail out */
    if (*PtrReturnedLength < (LONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
        return STATUS_BUFFER_OVERFLOW;

    /* Save file name length, and as much file len, as buffer length allows */
    PtrBuffer->FileNameLength = FileObject->FileName.Length;

    /* Calculate amount of bytes to copy not to overflow the buffer */
    BytesToCopy = min(FileObject->FileName.Length,
                      *PtrReturnedLength - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]));

    /* Fill in the bytes */
    RtlCopyMemory(PtrBuffer->FileName, FileObject->FileName.Buffer, BytesToCopy);

    /* Check if we could write more but are not able to */
    if (*PtrReturnedLength < (LONG)FileObject->FileName.Length + (LONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
    {
        /* Return number of bytes written */
        *PtrReturnedLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + BytesToCopy;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* We filled up as many bytes, as needed */
    *PtrReturnedLength -= (FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + FileObject->FileName.Length);

    return STATUS_SUCCESS;
} // end UDFGetFullNameInformation()

/*
    Return file short(8.3) name to the caller.
 */
NTSTATUS
UDFGetAltNameInformation(
    IN PFCB                        Fcb,
    IN PFILE_NAME_INFORMATION      PtrBuffer,
    IN OUT PLONG                   PtrReturnedLength
    )
{
    PDIR_INDEX_ITEM DirNdx;
    ULONG BytesToCopy;
    UNICODE_STRING ShortName;
    WCHAR ShortNameBuffer[13];

    AdPrint(("UDFGetAltNameInformation: \n"));

    *PtrReturnedLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);
    DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index);

    ShortName.MaximumLength = 13 * sizeof(WCHAR);
    ShortName.Buffer = (PWCHAR)&ShortNameBuffer;

    UDFGetDirNdxShortName(Fcb->Vcb, DirNdx, Fcb->FileInfo->Index < 2, &ShortName);

    if (*PtrReturnedLength < ShortName.Length) {
        return(STATUS_BUFFER_OVERFLOW);
    } else {
        BytesToCopy = ShortName.Length;
        *PtrReturnedLength -= ShortName.Length;
    }

    RtlCopyMemory( &(PtrBuffer->FileName),
                   ShortName.Buffer,
                   BytesToCopy );

    PtrBuffer->FileNameLength = ShortName.Length;

    return(STATUS_SUCCESS);
} // end UDFGetAltNameInformation()

/*
    Get file position information
 */
NTSTATUS
UDFGetPositionInformation(
    IN PFILE_OBJECT               FileObject,
    IN PFILE_POSITION_INFORMATION PtrBuffer,
 IN OUT PLONG                     PtrReturnedLength
    )
{
    if (*PtrReturnedLength < (LONG)sizeof(FILE_POSITION_INFORMATION)) {
        return(STATUS_BUFFER_OVERFLOW);
    }
    PtrBuffer->CurrentByteOffset = FileObject->CurrentByteOffset;
    // Modify the local variable for BufferLength appropriately.
    *PtrReturnedLength -= sizeof(FILE_POSITION_INFORMATION);

    return(STATUS_SUCCESS);
} // end UDFGetAltNameInformation()

/*
    Get file file stream(s) information
 */
NTSTATUS
UDFGetFileStreamInformation(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_STREAM_INFORMATION PtrBuffer,
    IN OUT PULONG PtrReturnedLength
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    PUDF_FILE_INFO  FileInfo;
    PUDF_FILE_INFO  SDirInfo = NULL;
    PVCB            Vcb = Fcb->Vcb;

    uint_di         i;
    ULONG CurrentSize;
    ULONG AlignedSize;
    ULONG BufferLength;
    ULONG TotalBytesWritten = 0;
    ULONG PreviousOffset = 0;   // padding of the previous entry (aligned - real)
    PDIR_INDEX_HDR  hSDirIndex;
    PDIR_INDEX_ITEM SDirIndex;
    PDIR_INDEX_ITEM DirNdx;
    PFILE_BOTH_DIR_INFORMATION NTFileInfo = NULL;

    PFILE_STREAM_INFORMATION CurrentInfo = PtrBuffer;
    PFILE_STREAM_INFORMATION Previous = NULL;

    AdPrint(("UDFGetFileStreamInformation\n"));

    DECLARE_CONST_UNICODE_STRING(StreamPrefix, L":");
    DECLARE_CONST_UNICODE_STRING(StreamSuffix, L":$DATA");

    _SEH2_TRY {

        FileInfo = Fcb->FileInfo;
        if (!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! UDFGetFileStreamInformation to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        Vcb = Fcb->Vcb;

        DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);
        ASSERT(DirNdx);

        NTFileInfo = (PFILE_BOTH_DIR_INFORMATION)MyAllocatePool__(NonPagedPool, sizeof(FILE_BOTH_DIR_INFORMATION)+UDF_NAME_LEN*sizeof(WCHAR));
        if (!NTFileInfo) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

        RC = UDFFileDirInfoToNT(IrpContext, Vcb, DirNdx, NTFileInfo);

        if (!NT_SUCCESS(RC)) {
            try_return(RC);
        }

        // Clear output buffer so alignment padding between entries and the final
        // NextEntryOffset are guaranteed zero.
        BufferLength = *PtrReturnedLength;
        RtlZeroMemory(PtrBuffer, BufferLength);

        CurrentSize = FIELD_OFFSET(FILE_STREAM_INFORMATION, StreamName) + StreamPrefix.Length + StreamSuffix.Length;

        if (CurrentSize > BufferLength) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        CurrentInfo->StreamNameLength = StreamPrefix.Length + StreamSuffix.Length;
        CurrentInfo->StreamSize = NTFileInfo->EndOfFile;
        CurrentInfo->StreamAllocationSize = NTFileInfo->AllocationSize;

        RtlCopyMemory(&CurrentInfo->StreamName[0], StreamPrefix.Buffer, StreamPrefix.Length);
        RtlCopyMemory(&CurrentInfo->StreamName[1], StreamSuffix.Buffer, StreamSuffix.Length);

        // Each entry must start on an 8-byte boundary: advance by the aligned
        // size, but count only the real size as written.
        TotalBytesWritten = CurrentSize;
        AlignedSize = UDFQuadAlign(CurrentSize);
        PreviousOffset = AlignedSize - CurrentSize;

        Previous = CurrentInfo;
        CurrentInfo = (PFILE_STREAM_INFORMATION)((ULONG_PTR)CurrentInfo + AlignedSize);

        if (
            UDFIsAStreamDir(FileInfo) ||
            UDFIsAStream(FileInfo) || 
            !UDFHasAStreamDir(FileInfo) ||
            !NT_SUCCESS(UDFOpenStreamDir__(IrpContext, Vcb, FileInfo, &SDirInfo))
        ) {
            if (SDirInfo) {
                UDFCleanUpFile__(Vcb, SDirInfo);

                MyFreePool__(SDirInfo);
                SDirInfo = NULL;
            }

            *PtrReturnedLength = BufferLength - TotalBytesWritten;
            
            try_return(RC = STATUS_SUCCESS);
        }

        hSDirIndex = SDirInfo->Dloc->DirIndex;

        for(i=2; (SDirIndex = UDFDirIndex(hSDirIndex,i)); i++) {
            if ((SDirIndex->FI_Flags & UDF_FI_FLAG_FI_INTERNAL) ||
                UDFIsDeleted(SDirIndex) ||
                !SDirIndex->FName.Buffer )
                continue;

            CurrentSize = FIELD_OFFSET(FILE_STREAM_INFORMATION, StreamName) +
                            StreamPrefix.Length + SDirIndex->FName.Length + StreamSuffix.Length;

            // Need room for the previous entry's alignment padding plus this entry.
            if (PreviousOffset + CurrentSize > BufferLength - TotalBytesWritten) {
                RC = STATUS_BUFFER_OVERFLOW;
                break;
            }

            RC = UDFFileDirInfoToNT(IrpContext, Vcb, SDirIndex, NTFileInfo);

            if (!NT_SUCCESS(RC)) {
                try_return(RC);
            }

            CurrentInfo->StreamNameLength = StreamPrefix.Length + SDirIndex->FName.Length + StreamSuffix.Length;
            CurrentInfo->StreamSize = NTFileInfo->EndOfFile;
            CurrentInfo->StreamAllocationSize = NTFileInfo->AllocationSize;

            RtlCopyMemory(&CurrentInfo->StreamName[0], StreamPrefix.Buffer, StreamPrefix.Length);
            RtlCopyMemory(&CurrentInfo->StreamName[1], SDirIndex->FName.Buffer, SDirIndex->FName.Length);
            RtlCopyMemory(&CurrentInfo->StreamName[1 + SDirIndex->FName.Length / sizeof(WCHAR)], StreamSuffix.Buffer, StreamSuffix.Length);

            // Link the previous entry to this one using the aligned distance.
            Previous->NextEntryOffset = (ULONG)((ULONG_PTR)CurrentInfo - (ULONG_PTR)Previous);

            TotalBytesWritten = (ULONG)((ULONG_PTR)CurrentInfo - (ULONG_PTR)PtrBuffer) + CurrentSize;
            AlignedSize = UDFQuadAlign(CurrentSize);
            PreviousOffset = AlignedSize - CurrentSize;

            Previous = CurrentInfo;
            CurrentInfo = (PFILE_STREAM_INFORMATION)((ULONG_PTR)CurrentInfo + AlignedSize);
        }

        // Returned length is the real bytes used (no trailing padding).
        *PtrReturnedLength = BufferLength - TotalBytesWritten;

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (SDirInfo) {
            UDFCloseFile__(IrpContext, Vcb, SDirInfo);
            if (UDFCleanUpFile__(Vcb, SDirInfo))
                MyFreePool__(SDirInfo);
        }
        if (NTFileInfo)
           MyFreePool__(NTFileInfo);
    } _SEH2_END;
    return(RC);
} // end UDFGetFileStreamInformation()

//*******************************************************************
/*
    Set some time-stamps and file attributes supplied by the caller.
 */
NTSTATUS
UDFSetBasicInformation(
    IN PIRP_CONTEXT                IrpContext,
    IN PFCB                        Fcb,
    IN PCCB                        Ccb,
    IN PFILE_OBJECT                FileObject,
    IN PFILE_BASIC_INFORMATION PtrBuffer)
{
    NTSTATUS        RC = STATUS_SUCCESS;
    ULONG           NotifyFilter = 0;
    PDIR_INDEX_ITEM DirNdx;

    AdPrint(("UDFSetBasicInformation\n"));

    _SEH2_TRY {

        // If the user is specifying -1 for a field, that means
        // we should leave that field unchanged, even if we might
        // have otherwise set it ourselves.  We'll set the Ccb flag
        // saying that the user set the field so that we
        // don't do our default updating.

        // We set the field to 0 then so we know not to actually
        // set the field to the user-specified (and in this case,
        // illegal) value.

        if (PtrBuffer->LastWriteTime.QuadPart == -1) {

            SetFlag(Ccb->Flags, UDF_CCB_WRITE_TIME_SET);
            PtrBuffer->LastWriteTime.QuadPart = 0;
        }

        if (PtrBuffer->LastAccessTime.QuadPart == -1) {

            SetFlag(Ccb->Flags, UDF_CCB_ACCESS_TIME_SET);
            PtrBuffer->LastAccessTime.QuadPart = 0;
        }

        if (PtrBuffer->CreationTime.QuadPart == -1) {

            SetFlag(Ccb->Flags, UDF_CCB_CREATE_TIME_SET);
            PtrBuffer->CreationTime.QuadPart = 0;
        }

        // Obtain a pointer to the directory entry associated with
        // the FCB being modifed. The directory entry is obviously
        // part of the data associated with the parent directory that
        // contains this particular file stream.
        if (PtrBuffer->FileAttributes) {
            UDFUpdateAttrTime(Fcb->Vcb, Fcb->FileInfo);
        } else
        if ( UDFIsADirectory(Fcb->FileInfo) &&
            !(Fcb->Vcb->CompatFlags & UDF_VCB_IC_UPDATE_UCHG_DIR_ACCESS_TIME) &&
              ((Fcb->FileInfo->Dloc->DataLoc.Modified ||
                Fcb->FileInfo->Dloc->AllocLoc.Modified ||
                (Fcb->FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
                Fcb->FileInfo->Dloc->FELoc.Modified))
             ) {
            // ignore Access Time Modification for unchanged Dir
            if (!PtrBuffer->CreationTime.QuadPart &&
               PtrBuffer->LastAccessTime.QuadPart &&
               !PtrBuffer->ChangeTime.QuadPart &&
               !PtrBuffer->LastWriteTime.QuadPart)
                try_return(RC);
        }

        UDFSetFileXTime(Fcb->FileInfo,
            &(PtrBuffer->CreationTime.QuadPart),
            &(PtrBuffer->LastAccessTime.QuadPart),
            &(PtrBuffer->ChangeTime.QuadPart),
            &(PtrBuffer->LastWriteTime.QuadPart) );

        if (PtrBuffer->CreationTime.QuadPart) {
            // The interesting thing here is that the user has set certain time
            // fields. However, before doing this, the user may have performed
            // I/O which in turn would have caused FSD to mark the fact that
            // write/access time should be modifed at cleanup.
            // We'll mark the fact that such updates are no longer
            // required since the user has explicitly specified the values he
            // wishes to see associated with the file stream.
            Fcb->CreationTime = PtrBuffer->CreationTime;
            Ccb->Flags |= UDF_CCB_CREATE_TIME_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
        }
        if (PtrBuffer->LastAccessTime.QuadPart) {
            Fcb->LastAccessTime = PtrBuffer->LastAccessTime;
            Ccb->Flags |= UDF_CCB_ACCESS_TIME_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }
        if (PtrBuffer->ChangeTime.QuadPart) {
            Fcb->ChangeTime = PtrBuffer->ChangeTime;
            Ccb->Flags |= UDF_CCB_MODIFY_TIME_SET;
        }
        if (PtrBuffer->LastWriteTime.QuadPart) {
            Fcb->LastWriteTime = PtrBuffer->LastWriteTime;
            Ccb->Flags |= UDF_CCB_WRITE_TIME_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }

        // Now come the attributes.
        if (PtrBuffer->FileAttributes) {
            // We have a non-zero attribute value.
            // The presence of a particular attribute indicates that the
            // user wishes to set the attribute value. The absence indicates
            // the user wishes to clear the particular attribute.

            // Our routine ignores unsupported flags
            PtrBuffer->FileAttributes &= ~(FILE_ATTRIBUTE_NORMAL);

            // Similarly, we should pick out other invalid flag values.
            if ( (PtrBuffer->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
               !(Fcb->FcbState & UDF_FCB_DIRECTORY))
                try_return(RC = STATUS_INVALID_PARAMETER);

            if (PtrBuffer->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                if (Fcb->FcbState & UDF_FCB_DIRECTORY)
                    try_return(RC = STATUS_INVALID_PARAMETER);
                FileObject->Flags |= FO_TEMPORARY_FILE;
            } else {
                FileObject->Flags &= ~FO_TEMPORARY_FILE;
            }

            if (PtrBuffer->FileAttributes & FILE_ATTRIBUTE_READONLY) {
                Fcb->FcbState |= UDF_FCB_READ_ONLY;
            } else {
                Fcb->FcbState &= ~UDF_FCB_READ_ONLY;
            }

            DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo),
                                 Fcb->FileInfo->Index);
            UDFAttributesToUDF(DirNdx, NULL, PtrBuffer->FileAttributes);
            /* A later open checks this cached value for READONLY.  Keep it
             * in step with the FE and directory index updated above. */
            Fcb->FileAttributes = DirNdx->SysAttr;
            DirNdx->FI_Flags |= UDF_FI_FLAG_SYS_ATTR;
            // If the FSD supports file compression, we may wish to
            // note the user's preferences for compressing/not compressing
            // the file at this time.
            Ccb->Flags |= UDF_CCB_ATTRIBUTES_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
        }

        if (NotifyFilter) {

            UDFNotifyReportChange(IrpContext, Fcb->Vcb,
                                      Fcb,
                                      NotifyFilter,
                                      FILE_ACTION_MODIFIED,
                                      Ccb->Lcb, FileObject);

            UDFSetFileSizeInDirNdx(Fcb->Vcb, Fcb->FileInfo, NULL);
            Fcb->FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        }

try_exit: NOTHING;
    } _SEH2_FINALLY {
        ;
    } _SEH2_END;
    return(RC);
} // end UDFSetBasicInformation()

NTSTATUS
UDFMarkStreamsForDeletion(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB           Vcb,
    IN PFCB           Fcb,
    IN BOOLEAN        ForDel
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    PUDF_FILE_INFO  SDirInfo = NULL;
    PUDF_FILE_INFO  FileInfo = NULL;
    ULONG lc;
    BOOLEAN SDirAcq = FALSE;
    BOOLEAN StrAcq = FALSE;
    BOOLEAN FileOpened = FALSE;
    BOOLEAN FileClosed = FALSE;
    BOOLEAN SDirClosed = FALSE;
    PFCB* StreamFcbs = NULL;
    uint_di StreamCount = 0;
    uint_di d,i;

    _SEH2_TRY {

        // In some cases we needn't marking Streams for deleteion
        // (Not opened or Don't exist)
        if (UDFIsAStream(Fcb->FileInfo) ||
           UDFIsAStreamDir(Fcb->FileInfo) ||
           !UDFHasAStreamDir(Fcb->FileInfo) ||
           !Fcb->FileInfo->Dloc->SDirInfo ||
           UDFIsSDirDeleted(Fcb->FileInfo->Dloc->SDirInfo) ||
           (UDFGetFileNameLinkCount(Fcb->FileInfo) > 1) )
            try_return (RC /*=STATUS_SUCCESS*/);

        // We shall mark Streams for deletion if there is no
        // Links to the file. Otherwise we'll delete only the file.
        // If we are asked to unmark Streams, we'll precess the whole Tree
        RC = UDFOpenStreamDir__(IrpContext, Vcb, Fcb->FileInfo, &SDirInfo);
        if (!NT_SUCCESS(RC))
            try_return(RC);

        if (SDirInfo->Fcb) {
            UDF_CHECK_PAGING_IO_RESOURCE(SDirInfo->Fcb);
            UDFAcquireFcbExclusive(IrpContext, SDirInfo->Fcb, TRUE);
            SDirAcq = TRUE;
        }

        /* Refuse the delete before setting any stream, SDir or base delete
         * state.  A later refusal in UDFUnlinkFile__ would be reported by
         * cleanup as a pretend delete and could leave allocated streams
         * without a durable namespace owner. */
        if (ForDel && UDFStreamDirectoryHasLiveHandles(SDirInfo))
            try_return(RC = STATUS_CANNOT_DELETE);

        if (!ForDel || ((lc = UDFGetFileNameLinkCount(Fcb->FileInfo)) < 2)) {

            UDF_DIR_SCAN_CONTEXT ScanContext;
            PDIR_INDEX_ITEM DirNdx;
            PUDF_FILE_INFO ScanFileInfo = NULL;

            // It is not worth checking whether the Stream can be deleted if
            // Undelete requested
            if (ForDel &&
                // scan DirIndex
                UDFDirIndexInitScan(SDirInfo, &ScanContext, 2)) {

                // Check if we can delete Streams
                while((DirNdx = UDFDirIndexScan(&ScanContext, &ScanFileInfo))) {
                    if (!ScanFileInfo)
                        continue;
                    if (ScanFileInfo->Fcb) {

                        MmPrint(("    MmFlushImageSection() for Stream\n"));
                        if (!MmFlushImageSection(&(ScanFileInfo->Fcb->FcbNonpaged->SegmentObject), MmFlushForDelete)) {

                            try_return(RC = STATUS_CANNOT_DELETE);
                        }

                    }
                }
            }
            /* Preflight every stream before changing any delete flags.  A
             * later open, link-count or close failure must not leave the
             * earlier streams delete-pending while the base FID survives. */
            d = UDFDirIndexGetLastIndex(SDirInfo->Dloc->DirIndex);
            if (d > 2) {
                if (d - 2 > MAXULONG / sizeof(PFCB))
                    try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                StreamFcbs = (PFCB*)MyAllocatePool__(NonPagedPool,
                                                     (d - 2) * sizeof(PFCB));
                if (!StreamFcbs)
                    try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            }

            // Open by index to obtain the correct ParentFile for linked FI.
            for(i=2; i<d; i++) {
                RC = UDFOpenFile__(IrpContext,
                                   Vcb,
                                   FALSE,TRUE,NULL,
                                   SDirInfo,&FileInfo,&i);
                if (NT_SUCCESS(RC)) {
                    FileOpened = TRUE;
                    if (FileInfo->Fcb) {

                        UDF_CHECK_PAGING_IO_RESOURCE(FileInfo->Fcb);
                        UDFAcquireFcbExclusive(IrpContext, FileInfo->Fcb, TRUE);
                        StrAcq = TRUE;

#ifndef UDF_ALLOW_LINKS_TO_STREAMS
                        if (UDFGetFileLinkCount(FileInfo) >= 2) {
                            // Currently, UDF_INFO package doesn't
                            // support this case, so we'll inform developer
                            // about this to prevent on-disk space leaks...
                            BrutePoint();
                            try_return(RC = STATUS_CANNOT_DELETE);
                        }
#endif //UDF_ALLOW_LINKS_TO_STREAMS
#ifdef UDF_DBG
                        if (ForDel) {
                            ASSERT(!(FileInfo->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
                            if (FileInfo->ParentFile &&
                               FileInfo->ParentFile->Fcb) {
                                ASSERT(!(FileInfo->ParentFile->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
                            }
                        }
#endif // UDF_DBG
                        /* Keep the FCB alive after closing this temporary FI
                         * and until the entire set is committed. */
                        InterlockedIncrement((PLONG)&FileInfo->Fcb->FcbReference);
                        StreamFcbs[StreamCount++] = FileInfo->Fcb;
                    }
                    RC = UDFCloseFile__(IrpContext, Vcb, FileInfo);
                    FileClosed = TRUE;
                    if (!NT_SUCCESS(RC))
                        try_return(RC);
                } else if (RC == STATUS_FILE_DELETED) {
                    // That's OK if STATUS_FILE_DELETED returned...
                    RC = STATUS_SUCCESS;
                } else
                    try_return(RC);
                if (FileInfo) {
                    if (UDFCleanUpFile__(Vcb, FileInfo)) {
                        ASSERT(!StrAcq && !(FileInfo->Fcb));
                        MyFreePool__(FileInfo);
                    }
                    if (StrAcq) {
                        UDFReleaseFcb(IrpContext, FileInfo->Fcb);
                        StrAcq = FALSE;
                    }
                }
                FileInfo = NULL;
                FileOpened = FALSE;
                FileClosed = FALSE;
            }
            /* A failed stream-directory close is also a precommit failure. */
            RC = UDFCloseFile__(IrpContext, Vcb, SDirInfo);
            SDirClosed = TRUE;
            if (!NT_SUCCESS(RC))
                try_return(RC);

            for (i = 0; i < StreamCount; i++) {
                UDF_CHECK_PAGING_IO_RESOURCE(StreamFcbs[i]);
                UDFAcquireFcbExclusive(IrpContext, StreamFcbs[i], TRUE);
                if (ForDel)
                    StreamFcbs[i]->FcbState |= (UDF_FCB_DELETE_ON_CLOSE |
                                                UDF_FCB_DELETE_PARENT);
                else
                    StreamFcbs[i]->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                                 UDF_FCB_DELETE_PARENT);
                UDFReleaseFcb(IrpContext, StreamFcbs[i]);
            }
            // Mark SDir for deletion
            if (SDirInfo->Fcb) {
                if (ForDel) {
#ifdef UDF_DBG
                    ASSERT(!(SDirInfo->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
                    if (SDirInfo->ParentFile &&
                       SDirInfo->ParentFile->Fcb) {
                        ASSERT(!(SDirInfo->ParentFile->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
                    }
#endif // UDF_DBG
                    AdPrint(("    SET stream dir DeleteOnClose\n"));
                    SDirInfo->Fcb->FcbState |= (UDF_FCB_DELETE_ON_CLOSE |
                                                UDF_FCB_DELETE_PARENT);
                } else {
                    AdPrint(("    CLEAR stream dir DeleteOnClose\n"));
                    SDirInfo->Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                                 UDF_FCB_DELETE_PARENT);
                }
            }
        } else
        if (lc >= 2) {
            RC = UDFCloseFile__(IrpContext, Vcb, SDirInfo);
            SDirClosed = TRUE;
            if (!NT_SUCCESS(RC))
                try_return(RC);
            // if caller wants us to perform DelTree for Streams, but
            // someone keeps Stream opened and there is a Link to this
            // file, we can't delete it immediately (on Cleanup) & should
            // not delete the whole Tree. Instead, we'll set DELETE_PARENT
            // flag in SDir to kill this file later, when all the Handles
            // to Streams, opened via this file, would be closed
#ifdef UDF_DBG
            ASSERT(!(SDirInfo->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
            if (SDirInfo->ParentFile &&
               SDirInfo->ParentFile->Fcb) {
                ASSERT(!(SDirInfo->ParentFile->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
            }
#endif // UDF_DBG
            if (SDirInfo->Fcb)
                SDirInfo->Fcb->FcbState |= UDF_FCB_DELETE_PARENT;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (FileInfo) {
            if (FileOpened && !FileClosed)
                UDFCloseFile__(IrpContext, Vcb, FileInfo);
            if (UDFCleanUpFile__(Vcb, FileInfo)) {
                ASSERT(!StrAcq && !(FileInfo->Fcb));
                MyFreePool__(FileInfo);
            }
            if (StrAcq) {
                UDFReleaseFcb(IrpContext, FileInfo->Fcb);
            }
        }
        if (SDirInfo) {
            if (!SDirClosed) {
                NTSTATUS CloseStatus = UDFCloseFile__(IrpContext, Vcb, SDirInfo);
                if (NT_SUCCESS(RC) && !NT_SUCCESS(CloseStatus))
                    RC = CloseStatus;
            }
            if (SDirAcq) {
                UDFReleaseFcb(IrpContext, SDirInfo->Fcb);
            }
            if (UDFCleanUpFile__(Vcb, SDirInfo)) {
                MyFreePool__(SDirInfo);
            }
            SDirInfo = NULL;
        }
        for (i = 0; i < StreamCount; i++)
            InterlockedDecrement((PLONG)&StreamFcbs[i]->FcbReference);
        if (StreamFcbs)
            MyFreePool__(StreamFcbs);
    } _SEH2_END;
    return RC;
} // end UDFMarkStreamsForDeletion()

/*
    (Un)Mark file for deletion.
 */
NTSTATUS
UDFSetDispositionInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_DISPOSITION_INFORMATION Buffer
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
    PVCB Vcb = IrpContext->Vcb;
//    PUDF_FILE_INFO  SDirInfo = NULL;
//    PUDF_FILE_INFO  FileInfo = NULL;
    ULONG lc;

    // Skip if this is the same FCB as Volume DASD

    if (Fcb == Vcb->VolumeDasdFcb) {

        return STATUS_ACCESS_DENIED;
    }

    if (!Buffer->DeleteFile) {

        // The user doesn't want to delete the file so clear
        // the delete on close bit

        Fcb->FcbState &= ~UDF_FCB_DELETE_ON_CLOSE;
        FileObject->DeletePending = FALSE;

        RC = UDFMarkStreamsForDeletion(IrpContext, Vcb, Fcb, FALSE); // Undelete
        return RC;
    }

    _SEH2_TRY {

        AdPrint(("    SET DeleteOnClose\n"));

        // The easy part is over. Now, we know that the user wishes to
        // delete the corresponding directory entry (of course, if this
        // is the only link to the file stream, any on-disk storage space
        // associated with the file stream will also be released when the
        // (only) link is deleted!)

        // Do some checking to see if the file can even be deleted.
        if (Fcb->FcbState & UDF_FCB_DELETE_ON_CLOSE) {
            // All done!
            try_return(RC);
        }

        if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {
            try_return(RC = STATUS_CANNOT_DELETE);
        }

        if (Fcb->FcbState & UDF_FCB_READ_ONLY) {
            RC = UDFCheckAccessRights(NULL, NULL, Fcb->ParentFcb, NULL, FILE_DELETE_CHILD, 0);
            if (!NT_SUCCESS(RC)) {
                try_return (RC = STATUS_CANNOT_DELETE);
            }
        }

        // It would not be prudent to allow deletion of either a root
        // directory or a directory that is not empty.
        if (Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY)
            try_return(RC = STATUS_CANNOT_DELETE);

        lc = UDFGetFileNameLinkCount(Fcb->FileInfo);

        if (Fcb->FcbState & UDF_FCB_DIRECTORY) {
            // Perform check to determine whether the directory
            // is empty or not.
            if (!UDFIsDirEmpty__(Fcb->FileInfo)) {
                 try_return(RC = STATUS_DIRECTORY_NOT_EMPTY);
            }

        } else {
            // An important step is to check if the file stream has been
            // mapped by any process. The delete cannot be allowed to proceed
            // in this case.
            MmPrint(("    MmFlushImageSection()\n"));

            if (!MmFlushImageSection(&Fcb->FcbNonpaged->SegmentObject,
                    (lc > 1) ? MmFlushForWrite : MmFlushForDelete)) {

                try_return(RC = STATUS_CANNOT_DELETE);
            }
        }
        // We should also mark Streams for deletion if there are no
        // Links to the file. Otherwise we'll delete only the file

        if (lc > 1) {
            RC = STATUS_SUCCESS;
        } else {
            RC = UDFMarkStreamsForDeletion(IrpContext, Vcb, Fcb, TRUE); // Delete
            if (!NT_SUCCESS(RC))
                try_return(RC);
        }

        // Set a flag to indicate that this directory entry will become history
        // at cleanup.
        Fcb->FcbState |= UDF_FCB_DELETE_ON_CLOSE;
        if (FileObject)
            FileObject->DeletePending = TRUE;

        if ((Fcb->FcbState & UDF_FCB_DIRECTORY) && Ccb) {
            FsRtlNotifyFullChangeDirectory( Vcb->NotifySync, &(Vcb->NextNotifyIRP),
                                            (PVOID)Ccb, NULL, FALSE, FALSE,
                                            0, NULL, NULL, NULL );
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        ;
    } _SEH2_END;
    return(RC);
} // end UDFSetDispositionInformation()


/*
      Change file allocation length.
 */
NTSTATUS
UDFSetAllocationInfo(
    IN PFCB                            Fcb,
    IN PCCB                            Ccb,
    IN PVCB                            Vcb,
    IN PFILE_OBJECT                    FileObject,
    IN PIRP_CONTEXT IrpContext,
    IN PIRP                            Irp,
    IN PFILE_ALLOCATION_INFORMATION    Buffer
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    BOOLEAN         TruncatedFile = FALSE;
    BOOLEAN         ModifiedAllocSize = FALSE;
    BOOLEAN         CacheMapInitialized = FALSE;

    // Allocation is only allowed on a file and not a directory

    if (NodeType(Fcb) != UDF_NODE_TYPE_DATA) {

        return STATUS_INVALID_PARAMETER;
    }

    UDFAcquirePagingIoExclusive(IrpContext, Fcb);

    _SEH2_TRY {
        // Increasing the allocation size associated with a file stream
        // is relatively easy. All we have to do is execute some FSD
        // specific code to check whether we have enough space available
        // (and if the FSD supports user/volume quotas, whether the user
        // is not exceeding quota), and then increase the file size in the
        // corresponding on-disk and in-memory structures.
        // Then, all we should do is inform the Cache Manager about the
        // increased allocation size.

        Fcb->Header.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

        if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
            (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
            !FlagOn(Irp->Flags, IRP_PAGING_IO)) {
            ASSERT( !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) );
            //  Now initialize the cache map.
            MmPrint(("    CcInitializeCacheMap()\n"));
            CcInitializeCacheMap(FileObject,
                                 (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                 FALSE,
                                 &(UdfData.CacheMgrCallBacks),
                                 Fcb);

            CacheMapInitialized = TRUE;
        }

        // Are we increasing the allocation size?
        if (Fcb->Header.AllocationSize.QuadPart <
            Buffer->AllocationSize.QuadPart) {

            // Yes. Do the FSD specific stuff i.e. increase reserved
            // space on disk.
            if (
                ( (Vcb->BitmapFcb ? Vcb->FreeAllocUnits : (LONGLONG)UDFGetFreeSpace(Vcb))
                    << Vcb->SectorShift
                ) 
                    < Buffer->AllocationSize.QuadPart
            ) {
                try_return(RC = STATUS_DISK_FULL);
            }
//          RC = STATUS_SUCCESS;
            ModifiedAllocSize = TRUE;

        } else if (Fcb->Header.AllocationSize.QuadPart > Buffer->AllocationSize.QuadPart) {
            // This is the painful part. See if the VMM will allow us to proceed.
            // The VMM will deny the request if:
            // (a) any image section exists OR
            // (b) a data section exists and the size of the user mapped view
            //       is greater than the new size
            // Otherwise, the VMM should allow the request to proceed.
            MmPrint(("    MmCanFileBeTruncated()\n"));
            if (!MmCanFileBeTruncated(&Fcb->FcbNonpaged->SegmentObject, &Buffer->AllocationSize)) {
                // VMM said no way!
                try_return(RC = STATUS_USER_MAPPED_FILE);
            }

            // Perform our directory entry modifications. Release any on-disk
            // space we may need to in the process.
            ModifiedAllocSize = TRUE;
            TruncatedFile = TRUE;
        }

        ASSERT(NT_SUCCESS(RC));
        // This is a good place to check if we have performed a truncate
        // operation. If we have perform a truncate (whether we extended
        // or reduced file size or even leave it intact), we should update
        // file time stamps.
        FileObject->Flags |= FO_FILE_MODIFIED;

        // Last, but not the lease, we must inform the Cache Manager of file size changes.
        if (ModifiedAllocSize) {

            // If we decreased the allocation size to less than the
            // current file size, modify the file size value.
            // Similarly, if we decreased the value to less than the
            // current valid data length, modify that value as well.

            // Update the FCB Header with the new allocation size.
            if (TruncatedFile) {
                if (Fcb->Header.FileSize.QuadPart > Buffer->AllocationSize.QuadPart) {
                    /* A shrink can be rejected before touching the on-disk
                     * file, including by the ordered-truncate layout guard.
                     * Keep the live FCB and Cache Manager sizes unchanged
                     * until the file-system resize has actually succeeded. */
                    RC = UDFResizeFile__(IrpContext, Vcb, Fcb->FileInfo, Buffer->AllocationSize.QuadPart);
                    if (!NT_SUCCESS(RC))
                        try_return(RC);
                    Fcb->Header.FileSize = Buffer->AllocationSize;
//                    UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, NULL);
                }
                if (Fcb->Header.ValidDataLength.QuadPart > Buffer->AllocationSize.QuadPart) {
                    // Decrease the valid data length value.
                    Fcb->Header.ValidDataLength = Buffer->AllocationSize;
                }
            } else {
                Fcb->Header.AllocationSize = Buffer->AllocationSize;
//                UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo,
//                                       &(PtrBuffer->AllocationSize.QuadPart));
            }

            // If the FCB has not had caching initiated, it is still valid
            // for us to invoke the NT Cache Manager. It is possible in such
            // situations for the call to be no'oped (unless some user has
            // mapped in the file)

            // NOTE: The invocation to CcSetFileSizes() will quite possibly
            //  result in a recursive call back into the file system.
            //  This is because the NT Cache Manager will typically
            //  perform a flush before telling the VMM to purge pages
            //  especially when caching has not been initiated on the
            //  file stream, but the user has mapped the file into
            //  the process' virtual address space.
            MmPrint(("    CcSetFileSizes()\n"));

            CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&(Fcb->Header.AllocationSize));

            Fcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;

            // Inform any pending IRPs (notify change directory).
            if (UDFIsAStream(Fcb->FileInfo)) {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb,
                                          FILE_NOTIFY_CHANGE_STREAM_SIZE,
                                          FILE_ACTION_MODIFIED_STREAM,
                                          Ccb->Lcb, FileObject);
            } else {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb,
                                          FILE_NOTIFY_CHANGE_SIZE,
                                          FILE_ACTION_MODIFIED,
                                          Ccb->Lcb, FileObject);
            }
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if (CacheMapInitialized) {

            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, NULL, NULL);
        }

        UDFReleasePagingIo(IrpContext, Fcb);

    } _SEH2_END;
    return(RC);
} // end UDFSetAllocationInformation()

/*
    Set end of file (resize).
 */
NTSTATUS
UDFSetEndOfFileInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION              IrpSp,
    IN PFCB                            Fcb,
    IN PCCB                            Ccb,
    IN PVCB                            Vcb,
    IN PFILE_OBJECT                    FileObject,
    IN PIRP                            Irp,
    IN PFILE_END_OF_FILE_INFORMATION   PtrBuffer
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    NTSTATUS        CloseStatus = STATUS_SUCCESS;
    BOOLEAN         TruncatedFile = FALSE;
    BOOLEAN         ModifiedAllocSize = FALSE;
    ULONG           Attr;
    PDIR_INDEX_ITEM DirNdx;
    LONGLONG        OldFileSize;
//    BOOLEAN         ZeroBlock;
    BOOLEAN         CacheMapInitialized = FALSE;

    // Allocation is only allowed on a file and not a directory

    if (NodeType(Fcb) != UDF_NODE_TYPE_DATA) {

        return STATUS_INVALID_PARAMETER;
    }

    UDFAcquirePagingIoExclusive(IrpContext, Fcb);

    _SEH2_TRY {
        // Increasing the allocation size associated with a file stream
        // is relatively easy. All we have to do is execute some FSD
        // specific code to check whether we have enough space available
        // (and if the FSD supports user/volume quotas, whether the user
        // is not exceeding quota), and then increase the file size in the
        // corresponding on-disk and in-memory structures.
        // Then, all we should do is inform the Cache Manager about the
        // increased allocation size.

        if ((Fcb->FcbState & UDF_FCB_DELETED) ||
           (Fcb->NtReqFCBFlags & UDF_NTREQ_FCB_DELETED)) {
#ifdef UDF_DBG
            if (UDFGetFileLinkCount(Fcb->FileInfo) < 1) {
                BrutePoint();
                try_return(RC = STATUS_SUCCESS);
            } else
#endif // UDF_DBG
                try_return(RC = STATUS_SUCCESS);
        }

        Fcb->Header.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

        if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
            (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
            !(Irp->Flags & IRP_PAGING_IO)) {

            ASSERT( !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) );
            //  Now initialize the cache map.
            MmPrint(("    CcInitializeCacheMap()\n"));
            CcInitializeCacheMap(FileObject,
                                 (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                 FALSE,
                                 &UdfData.CacheMgrCallBacks,
                                 Fcb);

            CacheMapInitialized = TRUE;
        }

        //  Do a special case here for the lazy write of file sizes.
        if (IrpSp->Parameters.SetFile.AdvanceOnly) {
            //  Never have the dirent filesize larger than the fcb filesize
            PtrBuffer->EndOfFile.QuadPart =
                min(PtrBuffer->EndOfFile.QuadPart, Fcb->Header.FileSize.QuadPart);
            //  Only advance the file size, never reduce it with this call
            RC = STATUS_SUCCESS;
            if (UDFGetFileSizeFromDirNdx(Vcb, Fcb->FileInfo) >=
               PtrBuffer->EndOfFile.QuadPart)
                try_return(RC);

            UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, &(PtrBuffer->EndOfFile.QuadPart));
            goto notify_size_changes;
        }

        //             !!! IMPORTANT !!!

        // We can get here after all Handles to the file are closed
        // To prevent allocation size incoherency we should
        // reference FileInfo _before_ call to UDFResizeFile__()
        // and use UDFCloseFile__() _after_ that

        // Are we increasing the allocation size?
        OldFileSize = Fcb->Header.FileSize.QuadPart;
        if (OldFileSize < PtrBuffer->EndOfFile.QuadPart) {

            // Yes. Do the FSD specific stuff i.e. increase reserved
            // space on disk.
/*
            if (FileObject->PrivateCacheMap)
                ZeroBlock = TRUE;
*/

            // reference file to pretend that it is opened
            UDFReferenceFile__(Fcb->FileInfo);
            InterlockedIncrement((PLONG)&Fcb->FcbReference);
            // perform resize operation
            RC = UDFResizeFile__(IrpContext, Vcb, Fcb->FileInfo, PtrBuffer->EndOfFile.QuadPart);
            // dereference file
            CloseStatus = UDFCloseFile__(IrpContext, Vcb, Fcb->FileInfo);
            InterlockedDecrement((PLONG)&Fcb->FcbReference);
            if (!NT_SUCCESS(RC)) {
                try_return(RC);
            }
            // update values in NtReqFcb
            Fcb->Header.FileSize.QuadPart =
//            NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart =
                PtrBuffer->EndOfFile.QuadPart;
            ModifiedAllocSize = TRUE;

        } else if (Fcb->Header.FileSize.QuadPart > PtrBuffer->EndOfFile.QuadPart) {

            // This is the painful part. See if the VMM will allow us to proceed.
            // The VMM will deny the request if:
            // (a) any image section exists OR
            // (b) a data section exists and the size of the user mapped view
            //       is greater than the new size
            // Otherwise, the VMM should allow the request to proceed.

            MmPrint(("    MmCanFileBeTruncated()\n"));
            if (!MmCanFileBeTruncated(&Fcb->FcbNonpaged->SegmentObject, &PtrBuffer->EndOfFile)) {
                // VMM said no way!
                try_return(RC = STATUS_USER_MAPPED_FILE);
            }

            // Perform directory entry modifications. Release any on-disk
            // space we may need to in the process.
            UDFReferenceFile__(Fcb->FileInfo);
            InterlockedIncrement((PLONG)&Fcb->FcbReference);
            // perform resize operation
            RC = UDFResizeFile__(IrpContext, Vcb, Fcb->FileInfo, PtrBuffer->EndOfFile.QuadPart);
            // dereference file
            CloseStatus = UDFCloseFile__(IrpContext, Vcb, Fcb->FileInfo);
            InterlockedDecrement((PLONG)&Fcb->FcbReference);

            ModifiedAllocSize = TRUE;
            TruncatedFile = TRUE;
        }

        // This is a good place to check if we have performed a truncate
        // operation. If we have perform a truncate (whether we extended
        // or reduced file size), we should update file time stamps.

        // Last, but not the least, we must inform the Cache Manager of file size changes.
        if (ModifiedAllocSize && NT_SUCCESS(RC)) {
            // If we decreased the allocation size to less than the
            // current file size, modify the file size value.
            // Similarly, if we decreased the value to less than the
            // current valid data length, modify that value as well.
            if (TruncatedFile) {
                if (Fcb->Header.ValidDataLength.QuadPart > PtrBuffer->EndOfFile.QuadPart) {
                    // Decrease the valid data length value.
                    Fcb->Header.ValidDataLength = PtrBuffer->EndOfFile;
                }
                if (Fcb->Header.FileSize.QuadPart > PtrBuffer->EndOfFile.QuadPart) {
                    // Decrease the file size value.
                    Fcb->Header.FileSize = PtrBuffer->EndOfFile;
                }
                UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, NULL);
            } else {
                // Update the FCB Header with the new allocation size.
                // NT expects AllocationSize to be decreased on Close only
                Fcb->Header.AllocationSize.QuadPart =
                    PtrBuffer->EndOfFile.QuadPart;

                UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, &(PtrBuffer->EndOfFile.QuadPart));
            }

            FileObject->Flags |= FO_FILE_MODIFIED;
//                UDFGetFileAllocationSize(Vcb, Fcb->FileInfo);

            // If the FCB has not had caching initiated, it is still valid
            // for us to invoke the NT Cache Manager. It is possible in such
            // situations for the call to be no'oped (unless some user has
            // mapped in the file)

            // Archive bit
            if (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT) {
                DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index);
                Ccb->Flags &= ~UDF_CCB_ATTRIBUTES_SET;
                Attr = UDFAttributesToNT(DirNdx, Fcb->FileInfo->Dloc->FileEntry);
                if (!(Attr & FILE_ATTRIBUTE_ARCHIVE))
                    UDFAttributesToUDF(DirNdx, Fcb->FileInfo->Dloc->FileEntry, Attr | FILE_ATTRIBUTE_ARCHIVE);
            }

            // NOTE: The invocation to CcSetFileSizes() will quite possibly
            //  result in a recursive call back into the file system.
            //  This is because the NT Cache Manager will typically
            //  perform a flush before telling the VMM to purge pages
            //  especially when caching has not been initiated on the
            //  file stream, but the user has mapped the file into
            //  the process' virtual address space.
            MmPrint(("    CcSetFileSizes(), thrd:%8.8x\n",PsGetCurrentThread()));

            CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);

/*            if (ZeroBlock) {
                UDFZeroDataEx(NtReqFcb,
                              OldFileSize,
                              PtrBuffer->EndOfFile.QuadPart - OldFileSize,
                              TRUE // CanWait, Vcb, FileObject);
            }*/
            Fcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;

notify_size_changes:

            // Inform any pending IRPs (notify change directory).
            if (UDFIsAStream(Fcb->FileInfo)) {
                UDFNotifyReportChange( IrpContext, Vcb, Fcb,
                                           FILE_NOTIFY_CHANGE_STREAM_SIZE,
                                           FILE_ACTION_MODIFIED_STREAM,
                                           Ccb->Lcb, FileObject);
            } else {
                UDFNotifyReportChange( IrpContext, Vcb, Fcb,
                                           FILE_NOTIFY_CHANGE_SIZE,
                                           FILE_ACTION_MODIFIED,
                                           Ccb->Lcb, FileObject);
            }
        }

try_exit:
        /* Resize may already have committed a new FE when the final close
         * reports a metadata writeback error.  Keep FCB/Cc sizes in sync
         * with that resize, then return the close error to the caller. */
        if (NT_SUCCESS(RC) && !NT_SUCCESS(CloseStatus))
            RC = CloseStatus;

    } _SEH2_FINALLY {

        if (CacheMapInitialized) {

            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }

        UDFReleasePagingIo(IrpContext, Fcb);

    } _SEH2_END;
    return(RC);
} // end UDFSetEndOfFileInfo()

NTSTATUS
UDFPrepareForRenameMoveLink(
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PBOOLEAN SingleDir,
    PBOOLEAN AcquiredDir1,
    PBOOLEAN AcquiredFcb1,
    IN PCCB Ccb1,
    PUDF_FILE_INFO File1,
    PUDF_FILE_INFO Dir1,
    PUDF_FILE_INFO Dir2,
    BOOLEAN HardLink
    )
{
    // convert acquisition to Exclusive
    // this will prevent us from the following situation:
    // There is a pair of objects among input dirs &
    // one of them is a parent of another. Sequential resource
    // acquisition may lead to deadlock due to concurrent
    // cleanup operations or UDFTeardownStructures()
    InterlockedIncrement((PLONG)&Vcb->VcbReference);


    (*SingleDir) = ((Dir1 == Dir2) && (Dir1->Fcb));

    if (!(*SingleDir) ||
       (UDFGetFileNameLinkCount(File1) != 1)) {
        InterlockedDecrement((PLONG)&Vcb->VcbReference);
    } else {
        InterlockedDecrement((PLONG)&Vcb->VcbReference);

        // Child-first lock ordering
        // File1 (child) first, Dir1 (parent) second
        UDF_CHECK_PAGING_IO_RESOURCE(File1->Fcb);
        UDFAcquireFcbExclusive(IrpContext, File1->Fcb, TRUE);
        (*AcquiredFcb1) = TRUE;

        UDF_CHECK_PAGING_IO_RESOURCE(Dir1->Fcb);
        UDFAcquireFcbExclusive(IrpContext, Dir1->Fcb, TRUE);
        (*AcquiredDir1) = TRUE;
    }
    return STATUS_SUCCESS;
} // end UDFPrepareForRenameMoveLink()

/*
    Check if a directory subtree has any open handles (FcbCleanup != 0).
    Iterative depth-first walk via ChildLcbQueue — no recursion, kernel-safe.
    Returns TRUE if subtree is clean (no open handles), FALSE otherwise.
*/
BOOLEAN
UDFCheckDirOpenHandles(
    IN PFCB DirectoryFcb
    )
{
    PFCB CheckFcb = DirectoryFcb;
    PLIST_ENTRY CheckLink = CheckFcb->ChildLcbQueue.Flink;

    while (CheckFcb != NULL) {

        // Process all children at current level
        while (CheckLink != &CheckFcb->ChildLcbQueue) {
            PLCB ChildLcb = CONTAINING_RECORD(CheckLink, struct LCB, ParentFcbLinks);
            PFCB ChildFcb = ChildLcb->ChildFcb;
            CheckLink = CheckLink->Flink;

            if (!ChildFcb) continue;

            if (ChildFcb->FcbCleanup != 0) {
                return FALSE;
            }

            // If child is a directory with children, descend into it
            if ((ChildFcb->FcbState & UDF_FCB_DIRECTORY) &&
                !IsListEmpty(&ChildFcb->ChildLcbQueue)) {
                CheckFcb = ChildFcb;
                CheckLink = CheckFcb->ChildLcbQueue.Flink;
            }
        }

        // Back to root — done
        if (CheckFcb == DirectoryFcb) break;

        // Ascend: find our LCB in parent, advance to next sibling
        PLIST_ENTRY ParentLink;
        for (ParentLink = CheckFcb->ParentLcbQueue.Flink;
             ParentLink != &CheckFcb->ParentLcbQueue;
             ParentLink = ParentLink->Flink) {
            PLCB ParentLcb = CONTAINING_RECORD(ParentLink, struct LCB, ChildFcbLinks);
            if (ParentLcb->ParentFcb) {
                CheckFcb = ParentLcb->ParentFcb;
                CheckLink = ParentLcb->ParentFcbLinks.Flink;
                break;
            }
        }
    }

    return TRUE;
}

/* The source FCB and VCB resources are held exclusive by the caller.
 * IoCheckShareAccess does not count attribute-only opens. Permit those peers
 * only when the renaming FILE_OBJECT is the sole data/delete participant and
 * every outstanding cleanup belongs to this same live name link. This keeps
 * other data handles and unresolved/by-ID ownership on the conservative path.
 */
static BOOLEAN
UDFRenameHasOnlyMetadataPeers(
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_OBJECT FileObject
    )
{
    return FileObject->DeleteAccess &&
           (UDFIsADirectory(Fcb->FileInfo) ||
            UDFGetFileNameLinkCount(Fcb->FileInfo) == 1) &&
           Fcb->ShareAccess.OpenCount == 1 &&
           Ccb->Lcb &&
           Ccb->Lcb->ChildFcb == Fcb &&
           Ccb->Lcb->ParentFcb == Fcb->ParentFcb &&
           !FlagOn(Ccb->Lcb->Flags, UDF_LCB_FLAG_LINK_DELETED) &&
           Ccb->Lcb->Reference == Fcb->FcbCleanup;
}

/*
    Rename or move file
 */
NTSTATUS
UDFSetRenameInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_OBJECT FileObject,   // Source File
    IN PFILE_RENAME_INFORMATION PtrBuffer
    )
{
    PIRP Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT TargetFileObject = IrpSp->Parameters.SetFile.FileObject;
    BOOLEAN ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;

    PFCB ParentFcb = Fcb->ParentFcb;

    NTSTATUS RC;
    PVCB Vcb = Fcb->Vcb;
    PFCB TargetFcb;
    BOOLEAN IgnoreCase;
    BOOLEAN SourceFcbAcquired = FALSE;
    BOOLEAN ParentFcbAcquired = FALSE;
    BOOLEAN TargetParentFcbAcquired = FALSE;
    BOOLEAN StaleFcbAcquired = FALSE;
    PFCB StaleFcb = NULL;
    PLCB StaleLcb = NULL;
    BOOLEAN NeedRemovePrefix = FALSE;
    BOOLEAN SingleDir = TRUE;

    PUDF_FILE_INFO FileInfo;
    PUDF_FILE_INFO DirInfo;
    PUDF_FILE_INFO TargetDirInfo;

    UNICODE_STRING NewName;
    UNICODE_STRING LocalPath;

    LocalPath.Buffer = NULL;

    // Make sure we can wait for this request.

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        UDFRaiseStatus(IrpContext, STATUS_CANT_WAIT);
    }

    _SEH2_TRY {

        // do we try to rename Volume ?
        if (ParentFcb == NULL) {

             try_return (RC = STATUS_INVALID_PARAMETER);
        }

        // do we try to rename RootDir ?
        if (Fcb == Vcb->RootIndexFcb) {

            try_return (RC = STATUS_INVALID_PARAMETER);
        }

        /* Serialize the open/share counters and keep the source FI/LCB stable.
         * Use the same child-before-parent order as cleanup. */
        UDF_CHECK_PAGING_IO_RESOURCE(Fcb);
        UDFAcquireFcbExclusive(IrpContext, Fcb, FALSE);
        SourceFcbAcquired = TRUE;

        FileInfo = Fcb->FileInfo;
        ASSERT(FileInfo);
        DirInfo = FileInfo->ParentFile;
        ASSERT(DirInfo);

        // We don't allow this operation on a file opened by file Id.

        if (FlagOn(Ccb->Flags, CCB_FLAG_OPEN_BY_ID | CCB_FLAG_OPEN_RELATIVE_BY_ID)) {

            try_return (RC = STATUS_INVALID_PARAMETER);
        }

        // do we try to rename to RootDir or Volume ?
        if (!TargetFileObject) {

            TargetDirInfo = FileInfo->ParentFile;
            // For streams or same-dir rename, target FCB is the parent
            TargetFcb = TargetDirInfo->Fcb;

        } else {

            PCCB TargetCcb;
            UDFDecodeFileObject(TargetFileObject, &TargetFcb, &TargetCcb);

            ASSERT_FCB(TargetFcb);

            if (!TargetFcb) {
                try_return (RC = STATUS_INVALID_PARAMETER);
            }

            // Check if TargetFcb is a directory or a file
            // TargetFileObject should be a directory.
            // But if it points to a file (e.g., existing file to be replaced),
            // we need to use its parent directory for correct SingleDir calculation

            if (UDFIsADirectory(TargetFcb->FileInfo)) {

                TargetDirInfo = TargetFcb->FileInfo;

            } else {

                // TargetFileObject points to a file, use its parent directory
                TargetDirInfo = TargetFcb->FileInfo->ParentFile;

                if (!TargetDirInfo) {
                    try_return (RC = STATUS_INVALID_PARAMETER);
                }
            }
        }

        // invalid destination ?
        if (!TargetDirInfo) try_return (RC = STATUS_ACCESS_DENIED);

        // Stream can't be a Dir or have StreamDir
        if (UDFIsAStreamDir(TargetDirInfo)) {
            if (UDFIsADirectory(FileInfo) ||
               UDFHasAStreamDir(FileInfo)) {
                try_return (RC = STATUS_ACCESS_DENIED);
            }
        }

        // Parse NewName before acquiring locks to enable early-out optimizations
        if (!TargetFileObject) {
            //  Make sure the name is of legal length.
            if (PtrBuffer->FileNameLength > UDF_NAME_LEN*sizeof(WCHAR)) {
                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }
            NewName.Length = NewName.MaximumLength = (USHORT)(PtrBuffer->FileNameLength);
            NewName.Buffer = (PWCHAR)&(PtrBuffer->FileName);
        } else {
            // TargetFileObject->FileName is set up by UDFCommonCreate for
            // SL_OPEN_TARGET_DIRECTORY:
            //   Length      = parent directory path (trimmed to last '\')
            //   MaximumLength = full original path (parent + '\' + filename)
            // Extract the filename component starting after Length.
            USHORT FileNameStart = TargetFileObject->FileName.Length;

            if (FileNameStart < TargetFileObject->FileName.MaximumLength &&
                TargetFileObject->FileName.Buffer[FileNameStart / sizeof(WCHAR)] == L'\\') {
                FileNameStart += sizeof(WCHAR);
            }

            NewName.Length = TargetFileObject->FileName.MaximumLength - FileNameStart;
            NewName.MaximumLength = NewName.Length;
            NewName.Buffer = (PWCHAR)((PCHAR)TargetFileObject->FileName.Buffer + FileNameStart);
        }

        IgnoreCase = FlagOn(Ccb->Flags, CCB_FLAG_IGNORE_CASE);

        SingleDir = (TargetDirInfo->Fcb == ParentFcb);

        // Self-rename check: if same directory and same name → no-op
        if (SingleDir && !ReplaceIfExists && Ccb->Lcb) {
            if (FsRtlAreNamesEqual(&NewName, &Ccb->Lcb->FileName, IgnoreCase, NULL)) {
                try_return(RC = STATUS_SUCCESS);
            }
        }

        //
        // Always acquire source parent directory — needed for splay tree
        // modifications in UDFRenameMovePrefix and UDFRemovePrefix.
        //
        if (SingleDir) {

            UDFAcquireFcbExclusive(IrpContext, ParentFcb, FALSE);
            ParentFcbAcquired = TRUE;

        } else {

            // Cross-directory rename: acquire both parents.
            // Use address ordering to prevent ABBA deadlock.
            if ((ULONG_PTR)ParentFcb < (ULONG_PTR)TargetDirInfo->Fcb) {
                UDFAcquireFcbExclusive(IrpContext, ParentFcb, FALSE);
                ParentFcbAcquired = TRUE;
                UDFAcquireFcbExclusive(IrpContext, TargetDirInfo->Fcb, FALSE);
                TargetParentFcbAcquired = TRUE;
            } else if (ParentFcb != TargetDirInfo->Fcb) {
                UDFAcquireFcbExclusive(IrpContext, TargetDirInfo->Fcb, FALSE);
                TargetParentFcbAcquired = TRUE;
                UDFAcquireFcbExclusive(IrpContext, ParentFcb, FALSE);
                ParentFcbAcquired = TRUE;
            } else {
                // Same directory (shouldn't happen for !SingleDir, but handle gracefully)
                UDFAcquireFcbExclusive(IrpContext, ParentFcb, FALSE);
                ParentFcbAcquired = TRUE;
            }
        }

        // Extra metadata-only opens do not make the source data busy.
        if (Fcb->FcbCleanup > 1 &&
            !UDFRenameHasOnlyMetadataPeers(Fcb, Ccb, FileObject)) {

            try_return (RC = STATUS_ACCESS_DENIED);
        }

        ASSERT(Fcb->FcbCleanup);
        ASSERT(!Fcb->IrpContextLite);

        if (Fcb->IrpContextLite) {
            try_return (RC = STATUS_ACCESS_DENIED);
        }

        // For directories: check that no descendant has open handles.
        if ((Fcb->FcbState & UDF_FCB_DIRECTORY) &&
            !UDFCheckDirOpenHandles(Fcb)) {
            try_return (RC = STATUS_ACCESS_DENIED);
        }

        // Check if renaming to stream name - only allowed for streams
        if (!TargetFileObject && NewName.Length >= sizeof(WCHAR) && NewName.Buffer[0] == L':') {

            if (!UDFIsAStream(FileInfo)) {

                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }
        }

        // UDFDoesOSAllowFileToBeMoved__ (= UDFDoesOSAllowFileToBeUnlinked__)
        // checks DirNdx->FileInfo != NULL for each child — blocks if any
        // child has a cached FileInfo structure, even with FcbCleanup=0
        // (no user handles). This is too strict for rename/move where file
        // data stays intact. The subtree is already validated above via
        // UDFCheckDirOpenHandles which checks FcbCleanup (user handles).
        {

            // Note: With LCB model, FcbReference may not always be >= RefCount
            // for directories opened internally during path traversal

            // Pre-validate target for ReplaceIfExists: look up the target name
            // in the directory index, then search for its LCB by FCB pointer
            // (not by name). This correctly handles case-insensitive matches,
            // short name aliases, and avoids string comparison overhead.
            if (ReplaceIfExists) {

                PFCB TargetParentFcb = TargetDirInfo->Fcb;

                if (TargetParentFcb) {

                    // Look up target name in directory index
                    DIR_ENUM_CONTEXT TargetDirContext;
                    NTSTATUS FindStatus = UDFFindDirEntry(Vcb, TargetDirInfo,
                                                         &NewName, IgnoreCase,
                                                         TRUE, &TargetDirContext);

                    if (NT_SUCCESS(FindStatus) && TargetDirContext.DirNdx) {

                        PDIR_INDEX_ITEM FoundDirNdx = TargetDirContext.DirNdx;

                        // Cannot replace a directory
                        if (FoundDirNdx->FileCharacteristics & FILE_DIRECTORY) {

                            try_return(RC = STATUS_OBJECT_NAME_COLLISION);
                        }

                        // Skip stale LCB logic for self-rename (e.g. case change)
                        BOOLEAN IsSelfRename = (SingleDir &&
                                                FoundDirNdx->FileInfo == FileInfo);

                        if (!IsSelfRename) {

                            // If target was previously opened, find its LCB
                            // by matching ChildFcb pointer
                            if (FoundDirNdx->FileInfo &&
                                FoundDirNdx->FileInfo->Fcb) {

                                PFCB TargetFileFcb = FoundDirNdx->FileInfo->Fcb;

                                PLIST_ENTRY Link;
                                for (Link = TargetParentFcb->ChildLcbQueue.Flink;
                                     Link != &TargetParentFcb->ChildLcbQueue;
                                     Link = Link->Flink) {
                                    PLCB TestLcb = CONTAINING_RECORD(Link, struct LCB, ParentFcbLinks);
                                    if (TestLcb != Ccb->Lcb &&
                                        !(TestLcb->Flags & UDF_LCB_FLAG_LINK_DELETED) &&
                                        TestLcb->ChildFcb == TargetFileFcb) {
                                        StaleLcb = TestLcb;
                                        break;
                                    }
                                }
                            }
                            // If FileInfo is NULL (never opened), no FCB/LCB exists.
                            // UDFRenameMoveFile__ will handle the on-disk deletion.

                            if (StaleLcb) {

                                StaleFcb = StaleLcb->ChildFcb;

                                // Acquire target FCB to serialize with cleanup
                                if (StaleFcb) {
                                    UDF_CHECK_PAGING_IO_RESOURCE(StaleFcb);
                                    if (!UDFAcquireFcbExclusive(IrpContext, StaleFcb, TRUE)) {
                                        try_return(RC = STATUS_ACCESS_DENIED);
                                    }
                                    StaleFcbAcquired = TRUE;
                                }

                                // Cannot remove LCB that still has open references.
                                if (StaleLcb->Reference != 0) {

                                    try_return(RC = STATUS_ACCESS_DENIED);
                                }
                            }
                        }
                    }
                    // If target not found in directory, proceed normally.
                    // UDFRenameMoveFile__ will create the new entry.
                }
            }

            //
            // Pre-rename: mark stale LCB as deleted so concurrent
            // create lookups won't find it during disk operations.
            // Purge cache before modifying directory.
            //
            if (StaleLcb) {
                StaleLcb->Flags |= UDF_LCB_FLAG_LINK_DELETED;

                // Remove from splay trees immediately so the renamed LCB
                // can be inserted with the same name without duplicate conflict.
                if (StaleLcb->ParentFcb) {
                    UdfRemoveNameLinks(StaleLcb->ParentFcb, StaleLcb);
                }

                // Purge cache for target before disk operations
                if (StaleFcb && StaleFcb->FcbNonpaged) {
                    if (StaleFcb->FcbNonpaged->SegmentObject.DataSectionObject ||
                        StaleFcb->FcbNonpaged->SegmentObject.ImageSectionObject) {
                        MmPrint(("    CcPurgeCacheSection() stale target\n"));
                        CcPurgeCacheSection(&StaleFcb->FcbNonpaged->SegmentObject, NULL, 0, FALSE);
                    }
                }
            }

            RC = UDFRenameMoveFile__(IrpContext, Vcb, IgnoreCase, &ReplaceIfExists, &NewName, DirInfo, TargetDirInfo, FileInfo);
        }
        if (!NT_SUCCESS(RC)) {
            // Rename failed — restore original state if target was not
            // actually deleted on disk.
            if (StaleLcb && !(StaleFcb && (StaleFcb->FcbState & UDF_FCB_DELETED))) {
                ClearFlag(StaleLcb->Flags, UDF_LCB_FLAG_LINK_DELETED);
                // Re-insert into splay trees (was removed before rename attempt)
                if (StaleLcb->ParentFcb) {
                    UdfInsertNameLinks(StaleLcb->ParentFcb, StaleLcb);
                }
            }
            try_return (RC);
        }

        //
        // Rename succeeded — set NeedRemovePrefix so the finally
        // handler removes the stale LCB from queues.
        //
        if (StaleLcb) {
            NeedRemovePrefix = TRUE;
            if (StaleFcb) {
                StaleFcb->FcbState |= UDF_FCB_DELETED;
            }
        }

        ASSERT(UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex, FileInfo->Index)->FileInfo == FileInfo);

        {
            ULONG Filter = UDFIsADirectory(FileInfo)
                         ? FILE_NOTIFY_CHANGE_DIR_NAME
                         : FILE_NOTIFY_CHANGE_FILE_NAME;

            // Step 1: Notify OLD location (LCB still points to old parent)
            // Use FileObject->FileName — stable path independent of LCB chain
            if (SingleDir && !ReplaceIfExists) {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb, Filter, FILE_ACTION_RENAMED_OLD_NAME,
                                      Ccb->Lcb, FileObject);
            } else {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb, Filter, FILE_ACTION_REMOVED,
                                      Ccb->Lcb, FileObject);
            }

            // Step 2: Move LCB to new parent and update name
            if (Ccb->Lcb) {
                RC = UDFRenameMovePrefix(IrpContext, Ccb->Lcb, &NewName,
                                         SingleDir ? NULL : TargetDirInfo->Fcb);
                if (!NT_SUCCESS(RC)) {
                    try_return (RC);
                }
            }

            // Step 3: Update FCB parent pointer for cross-directory rename
            if (!SingleDir) {
                Fcb->ParentFcb = TargetDirInfo->Fcb;
            }

            // Step 4: Notify NEW location (LCB now points to new parent)
            // After UDFRenameMovePrefix, LCB has new name — pass it explicitly
            if (SingleDir && !ReplaceIfExists) {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb, Filter, FILE_ACTION_RENAMED_NEW_NAME,
                                      Ccb->Lcb, NULL);
            } else if (ReplaceIfExists) {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb,
                    FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                    FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
                    FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_EA,
                    FILE_ACTION_MODIFIED, Ccb->Lcb, NULL);
            } else {
                UDFNotifyReportChange(IrpContext, Vcb, Fcb, Filter, FILE_ACTION_ADDED,
                                      Ccb->Lcb, NULL);
            }
        }

        ASSERT(TargetDirInfo->RefCount || SingleDir);

        RC = STATUS_SUCCESS;

try_exit:    NOTHING;

    } _SEH2_FINALLY {

        //
        // Remove stale target LCB from queues (deferred here from
        // success path). NeedRemovePrefix is only set after rename
        // succeeded, so StaleFcb is already marked UDF_FCB_DELETED.
        // Guard against concurrent teardown that may have already
        // removed this LCB by checking ParentFcbLinks.
        //
        if (NeedRemovePrefix) {
            if (StaleLcb->ParentFcbLinks.Flink != &StaleLcb->ParentFcbLinks) {
                PFCB StaleParentFcb = StaleLcb->ParentFcb;

                UDFRemovePrefix(IrpContext, StaleLcb);
                // UDFAcquirePrefix holds one FCB and FileInfo reference on
                // the parent for every LCB it creates. Teardown normally
                // releases both; replacing a target removes its LCB here.
                if (StaleParentFcb) {
                    if (StaleParentFcb->FileInfo) {
                        UDFCloseFile__(IrpContext, Vcb, StaleParentFcb->FileInfo);
                    }
                    ASSERT(StaleParentFcb->FcbReference > 0);
                    InterlockedDecrement((PLONG)&StaleParentFcb->FcbReference);
                }
            }
            if (StaleFcb) {
                UDFLockFcbTable(IrpContext, Vcb);
                UDFLockVcb(IrpContext, Vcb);
                {
                    struct { FILE_ID FileId; PFCB Fcb; } _Key;
                    _Key.FileId = StaleFcb->FileId;
                    RtlDeleteElementGenericTable(&Vcb->FcbTable, &_Key);
                }
                UDFUnlockVcb(IrpContext, Vcb);
                UDFUnlockFcbTable(IrpContext, Vcb);
            }
        }
        if (StaleFcbAcquired) {
            UDFReleaseFcb(IrpContext, StaleFcb);
        }
        if (TargetParentFcbAcquired) {
            UDFReleaseFcb(IrpContext, TargetDirInfo->Fcb);
        }
        if (ParentFcbAcquired) {
            UDFReleaseFcb(IrpContext, DirInfo->Fcb);
        }
        if (SourceFcbAcquired) {
            UDFReleaseFcb(IrpContext, Fcb);
        }
        if (LocalPath.Buffer) {
            MyFreePool__(LocalPath.Buffer);
        }
    } _SEH2_END;

    return RC;
} // end UDFRename()

LONG
UDFFindFileId(
    IN PVCB Vcb,
    IN FILE_ID Id
    )
{
    if (!Vcb->FileIdCache) return (-1);
    for(ULONG i=0; i<Vcb->FileIdCount; i++) {
        if (Vcb->FileIdCache[i].Id.QuadPart == Id.QuadPart) return i;
    }
    return (-1);
} // end UDFFindFileId()

LONG
UDFFindFreeFileId(
    IN PVCB Vcb,
    IN FILE_ID FileId
    )
{
    if (!Vcb->FileIdCache) {
        if (!(Vcb->FileIdCache = (PUDFFileIDCacheItem)MyAllocatePool__(NonPagedPool, sizeof(UDFFileIDCacheItem)*FILE_ID_CACHE_GRANULARITY)))
            return (-1);
        RtlZeroMemory(Vcb->FileIdCache, FILE_ID_CACHE_GRANULARITY*sizeof(UDFFileIDCacheItem));
        Vcb->FileIdCount = FILE_ID_CACHE_GRANULARITY;
    }
    for(ULONG i=0; i<Vcb->FileIdCount; i++) {
        if (!Vcb->FileIdCache[i].FullName.Buffer) return i;
    }
    if (!MyReallocPool__((PCHAR)(Vcb->FileIdCache), Vcb->FileIdCount*sizeof(UDFFileIDCacheItem),
                     (PCHAR*)&(Vcb->FileIdCache), (Vcb->FileIdCount+FILE_ID_CACHE_GRANULARITY)*sizeof(UDFFileIDCacheItem))) {
        return (-1);
    }
    RtlZeroMemory(&(Vcb->FileIdCache[Vcb->FileIdCount]), FILE_ID_CACHE_GRANULARITY*sizeof(UDFFileIDCacheItem));
    Vcb->FileIdCount += FILE_ID_CACHE_GRANULARITY;
    return (Vcb->FileIdCount - FILE_ID_CACHE_GRANULARITY);
} // end UDFFindFreeFileId()

NTSTATUS
UDFStoreFileId(
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN PUDF_FILE_INFO fi,
    IN FILE_ID FileId
    )
{
    LONG i;
    NTSTATUS RC = STATUS_SUCCESS;

    // NOTE: This function appears to be unused legacy code (no callers found).
    // TODO: If ever used, need to add IrpContext parameter and use UDFBuildFullPathFromLcb
    // to build the path from LCB instead of FCBName (which no longer exists).

    if ((i = UDFFindFileId(Vcb, FileId)) == (-1)) {
        if ((i = UDFFindFreeFileId(Vcb, FileId)) == (-1)) return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        return STATUS_SUCCESS;
    }
    Vcb->FileIdCache[i].Id = FileId;
    Vcb->FileIdCache[i].IgnoreCase = BooleanFlagOn(Ccb->Flags, CCB_FLAG_IGNORE_CASE);

    // TODO: Build path from LCB using UDFBuildFullPathFromLcb (requires IrpContext)
    // For now, return error if this function is ever called
    RC = STATUS_NOT_IMPLEMENTED;

    return RC;
} // end UDFStoreFileId()

NTSTATUS
UDFRemoveFileId(
    IN PVCB Vcb,
    IN FILE_ID FileId
    )
{
    LONG i;

    if ((i = UDFFindFileId(Vcb, FileId)) == (-1)) return STATUS_INVALID_PARAMETER;
    MyFreePool__(Vcb->FileIdCache[i].FullName.Buffer);
    RtlZeroMemory(&(Vcb->FileIdCache[i]), sizeof(UDFFileIDCacheItem));
    return STATUS_SUCCESS;
} // end UDFRemoveFileId()

VOID
UDFReleaseFileIdCache(
    IN PVCB Vcb
    )
{
    if (!Vcb->FileIdCache) return;
    for(ULONG i=0; i<Vcb->FileIdCount; i++) {
        if (Vcb->FileIdCache[i].FullName.Buffer) {
            MyFreePool__(Vcb->FileIdCache[i].FullName.Buffer);
        }
    }
    MyFreePool__(Vcb->FileIdCache);
    Vcb->FileIdCache = NULL;
    Vcb->FileIdCount = 0;
} // end UDFReleaseFileIdCache()

NTSTATUS
UDFGetOpenParamsByFileId(
    IN PVCB Vcb,
    IN FILE_ID FileId,
    OUT PUNICODE_STRING* FName,
    OUT BOOLEAN* IgnoreCase
    )
{
    LONG i;

    if ((i = UDFFindFileId(Vcb, FileId)) == (-1)) return STATUS_NOT_FOUND;
    (*FName) = &(Vcb->FileIdCache[i].FullName);
    (*IgnoreCase) = (Vcb->FileIdCache[i].IgnoreCase);
    return STATUS_SUCCESS;
} // end UDFGetOpenParamsByFileId()

#ifdef UDF_ALLOW_HARD_LINKS
/*
    create hard link for the file
 */
NTSTATUS
UDFHardLink(
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb1,
    IN PCCB Ccb1,
    IN PFILE_OBJECT FileObject1,   // Source File
    IN PFILE_LINK_INFORMATION PtrBuffer
    )
{
    // Target Directory
    PFILE_OBJECT DirObject2 = IrpSp->Parameters.SetFile.FileObject;
    // Overwite Flag
    BOOLEAN Replace = IrpSp->Parameters.SetFile.ReplaceIfExists &&
                      PtrBuffer->ReplaceIfExists;
    NTSTATUS RC;
    PVCB Vcb = Fcb1->Vcb;
    PFCB Fcb2;
    BOOLEAN IgnoreCase;
    BOOLEAN AcquiredDir1 = FALSE;
    BOOLEAN AcquiredFcb1 = FALSE;
    BOOLEAN SingleDir = TRUE;

    PUDF_FILE_INFO File1;
    PUDF_FILE_INFO Dir1 = NULL;
    PUDF_FILE_INFO Dir2;

    UNICODE_STRING NewName;
    UNICODE_STRING LocalPath;
//    PtrUDFCCB CurCcb = NULL;

    AdPrint(("UDFHardLink\n"));

    LocalPath.Buffer = NULL;

    _SEH2_TRY {

        // do we try to link Volume ?
        if (!(File1 = Fcb1->FileInfo))
            try_return (RC = STATUS_ACCESS_DENIED);

        // do we try to link RootDir ?
        if (!(Dir1 = File1->ParentFile))
            try_return (RC = STATUS_ACCESS_DENIED);

        // do we try to link Stream / Stream Dir ?
#ifdef UDF_ALLOW_LINKS_TO_STREAMS
        if (UDFIsAStreamDir(File1))
            try_return (RC = STATUS_ACCESS_DENIED);
#else //UDF_ALLOW_LINKS_TO_STREAMS
        if (UDFIsAStream(File1) || UDFIsAStreamDir(File1) /*||
           UDFIsADirectory(File1) || UDFHasAStreamDir(File1)*/)
            try_return (RC = STATUS_ACCESS_DENIED);
#endif // UDF_ALLOW_LINKS_TO_STREAMS

        // do we try to link to RootDir or Volume ?
        if (!DirObject2) {
            Dir2 = File1->ParentFile;
            DirObject2 = FileObject1->RelatedFileObject;
        } else
        if (DirObject2->FsContext2 &&
          (Fcb2 = ((PCCB)(DirObject2->FsContext2))->Fcb)) {
            Dir2 = ((PCCB)(DirObject2->FsContext2))->Fcb->FileInfo;
        } else {
            try_return (RC = STATUS_INVALID_PARAMETER);
        }

        // check target dir
        if (!Dir2) try_return (RC = STATUS_ACCESS_DENIED);

        // Stream can't be a Dir or have Streams
        if (UDFIsAStreamDir(Dir2)) {
            try_return (RC = STATUS_ACCESS_DENIED);
/*            if (UDFIsADirectory(File1) ||
               UDFHasAStreamDir(File1)) {
                BrutePoint();
                try_return (RC = STATUS_ACCESS_DENIED);
            }*/
        }

/*        if (UDFIsAStreamDir(Dir2))
            try_return (RC = STATUS_ACCESS_DENIED);*/

        RC = UDFPrepareForRenameMoveLink(IrpContext, Vcb,
                                         &SingleDir,
                                         &AcquiredDir1, &AcquiredFcb1,
                                         Ccb1, File1,
                                         Dir1, Dir2,
                                         TRUE); // it is HLink operation
        if (!NT_SUCCESS(RC))
            try_return(RC);

        // check if the source file is used
        if (!DirObject2) {
            //  Make sure the name is of legal length.
            if (PtrBuffer->FileNameLength > UDF_NAME_LEN*sizeof(WCHAR)) {
                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }
            NewName.Length = NewName.MaximumLength = (USHORT)(PtrBuffer->FileNameLength);
            NewName.Buffer = (PWCHAR)&(PtrBuffer->FileName);
        } else {
            // After OpenTargetDirectory, FileName is split:
            //   Length = parent directory path (trimmed)
            //   MaximumLength = full original path (including target name)
            USHORT FullLength = DirObject2->FileName.MaximumLength;
            USHORT ParentLength = DirObject2->FileName.Length;
            PWCHAR Buffer = DirObject2->FileName.Buffer;
            USHORT FileNameStart;

            if (ParentLength < FullLength &&
                Buffer[ParentLength / sizeof(WCHAR)] == L'\\') {
                FileNameStart = ParentLength + sizeof(WCHAR);
            } else {
                FileNameStart = ParentLength;
            }

            NewName.Length = FullLength - FileNameStart;
            NewName.MaximumLength = NewName.Length;
            NewName.Buffer = (PWCHAR)((PCHAR)Buffer + FileNameStart);
        }

        IgnoreCase = FlagOn(Ccb1->Flags, CCB_FLAG_IGNORE_CASE);

        // Note: Fcb1 no longer has FCBName - removed debug print

        RC = UDFHardLinkFile__(IrpContext, Vcb, IgnoreCase, &Replace, &NewName, Dir1, Dir2, File1);
        if (!NT_SUCCESS(RC)) try_return (RC);

        // Update Parent Objects (mark 'em as modified)
        if (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) {
            if (DirObject2) {
                DirObject2->Flags |= FO_FILE_MODIFIED;
                if (!Replace)
                    DirObject2->Flags |= FO_FILE_SIZE_CHANGED;
            }
        }
        // report changes
        UDFNotifyReportChange( IrpContext, Vcb, File1->Fcb,
                                   FILE_NOTIFY_CHANGE_LAST_WRITE |
                                   FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                   FILE_ACTION_MODIFIED,
                                   Ccb1->Lcb, FileObject1);

        // Build full path for hardlink target
        if (Dir2->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY) {
            RC = MyCloneUnicodeString(&LocalPath, &UdfData.UnicodeStrRoot);
            if (!NT_SUCCESS(RC)) try_return (RC);
        } else {
            // Build parent path from LCB
            PLCB Dir2Lcb = NULL;
            if (!IsListEmpty(&Dir2->Fcb->ParentLcbQueue)) {
                PLIST_ENTRY ListEntry = Dir2->Fcb->ParentLcbQueue.Flink;
                Dir2Lcb = CONTAINING_RECORD(ListEntry, LCB, ChildFcbLinks);
                RC = UDFBuildFullPathFromLcb(IrpContext, Dir2Lcb, &LocalPath, FALSE);
                if (!NT_SUCCESS(RC)) try_return (RC);
            } else {
                RC = MyCloneUnicodeString(&LocalPath, &UdfData.UnicodeStrRoot);
                if (!NT_SUCCESS(RC)) try_return (RC);
            }
        }

        // Append separator and new name
        if (Dir2->ParentFile) {
            RC = MyAppendUnicodeToString(&LocalPath, L"\\");
            if (!NT_SUCCESS(RC)) try_return (RC);
        }
        RC = MyAppendUnicodeStringToStringTag(&LocalPath, &NewName, MEM_USHL_TAG);
        if (!NT_SUCCESS(RC)) try_return (RC);

        // Calculate TargetNameOffset (parent path length)
        USHORT HardLinkTargetNameOffset = (USHORT)(LocalPath.Length - NewName.Length);
        if (Dir2->Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY) {
            HardLinkTargetNameOffset = 0;
        }

        if (!Replace) {
/*          UDFNotifyFullReportChange( Vcb, File2,
                                       UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_ADDED );*/
            FsRtlNotifyFullReportChange( Vcb->NotifySync, &(Vcb->NextNotifyIRP),
                                         (PSTRING)&LocalPath,
                                         HardLinkTargetNameOffset,
                                         NULL,NULL,
                                         UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                         FILE_ACTION_ADDED,
                                         NULL);
        } else {
/*          UDFNotifyFullReportChange( Vcb, File2,
                                       FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                       FILE_NOTIFY_CHANGE_SIZE |
                                       FILE_NOTIFY_CHANGE_LAST_WRITE |
                                       FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                       FILE_NOTIFY_CHANGE_CREATION |
                                       FILE_NOTIFY_CHANGE_EA,
                                       FILE_ACTION_MODIFIED );*/
            FsRtlNotifyFullReportChange( Vcb->NotifySync, &(Vcb->NextNotifyIRP),
                                         (PSTRING)&LocalPath,
                                         HardLinkTargetNameOffset,
                                         NULL,NULL,
                                         UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                         FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                             FILE_NOTIFY_CHANGE_SIZE |
                                             FILE_NOTIFY_CHANGE_LAST_WRITE |
                                             FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                             FILE_NOTIFY_CHANGE_CREATION |
                                             FILE_NOTIFY_CHANGE_EA,
                                         NULL);
        }

        RC = STATUS_SUCCESS;

try_exit:    NOTHING;

    } _SEH2_FINALLY {

        // Release in reverse order of acquisition (parent first, then child)
        if (AcquiredDir1) {
            UDFReleaseFcb(IrpContext, Dir1->Fcb);
        }
        if (AcquiredFcb1) {
            UDFReleaseFcb(IrpContext, Fcb1);
        }

        if (LocalPath.Buffer) {
            MyFreePool__(LocalPath.Buffer);
        }
    } _SEH2_END;

    return RC;
} // end UDFHardLink()
#endif //UDF_ALLOW_HARD_LINKS
