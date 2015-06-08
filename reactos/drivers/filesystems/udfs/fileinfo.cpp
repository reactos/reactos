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
* Function: UDFFileInfo()
*
* Description:
*   The I/O Manager will invoke this routine to handle a set/query file
*   information request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*   to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFileInfo(
    PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    PIRP           Irp                 // I/O Request Packet
    )
{
    NTSTATUS         RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;
    BOOLEAN          AreWeTopLevel = FALSE;

    TmPrint(("UDFFileInfo: \n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);
    ASSERT(!UDFIsFSDevObj(DeviceObject));

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonFileInfo(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if(AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFFileInfo()


/*************************************************************************
*
* Function: UDFCommonFileInfo()
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
UDFCommonFileInfo(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp = NULL;
    PFILE_OBJECT            FileObject = NULL;
    PtrUDFFCB               Fcb = NULL;
    PtrUDFCCB               Ccb = NULL;
    PVCB                    Vcb = NULL;
    PtrUDFNTRequiredFCB     NtReqFcb = NULL;
    BOOLEAN                 MainResourceAcquired = FALSE;
    BOOLEAN                 ParentResourceAcquired = FALSE;
    BOOLEAN                 PagingIoResourceAcquired = FALSE;
    PVOID                   PtrSystemBuffer = NULL;
    LONG                    BufferLength = 0;
    FILE_INFORMATION_CLASS  FunctionalityRequested;
    BOOLEAN                 CanWait = FALSE;
    BOOLEAN                 PostRequest = FALSE;
    BOOLEAN                 AcquiredVcb = FALSE;
    PIRP                    TopIrp;

    TmPrint(("UDFCommonFileInfo: irp %x\n", Irp));

        TopIrp = IoGetTopLevelIrp();
        switch((ULONG)TopIrp) {
        case FSRTL_FSP_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_FSP_TOP_LEVEL_IRP\n"));
            break;
        case FSRTL_CACHE_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_CACHE_TOP_LEVEL_IRP\n"));
            break;
        case FSRTL_MOD_WRITE_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_MOD_WRITE_TOP_LEVEL_IRP\n"));
            break;
        case FSRTL_FAST_IO_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_FAST_IO_TOP_LEVEL_IRP\n"));
            BrutePoint()
            break;
        case NULL:
            KdPrint(("  NULL TOP_LEVEL_IRP\n"));
            break;
        default:
            if(TopIrp == Irp) {
                KdPrint(("  TOP_LEVEL_IRP\n"));
            } else {
                KdPrint(("  RECURSIVE_IRP, TOP = %x\n", TopIrp));
            }
        }

    _SEH2_TRY {
        // First, get a pointer to the current I/O stack location.
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        // Get the FCB and CCB pointers.
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        if(!Ccb) {
            // some applications sends us FO without Ccb
            // This is not allowed...
            RC = STATUS_INVALID_PARAMETER;
            try_return(RC);
        }
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);

        NtReqFcb = Fcb->NTRequiredFCB;

        CanWait = (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE;

        // If the caller has opened a logical volume and is attempting to
        // query information for it as a file stream, return an error.
        if(Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) {
            // This is not allowed. Caller must use get/set volume information instead.
            RC = STATUS_INVALID_PARAMETER;
            try_return(RC);
        }


        Vcb = (PVCB)(IrpSp->DeviceObject->DeviceExtension);
        ASSERT(Vcb);
        ASSERT(Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_FCB);
        //Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

#ifdef EVALUATION_TIME_LIMIT
        if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
            Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
        }
#endif //EVALUATION_TIME_LIMIT

        // The NT I/O Manager always allocates and supplies a system
        // buffer for query and set file information calls.
        // Copying information to/from the user buffer and the system
        // buffer is performed by the I/O Manager and the FSD need not worry about it.
        PtrSystemBuffer = Irp->AssociatedIrp.SystemBuffer;   

        UDFFlushTryBreak(Vcb);
        if(!UDFAcquireResourceShared(&(Vcb->VCBResource), CanWait)) {
            PostRequest = TRUE;
            try_return(RC = STATUS_PENDING);
        }
        AcquiredVcb = TRUE;

        if(IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) {
            // Now, obtain some parameters.
            BufferLength = IrpSp->Parameters.QueryFile.Length;
            FunctionalityRequested = IrpSp->Parameters.QueryFile.FileInformationClass;
#ifdef UDF_ENABLE_SECURITY
            RC = IoCheckFunctionAccess(
                Ccb->PreviouslyGrantedAccess,
                PtrIrpContext->MajorFunction,
                PtrIrpContext->MinorFunction,
                0,
                &FunctionalityRequested,
                NULL);
            if(!NT_SUCCESS(RC)) {
                try_return(RC);
            }
#endif //UDF_ENABLE_SECURITY
            // Acquire the MainResource shared (NOTE: for paging-IO on a
            // page file, we should avoid acquiring any resources and simply
            // trust the VMM to do the right thing, else we could possibly
            // run into deadlocks).
            if(!(Fcb->FCBFlags & UDF_FCB_PAGE_FILE)) {
                // Acquire the MainResource shared.
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                UDFAcquireResourceShared(&(NtReqFcb->MainResource), TRUE);
                MainResourceAcquired = TRUE;
            }

            // Do whatever the caller asked us to do
            switch (FunctionalityRequested) {
            case FileBasicInformation:
                RC = UDFGetBasicInformation(FileObject, Fcb, (PFILE_BASIC_INFORMATION)PtrSystemBuffer, &BufferLength);
                break;
            case FileStandardInformation:
                RC = UDFGetStandardInformation(Fcb, (PFILE_STANDARD_INFORMATION) PtrSystemBuffer, &BufferLength);
                break;
#if(_WIN32_WINNT >= 0x0400)
            case FileNetworkOpenInformation:
                 RC = UDFGetNetworkInformation(Fcb, (PFILE_NETWORK_OPEN_INFORMATION)PtrSystemBuffer, &BufferLength);
                break;
#endif  // _WIN32_WINNT >= 0x0400
            case FileInternalInformation:
                RC = UDFGetInternalInformation(PtrIrpContext, Fcb, Ccb, (PFILE_INTERNAL_INFORMATION) PtrSystemBuffer, &BufferLength);
                break;
            case FileEaInformation:
                RC = UDFGetEaInformation(PtrIrpContext, Fcb, (PFILE_EA_INFORMATION) PtrSystemBuffer, &BufferLength);
                break;
            case FileNameInformation:
                RC = UDFGetFullNameInformation(FileObject, (PFILE_NAME_INFORMATION) PtrSystemBuffer, &BufferLength);
                break;
            case FileAlternateNameInformation:
                RC = UDFGetAltNameInformation(Fcb, (PFILE_NAME_INFORMATION) PtrSystemBuffer, &BufferLength);
                break;
//            case FileCompressionInformation:
//                // RC = UDFGetCompressionInformation(...);
//                break;
            case FilePositionInformation:
                RC = UDFGetPositionInformation(FileObject, (PFILE_POSITION_INFORMATION)PtrSystemBuffer, &BufferLength);
                break;
            case FileStreamInformation:
                RC = UDFGetFileStreamInformation(Fcb, (PFILE_STREAM_INFORMATION) PtrSystemBuffer, &BufferLength);
                break;
            case FileAllInformation:
                // The I/O Manager supplies the Mode, Access, and Alignment
                // information. The rest is up to us to provide.
                // Therefore, decrement the BufferLength appropriately (assuming
                // that the above 3 types on information are already in the
                // buffer)
                {
                    PFILE_ALL_INFORMATION PtrAllInfo = (PFILE_ALL_INFORMATION)PtrSystemBuffer;

                    BufferLength -= (sizeof(FILE_MODE_INFORMATION) +
                                     sizeof(FILE_ACCESS_INFORMATION) +
                                     sizeof(FILE_ALIGNMENT_INFORMATION));

                    // Get the remaining stuff.
                    if(!NT_SUCCESS(RC = UDFGetBasicInformation(FileObject, Fcb, &(PtrAllInfo->BasicInformation), &BufferLength)) ||
                       !NT_SUCCESS(RC = UDFGetStandardInformation(Fcb, &(PtrAllInfo->StandardInformation), &BufferLength)) ||
                       !NT_SUCCESS(RC = UDFGetInternalInformation(PtrIrpContext, Fcb, Ccb, &(PtrAllInfo->InternalInformation), &BufferLength)) ||
                       !NT_SUCCESS(RC = UDFGetEaInformation(PtrIrpContext, Fcb, &(PtrAllInfo->EaInformation), &BufferLength)) ||
                       !NT_SUCCESS(RC = UDFGetPositionInformation(FileObject, &(PtrAllInfo->PositionInformation), &BufferLength)) || 
                       !NT_SUCCESS(RC = UDFGetFullNameInformation(FileObject, &(PtrAllInfo->NameInformation), &BufferLength))
                        )
                        try_return(RC);
                }
                break;
            default:
                RC = STATUS_INVALID_PARAMETER;
                try_return(RC);
            }

#ifndef UDF_READ_ONLY_BUILD
        } else {
//      if(IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
            ASSERT(IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION);
            // Now, obtain some parameters.
            FunctionalityRequested = IrpSp->Parameters.SetFile.FileInformationClass;
            if((Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) &&
               (FunctionalityRequested != FilePositionInformation)) {
                try_return(RC = STATUS_ACCESS_DENIED);
            }
#ifdef UDF_ENABLE_SECURITY
            RC = IoCheckFunctionAccess(
                Ccb->PreviouslyGrantedAccess,
                PtrIrpContext->MajorFunction,
                PtrIrpContext->MinorFunction,
                0,
                &FunctionalityRequested,
                NULL);
            if(!NT_SUCCESS(RC)) {
                try_return(RC);
            }
#endif //UDF_ENABLE_SECURITY
            //  If the FSD supports opportunistic locking,
            // then we should check whether the oplock state
            // allows the caller to proceed.

            // Rename, and link operations require creation of a directory
            // entry and possibly deletion of another directory entry.

            // Unless this is an operation on a page file, we should go ahead and
            // acquire the FCB exclusively at this time. Note that we will pretty
            // much block out anything being done to the FCB from this point on.
            if(!(Fcb->FCBFlags & UDF_FCB_PAGE_FILE) &&
                (FunctionalityRequested != FilePositionInformation) &&
                (FunctionalityRequested != FileRenameInformation) &&
                (FunctionalityRequested != FileLinkInformation)) {
                // Acquire the Parent & Main Resources exclusive.
                if(Fcb->FileInfo->ParentFile) {
                    UDF_CHECK_PAGING_IO_RESOURCE(Fcb->ParentFcb->NTRequiredFCB);
                    if(!UDFAcquireResourceExclusive(&(Fcb->ParentFcb->NTRequiredFCB->MainResource), CanWait)) {
                        PostRequest = TRUE;
                        try_return(RC = STATUS_PENDING);
                    }
                    ParentResourceAcquired = TRUE;
                }
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                if(!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
                    PostRequest = TRUE;
                    try_return(RC = STATUS_PENDING);
                }
                MainResourceAcquired = TRUE;
            } else
            // The only operations that could conceivably proceed from this point
            // on are paging-IO read/write operations. For delete, link (rename),
            // set allocation size, and set EOF, should also acquire the paging-IO
            // resource, thereby synchronizing with paging-IO requests.
            if((Fcb->FCBFlags & UDF_FCB_PAGE_FILE) &&
               ((FunctionalityRequested == FileDispositionInformation) ||
                (FunctionalityRequested == FileAllocationInformation) ||
                (FunctionalityRequested == FileEndOfFileInformation)) ) {

                // Acquire the MainResource shared.
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                if(!UDFAcquireResourceShared(&(NtReqFcb->MainResource), CanWait)) {
                    PostRequest = TRUE;
                    try_return(RC = STATUS_PENDING);
                }
                MainResourceAcquired = TRUE;
                // Acquire the PagingResource exclusive.
                if(!UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), CanWait)) {
                    PostRequest = TRUE;
                    try_return(RC = STATUS_PENDING);
                }
                PagingIoResourceAcquired = TRUE;
            } else if((FunctionalityRequested != FileRenameInformation) &&
                      (FunctionalityRequested != FileLinkInformation)) {
                // Acquire the MainResource shared.
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                if(!UDFAcquireResourceShared(&(NtReqFcb->MainResource), CanWait)) {
                    PostRequest = TRUE;
                    try_return(RC = STATUS_PENDING);
                }
                MainResourceAcquired = TRUE;
            }

            if((Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
               (FunctionalityRequested != FilePositionInformation)) {
                AdPrint(("    Can't change File Information on blank volume ;)\n"));
                try_return(RC = STATUS_ACCESS_DENIED);
            }

            // Do whatever the caller asked us to do
            switch (FunctionalityRequested) {
            case FileBasicInformation:
                RC = UDFSetBasicInformation(Fcb, Ccb, FileObject, (PFILE_BASIC_INFORMATION)PtrSystemBuffer);
                break;
            case FilePositionInformation: {
                // Check if no intermediate buffering has been specified.
                // If it was specified, do not allow non-aligned set file
                // position requests to succeed.
                PFILE_POSITION_INFORMATION       PtrFileInfoBuffer;

                PtrFileInfoBuffer = (PFILE_POSITION_INFORMATION)PtrSystemBuffer;

                if(FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
                    if(PtrFileInfoBuffer->CurrentByteOffset.LowPart & IrpSp->DeviceObject->AlignmentRequirement) {
                        // Invalid alignment.
                        try_return(RC = STATUS_INVALID_PARAMETER);
                    }
                }

                FileObject->CurrentByteOffset = PtrFileInfoBuffer->CurrentByteOffset;
                break;
            }
            case FileDispositionInformation:
                RC = UDFSetDispositionInformation(Fcb, Ccb, Vcb, FileObject,
                            ((PFILE_DISPOSITION_INFORMATION)PtrSystemBuffer)->DeleteFile ? TRUE : FALSE);
                break;
            case FileRenameInformation:
                if(!CanWait) {
                    PostRequest = TRUE;
                    try_return(RC = STATUS_PENDING);
                }
                RC = UDFRename(IrpSp, Fcb, Ccb, FileObject, (PFILE_RENAME_INFORMATION)PtrSystemBuffer);
                if(RC == STATUS_PENDING) {
                    PostRequest = TRUE;
                    try_return(RC);
                }
                break;
#ifdef UDF_ALLOW_HARD_LINKS
            case FileLinkInformation:
                if(!CanWait) {
                    PostRequest = TRUE;
                    try_return(RC = STATUS_PENDING);
                }
                RC = UDFHardLink(IrpSp, Fcb, Ccb, FileObject, (PFILE_LINK_INFORMATION)PtrSystemBuffer);
                break;
#endif //UDF_ALLOW_HARD_LINKS
            case FileAllocationInformation:
                RC = UDFSetAllocationInformation(Fcb, Ccb, Vcb, FileObject,
                                                  PtrIrpContext, Irp,
                                                  (PFILE_ALLOCATION_INFORMATION)PtrSystemBuffer);
                break;
            case FileEndOfFileInformation:
                RC = UDFSetEOF(IrpSp, Fcb, Ccb, Vcb, FileObject, Irp, (PFILE_END_OF_FILE_INFORMATION)PtrSystemBuffer);
                break;
            default:
                RC = STATUS_INVALID_PARAMETER;
                try_return(RC);
            }
#endif //UDF_READ_ONLY_BUILD
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if(PagingIoResourceAcquired) {
            UDFReleaseResource(&(NtReqFcb->PagingIoResource));
            PagingIoResourceAcquired = FALSE;
        }

        if(MainResourceAcquired) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            MainResourceAcquired = FALSE;
        }

        if(ParentResourceAcquired) {
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb->ParentFcb->NTRequiredFCB);
            UDFReleaseResource(&(Fcb->ParentFcb->NTRequiredFCB->MainResource));
            ParentResourceAcquired = FALSE;
        }

        // Post IRP if required
        if(PostRequest) {

            // Since, the I/O Manager gave us a system buffer, we do not
            // need to "lock" anything.

            // Perform the post operation which will mark the IRP pending
            // and will return STATUS_PENDING back to us
            RC = UDFPostRequest(PtrIrpContext, Irp);

        } else {

            if (!_SEH2_AbnormalTermination()) {
                Irp->IoStatus.Status = RC;
                // Set status for "query" requests
                if(IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) {
                    // Return the amount of information transferred.
                    Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - BufferLength;
#ifndef UDF_READ_ONLY_BUILD
#ifdef UDF_DELAYED_CLOSE
                } else
                if(NT_SUCCESS(RC)) {
                    if(FunctionalityRequested == FileDispositionInformation) {
                        if(AcquiredVcb) {
                            AcquiredVcb = FALSE;
                            UDFReleaseResource(&(Vcb->VCBResource));
                        }
                        UDFRemoveFromDelayedQueue(Fcb);
                    }
#endif //UDF_DELAYED_CLOSE
#endif //UDF_READ_ONLY_BUILD
                }
                // complete the IRP
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                // Free up the Irp Context
                UDFReleaseIrpContext(PtrIrpContext);
            } // can we complete the IRP ?

        }
        if(AcquiredVcb) {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
    } _SEH2_END;// end of "__finally" processing

    return(RC);
} // end UDFCommonFileInfo()

/*
    Return some time-stamps and file attributes to the caller.
 */
NTSTATUS
UDFGetBasicInformation(
    IN PFILE_OBJECT                FileObject,
    IN PtrUDFFCB                   Fcb,
    IN PFILE_BASIC_INFORMATION     PtrBuffer,
 IN OUT LONG*                      PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;
    PDIR_INDEX_ITEM     DirNdx;

    AdPrint(("UDFGetBasicInformation: \n"));

    _SEH2_TRY {

        if(*PtrReturnedLength < sizeof(FILE_BASIC_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_BASIC_INFORMATION));

        // Get information from the FCB and update TimesCache in DirIndex
        FileInfo = Fcb->FileInfo;

        if(!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! GetBasicInfo to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        
        DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);

        PtrBuffer->CreationTime = Fcb->NTRequiredFCB->CreationTime;
        DirNdx->CreationTime = PtrBuffer->CreationTime.QuadPart;

        PtrBuffer->LastAccessTime = Fcb->NTRequiredFCB->LastAccessTime;
        DirNdx->LastAccessTime = PtrBuffer->LastAccessTime.QuadPart;

        PtrBuffer->LastWriteTime = Fcb->NTRequiredFCB->LastWriteTime;
        DirNdx->LastWriteTime = PtrBuffer->LastWriteTime.QuadPart;

        PtrBuffer->ChangeTime = Fcb->NTRequiredFCB->ChangeTime;
        DirNdx->ChangeTime = PtrBuffer->ChangeTime.QuadPart;

        // Now fill in the attributes.
        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
#ifdef UDF_DBG
            if(!FileInfo->Dloc->DirIndex) AdPrint(("*****!!!!! Directory has no DirIndex !!!!!*****\n"));
#endif
        }
        // Similarly, fill in attributes indicating a hidden file, system
        // file, compressed file, temporary file, etc. if the FSD supports
        // such file attribute values.
        PtrBuffer->FileAttributes |= UDFAttributesToNT(DirNdx,NULL);
        if(FileObject->Flags & FO_TEMPORARY_FILE) {
            PtrBuffer->FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
        } else {
            PtrBuffer->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;
        }
        if(!PtrBuffer->FileAttributes) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(NT_SUCCESS(RC)) {
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
    IN PtrUDFFCB                   Fcb,
    IN PFILE_STANDARD_INFORMATION  PtrBuffer,
 IN OUT LONG*                      PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;
    PVCB Vcb;

    AdPrint(("UDFGetStandardInformation: \n"));

    _SEH2_TRY {

        if(*PtrReturnedLength < sizeof(FILE_STANDARD_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_STANDARD_INFORMATION));

        FileInfo = Fcb->FileInfo;

        if(!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! GetStandardInfo to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        Vcb = Fcb->Vcb;
        PtrBuffer->NumberOfLinks = UDFGetFileLinkCount(FileInfo);
        PtrBuffer->DeletePending = (Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE) ? TRUE : FALSE;
#ifdef EVALUATION_TIME_LIMIT
        if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
            Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
        }
#endif //EVALUATION_TIME_LIMIT
        //  Case on whether this is a file or a directory, and extract
        //  the information and fill in the fcb/dcb specific parts
        //  of the output buffer
        if(UDFIsADirectory(Fcb->FileInfo)) {
            PtrBuffer->Directory = TRUE;
        } else {
            if(Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize.LowPart == 0xffffffff) {
                Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize.QuadPart =
                    UDFSysGetAllocSize(Fcb->Vcb, UDFGetFileSize(FileInfo));
            }
            PtrBuffer->AllocationSize = Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize;
            PtrBuffer->EndOfFile = Fcb->NTRequiredFCB->CommonFCBHeader.FileSize;

            PtrBuffer->Directory = FALSE;
        }

        try_exit: NOTHING;
    } _SEH2_FINALLY {
        if(NT_SUCCESS(RC)) {
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
    IN PtrUDFFCB                      Fcb,
    IN PFILE_NETWORK_OPEN_INFORMATION PtrBuffer,
 IN OUT PLONG                         PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;

    AdPrint(("UDFGetNetworkInformation: \n"));

    _SEH2_TRY {

        if(*PtrReturnedLength < sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_NETWORK_OPEN_INFORMATION));

        // Get information from the FCB.
        PtrBuffer->CreationTime = Fcb->NTRequiredFCB->CreationTime;
        PtrBuffer->LastAccessTime = Fcb->NTRequiredFCB->LastAccessTime;
        PtrBuffer->LastWriteTime = Fcb->NTRequiredFCB->LastWriteTime;

        FileInfo = Fcb->FileInfo;

        if(!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! UDFGetNetworkInformation to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        // Now fill in the attributes.
        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
#ifdef UDF_DBG
            if(!FileInfo->Dloc->DirIndex) AdPrint(("*****!!!!! Directory has no DirIndex !!!!!*****\n"));
#endif
        }
        // Similarly, fill in attributes indicating a hidden file, system
        // file, compressed file, temporary file, etc. if the FSD supports
        // such file attribute values.
        PtrBuffer->FileAttributes |= UDFAttributesToNT(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index),NULL);
        if(!PtrBuffer->FileAttributes) {
            PtrBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(NT_SUCCESS(RC)) {
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
    PtrUDFIrpContext              PtrIrpContext,
    IN PtrUDFFCB                  Fcb,
    IN PtrUDFCCB                  Ccb,
    IN PFILE_INTERNAL_INFORMATION PtrBuffer,
 IN OUT PLONG                     PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PUDF_FILE_INFO      FileInfo;
    PVCB Vcb;

    AdPrint(("UDFGetInternalInformation\n"));

    _SEH2_TRY {

        if(*PtrReturnedLength < sizeof(FILE_INTERNAL_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, sizeof(FILE_INTERNAL_INFORMATION));

        FileInfo = Fcb->FileInfo;

        if(!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! UDFGetInternalInformation to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }

        Vcb = Fcb->Vcb;
        PtrBuffer->IndexNumber.QuadPart = UDFGetNTFileId(Vcb, FileInfo, &(Fcb->FCBName->ObjectName));

        UDFAcquireResourceExclusive(&(Fcb->Vcb->FileIdResource), TRUE);
        // remember File Id & full path
        UDFStoreFileId(Fcb->Vcb, Ccb, FileInfo, PtrBuffer->IndexNumber.QuadPart);
        UDFReleaseResource(&(Fcb->Vcb->FileIdResource));

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(NT_SUCCESS(RC)) {
            // Return the amount of information filled in.
            *PtrReturnedLength -= sizeof(FILE_INTERNAL_INFORMATION);
        }
    } _SEH2_END;
    return(RC);
} // end UDFGetInternalInformation()

/*
    Return zero-filled EAs to the caller.
 */
NTSTATUS
UDFGetEaInformation(
    PtrUDFIrpContext        PtrIrpContext,
    IN PtrUDFFCB            Fcb,
    IN PFILE_EA_INFORMATION PtrBuffer,
 IN OUT PLONG               PtrReturnedLength
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;

    AdPrint(("UDFGetEaInformation\n"));

    _SEH2_TRY {

        if(*PtrReturnedLength < sizeof(FILE_EA_INFORMATION)) {
            try_return(RC = STATUS_BUFFER_OVERFLOW);
        }

        // Zero out the supplied buffer.
        PtrBuffer->EaSize = 0;

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(NT_SUCCESS(RC)) {
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
    NTSTATUS RC = STATUS_SUCCESS;


    AdPrint(("UDFGetFullNameInformation\n"));

    PtrBuffer->FileNameLength = FileObject->FileName.Length;

    if (PtrBuffer->FileNameLength + sizeof( ULONG ) > (ULONG)(*PtrReturnedLength)) {

        PtrBuffer->FileNameLength = *PtrReturnedLength - sizeof( ULONG );
        RC = STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory( PtrBuffer->FileName, FileObject->FileName.Buffer, PtrBuffer->FileNameLength );

    //  Reduce the available bytes by the amount stored into this buffer.
    *PtrReturnedLength -= sizeof( ULONG ) + PtrBuffer->FileNameLength;

    return RC;
} // end UDFGetFullNameInformation()

/*
    Return file short(8.3) name to the caller.
 */
NTSTATUS
UDFGetAltNameInformation(
    IN PtrUDFFCB                   Fcb,
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

    UDFDOSName__(Fcb->Vcb, &ShortName, &(DirNdx->FName), Fcb->FileInfo);

    if(*PtrReturnedLength < ShortName.Length) {
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
    if(*PtrReturnedLength < sizeof(FILE_POSITION_INFORMATION)) {
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
    IN PtrUDFFCB                  Fcb,
    IN PFILE_STREAM_INFORMATION   PtrBuffer,
 IN OUT PLONG                     PtrReturnedLength
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    PUDF_FILE_INFO  FileInfo;
    PUDF_FILE_INFO  SDirInfo;
    PVCB            Vcb;
    BOOLEAN         FcbAcquired = FALSE;
    uint_di         i;
    LONG            l;
    PDIR_INDEX_HDR  hSDirIndex;
    PDIR_INDEX_ITEM SDirIndex;
    PFILE_BOTH_DIR_INFORMATION NTFileInfo = NULL;

    AdPrint(("UDFGetFileStreamInformation\n"));

    _SEH2_TRY {

        UDFAcquireResourceExclusive(&(Fcb->Vcb->FileIdResource), TRUE);
        FcbAcquired = TRUE;

        FileInfo = Fcb->FileInfo;
        if(!FileInfo) {
            AdPrint(("!!!!!!!! Bu-u-u-u-u-g !!!!!!!!!!!\n"));
            AdPrint(("!!!! UDFGetFileStreamInformation to unopened file !!!!\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        Vcb = Fcb->Vcb;
        // Zero out the supplied buffer.
        RtlZeroMemory(PtrBuffer, *PtrReturnedLength);
        if(!(SDirInfo = FileInfo->Dloc->SDirInfo) ||
             UDFIsSDirDeleted(SDirInfo) ) {
            (*PtrReturnedLength) -= (sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR));
            try_return(RC = STATUS_SUCCESS);
        }

        hSDirIndex = SDirInfo->Dloc->DirIndex;
        NTFileInfo = (PFILE_BOTH_DIR_INFORMATION)MyAllocatePool__(NonPagedPool, sizeof(FILE_BOTH_DIR_INFORMATION)+UDF_NAME_LEN*sizeof(WCHAR));
        if(!NTFileInfo) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

        for(i=2; SDirIndex = UDFDirIndex(hSDirIndex,i); i++) {
            if((SDirIndex->FI_Flags & UDF_FI_FLAG_FI_INTERNAL) ||
                UDFIsDeleted(SDirIndex) ||
                !SDirIndex->FName.Buffer )
                continue;
            // copy data to buffer
            if(*PtrReturnedLength < (l = ((sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR)) +
                                           SDirIndex->FName.Length + 3) & (~3)) ) {
                try_return(RC = STATUS_BUFFER_OVERFLOW);
            }
            RC = UDFFileDirInfoToNT(Vcb, SDirIndex, NTFileInfo);

            PtrBuffer->NextEntryOffset = l;
            PtrBuffer->StreamNameLength = SDirIndex->FName.Length;
            PtrBuffer->StreamSize = NTFileInfo->EndOfFile;
            PtrBuffer->StreamAllocationSize = NTFileInfo->AllocationSize;
            RtlCopyMemory(&(PtrBuffer->StreamName), SDirIndex->FName.Buffer, SDirIndex->FName.Length);
            *PtrReturnedLength -= l;
            *((PCHAR*)(&PtrBuffer)) += l;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(FcbAcquired)
            UDFReleaseResource(&(Fcb->Vcb->FileIdResource));
        if(NTFileInfo)
           MyFreePool__(NTFileInfo);
    } _SEH2_END;
    return(RC);
} // end UDFGetFileStreamInformation()

//*******************************************************************

#ifndef UDF_READ_ONLY_BUILD

/*
    Set some time-stamps and file attributes supplied by the caller.
 */
NTSTATUS
UDFSetBasicInformation(
    IN PtrUDFFCB                   Fcb,
    IN PtrUDFCCB                   Ccb,
    IN PFILE_OBJECT                FileObject,
    IN PFILE_BASIC_INFORMATION PtrBuffer)
{
    NTSTATUS        RC = STATUS_SUCCESS;
    ULONG           NotifyFilter = 0;
    
    AdPrint(("UDFSetBasicInformation\n"));

    _SEH2_TRY {

        // Obtain a pointer to the directory entry associated with
        // the FCB being modifed. The directory entry is obviously
        // part of the data associated with the parent directory that
        // contains this particular file stream.
        if(PtrBuffer->FileAttributes) {
            UDFUpdateAttrTime(Fcb->Vcb, Fcb->FileInfo);
        } else
        if( UDFIsADirectory(Fcb->FileInfo) &&
            !(Fcb->Vcb->CompatFlags & UDF_VCB_IC_UPDATE_UCHG_DIR_ACCESS_TIME) &&
              ((Fcb->FileInfo->Dloc->DataLoc.Modified ||
                Fcb->FileInfo->Dloc->AllocLoc.Modified ||
                (Fcb->FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
                Fcb->FileInfo->Dloc->FELoc.Modified))
             ) {
            // ignore Access Time Modification for unchanged Dir
            if(!PtrBuffer->CreationTime.QuadPart &&
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

        if(PtrBuffer->CreationTime.QuadPart) {
            // The interesting thing here is that the user has set certain time
            // fields. However, before doing this, the user may have performed
            // I/O which in turn would have caused FSD to mark the fact that
            // write/access time should be modifed at cleanup.
            // We'll mark the fact that such updates are no longer
            // required since the user has explicitly specified the values he
            // wishes to see associated with the file stream.
            Fcb->NTRequiredFCB->CreationTime = PtrBuffer->CreationTime;
            Ccb->CCBFlags |= UDF_CCB_CREATE_TIME_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
        }
        if(PtrBuffer->LastAccessTime.QuadPart) {
            Fcb->NTRequiredFCB->LastAccessTime = PtrBuffer->LastAccessTime;
            Ccb->CCBFlags |= UDF_CCB_ACCESS_TIME_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }
        if(PtrBuffer->ChangeTime.QuadPart) {
            Fcb->NTRequiredFCB->ChangeTime = PtrBuffer->ChangeTime;
            Ccb->CCBFlags |= UDF_CCB_MODIFY_TIME_SET;
        }
        if(PtrBuffer->LastWriteTime.QuadPart) {
            Fcb->NTRequiredFCB->LastWriteTime = PtrBuffer->LastWriteTime;
            Ccb->CCBFlags |= UDF_CCB_WRITE_TIME_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }

        // Now come the attributes.
        if(PtrBuffer->FileAttributes) {
            // We have a non-zero attribute value.
            // The presence of a particular attribute indicates that the
            // user wishes to set the attribute value. The absence indicates
            // the user wishes to clear the particular attribute.

            // Our routine ignores unsupported flags
            PtrBuffer->FileAttributes &= ~(FILE_ATTRIBUTE_NORMAL);

            // Similarly, we should pick out other invalid flag values.
            if( (PtrBuffer->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
               !(Fcb->FCBFlags & UDF_FCB_DIRECTORY))
                try_return(RC = STATUS_INVALID_PARAMETER);

            if(PtrBuffer->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                if(Fcb->FCBFlags & UDF_FCB_DIRECTORY)
                    try_return(RC = STATUS_INVALID_PARAMETER);
                FileObject->Flags |= FO_TEMPORARY_FILE;
            } else {
                FileObject->Flags &= ~FO_TEMPORARY_FILE;
            }

            if(PtrBuffer->FileAttributes & FILE_ATTRIBUTE_READONLY) {
                Fcb->FCBFlags |= UDF_FCB_READ_ONLY;
            } else {
                Fcb->FCBFlags &= ~UDF_FCB_READ_ONLY;
            }

            UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index),
                               NULL, PtrBuffer->FileAttributes);

            (UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index))
                ->FI_Flags |= UDF_FI_FLAG_SYS_ATTR;
            // If the FSD supports file compression, we may wish to
            // note the user's preferences for compressing/not compressing
            // the file at this time.
            Ccb->CCBFlags |= UDF_CCB_ATTRIBUTES_SET;
            NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
        }

        if(NotifyFilter) {
            UDFNotifyFullReportChange( Fcb->Vcb, Fcb->FileInfo,
                                       NotifyFilter, FILE_ACTION_MODIFIED);
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
    IN PVCB           Vcb,
    IN PtrUDFFCB      Fcb,
    IN BOOLEAN        ForDel
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    PUDF_FILE_INFO  SDirInfo = NULL;
    PUDF_FILE_INFO  FileInfo = NULL;
    ULONG lc;
    BOOLEAN SDirAcq = FALSE;
    BOOLEAN StrAcq = FALSE;
    uint_di d,i;

    _SEH2_TRY {

        // In some cases we needn't marking Streams for deleteion
        // (Not opened or Don't exist)
        if(UDFIsAStream(Fcb->FileInfo) ||
           UDFIsAStreamDir(Fcb->FileInfo) ||
           !UDFHasAStreamDir(Fcb->FileInfo) ||
           !Fcb->FileInfo->Dloc->SDirInfo ||
           UDFIsSDirDeleted(Fcb->FileInfo->Dloc->SDirInfo) ||
           (UDFGetFileLinkCount(Fcb->FileInfo) > 1) )
            try_return (RC /*=STATUS_SUCCESS*/);
        
        // We shall mark Streams for deletion if there is no
        // Links to the file. Otherwise we'll delete only the file.
        // If we are asked to unmark Streams, we'll precess the whole Tree
        RC = UDFOpenStreamDir__(Vcb, Fcb->FileInfo, &SDirInfo);
        if(!NT_SUCCESS(RC))
            try_return(RC);

        if(SDirInfo->Fcb &&
           SDirInfo->Fcb->NTRequiredFCB) {
            UDF_CHECK_PAGING_IO_RESOURCE(SDirInfo->Fcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(&(SDirInfo->Fcb->NTRequiredFCB->MainResource),TRUE);
            SDirAcq = TRUE;
        }

        if(!ForDel || ((lc = UDFGetFileLinkCount(Fcb->FileInfo)) < 2)) {

            UDF_DIR_SCAN_CONTEXT ScanContext;
            PDIR_INDEX_ITEM DirNdx;

            // It is not worth checking whether the Stream can be deleted if
            // Undelete requested
            if(ForDel &&
                // scan DirIndex
                UDFDirIndexInitScan(SDirInfo, &ScanContext, 2)) {

                // Check if we can delete Streams 
                while(DirNdx = UDFDirIndexScan(&ScanContext, &FileInfo)) {
                    if(!FileInfo)
                        continue;
                    if(FileInfo->Fcb) {
                        FileInfo->Fcb->NTRequiredFCB->AcqFlushCount++;
                        MmPrint(("    MmFlushImageSection() for Stream\n"));
                        if(!MmFlushImageSection(&(FileInfo->Fcb->NTRequiredFCB->SectionObject), MmFlushForDelete)) {
                            FileInfo->Fcb->NTRequiredFCB->AcqFlushCount--;
                            try_return(RC = STATUS_CANNOT_DELETE);
                        }
                        FileInfo->Fcb->NTRequiredFCB->AcqFlushCount--;
                    }
                }
            }
            // (Un)Mark Streams for deletion

            // Perform sequencial Open for Streams & mark 'em
            // for deletion. We should not  get FileInfo pointers directly
            // from DirNdx[i] to prevent great troubles with linked
            // files. We should mark for deletion FI with proper ParentFile
            // pointer.
            d = UDFDirIndexGetLastIndex(SDirInfo->Dloc->DirIndex);
            for(i=2; i<d; i++) {
                RC = UDFOpenFile__(Vcb,
                                   FALSE,TRUE,NULL,
                                   SDirInfo,&FileInfo,&i);
                ASSERT(NT_SUCCESS(RC) || (RC == STATUS_FILE_DELETED));
                if(NT_SUCCESS(RC)) {
                    if(FileInfo->Fcb) {
                        if(FileInfo->Fcb->NTRequiredFCB) {
                            UDF_CHECK_PAGING_IO_RESOURCE(FileInfo->Fcb->NTRequiredFCB);
                            UDFAcquireResourceExclusive(&(FileInfo->Fcb->NTRequiredFCB->MainResource),TRUE);
                            StrAcq = TRUE;
                        }
#ifndef UDF_ALLOW_LINKS_TO_STREAMS
                        if(UDFGetFileLinkCount(FileInfo) >= 2) {
                            // Currently, UDF_INFO package doesn't
                            // support this case, so we'll inform developer
                            // about this to prevent on-disk space leaks...
                            BrutePoint();
                            try_return(RC = STATUS_CANNOT_DELETE);
                        }
#endif //UDF_ALLOW_LINKS_TO_STREAMS
                        if(ForDel) {
                            AdPrint(("    SET stream DeleteOnClose\n"));
#ifdef UDF_DBG
                            ASSERT(!(FileInfo->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                            if(FileInfo->ParentFile &&
                               FileInfo->ParentFile->Fcb) {
                                ASSERT(!(FileInfo->ParentFile->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                            }
#endif // UDF_DBG
                            FileInfo->Fcb->FCBFlags |= (UDF_FCB_DELETE_ON_CLOSE |
                                                        UDF_FCB_DELETE_PARENT);
                        } else {
                            AdPrint(("    CLEAR stream DeleteOnClose\n"));
                            FileInfo->Fcb->FCBFlags &= !(UDF_FCB_DELETE_ON_CLOSE |
                                                         UDF_FCB_DELETE_PARENT);
                        }
                    }
                    UDFCloseFile__(Vcb, FileInfo);
                } else
                if(RC == STATUS_FILE_DELETED) {
                    // That's OK if STATUS_FILE_DELETED returned...
                    RC = STATUS_SUCCESS;
                }
                if(FileInfo) {
                    if(UDFCleanUpFile__(Vcb, FileInfo)) {
                        ASSERT(!StrAcq && !(FileInfo->Fcb));
                        MyFreePool__(FileInfo);
                    }
                    if(StrAcq) {
                        UDF_CHECK_PAGING_IO_RESOURCE(FileInfo->Fcb->NTRequiredFCB);
                        UDFReleaseResource(&(FileInfo->Fcb->NTRequiredFCB->MainResource));
                        StrAcq = FALSE;
                    }
                }
                FileInfo = NULL;
            }
            // Mark SDir for deletion
            if(SDirInfo->Fcb) {
                if(ForDel) {
#ifdef UDF_DBG
                    ASSERT(!(SDirInfo->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                    if(SDirInfo->ParentFile &&
                       SDirInfo->ParentFile->Fcb) {
                        ASSERT(!(SDirInfo->ParentFile->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                    }
#endif // UDF_DBG
                    AdPrint(("    SET stream dir DeleteOnClose\n"));
                    SDirInfo->Fcb->FCBFlags |= (UDF_FCB_DELETE_ON_CLOSE |
                                                UDF_FCB_DELETE_PARENT);
                } else {
                    AdPrint(("    CLEAR stream dir DeleteOnClose\n"));
                    SDirInfo->Fcb->FCBFlags &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                                 UDF_FCB_DELETE_PARENT);
                }
            }
        } else
        if(lc >= 2) {
            // if caller wants us to perform DelTree for Streams, but
            // someone keeps Stream opened and there is a Link to this
            // file, we can't delete it immediately (on Cleanup) & should
            // not delete the whole Tree. Instead, we'll set DELETE_PARENT
            // flag in SDir to kill this file later, when all the Handles
            // to Streams, opened via this file, would be closed
#ifdef UDF_DBG
            ASSERT(!(SDirInfo->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
            if(SDirInfo->ParentFile &&
               SDirInfo->ParentFile->Fcb) {
                ASSERT(!(SDirInfo->ParentFile->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
            }
#endif // UDF_DBG
            if(SDirInfo->Fcb)
                SDirInfo->Fcb->FCBFlags |= UDF_FCB_DELETE_PARENT;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(FileInfo) {
            UDFCloseFile__(Vcb, FileInfo);
            if(UDFCleanUpFile__(Vcb, FileInfo)) {
                ASSERT(!StrAcq && !(FileInfo->Fcb));
                MyFreePool__(FileInfo);
            }
            if(StrAcq) {
                UDF_CHECK_PAGING_IO_RESOURCE(FileInfo->Fcb->NTRequiredFCB);
                UDFReleaseResource(&(FileInfo->Fcb->NTRequiredFCB->MainResource));
            }
            SDirInfo = NULL;
        }
        if(SDirInfo) {
            UDFCloseFile__(Vcb, SDirInfo);
            if(SDirAcq) {
                UDF_CHECK_PAGING_IO_RESOURCE(SDirInfo->Fcb->NTRequiredFCB);
                UDFReleaseResource(&(SDirInfo->Fcb->NTRequiredFCB->MainResource));
            }
            if(UDFCleanUpFile__(Vcb, SDirInfo)) {
                MyFreePool__(SDirInfo);
            }
            SDirInfo = NULL;
        }
    } _SEH2_END;
    return RC;
} // end UDFMarkStreamsForDeletion()

/*
    (Un)Mark file for deletion.
 */
NTSTATUS
UDFSetDispositionInformation(
    IN PtrUDFFCB                       Fcb,
    IN PtrUDFCCB                       Ccb,
    IN PVCB                            Vcb,
    IN PFILE_OBJECT                    FileObject,
    IN BOOLEAN                         Delete
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
//    PUDF_FILE_INFO  SDirInfo = NULL;
//    PUDF_FILE_INFO  FileInfo = NULL;
    ULONG lc;

    AdPrint(("UDFSetDispositionInformation\n"));

    _SEH2_TRY {

        if(!Delete) {
            AdPrint(("    CLEAR DeleteOnClose\n"));
            // "un-delete" the file.
            Fcb->FCBFlags &= ~UDF_FCB_DELETE_ON_CLOSE;
            if(FileObject)
                FileObject->DeletePending = FALSE;
            RC = UDFMarkStreamsForDeletion(Vcb, Fcb, FALSE); // Undelete
            try_return(RC);
        }
        AdPrint(("    SET DeleteOnClose\n"));

        // The easy part is over. Now, we know that the user wishes to
        // delete the corresponding directory entry (of course, if this
        // is the only link to the file stream, any on-disk storage space
        // associated with the file stream will also be released when the
        // (only) link is deleted!)

        // Do some checking to see if the file can even be deleted.
        if(Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE) {
            // All done!
            try_return(RC);
        }

        if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) {
            try_return(RC = STATUS_CANNOT_DELETE);
        }

        if(Fcb->FCBFlags & UDF_FCB_READ_ONLY) {
            RC = UDFCheckAccessRights(NULL, NULL, Fcb->ParentFcb, NULL, FILE_DELETE_CHILD, 0);
            if(!NT_SUCCESS(RC)) {
                try_return (RC = STATUS_CANNOT_DELETE);
            }
        }

        // It would not be prudent to allow deletion of either a root
        // directory or a directory that is not empty.
        if(Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY)
            try_return(RC = STATUS_CANNOT_DELETE);

        lc = UDFGetFileLinkCount(Fcb->FileInfo);

        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            // Perform check to determine whether the directory
            // is empty or not.
            if(!UDFIsDirEmpty__(Fcb->FileInfo)) {
                 try_return(RC = STATUS_DIRECTORY_NOT_EMPTY);
            }

        } else {
            // An important step is to check if the file stream has been
            // mapped by any process. The delete cannot be allowed to proceed
            // in this case.
            MmPrint(("    MmFlushImageSection()\n"));
            Fcb->NTRequiredFCB->AcqFlushCount++;
            if(!MmFlushImageSection(&(Fcb->NTRequiredFCB->SectionObject),
                    (lc > 1) ? MmFlushForWrite : MmFlushForDelete)) {
                Fcb->NTRequiredFCB->AcqFlushCount--;
                try_return(RC = STATUS_CANNOT_DELETE);
            }
            Fcb->NTRequiredFCB->AcqFlushCount--;
        }
        // We should also mark Streams for deletion if there are no
        // Links to the file. Otherwise we'll delete only the file

        if(lc > 1) {
            RC = STATUS_SUCCESS;
        } else {
            RC = UDFMarkStreamsForDeletion(Vcb, Fcb, TRUE); // Delete
            if(!NT_SUCCESS(RC))
                try_return(RC);
        }

        // Set a flag to indicate that this directory entry will become history
        // at cleanup.
        Fcb->FCBFlags |= UDF_FCB_DELETE_ON_CLOSE;
        if(FileObject)
            FileObject->DeletePending = TRUE;

        if((Fcb->FCBFlags & UDF_FCB_DIRECTORY) && Ccb) {
            FsRtlNotifyFullChangeDirectory( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
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
UDFSetAllocationInformation(
    IN PtrUDFFCB                       Fcb,
    IN PtrUDFCCB                       Ccb,
    IN PVCB                            Vcb,
    IN PFILE_OBJECT                    FileObject,
    IN PtrUDFIrpContext                PtrIrpContext,
    IN PIRP                            Irp,
    IN PFILE_ALLOCATION_INFORMATION    PtrBuffer
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    BOOLEAN         TruncatedFile = FALSE;
    BOOLEAN         ModifiedAllocSize = FALSE;
    BOOLEAN         CacheMapInitialized = FALSE;
    BOOLEAN         AcquiredPagingIo = FALSE;

    AdPrint(("UDFSetAllocationInformation\n"));

    _SEH2_TRY {
        // Increasing the allocation size associated with a file stream
        // is relatively easy. All we have to do is execute some FSD
        // specific code to check whether we have enough space available
        // (and if the FSD supports user/volume quotas, whether the user
        // is not exceeding quota), and then increase the file size in the
        // corresponding on-disk and in-memory structures.
        // Then, all we should do is inform the Cache Manager about the
        // increased allocation size.

        // First, do whatever error checking is appropriate here (e.g. whether
        // the caller is trying the change size for a directory, etc.).
        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY)
            try_return(RC = STATUS_INVALID_PARAMETER);

        Fcb->NTRequiredFCB->CommonFCBHeader.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

        if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
            (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
            !FlagOn(Irp->Flags, IRP_PAGING_IO)) {
            ASSERT( !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) );
            //  Now initialize the cache map.
            MmPrint(("    CcInitializeCacheMap()\n"));
            CcInitializeCacheMap( FileObject,
                                  (PCC_FILE_SIZES)&Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize,
                                  FALSE,
                                  &(UDFGlobalData.CacheMgrCallBacks),
                                  Fcb->NTRequiredFCB );

            CacheMapInitialized = TRUE;
        }

        // Are we increasing the allocation size?
        if(Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize.QuadPart <
            PtrBuffer->AllocationSize.QuadPart) {

            // Yes. Do the FSD specific stuff i.e. increase reserved
            // space on disk.
            if(((LONGLONG)UDFGetFreeSpace(Vcb) << Vcb->LBlockSizeBits) < PtrBuffer->AllocationSize.QuadPart) {
                try_return(RC = STATUS_DISK_FULL);
            }
//          RC = STATUS_SUCCESS;
            ModifiedAllocSize = TRUE;

        } else if(Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize.QuadPart >
                                                                PtrBuffer->AllocationSize.QuadPart) {
            // This is the painful part. See if the VMM will allow us to proceed.
            // The VMM will deny the request if:
            // (a) any image section exists OR
            // (b) a data section exists and the size of the user mapped view
            //       is greater than the new size
            // Otherwise, the VMM should allow the request to proceed.
            MmPrint(("    MmCanFileBeTruncated()\n"));
            if(!MmCanFileBeTruncated(&(Fcb->NTRequiredFCB->SectionObject), &(PtrBuffer->AllocationSize))) {
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
        if(ModifiedAllocSize) {

            // If we decreased the allocation size to less than the
            // current file size, modify the file size value.
            // Similarly, if we decreased the value to less than the
            // current valid data length, modify that value as well.

            AcquiredPagingIo = UDFAcquireResourceExclusiveWithCheck(&(Fcb->NTRequiredFCB->PagingIoResource));
            // Update the FCB Header with the new allocation size.
            if(TruncatedFile) {
                if(Fcb->NTRequiredFCB->CommonFCBHeader.ValidDataLength.QuadPart >
                    PtrBuffer->AllocationSize.QuadPart) {
                    // Decrease the valid data length value.
                    Fcb->NTRequiredFCB->CommonFCBHeader.ValidDataLength =
                        PtrBuffer->AllocationSize;
                }
                if(Fcb->NTRequiredFCB->CommonFCBHeader.FileSize.QuadPart >
                    PtrBuffer->AllocationSize.QuadPart) {
                    // Decrease the file size value.
                    Fcb->NTRequiredFCB->CommonFCBHeader.FileSize =
                        PtrBuffer->AllocationSize;
                    RC = UDFResizeFile__(Vcb, Fcb->FileInfo, PtrBuffer->AllocationSize.QuadPart);
//                    UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, NULL);
                }
            } else {
                Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize = PtrBuffer->AllocationSize;
//                UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo,
//                                       &(PtrBuffer->AllocationSize.QuadPart));
            }
            if(AcquiredPagingIo) {
                UDFReleaseResource(&(Fcb->NTRequiredFCB->PagingIoResource));
                AcquiredPagingIo = FALSE;
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
            Fcb->NTRequiredFCB->AcqFlushCount++;
            CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&(Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize));
            Fcb->NTRequiredFCB->AcqFlushCount--;
            Fcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;

            // Inform any pending IRPs (notify change directory).
            if(UDFIsAStream(Fcb->FileInfo)) {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_STREAM_SIZE,
                                           FILE_ACTION_MODIFIED_STREAM);
            } else {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_SIZE,
                                           FILE_ACTION_MODIFIED);
            }
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(AcquiredPagingIo) {
            UDFReleaseResource(&(Fcb->NTRequiredFCB->PagingIoResource));
            AcquiredPagingIo = FALSE;
        }
        if (CacheMapInitialized) {

            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }
    } _SEH2_END;
    return(RC);
} // end UDFSetAllocationInformation()

/*
    Set end of file (resize).
 */
NTSTATUS
UDFSetEOF(
    IN PIO_STACK_LOCATION              PtrSp,
    IN PtrUDFFCB                       Fcb,
    IN PtrUDFCCB                       Ccb,
    IN PVCB                            Vcb,
    IN PFILE_OBJECT                    FileObject,
    IN PIRP                            Irp,
    IN PFILE_END_OF_FILE_INFORMATION   PtrBuffer
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    BOOLEAN         TruncatedFile = FALSE;
    BOOLEAN         ModifiedAllocSize = FALSE;
    ULONG           Attr;
    PDIR_INDEX_ITEM DirNdx;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
    LONGLONG        OldFileSize;
    BOOLEAN         ZeroBlock;
    BOOLEAN         CacheMapInitialized = FALSE;
    BOOLEAN         AcquiredPagingIo = FALSE;

    AdPrint(("UDFSetEOF\n"));

    _SEH2_TRY {
        // Increasing the allocation size associated with a file stream
        // is relatively easy. All we have to do is execute some FSD
        // specific code to check whether we have enough space available
        // (and if the FSD supports user/volume quotas, whether the user
        // is not exceeding quota), and then increase the file size in the
        // corresponding on-disk and in-memory structures.
        // Then, all we should do is inform the Cache Manager about the
        // increased allocation size.

        // First, do whatever error checking is appropriate here (e.g. whether
        // the caller is trying the change size for a directory, etc.).
        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY)
            try_return(RC = STATUS_INVALID_PARAMETER);

        NtReqFcb = Fcb->NTRequiredFCB;

        if((Fcb->FCBFlags & UDF_FCB_DELETED) ||
           (NtReqFcb->NtReqFCBFlags & UDF_NTREQ_FCB_DELETED)) {
#ifdef UDF_DBG
            if(UDFGetFileLinkCount(Fcb->FileInfo) < 1) {
                BrutePoint();
                try_return(RC = STATUS_SUCCESS);
            } else
#endif // UDF_DBG
                try_return(RC = STATUS_SUCCESS);
        }

        NtReqFcb->CommonFCBHeader.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

        if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
            (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
            !(Irp->Flags & IRP_PAGING_IO)) {
            ASSERT( !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) );
            //  Now initialize the cache map.
            MmPrint(("    CcInitializeCacheMap()\n"));
            CcInitializeCacheMap( FileObject,
                                  (PCC_FILE_SIZES)&Fcb->NTRequiredFCB->CommonFCBHeader.AllocationSize,
                                  FALSE,
                                  &(UDFGlobalData.CacheMgrCallBacks),
                                  Fcb->NTRequiredFCB );

            CacheMapInitialized = TRUE;
        }

        AcquiredPagingIo = UDFAcquireResourceExclusiveWithCheck(&(Fcb->NTRequiredFCB->PagingIoResource));
        //  Do a special case here for the lazy write of file sizes.
        if(PtrSp->Parameters.SetFile.AdvanceOnly) {
            //  Never have the dirent filesize larger than the fcb filesize
            PtrBuffer->EndOfFile.QuadPart =
                min(PtrBuffer->EndOfFile.QuadPart,
                    NtReqFcb->CommonFCBHeader.FileSize.QuadPart);
            //  Only advance the file size, never reduce it with this call
            RC = STATUS_SUCCESS;
            if(UDFGetFileSizeFromDirNdx(Vcb, Fcb->FileInfo) >= 
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
        OldFileSize = NtReqFcb->CommonFCBHeader.FileSize.QuadPart;
        if(OldFileSize < PtrBuffer->EndOfFile.QuadPart) {

            // Yes. Do the FSD specific stuff i.e. increase reserved
            // space on disk.
            if (FileObject->PrivateCacheMap)
                ZeroBlock = TRUE;

            // reference file to pretend that it is opened
            UDFReferenceFile__(Fcb->FileInfo);
            UDFInterlockedIncrement((PLONG)&(Fcb->ReferenceCount));
            UDFInterlockedIncrement((PLONG)&(NtReqFcb->CommonRefCount));
            // perform resize operation
            RC = UDFResizeFile__(Vcb, Fcb->FileInfo, PtrBuffer->EndOfFile.QuadPart);
            // dereference file
            UDFCloseFile__(Vcb, Fcb->FileInfo);
            UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
            UDFInterlockedDecrement((PLONG)&(NtReqFcb->CommonRefCount));
            // update values in NtReqFcb
            NtReqFcb->CommonFCBHeader.FileSize.QuadPart =
//            NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart =
                PtrBuffer->EndOfFile.QuadPart;
            ModifiedAllocSize = TRUE;

        } else if(NtReqFcb->CommonFCBHeader.FileSize.QuadPart >
                                         PtrBuffer->EndOfFile.QuadPart) {

            // This is the painful part. See if the VMM will allow us to proceed.
            // The VMM will deny the request if:
            // (a) any image section exists OR
            // (b) a data section exists and the size of the user mapped view
            //       is greater than the new size
            // Otherwise, the VMM should allow the request to proceed.

            MmPrint(("    MmCanFileBeTruncated()\n"));
            if(!MmCanFileBeTruncated(&(NtReqFcb->SectionObject), &(PtrBuffer->EndOfFile))) {
                // VMM said no way!
                try_return(RC = STATUS_USER_MAPPED_FILE);
            }

            // Perform directory entry modifications. Release any on-disk
            // space we may need to in the process.
            UDFReferenceFile__(Fcb->FileInfo);
            UDFInterlockedIncrement((PLONG)&(Fcb->ReferenceCount));
            UDFInterlockedIncrement((PLONG)&(NtReqFcb->CommonRefCount));
            // perform resize operation
            RC = UDFResizeFile__(Vcb, Fcb->FileInfo, PtrBuffer->EndOfFile.QuadPart);
            // dereference file
            UDFCloseFile__(Vcb, Fcb->FileInfo);
            UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
            UDFInterlockedDecrement((PLONG)&(NtReqFcb->CommonRefCount));
            
            ModifiedAllocSize = TRUE;
            TruncatedFile = TRUE;
        }

        // This is a good place to check if we have performed a truncate
        // operation. If we have perform a truncate (whether we extended
        // or reduced file size), we should update file time stamps.

        // Last, but not the least, we must inform the Cache Manager of file size changes.
        if(ModifiedAllocSize && NT_SUCCESS(RC)) {
            // If we decreased the allocation size to less than the
            // current file size, modify the file size value.
            // Similarly, if we decreased the value to less than the
            // current valid data length, modify that value as well.
            if(TruncatedFile) {
                if(NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart >
                    PtrBuffer->EndOfFile.QuadPart) {
                    // Decrease the valid data length value.
                    NtReqFcb->CommonFCBHeader.ValidDataLength =
                        PtrBuffer->EndOfFile;
                }
                if(NtReqFcb->CommonFCBHeader.FileSize.QuadPart >
                    PtrBuffer->EndOfFile.QuadPart) {
                    // Decrease the file size value.
                    NtReqFcb->CommonFCBHeader.FileSize =
                        PtrBuffer->EndOfFile;
                }
                UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, NULL);
            } else {
                // Update the FCB Header with the new allocation size.
                // NT expects AllocationSize to be decreased on Close only
                NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart =
                    PtrBuffer->EndOfFile.QuadPart;
//                    UDFSysGetAllocSize(Vcb, UDFGetFileSize(Fcb->FileInfo));
                UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, &(PtrBuffer->EndOfFile.QuadPart));
            }

            FileObject->Flags |= FO_FILE_MODIFIED;
//                UDFGetFileAllocationSize(Vcb, Fcb->FileInfo);

            // If the FCB has not had caching initiated, it is still valid
            // for us to invoke the NT Cache Manager. It is possible in such
            // situations for the call to be no'oped (unless some user has
            // mapped in the file)

            // Archive bit
            if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT) {
                DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index);
                Ccb->CCBFlags &= ~UDF_CCB_ATTRIBUTES_SET;
                Attr = UDFAttributesToNT(DirNdx, Fcb->FileInfo->Dloc->FileEntry);
                if(!(Attr & FILE_ATTRIBUTE_ARCHIVE))
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
            Fcb->NTRequiredFCB->AcqFlushCount++;
            CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&(NtReqFcb->CommonFCBHeader.AllocationSize));
            Fcb->NTRequiredFCB->AcqFlushCount--;
/*            if(ZeroBlock) {
                UDFZeroDataEx(NtReqFcb,
                              OldFileSize,
                              PtrBuffer->EndOfFile.QuadPart - OldFileSize,
                              TRUE/*CanWait, Vcb, FileObject);
            }*/
            Fcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;

notify_size_changes:
            if(AcquiredPagingIo) {
                UDFReleaseResource(&(Fcb->NTRequiredFCB->PagingIoResource));
                AcquiredPagingIo = FALSE;
            }

            // Inform any pending IRPs (notify change directory).
            if(UDFIsAStream(Fcb->FileInfo)) {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_STREAM_SIZE,
                                           FILE_ACTION_MODIFIED_STREAM);
            } else {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_SIZE,
                                           FILE_ACTION_MODIFIED);
            }
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(AcquiredPagingIo) {
            UDFReleaseResource(&(Fcb->NTRequiredFCB->PagingIoResource));
            AcquiredPagingIo = FALSE;
        }
        if (CacheMapInitialized) {

            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }
    } _SEH2_END;
    return(RC);
} // end UDFSetEOF()

NTSTATUS
UDFPrepareForRenameMoveLink(
    PVCB Vcb,
    PBOOLEAN AcquiredVcb,
    PBOOLEAN AcquiredVcbEx,
    PBOOLEAN SingleDir,
    PBOOLEAN AcquiredDir1,
    PBOOLEAN AcquiredFcb1,
    IN PtrUDFCCB Ccb1,
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
    // CleanUpFcbChain() or UDFCloseFileInfoChain()
    UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));
    UDFReleaseResource(&(Vcb->VCBResource));
    (*AcquiredVcb) = FALSE;

    // At first, make system to issue last Close request
    // for our Source & Target ...
    // we needn't flush/purge for Source on HLink
    UDFRemoveFromSystemDelayedQueue(Dir2->Fcb);
    if(!HardLink && (Dir2 != Dir1))
        UDFRemoveFromSystemDelayedQueue(File1->Fcb);

#ifdef UDF_DELAYED_CLOSE
    _SEH2_TRY {
        // Do actual close for all "delayed close" calls

        // ... and now remove the rest from our queue
        if(!HardLink) {
            UDFCloseAllDelayedInDir(Vcb, Dir1);
            if(Dir2 != Dir1)
                UDFCloseAllDelayedInDir(Vcb, Dir2);
        } else {
            UDFCloseAllDelayedInDir(Vcb, Dir2);
        }

    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));
        return (STATUS_DRIVER_INTERNAL_ERROR);
    } _SEH2_END;
#endif //UDF_DELAYED_CLOSE

    (*SingleDir) = ((Dir1 == Dir2) && (Dir1->Fcb));

    if(!(*SingleDir) ||
       (UDFGetFileLinkCount(File1) != 1)) {
        UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
        (*AcquiredVcb) = TRUE;
        (*AcquiredVcbEx) = TRUE;
        UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));
    } else {
        UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
        (*AcquiredVcb) = TRUE;
        UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));

        UDF_CHECK_PAGING_IO_RESOURCE(Dir1->Fcb->NTRequiredFCB);
        UDFAcquireResourceExclusive(&(Dir1->Fcb->NTRequiredFCB->MainResource),TRUE);
        (*AcquiredDir1) = TRUE;

        UDF_CHECK_PAGING_IO_RESOURCE(File1->Fcb->NTRequiredFCB);
        UDFAcquireResourceExclusive(&(File1->Fcb->NTRequiredFCB->MainResource),TRUE);
        (*AcquiredFcb1) = TRUE;
    }
    return STATUS_SUCCESS;
} // end UDFPrepareForRenameMoveLink()

/*
    Rename or move file
 */
NTSTATUS
UDFRename(
    IN PIO_STACK_LOCATION PtrSp,
    IN PtrUDFFCB Fcb1,
    IN PtrUDFCCB Ccb1,
    IN PFILE_OBJECT FileObject1,   // Source File
    IN PFILE_RENAME_INFORMATION PtrBuffer
    )
{
    // Source Directory
    PFILE_OBJECT DirObject1 = FileObject1->RelatedFileObject;
    // Target Directory
    PFILE_OBJECT DirObject2 = PtrSp->Parameters.SetFile.FileObject;
    // Overwite Flag
    BOOLEAN Replace = PtrSp->Parameters.SetFile.ReplaceIfExists &&
                      PtrBuffer->ReplaceIfExists;
    NTSTATUS RC;
    PVCB Vcb = Fcb1->Vcb;
    PtrUDFFCB Fcb2;
    BOOLEAN ic;
    BOOLEAN AcquiredVcb = TRUE;
    BOOLEAN AcquiredVcbEx = FALSE;
    BOOLEAN AcquiredDir1 = FALSE;
    BOOLEAN AcquiredFcb1 = FALSE;
    BOOLEAN SingleDir = TRUE;
    BOOLEAN UseClose;

    PUDF_FILE_INFO File1;
    PUDF_FILE_INFO Dir1;
    PUDF_FILE_INFO Dir2;
    PUDF_FILE_INFO NextFileInfo, fi;

    UNICODE_STRING NewName;
    UNICODE_STRING LocalPath;
    PtrUDFCCB CurCcb = NULL;
    PLIST_ENTRY Link;
    ULONG i;
    ULONG DirRefCount;
    ULONG FileInfoRefCount;
    ULONG Attr;
    PDIR_INDEX_ITEM DirNdx;

    AdPrint(("UDFRename %8.8x\n", DirObject2));
    
    LocalPath.Buffer = NULL;

    _SEH2_TRY {
        // do we try to rename Volume ?
#ifdef UDF_ALLOW_RENAME_MOVE
        if(!(File1 = Fcb1->FileInfo))
#endif //UDF_ALLOW_RENAME_MOVE
             try_return (RC = STATUS_ACCESS_DENIED);

        // do we try to rename RootDir ?
        if(!(Dir1 = File1->ParentFile))
            try_return (RC = STATUS_ACCESS_DENIED);

        // do we try to rename to RootDir or Volume ?
        if(!DirObject2) {
            Dir2 = File1->ParentFile;
            DirObject2 = DirObject1;
        } else
        if(DirObject2->FsContext2 &&
          (Fcb2 = ((PtrUDFCCB)(DirObject2->FsContext2))->Fcb)) {
            Dir2 = ((PtrUDFCCB)(DirObject2->FsContext2))->Fcb->FileInfo;
        } else {
            try_return (RC = STATUS_INVALID_PARAMETER);
        }
        // invalid destination ?
        if(!Dir2) try_return (RC = STATUS_ACCESS_DENIED);

        // Stream can't be a Dir or have StreamDir
        if(UDFIsAStreamDir(Dir2)) {
#ifdef UDF_ENABLE_SECURITY
            if(UDFIsADirectory(File1)) {
                try_return (RC = STATUS_ACCESS_DENIED);
            }
            // We should check whether File1 has only Internal
            // (or Deleted) streams. In this case SDir should be
            // removed (in UDFRenameMoveFile__()). Otherwise
            // return STATUS_ACCESS_DENIED
            if(UDFHasAStreamDir(File1)) {
                KdPrint(("TODO: We should remove Streams from source file\n"));
                try_return (RC = STATUS_ACCESS_DENIED);
            }
#else  //UDF_ENABLE_SECURITY
            if(UDFIsADirectory(File1) ||
               UDFHasAStreamDir(File1)) {
                try_return (RC = STATUS_ACCESS_DENIED);
            }
#endif //UDF_ENABLE_SECURITY
        }

        RC = UDFPrepareForRenameMoveLink(Vcb, &AcquiredVcb, &AcquiredVcbEx,
                                         &SingleDir,
                                         &AcquiredDir1, &AcquiredFcb1,
                                         Ccb1, File1,
                                         Dir1, Dir2,
                                         FALSE);  // it is Rename operation
        if(!NT_SUCCESS(RC))
            try_return(RC);

        // check if the source file is in use
        if(Fcb1->OpenHandleCount > 1)
            try_return (RC = STATUS_ACCESS_DENIED);
        ASSERT(Fcb1->OpenHandleCount);
        ASSERT(!Fcb1->IrpContextLite);
        if(Fcb1->IrpContextLite) {
            try_return (RC = STATUS_ACCESS_DENIED);
        }
        // Check if we have parallel/pending Close threads
        if(Fcb1->CcbCount && !SingleDir) {
            // if this is the 1st attempt, we'll try to
            // synchronize with Close requests
            // otherwise fail request
            RC = STATUS_ACCESS_DENIED;
post_rename:
            if(Fcb1->FCBFlags & UDF_FCB_POSTED_RENAME) {
                Fcb1->FCBFlags &= ~UDF_FCB_POSTED_RENAME;
                try_return (RC);
            }
            Fcb1->FCBFlags |= UDF_FCB_POSTED_RENAME;
            try_return (RC = STATUS_PENDING);
        }

        if(!DirObject2) {
            //  Make sure the name is of legal length.
            if(PtrBuffer->FileNameLength > UDF_NAME_LEN*sizeof(WCHAR)) {
                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }
            NewName.Length = NewName.MaximumLength = (USHORT)(PtrBuffer->FileNameLength);
            NewName.Buffer = (PWCHAR)&(PtrBuffer->FileName);
        } else {
            //  This name is by definition legal.
            NewName = *((PUNICODE_STRING)&DirObject2->FileName);
        }
    
        ic = (Ccb1->CCBFlags & UDF_CCB_CASE_SENSETIVE) ? FALSE : TRUE;
    
        AdPrint(("  %ws ->\n    %ws\n",
            Fcb1->FCBName->ObjectName.Buffer,
            NewName.Buffer));

        if(UDFIsDirOpened__(File1)) {
            // We can't rename file because of unclean references.
            // UDF_INFO package can safely do it, but NT side cannot.
            // In this case NT requires STATUS_OBJECT_NAME_COLLISION
            // rather than STATUS_ACCESS_DENIED
            if(NT_SUCCESS(UDFFindFile__(Vcb, ic, &NewName, Dir2)))
                try_return(RC = STATUS_OBJECT_NAME_COLLISION);
            try_return (RC = STATUS_ACCESS_DENIED);
        } else {
            // Last check before Moving.
            // We can't move across Dir referenced (even internally) file
            if(!SingleDir) {
                RC = UDFDoesOSAllowFileToBeMoved__(File1);
                if(!NT_SUCCESS(RC)) {
//                    try_return(RC);
                    goto post_rename;
                }
            }

            ASSERT_REF(Fcb1->ReferenceCount >= File1->RefCount);
            ASSERT_REF(Dir1->Fcb->ReferenceCount >= Dir1->RefCount);
            ASSERT_REF(Dir2->Fcb->ReferenceCount >= Dir2->RefCount);

            RC = UDFRenameMoveFile__(Vcb, ic, &Replace, &NewName, Dir1, Dir2, File1);
        }
        if(!NT_SUCCESS(RC))
            try_return (RC);

        ASSERT(UDFDirIndex(File1->ParentFile->Dloc->DirIndex, File1->Index)->FileInfo == File1);

        RC = MyCloneUnicodeString(&LocalPath, (Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ? 
                                                    &UDFGlobalData.UnicodeStrRoot : 
                                                    &(Dir2->Fcb->FCBName->ObjectName) );
        if(!NT_SUCCESS(RC)) try_return (RC);
//        RC = MyAppendUnicodeStringToString(&LocalPath, (Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ? &(UDFGlobalData.UnicodeStrRoot) : &(Dir2->Fcb->FCBName->ObjectName));
//        if(!NT_SUCCESS(RC)) try_return (RC);
        if(Dir2->ParentFile) {
            RC = MyAppendUnicodeToString(&LocalPath, L"\\");
            if(!NT_SUCCESS(RC)) try_return (RC);
        }
        RC = MyAppendUnicodeStringToStringTag(&LocalPath, &NewName, MEM_USREN_TAG);
        if(!NT_SUCCESS(RC)) try_return (RC);

        // Set Archive bit
        DirNdx = UDFDirIndex(File1->ParentFile->Dloc->DirIndex, File1->Index);
        if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT) {
            Attr = UDFAttributesToNT(DirNdx, File1->Dloc->FileEntry);
            if(!(Attr & FILE_ATTRIBUTE_ARCHIVE))
                UDFAttributesToUDF(DirNdx, File1->Dloc->FileEntry, Attr | FILE_ATTRIBUTE_ARCHIVE);
        }
        // Update Parent Objects (mark 'em as modified)
        if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) {
            if(DirObject1)
                DirObject1->Flags |= FO_FILE_MODIFIED;
            if(DirObject2) {
                DirObject2->Flags |= FO_FILE_MODIFIED;
                if(!Replace)
                    DirObject2->Flags |= FO_FILE_SIZE_CHANGED;
            }
        }
        // report changes
        if(SingleDir && !Replace) {
            UDFNotifyFullReportChange( Vcb, File1,
                                       UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_RENAMED_OLD_NAME);
/*          UDFNotifyFullReportChange( Vcb, File2,
                                       UDFIsADirectory(File2) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_RENAMED_NEW_NAME );*/
            FsRtlNotifyFullReportChange( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
                                         (PSTRING)&LocalPath,
                                         ((Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ? 0 : Dir2->Fcb->FCBName->ObjectName.Length) + sizeof(WCHAR),
                                         NULL,NULL,
                                         UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                         FILE_ACTION_RENAMED_NEW_NAME,
                                         NULL);
        } else {
            UDFNotifyFullReportChange( Vcb, File1,
                                       UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_REMOVED);
            if(Replace) {
/*              UDFNotifyFullReportChange( Vcb, File2,
                                       FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                       FILE_NOTIFY_CHANGE_SIZE |
                                       FILE_NOTIFY_CHANGE_LAST_WRITE |
                                       FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                       FILE_NOTIFY_CHANGE_CREATION |
                                       FILE_NOTIFY_CHANGE_EA,
                                       FILE_ACTION_MODIFIED );*/
                FsRtlNotifyFullReportChange( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
                                             (PSTRING)&LocalPath,
                                             ((Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ?
                                                 0 : Dir2->Fcb->FCBName->ObjectName.Length) + sizeof(WCHAR),
                                             NULL,NULL,
                                             FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                                 FILE_NOTIFY_CHANGE_SIZE |
                                                 FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                 FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                                 FILE_NOTIFY_CHANGE_CREATION |
                                                 FILE_NOTIFY_CHANGE_EA,
                                             FILE_ACTION_MODIFIED,
                                             NULL);
            } else {
/*              UDFNotifyFullReportChange( Vcb, File2,
                                       UDFIsADirectory(File2) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_ADDED );*/
                FsRtlNotifyFullReportChange( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
                                             (PSTRING)&LocalPath,
                                             ((Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ?
                                                 0 : Dir2->Fcb->FCBName->ObjectName.Length) + sizeof(WCHAR),
                                             NULL,NULL,
                                             UDFIsADirectory(File1) ?
                                                 FILE_NOTIFY_CHANGE_DIR_NAME :
                                                 FILE_NOTIFY_CHANGE_FILE_NAME,
                                             FILE_ACTION_ADDED,
                                             NULL);
            }
        }

        // this will prevent structutre release before call to
        // UDFCleanUpFcbChain()
        UDFInterlockedIncrement((PLONG)&(Dir1->Fcb->ReferenceCount));
        UDFInterlockedIncrement((PLONG)&(Dir1->Fcb->NTRequiredFCB->CommonRefCount));
        ASSERT_REF(Dir1->Fcb->ReferenceCount >= Dir1->RefCount);

        // Look through Ccb list & decrement OpenHandleCounter(s)
        // acquire CcbList
        if(!SingleDir) {
            UDFAcquireResourceExclusive(&(Fcb1->CcbListResource),TRUE);
            Link = Fcb1->NextCCB.Flink;
            DirRefCount = 0;
            FileInfoRefCount = 0;
            ASSERT(Link != &(Fcb1->NextCCB));
            while (Link != &(Fcb1->NextCCB)) {
                NextFileInfo = Dir1;
                CurCcb = CONTAINING_RECORD(Link, UDFCCB, NextCCB);
                ASSERT(CurCcb->TreeLength);
                i = (CurCcb->TreeLength) ? (CurCcb->TreeLength - 1) : 0;
                Link = Link->Flink;
                UseClose = (CurCcb->CCBFlags & UDF_CCB_CLEANED) ? FALSE : TRUE;

                AdPrint(("  Ccb:%x:%s:i:%x\n", CurCcb, UseClose ? "Close" : "",i));
                // cleanup old parent chain
                for(; i && NextFileInfo; i--) {
                    // remember parent file now
                    // it will prevent us from data losses
                    // due to eventual structure release
                    fi = NextFileInfo->ParentFile;
                    if(UseClose) {
                        ASSERT_REF(NextFileInfo->Fcb->ReferenceCount >= NextFileInfo->RefCount);
                        UDFCloseFile__(Vcb, NextFileInfo);
                    }
                    ASSERT_REF(NextFileInfo->Fcb->ReferenceCount > NextFileInfo->RefCount);
                    ASSERT_REF(NextFileInfo->Fcb->ReferenceCount);
                    ASSERT_REF(NextFileInfo->Fcb->NTRequiredFCB->CommonRefCount);
                    UDFInterlockedDecrement((PLONG)&(NextFileInfo->Fcb->ReferenceCount));
                    UDFInterlockedDecrement((PLONG)&(NextFileInfo->Fcb->NTRequiredFCB->CommonRefCount));
                    ASSERT_REF(NextFileInfo->Fcb->ReferenceCount >= NextFileInfo->RefCount);
                    NextFileInfo = fi;
                }

                if(CurCcb->TreeLength > 1) {
                    DirRefCount++;
                    if(UseClose)
                        FileInfoRefCount++;
                    CurCcb->TreeLength = 2;
#ifdef UDF_DBG
                } else {
                    BrutePoint();
#endif // UDF_DBG
                }
            }
            UDFReleaseResource(&(Fcb1->CcbListResource));

            ASSERT_REF(DirRefCount >= FileInfoRefCount);
            // update counters & pointers
            Fcb1->ParentFcb = Dir2->Fcb;
            // move references to Dir2
            UDFInterlockedExchangeAdd((PLONG)&(Dir2->Fcb->ReferenceCount), DirRefCount);
            UDFInterlockedExchangeAdd((PLONG)&(Dir2->Fcb->NTRequiredFCB->CommonRefCount), DirRefCount);
            ASSERT_REF(Dir2->Fcb->ReferenceCount > Dir2->RefCount);
            UDFReferenceFileEx__(Dir2,FileInfoRefCount);
            ASSERT_REF(Dir2->Fcb->ReferenceCount >= Dir2->RefCount);
        }
        ASSERT_REF(Dir2->Fcb->ReferenceCount >= Dir2->RefCount);
        ASSERT_REF(Dir2->RefCount);

        ASSERT_REF(Dir1->Fcb->ReferenceCount >= Dir1->RefCount);
        // Modify name in Fcb1
        if(Fcb1->FCBName) {
            if(Fcb1->FCBName->ObjectName.Buffer) {
                MyFreePool__(Fcb1->FCBName->ObjectName.Buffer);
            }
            UDFReleaseObjectName(Fcb1->FCBName);
        }
        Fcb1->FCBName = UDFAllocateObjectName();
        if(!(Fcb1->FCBName)) {
insuf_res:
            BrutePoint();
            // UDFCleanUpFcbChain()...
            if(AcquiredFcb1) {
                UDF_CHECK_PAGING_IO_RESOURCE(Fcb1->NTRequiredFCB);
                UDFReleaseResource(&(Fcb1->NTRequiredFCB->MainResource));
                AcquiredDir1 = FALSE;
            }
            if(AcquiredDir1) {
                UDF_CHECK_PAGING_IO_RESOURCE(Dir1->Fcb->NTRequiredFCB);
                UDFReleaseResource(&(Dir1->Fcb->NTRequiredFCB->MainResource));
                AcquiredDir1 = FALSE;
            }
            UDFCleanUpFcbChain(Vcb, Dir1, 1, TRUE);
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }

        RC = MyCloneUnicodeString(&(Fcb1->FCBName->ObjectName), &(Fcb2->FCBName->ObjectName));
        if(!NT_SUCCESS(RC))
            goto insuf_res;
/*        RC = MyAppendUnicodeStringToString(&(Fcb1->FCBName->ObjectName), &(Fcb2->FCBName->ObjectName));
        if(!NT_SUCCESS(RC))
            goto insuf_res;*/
        // if Dir2 is a RootDir, we shoud not append '\\' because
        // uit will be the 2nd '\\' character (RootDir's name is also '\\')
        if(Dir2->ParentFile) {
            RC = MyAppendUnicodeToString(&(Fcb1->FCBName->ObjectName), L"\\");
            if(!NT_SUCCESS(RC))
                goto insuf_res;
        }
        RC = MyAppendUnicodeStringToStringTag(&(Fcb1->FCBName->ObjectName), &NewName, MEM_USREN2_TAG);
        if(!NT_SUCCESS(RC))
            goto insuf_res;

        ASSERT_REF(Fcb1->ReferenceCount >= File1->RefCount);
        ASSERT_REF(Dir1->Fcb->ReferenceCount >= Dir1->RefCount);
        ASSERT_REF(Dir2->Fcb->ReferenceCount >= Dir2->RefCount);

        RC = STATUS_SUCCESS;

try_exit:    NOTHING;

    } _SEH2_FINALLY {

        if(AcquiredFcb1) {
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb1->NTRequiredFCB);
            UDFReleaseResource(&(Fcb1->NTRequiredFCB->MainResource));
        }
        if(AcquiredDir1) {
            UDF_CHECK_PAGING_IO_RESOURCE(Dir1->Fcb->NTRequiredFCB);
            UDFReleaseResource(&(Dir1->Fcb->NTRequiredFCB->MainResource));
        }
        // perform protected structure release
        if(NT_SUCCESS(RC) &&
           (RC != STATUS_PENDING)) {
            ASSERT(AcquiredVcb);
            UDFCleanUpFcbChain(Vcb, Dir1, 1, TRUE);
            ASSERT_REF(Fcb1->ReferenceCount >= File1->RefCount);
            ASSERT_REF(Dir2->Fcb->ReferenceCount >= Dir2->RefCount);
        }

        if(AcquiredVcb) {
            if(AcquiredVcbEx)
                UDFConvertExclusiveToSharedLite(&(Vcb->VCBResource));
        } else {
            // caller assumes Vcb to be acquired shared
            BrutePoint();
            UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
        }

        if(LocalPath.Buffer) {
            MyFreePool__(LocalPath.Buffer);
        }
    } _SEH2_END;

    return RC;
} // end UDFRename()

#endif //UDF_READ_ONLY_BUILD

LONG
UDFFindFileId(
    IN PVCB Vcb,
    IN LONGLONG Id
    )
{
    if(!Vcb->FileIdCache) return (-1);
    for(ULONG i=0; i<Vcb->FileIdCount; i++) {
        if(Vcb->FileIdCache[i].Id == Id) return i;
    }
    return (-1);
} // end UDFFindFileId()

LONG
UDFFindFreeFileId(
    IN PVCB Vcb,
    IN LONGLONG Id
    )
{
    if(!Vcb->FileIdCache) {
        if(!(Vcb->FileIdCache = (PUDFFileIDCacheItem)MyAllocatePool__(NonPagedPool, sizeof(UDFFileIDCacheItem)*FILE_ID_CACHE_GRANULARITY)))
            return (-1);
        RtlZeroMemory(Vcb->FileIdCache, FILE_ID_CACHE_GRANULARITY*sizeof(UDFFileIDCacheItem));
        Vcb->FileIdCount = FILE_ID_CACHE_GRANULARITY;
    }
    for(ULONG i=0; i<Vcb->FileIdCount; i++) {
        if(!Vcb->FileIdCache[i].FullName.Buffer) return i;
    }
    if(!MyReallocPool__((PCHAR)(Vcb->FileIdCache), Vcb->FileIdCount*sizeof(UDFFileIDCacheItem),
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
    IN PtrUDFCCB Ccb,
    IN PUDF_FILE_INFO fi,
    IN LONGLONG Id
    )
{
    LONG i;
    NTSTATUS RC = STATUS_SUCCESS;

    if((i = UDFFindFileId(Vcb, Id)) == (-1)) {
        if((i = UDFFindFreeFileId(Vcb, Id)) == (-1)) return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        return STATUS_SUCCESS;
    }
    Vcb->FileIdCache[i].Id = Id;
    Vcb->FileIdCache[i].CaseSens = (Ccb->CCBFlags & UDF_CCB_CASE_SENSETIVE) ? TRUE : FALSE;
    RC = MyCloneUnicodeString(&(Vcb->FileIdCache[i].FullName), &(Ccb->Fcb->FCBName->ObjectName));
/*    if(NT_SUCCESS(RC)) {
        RC = MyAppendUnicodeStringToStringTag(&(Vcb->FileIdCache[i].FullName), &(Ccb->Fcb->FCBName->ObjectName), MEM_USFIDC_TAG);
    }*/
    return RC;
} // end UDFStoreFileId()

NTSTATUS
UDFRemoveFileId(
    IN PVCB Vcb,
    IN LONGLONG Id
    )
{
    LONG i;

    if((i = UDFFindFileId(Vcb, Id)) == (-1)) return STATUS_INVALID_PARAMETER;
    MyFreePool__(Vcb->FileIdCache[i].FullName.Buffer);
    RtlZeroMemory(&(Vcb->FileIdCache[i]), sizeof(UDFFileIDCacheItem));
    return STATUS_SUCCESS;
} // end UDFRemoveFileId()

VOID
UDFReleaseFileIdCache(
    IN PVCB Vcb
    )
{
    if(!Vcb->FileIdCache) return;
    for(ULONG i=0; i<Vcb->FileIdCount; i++) {
        if(Vcb->FileIdCache[i].FullName.Buffer) {
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
    IN LONGLONG Id,
    OUT PUNICODE_STRING* FName,
    OUT BOOLEAN* CaseSens
    )
{
    LONG i;

    if((i = UDFFindFileId(Vcb, Id)) == (-1)) return STATUS_NOT_FOUND;
    (*FName) = &(Vcb->FileIdCache[i].FullName);
    (*CaseSens) = !(Vcb->FileIdCache[i].CaseSens);
    return STATUS_SUCCESS;
} // end UDFGetOpenParamsByFileId()

#ifndef UDF_READ_ONLY_BUILD

#ifdef UDF_ALLOW_HARD_LINKS
/*
    create hard link for the file
 */
NTSTATUS
UDFHardLink(
    IN PIO_STACK_LOCATION PtrSp,
    IN PtrUDFFCB Fcb1,
    IN PtrUDFCCB Ccb1,
    IN PFILE_OBJECT FileObject1,   // Source File
    IN PFILE_LINK_INFORMATION PtrBuffer
    )
{
    // Target Directory
    PFILE_OBJECT DirObject2 = PtrSp->Parameters.SetFile.FileObject;
    // Overwite Flag
    BOOLEAN Replace = PtrSp->Parameters.SetFile.ReplaceIfExists &&
                      PtrBuffer->ReplaceIfExists;
    NTSTATUS RC;
    PVCB Vcb = Fcb1->Vcb;
    PtrUDFFCB Fcb2;
    BOOLEAN ic;
    BOOLEAN AcquiredVcb = TRUE;
    BOOLEAN AcquiredVcbEx = FALSE;
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
        if(!(File1 = Fcb1->FileInfo))
            try_return (RC = STATUS_ACCESS_DENIED);

        // do we try to link RootDir ?
        if(!(Dir1 = File1->ParentFile))
            try_return (RC = STATUS_ACCESS_DENIED);

        // do we try to link Stream / Stream Dir ?
#ifdef UDF_ALLOW_LINKS_TO_STREAMS
        if(UDFIsAStreamDir(File1))
            try_return (RC = STATUS_ACCESS_DENIED);
#else //UDF_ALLOW_LINKS_TO_STREAMS
        if(UDFIsAStream(File1) || UDFIsAStreamDir(File1) /*||
           UDFIsADirectory(File1) || UDFHasAStreamDir(File1)*/)
            try_return (RC = STATUS_ACCESS_DENIED);
#endif // UDF_ALLOW_LINKS_TO_STREAMS

        // do we try to link to RootDir or Volume ?
        if(!DirObject2) {
            Dir2 = File1->ParentFile;
            DirObject2 = FileObject1->RelatedFileObject;
        } else
        if(DirObject2->FsContext2 &&
          (Fcb2 = ((PtrUDFCCB)(DirObject2->FsContext2))->Fcb)) {
            Dir2 = ((PtrUDFCCB)(DirObject2->FsContext2))->Fcb->FileInfo;
        } else {
            try_return (RC = STATUS_INVALID_PARAMETER);
        }

        // check target dir
        if(!Dir2) try_return (RC = STATUS_ACCESS_DENIED);

        // Stream can't be a Dir or have Streams
        if(UDFIsAStreamDir(Dir2)) {
            try_return (RC = STATUS_ACCESS_DENIED);
/*            if(UDFIsADirectory(File1) ||
               UDFHasAStreamDir(File1)) {
                BrutePoint();
                try_return (RC = STATUS_ACCESS_DENIED);
            }*/
        }

/*        if(UDFIsAStreamDir(Dir2))
            try_return (RC = STATUS_ACCESS_DENIED);*/

        RC = UDFPrepareForRenameMoveLink(Vcb, &AcquiredVcb, &AcquiredVcbEx,
                                         &SingleDir,
                                         &AcquiredDir1, &AcquiredFcb1,
                                         Ccb1, File1,
                                         Dir1, Dir2,
                                         TRUE); // it is HLink operation
        if(!NT_SUCCESS(RC))
            try_return(RC);

        // check if the source file is used
        if(!DirObject2) {
            //  Make sure the name is of legal length.
            if(PtrBuffer->FileNameLength > UDF_NAME_LEN*sizeof(WCHAR)) {
                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }
            NewName.Length = NewName.MaximumLength = (USHORT)(PtrBuffer->FileNameLength);
            NewName.Buffer = (PWCHAR)&(PtrBuffer->FileName);
        } else {
            //  This name is by definition legal.
            NewName = *((PUNICODE_STRING)&DirObject2->FileName);
        }
    
        ic = (Ccb1->CCBFlags & UDF_CCB_CASE_SENSETIVE) ? FALSE : TRUE;

        AdPrint(("  %ws ->\n    %ws\n",
            Fcb1->FCBName->ObjectName.Buffer,
            NewName.Buffer));

        RC = UDFHardLinkFile__(Vcb, ic, &Replace, &NewName, Dir1, Dir2, File1);
        if(!NT_SUCCESS(RC)) try_return (RC);

        // Update Parent Objects (mark 'em as modified)
        if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) {
            if(DirObject2) {
                DirObject2->Flags |= FO_FILE_MODIFIED;
                if(!Replace)
                    DirObject2->Flags |= FO_FILE_SIZE_CHANGED;
            }
        }
        // report changes
        UDFNotifyFullReportChange( Vcb, File1,
                                   FILE_NOTIFY_CHANGE_LAST_WRITE |
                                   FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                   FILE_ACTION_MODIFIED );

        RC = MyCloneUnicodeString(&LocalPath, (Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ?
                                                    &UDFGlobalData.UnicodeStrRoot :
                                                    &(Dir2->Fcb->FCBName->ObjectName));
        if(!NT_SUCCESS(RC)) try_return (RC);
/*        RC = MyAppendUnicodeStringToString(&LocalPath, (Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ? &(UDFGlobalData.UnicodeStrRoot) : &(Dir2->Fcb->FCBName->ObjectName));
        if(!NT_SUCCESS(RC)) try_return (RC);*/
        // if Dir2 is a RootDir, we shoud not append '\\' because
        // it will be the 2nd '\\' character (RootDir's name is also '\\')
        if(Dir2->ParentFile) {
            RC = MyAppendUnicodeToString(&LocalPath, L"\\");
            if(!NT_SUCCESS(RC)) try_return (RC);
        }
        RC = MyAppendUnicodeStringToStringTag(&LocalPath, &NewName, MEM_USHL_TAG);
        if(!NT_SUCCESS(RC)) try_return (RC);

        if(!Replace) {
/*          UDFNotifyFullReportChange( Vcb, File2,
                                       UDFIsADirectory(File1) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_ADDED );*/
            FsRtlNotifyFullReportChange( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
                                         (PSTRING)&LocalPath,
                                         ((Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ? 0 : Dir2->Fcb->FCBName->ObjectName.Length) + sizeof(WCHAR),
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
            FsRtlNotifyFullReportChange( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
                                         (PSTRING)&LocalPath,
                                         ((Dir2->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY) ? 0 : Dir2->Fcb->FCBName->ObjectName.Length) + sizeof(WCHAR),
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

        if(AcquiredFcb1) {
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb1->NTRequiredFCB);
            UDFReleaseResource(&(Fcb1->NTRequiredFCB->MainResource));
        }
        if(AcquiredDir1) {
            UDF_CHECK_PAGING_IO_RESOURCE(Dir1->Fcb->NTRequiredFCB);
            UDFReleaseResource(&(Dir1->Fcb->NTRequiredFCB->MainResource));
        }
        if(AcquiredVcb) {
            if(AcquiredVcbEx)
                UDFConvertExclusiveToSharedLite(&(Vcb->VCBResource));
        } else {
            // caller assumes Vcb to be acquired shared
            BrutePoint();
            UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
        }

        if(LocalPath.Buffer) {
            MyFreePool__(LocalPath.Buffer);
        }
    } _SEH2_END;

    return RC;
} // end UDFHardLink()
#endif //UDF_ALLOW_HARD_LINKS

#endif //UDF_READ_ONLY_BUILD
