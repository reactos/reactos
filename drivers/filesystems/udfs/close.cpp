////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Close.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Close" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_CLOSE

typedef BOOLEAN      (*PCHECK_TREE_ITEM) (IN PUDF_FILE_INFO   FileInfo);
#define TREE_ITEM_LIST_GRAN 32

NTSTATUS
UDFBuildTreeItemsList(
    IN PVCB               Vcb,
    IN PUDF_FILE_INFO     FileInfo,
    IN PCHECK_TREE_ITEM   CheckItemProc,
    IN PUDF_DATALOC_INFO** PassedList,
    IN PULONG             PassedListSize,
    IN PUDF_DATALOC_INFO** FoundList,
    IN PULONG             FoundListSize);

// callbacks, can't be __fastcall
BOOLEAN
UDFIsInDelayedCloseQueue(
    PUDF_FILE_INFO FileInfo);

BOOLEAN
UDFIsLastClose(
    PUDF_FILE_INFO FileInfo);

/*************************************************************************
*
* Function: UDFClose()
*
* Description:
*   The I/O Manager will invoke this routine to handle a close
*   request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*   to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS
*
*************************************************************************/
NTSTATUS
NTAPI
UDFClose(
    PDEVICE_OBJECT  DeviceObject,  // the logical volume device object
    PIRP            Irp            // I/O Request Packet
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext = NULL;
    BOOLEAN             AreWeTopLevel = FALSE;

    AdPrint(("UDFClose: \n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    if (UDFIsFSDevObj(DeviceObject)) {
        // this is a close of the FSD itself
        Irp->IoStatus.Status = RC;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        FsRtlExitFileSystem();
        return(RC);
    }

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        ASSERT(PtrIrpContext);

        RC = UDFCommonClose(PtrIrpContext, Irp);

    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
}




/*************************************************************************
*
* Function: UDFCommonClose()
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
* Return Value: must be STATUS_SUCCESS
*
*************************************************************************/
NTSTATUS
UDFCommonClose(
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
//    PERESOURCE              PtrResourceAcquired = NULL;
    BOOLEAN                 AcquiredVcb = FALSE;
    BOOLEAN                 AcquiredGD = FALSE;
    PUDF_FILE_INFO          fi;
    ULONG                   i = 0;
//    ULONG                   clean_stat = 0;

//    BOOLEAN                 CompleteIrp = TRUE;
    BOOLEAN                 PostRequest = FALSE;

#ifdef UDF_DBG
    UNICODE_STRING          CurName;
    PDIR_INDEX_HDR          DirNdx;
#endif

    AdPrint(("UDFCommonClose: \n"));

    _SEH2_TRY {
        if (Irp) {

            // If this is the first (IOManager) request
            // First, get a pointer to the current I/O stack location
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            ASSERT(IrpSp);

            FileObject = IrpSp->FileObject;
            ASSERT(FileObject);

            // Get the FCB and CCB pointers
            Ccb = (PtrUDFCCB)(FileObject->FsContext2);
            ASSERT(Ccb);
            if(Ccb->CCBFlags & UDF_CCB_READ_ONLY) {
                PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_READ_ONLY;
            }
            Fcb = Ccb->Fcb;
        } else {
            // If this is a queued call (for our dispatch)
            // Get saved Fcb address
            Fcb = PtrIrpContext->Fcb;
            i = PtrIrpContext->TreeLength;
        }

        ASSERT(Fcb);
        Vcb = (PVCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
        ASSERT(Vcb);
        ASSERT(Vcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB);
//        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

        // Steps we shall take at this point are:
        // (a) Acquire the VCB shared
        // (b) Acquire the FCB's CCB list exclusively
        // (c) Delete the CCB structure (free memory)
        // (d) If this is the last close, release the FCB structure
        //       (unless we keep these around for "delayed close" functionality.
        // Note that it is often the case that the close dispatch entry point is invoked
        // in the most inconvenient of situations (when it is not possible, for example,
        // to safely acquire certain required resources without deadlocking or waiting).
        // Therefore, be extremely careful in implementing this close dispatch entry point.
        // Also note that we do not have the option of returning a failure code from the
        // close dispatch entry point; the system expects that the close will always succeed.

        UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
        AcquiredVcb = TRUE;

        // Is this is the first (IOManager) request ?
        if (Irp) {
            PtrIrpContext->TreeLength =
            i = Ccb->TreeLength;
            // remember the number of incomplete Close requests
            InterlockedIncrement((PLONG)&(Fcb->CcbCount));
            // we can release CCB in any case
            UDFCleanUpCCB(Ccb);
            FileObject->FsContext2 = NULL;
#ifdef DBG
/*        } else {
            ASSERT(Fcb->NTRequiredFCB);
            if(Fcb->NTRequiredFCB) {
                ASSERT(Fcb->NTRequiredFCB->FileObject);
                if(Fcb->NTRequiredFCB->FileObject) {
                    ASSERT(!Fcb->NTRequiredFCB->FileObject->FsContext2);
                }
            }*/
#endif //DBG
        }

#ifdef UDF_DELAYED_CLOSE
        // check if this is the last Close (no more Handles)
        // and try to Delay it....
        if((Fcb->FCBFlags & UDF_FCB_DELAY_CLOSE) &&
           (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) &&
          !(Vcb->VCBFlags & UDF_VCB_FLAGS_NO_DELAYED_CLOSE) &&
          !(Fcb->OpenHandleCount)) {
            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVcb = FALSE;
            if((RC = UDFQueueDelayedClose(PtrIrpContext,Fcb)) == STATUS_SUCCESS)
                try_return(RC = STATUS_SUCCESS);
            // do standard Close if we can't Delay this opeartion
            AdPrint(("   Cant queue Close Irp, status=%x\n", RC));
        }
#endif //UDF_DELAYED_CLOSE

        if(Irp) {
            // We should post actual procesing if this is a recursive call
            if((PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_NOT_TOP_LEVEL) ||
               (Fcb->NTRequiredFCB->AcqFlushCount)) {
                AdPrint(("   post NOT_TOP_LEVEL Irp\n"));
                PostRequest = TRUE;
                try_return(RC = STATUS_SUCCESS);
            }
        }

        // Close request is near completion, Vcb is acquired.
        // Now we can safely decrease CcbCount, because no Rename
        // operation can run until Vcb release.
        InterlockedDecrement((PLONG)&(Fcb->CcbCount));

        UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));
        if(PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_READ_ONLY)
            UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCountRO));

        if(!i || (Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB)) {

            AdPrint(("UDF: Closing volume\n"));
            AdPrint(("UDF: ReferenceCount:  %x\n",Fcb->ReferenceCount));

            if (Vcb->VCBOpenCount > UDF_RESIDUAL_REFERENCE) {
                ASSERT(Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB);
                UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
                ASSERT(Fcb->NTRequiredFCB);
                UDFInterlockedDecrement((PLONG)&(Fcb->NTRequiredFCB->CommonRefCount));

                try_return(RC = STATUS_SUCCESS);
            }

            UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));

            if(AcquiredVcb) {
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            } else {
                BrutePoint();
            }
            // Acquire GlobalDataResource
            UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);
            AcquiredGD = TRUE;
//            // Acquire Vcb
            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
            AcquiredVcb = TRUE;

            UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));


            ASSERT(Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB);
            UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
            ASSERT(Fcb->NTRequiredFCB);
            UDFInterlockedDecrement((PLONG)&(Fcb->NTRequiredFCB->CommonRefCount));

            //AdPrint(("UDF: Closing volume, reset driver (e.g. stop BGF)\n"));
            //UDFResetDeviceDriver(Vcb, Vcb->TargetDeviceObject, FALSE);

            AdPrint(("UDF: Closing volume, reset write status\n"));
            RC = UDFPhSendIOCTL(IOCTL_CDRW_RESET_WRITE_STATUS, Vcb->TargetDeviceObject,
                NULL, 0, NULL, 0, TRUE, NULL);

            if((Vcb->VCBFlags & UDF_VCB_FLAGS_BEING_DISMOUNTED) ||
                ((!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) && (Vcb->VCBOpenCount <= UDF_RESIDUAL_REFERENCE))) {
                // Try to KILL dismounted volume....
                // w2k requires this, NT4 - recomends
                AcquiredVcb = UDFCheckForDismount(PtrIrpContext, Vcb, TRUE);
            }

            try_return(RC = STATUS_SUCCESS);
        }

        fi = Fcb->FileInfo;
#ifdef UDF_DBG
        if(!fi) {
            BrutePoint();
        }

        DirNdx = UDFGetDirIndexByFileInfo(fi);
        if(DirNdx) {
            CurName.Buffer = UDFDirIndex(DirNdx,fi->Index)->FName.Buffer;
            if(CurName.Buffer) {
                AdPrint(("Closing file: %ws %8.8x\n", CurName.Buffer, FileObject));
            } else {
                AdPrint(("Closing file: ??? \n"));
            }
        }
        AdPrint(("UDF: ReferenceCount:  %x\n",Fcb->ReferenceCount));
#endif // UDF_DBG
        // try to clean up as long chain as it is possible
        UDFCleanUpFcbChain(Vcb, fi, i, TRUE);

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(AcquiredVcb) {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
        if(AcquiredGD) {
            UDFReleaseResource(&(UDFGlobalData.GlobalDataResource));
        }

        // Post IRP if required
        if (PostRequest) {

            // Perform the post operation & complete the IRP
            // if this is first call of UDFCommonClose
            // and will return STATUS_SUCCESS back to us
            PtrIrpContext->Irp = NULL;
            PtrIrpContext->Fcb = Fcb;
            UDFPostRequest(PtrIrpContext, NULL);
        }

        if (!_SEH2_AbnormalTermination()) {
            // If this is not async close complete the IRP
            if (Irp) {
/*                if( FileObject ) {
                    if(clean_stat & UDF_CLOSE_NTREQFCB_DELETED) {
//                        ASSERT(!FileObject->FsContext2);
                        FileObject->FsContext = NULL;
#ifdef DBG
                    } else {
                        UDFNTRequiredFCB*  NtReqFcb = ((UDFNTRequiredFCB*)(FileObject->FsContext));
                        if(NtReqFcb->FileObject == FileObject) {
                            NtReqFcb->FileObject = NULL;
                        }
#endif //DBG
                    }
                }*/
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            }
            // Free up the Irp Context
            if(!PostRequest)
                UDFReleaseIrpContext(PtrIrpContext);
        }

    } _SEH2_END; // end of "__finally" processing

    return STATUS_SUCCESS ;
} // end UDFCommonClose()

/*
    This routine walks through the tree to RootDir & kills all unreferenced
    structures....
    imho, Useful feature
 */
ULONG
UDFCleanUpFcbChain(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO fi,
    IN ULONG TreeLength,
    IN BOOLEAN VcbAcquired
    )
{
    PtrUDFFCB      Fcb = NULL;
    PtrUDFFCB      ParentFcb = NULL;
    PUDF_FILE_INFO ParentFI;
    UDFNTRequiredFCB* NtReqFcb;
    ULONG CleanCode;
    LONG RefCount, ComRefCount;
    BOOLEAN Delete = FALSE;
    ULONG          ret_val = 0;

    ValidateFileInfo(fi);
    AdPrint(("UDFCleanUpFcbChain\n"));

    ASSERT(TreeLength);

    // we can't process Tree until we can acquire Vcb
    if(!VcbAcquired)
        UDFAcquireResourceShared(&(Vcb->VCBResource),TRUE);

    // cleanup parent chain (if any & unused)
    while(fi) {

        // acquire parent
        if((ParentFI = fi->ParentFile)) {
            ASSERT(fi->Fcb);
            ParentFcb = fi->Fcb->ParentFcb;
            ASSERT(ParentFcb);
            ASSERT(ParentFcb->NTRequiredFCB);
            UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(&(ParentFcb->NTRequiredFCB->MainResource),TRUE);
        } else {
            // we get to RootDir, it has no parent
            if(!VcbAcquired)
                UDFAcquireResourceShared(&(Vcb->VCBResource),TRUE);
        }
        Fcb = fi->Fcb;
        ASSERT(Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_FCB);

        NtReqFcb = Fcb->NTRequiredFCB;
        ASSERT(NtReqFcb->CommonFCBHeader.NodeTypeCode == UDF_NODE_TYPE_NT_REQ_FCB);

        // acquire current file/dir
        // we must assure that no more threads try to re-use this object
#ifdef UDF_DBG
        _SEH2_TRY {
#endif // UDF_DBG
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFAcquireResourceExclusive(&(NtReqFcb->MainResource),TRUE);
#ifdef UDF_DBG
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            BrutePoint();
            if(ParentFI) {
                UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
                UDFReleaseResource(&(ParentFcb->NTRequiredFCB->MainResource));
            } else {
                if(!VcbAcquired)
                    UDFReleaseResource(&(Vcb->VCBResource));
            }
            break;
        } _SEH2_END;
#endif // UDF_DBG
        ASSERT_REF((Fcb->ReferenceCount > fi->RefCount) || !TreeLength);
        // If we haven't pass through all files opened
        // in UDFCommonCreate before target file (TreeLength specfies
        // the number of such files) dereference them.
        // Otherwise we'll just check if the file has no references.
#ifdef UDF_DBG
        if(Fcb) {
            if(TreeLength) {
                ASSERT(Fcb->ReferenceCount);
                ASSERT(NtReqFcb->CommonRefCount);
                RefCount = UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
                ComRefCount = UDFInterlockedDecrement((PLONG)&(NtReqFcb->CommonRefCount));
            }
        } else {
            BrutePoint();
        }
        if(TreeLength)
            TreeLength--;
        ASSERT(Fcb->OpenHandleCount <= Fcb->ReferenceCount);
#else
        if(TreeLength) {
            RefCount = UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
            ComRefCount = UDFInterlockedDecrement((PLONG)&(NtReqFcb->CommonRefCount));
            TreeLength--;
        }
#endif

/*        if(Fcb && Fcb->FCBName && Fcb->FCBName->ObjectName.Buffer) {
            AdPrint(("    %ws (%x)\n",
                       Fcb->FCBName->ObjectName.Buffer,Fcb->ReferenceCount));
        } else if (Fcb) {
            AdPrint(("    ??? (%x)\n",Fcb->ReferenceCount));
        } else {
            AdPrint(("    ??? (??)\n"));
        }*/
        // ...and delete if it has gone

        if(!RefCount && !Fcb->OpenHandleCount) {
            // no more references... current file/dir MUST DIE!!!
            BOOLEAN AutoInherited = UDFIsAStreamDir(fi) || UDFIsAStream(fi);

            if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) {
                // do nothing
            } else
#ifndef UDF_READ_ONLY_BUILD
            if(Delete) {
/*                if(!(Fcb->FCBFlags & UDF_FCB_DIRECTORY)) {
                    // set file size to zero (for UdfInfo package)
                    // we should not do this for directories
                    UDFResizeFile__(Vcb, fi, 0);
                }*/
                UDFReferenceFile__(fi);
                ASSERT(Fcb->ReferenceCount < fi->RefCount);
                UDFFlushFile__(Vcb, fi);
                UDFUnlinkFile__(Vcb, fi, TRUE);
                UDFCloseFile__(Vcb, fi);
                ASSERT(Fcb->ReferenceCount == fi->RefCount);
                Fcb->FCBFlags |= UDF_FCB_DELETED;
                Delete = FALSE;
            } else
#endif //UDF_READ_ONLY_BUILD
            if(!(Fcb->FCBFlags & UDF_FCB_DELETED)) {
                UDFFlushFile__(Vcb, fi);
            } else {
//                BrutePoint();
            }
#ifndef UDF_READ_ONLY_BUILD
            // check if we should try to delete Parent for the next time
            if(Fcb->FCBFlags & UDF_FCB_DELETE_PARENT)
                Delete = TRUE;
#endif //UDF_READ_ONLY_BUILD

            // remove references to OS-specific structures
            // to let UDF_INFO release FI & Co
            fi->Fcb = NULL;
            if(!ComRefCount) {
                // CommonFcb is also completly dereferenced
                // Kill it!
                fi->Dloc->CommonFcb = NULL;
            }

            if((CleanCode = UDFCleanUpFile__(Vcb, fi))) {
                // Check, if we can uninitialize & deallocate CommonFcb part
                // kill some cross links
                Fcb->FileInfo = NULL;
                // release allocated resources
                if(CleanCode & UDF_FREE_DLOC) {
                    // Obviously, it is a good time & place to release
                    // CommonFcb structure

//                    NtReqFcb->NtReqFCBFlags &= ~UDF_NTREQ_FCB_VALID;
                    // Unitialize byte-range locks support structure
                    FsRtlUninitializeFileLock(&(NtReqFcb->FileLock));
                    // Remove resources
                    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                    UDFReleaseResource(&(NtReqFcb->MainResource));
                    if(NtReqFcb->CommonFCBHeader.Resource) {
                        UDFDeleteResource(&(NtReqFcb->MainResource));
                        UDFDeleteResource(&(NtReqFcb->PagingIoResource));
                    }
                    NtReqFcb->CommonFCBHeader.Resource =
                    NtReqFcb->CommonFCBHeader.PagingIoResource = NULL;
                    UDFDeassignAcl(NtReqFcb, AutoInherited);
                    UDFPrint(("UDFReleaseNtReqFCB: %x\n", NtReqFcb));
#ifdef DBG
//                    NtReqFcb->FileObject->FsContext2 = NULL;
//                    ASSERT(NtReqFcb->FileObject);
/*                    if(NtReqFcb->FileObject) {
                        ASSERT(!NtReqFcb->FileObject->FsContext2);
                        NtReqFcb->FileObject->FsContext = NULL;
                        NtReqFcb->FileObject->SectionObjectPointer = NULL;
                    }*/
#endif //DBG
                    MyFreePool__(NtReqFcb);
                    ret_val |= UDF_CLOSE_NTREQFCB_DELETED;
                } else {
                    // we usually get here when the file has some opened links
                    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                    UDFReleaseResource(&(NtReqFcb->MainResource));
                }
                // remove some references & free Fcb structure
                Fcb->NTRequiredFCB = NULL;
                Fcb->ParentFcb = NULL;
                UDFCleanUpFCB(Fcb);
                MyFreePool__(fi);
                ret_val |= UDF_CLOSE_FCB_DELETED;
                // get pointer to parent FCB
                fi = ParentFI;
                // free old parent's resource...
                if(fi) {
                    UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
                    UDFReleaseResource(&(ParentFcb->NTRequiredFCB->MainResource));
                } else {
                    if(!VcbAcquired)
                        UDFReleaseResource(&(Vcb->VCBResource));
                }
            } else {
                // Stop cleaning up

                // Restore pointers
                fi->Fcb = Fcb;
                fi->Dloc->CommonFcb = NtReqFcb;
                // free all acquired resources
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                UDFReleaseResource(&(NtReqFcb->MainResource));
                fi = ParentFI;
                if(fi) {
                    UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
                    UDFReleaseResource(&(ParentFcb->NTRequiredFCB->MainResource));
                } else {
                    if(!VcbAcquired)
                        UDFReleaseResource(&(Vcb->VCBResource));
                }
                // If we have dereferenced all parents 'associated'
                // with input file & current file is still in use
                // then it isn't worth walking down the tree
                // 'cause in this case all the rest files are also used
                if(!TreeLength)
                    break;
//                AdPrint(("Stop on referenced File/Dir\n"));
            }
        } else {
            // we get to referenced file/dir. Stop search & release resource
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            if(ParentFI) {
                UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
                UDFReleaseResource(&(ParentFcb->NTRequiredFCB->MainResource));
            } else {
                if(!VcbAcquired)
                    UDFReleaseResource(&(Vcb->VCBResource));
            }
            Delete = FALSE;
            if(!TreeLength)
                break;
            fi = ParentFI;
        }
    }
    if(fi) {
        Fcb = fi->Fcb;
        for(;TreeLength && fi;TreeLength--) {
            if(Fcb) {
                ParentFcb = Fcb->ParentFcb;
                ASSERT(Fcb->ReferenceCount);
                ASSERT(Fcb->NTRequiredFCB->CommonRefCount);
                ASSERT_REF(Fcb->ReferenceCount > fi->RefCount);
                UDFInterlockedDecrement((PLONG)&(Fcb->ReferenceCount));
                UDFInterlockedDecrement((PLONG)&(Fcb->NTRequiredFCB->CommonRefCount));
#ifdef UDF_DBG
            } else {
                BrutePoint();
#endif
            }
            Fcb = ParentFcb;
        }
    }
    if(!VcbAcquired)
        UDFReleaseResource(&(Vcb->VCBResource));
    return ret_val;

} // end UDFCleanUpFcbChain()

VOID
UDFDoDelayedClose(
    IN PtrUDFIrpContextLite    NextIrpContextLite
    )
{
    PtrUDFIrpContext   IrpContext;

    AdPrint(("  UDFDoDelayedClose\n"));
    UDFInitializeIrpContextFromLite(&IrpContext,NextIrpContextLite);
    IrpContext->Fcb->IrpContextLite = NULL;
    MyFreePool__(NextIrpContextLite);
    IrpContext->Fcb->FCBFlags &= ~UDF_FCB_DELAY_CLOSE;
    UDFCommonClose(IrpContext,NULL);
} // end UDFDoDelayedClose()

/*
    This routine removes request from Delayed Close queue.
    It operates until reach lower threshold
 */
VOID
NTAPI
UDFDelayedClose(
    PVOID unused
    )
{
    PLIST_ENTRY             Entry;
    PtrUDFIrpContextLite    NextIrpContextLite;

    AdPrint(("  UDFDelayedClose\n"));
    // Acquire DelayedCloseResource
    UDFAcquireResourceExclusive(&(UDFGlobalData.DelayedCloseResource), TRUE);

    while (UDFGlobalData.ReduceDelayedClose &&
          (UDFGlobalData.DelayedCloseCount > UDFGlobalData.MinDelayedCloseCount)) {

        Entry = UDFGlobalData.DelayedCloseQueue.Flink;

        if (!IsListEmpty(Entry)) {
            //  Extract the IrpContext.
            NextIrpContextLite = CONTAINING_RECORD( Entry,
                                                    UDFIrpContextLite,
                                                    DelayedCloseLinks );

            RemoveEntryList( Entry );
            UDFGlobalData.DelayedCloseCount--;
            UDFDoDelayedClose(NextIrpContextLite);
        } else {
            BrutePoint();
        }
    }

    while (UDFGlobalData.ReduceDirDelayedClose &&
          (UDFGlobalData.DirDelayedCloseCount > UDFGlobalData.MinDirDelayedCloseCount)) {

        Entry = UDFGlobalData.DirDelayedCloseQueue.Flink;

        if (!IsListEmpty(Entry)) {
            //  Extract the IrpContext.
            NextIrpContextLite = CONTAINING_RECORD( Entry,
                                                    UDFIrpContextLite,
                                                    DelayedCloseLinks );

            RemoveEntryList( Entry );
            UDFGlobalData.DirDelayedCloseCount--;
            UDFDoDelayedClose(NextIrpContextLite);
        } else {
            BrutePoint();
        }
    }

    UDFGlobalData.FspCloseActive = FALSE;
    UDFGlobalData.ReduceDelayedClose = FALSE;
    UDFGlobalData.ReduceDirDelayedClose = FALSE;

    // Release DelayedCloseResource
    UDFReleaseResource(&(UDFGlobalData.DelayedCloseResource));

    return;
} // end UDFDelayedClose()

/*
    This routine performs Close operation for all files from
    Delayed Close queue.
 */
VOID
UDFCloseAllDelayed(
    IN PVCB Vcb
    )
{
    PLIST_ENTRY             Entry;
    PtrUDFIrpContextLite    NextIrpContextLite;
    BOOLEAN                 GlobalDataAcquired = FALSE;

    AdPrint(("  UDFCloseAllDelayed\n"));
    // Acquire DelayedCloseResource
    if (!ExIsResourceAcquiredExclusive(&UDFGlobalData.GlobalDataResource)) {
        UDFAcquireResourceExclusive(&(UDFGlobalData.DelayedCloseResource), TRUE);
        GlobalDataAcquired = TRUE;
    }

    Entry = UDFGlobalData.DelayedCloseQueue.Flink;

    while (Entry != &UDFGlobalData.DelayedCloseQueue) {
        //  Extract the IrpContext.
        NextIrpContextLite = CONTAINING_RECORD( Entry,
                                                UDFIrpContextLite,
                                                DelayedCloseLinks );
        Entry = Entry->Flink;
        if (NextIrpContextLite->Fcb->Vcb == Vcb) {
            RemoveEntryList( &(NextIrpContextLite->DelayedCloseLinks) );
            UDFGlobalData.DelayedCloseCount--;
            UDFDoDelayedClose(NextIrpContextLite);
        }
    }

    Entry = UDFGlobalData.DirDelayedCloseQueue.Flink;

    while (Entry != &UDFGlobalData.DirDelayedCloseQueue) {
        //  Extract the IrpContext.
        NextIrpContextLite = CONTAINING_RECORD( Entry,
                                                UDFIrpContextLite,
                                                DelayedCloseLinks );
        Entry = Entry->Flink;
        if (NextIrpContextLite->Fcb->Vcb == Vcb) {
            RemoveEntryList( &(NextIrpContextLite->DelayedCloseLinks) );
            UDFGlobalData.DirDelayedCloseCount--;
            UDFDoDelayedClose(NextIrpContextLite);
        }
    }

    // Release DelayedCloseResource
    if(GlobalDataAcquired)
        UDFReleaseResource(&(UDFGlobalData.DelayedCloseResource));

} // end UDFCloseAllDelayed()

NTSTATUS
UDFBuildTreeItemsList(
    IN PVCB               Vcb,
    IN PUDF_FILE_INFO     FileInfo,
    IN PCHECK_TREE_ITEM   CheckItemProc,
    IN PUDF_FILE_INFO**   PassedList,
    IN PULONG             PassedListSize,
    IN PUDF_FILE_INFO**   FoundList,
    IN PULONG             FoundListSize
    )
{
    PDIR_INDEX_HDR     hDirNdx;
    PUDF_FILE_INFO     SDirInfo;
    ULONG              i;

    UDFPrint(("    UDFBuildTreeItemsList():\n"));
    if(!(*PassedList) || !(*FoundList)) {

        (*PassedList) = (PUDF_FILE_INFO*)
            MyAllocatePool__(NonPagedPool, sizeof(PUDF_FILE_INFO)*TREE_ITEM_LIST_GRAN);
        if(!(*PassedList))
            return STATUS_INSUFFICIENT_RESOURCES;
        (*PassedListSize) = 0;

        (*FoundList) = (PUDF_FILE_INFO*)
            MyAllocatePool__(NonPagedPool, sizeof(PUDF_FILE_INFO)*TREE_ITEM_LIST_GRAN);
        if(!(*FoundList)) {
            MyFreePool__(*PassedList);
            *PassedList = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        (*FoundListSize) = 0;
    }

    // check if already passed
    for(i=0;i<(*PassedListSize);i++) {
        if( ((*PassedList)[i]) == FileInfo )
            return STATUS_SUCCESS;
    }
    // remember passed object
    // we should not proceed linked objects twice
    (*PassedListSize)++;
    if( !((*PassedListSize) & (TREE_ITEM_LIST_GRAN - 1)) ) {
        if(!MyReallocPool__((PCHAR)(*PassedList), (*PassedListSize)*sizeof(PUDF_FILE_INFO),
                         (PCHAR*)PassedList, ((*PassedListSize)+TREE_ITEM_LIST_GRAN)*sizeof(PUDF_FILE_INFO))) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    (*PassedList)[(*PassedListSize)-1] = FileInfo;

    // check if this object matches our conditions
    if(CheckItemProc(FileInfo)) {
        // remember matched object
        (*FoundListSize)++;
        if( !((*FoundListSize) & (TREE_ITEM_LIST_GRAN - 1)) ) {
            if(!MyReallocPool__((PCHAR)(*FoundList), (*FoundListSize)*sizeof(PUDF_DATALOC_INFO),
                             (PCHAR*)FoundList, ((*FoundListSize)+TREE_ITEM_LIST_GRAN)*sizeof(PUDF_DATALOC_INFO))) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        (*FoundList)[(*FoundListSize)-1] = FileInfo;
    }

    // walk through SDir (if any)
    if((SDirInfo = FileInfo->Dloc->SDirInfo))
        UDFBuildTreeItemsList(Vcb, SDirInfo, CheckItemProc,
                 PassedList, PassedListSize, FoundList, FoundListSize);

    // walk through subsequent objects (if any)
    if((hDirNdx = FileInfo->Dloc->DirIndex)) {

        // scan DirIndex
        UDF_DIR_SCAN_CONTEXT ScanContext;
        PDIR_INDEX_ITEM DirNdx;
        PUDF_FILE_INFO CurFileInfo;

        if(UDFDirIndexInitScan(FileInfo, &ScanContext, 2)) {
            while((DirNdx = UDFDirIndexScan(&ScanContext, &CurFileInfo))) {
                if(!CurFileInfo)
                    continue;
                UDFBuildTreeItemsList(Vcb, CurFileInfo, CheckItemProc,
                         PassedList, PassedListSize, FoundList, FoundListSize);
            }
        }

    }
    return STATUS_SUCCESS;
} // end UDFBuildTreeItemsList()

BOOLEAN
UDFIsInDelayedCloseQueue(
    PUDF_FILE_INFO FileInfo)
{
    ASSERT(FileInfo);
    return (FileInfo->Fcb && FileInfo->Fcb->IrpContextLite);
} // end UDFIsInDelayedCloseQueue()

BOOLEAN
UDFIsLastClose(
    PUDF_FILE_INFO FileInfo)
{
    ASSERT(FileInfo);
    PtrUDFFCB Fcb = FileInfo->Fcb;
    if( Fcb &&
       !Fcb->OpenHandleCount &&
        Fcb->ReferenceCount &&
        Fcb->NTRequiredFCB->SectionObject.DataSectionObject) {
        return TRUE;
    }
    return FALSE;
} // UDFIsLastClose()

NTSTATUS
UDFCloseAllXXXDelayedInDir(
    IN PVCB             Vcb,
    IN PUDF_FILE_INFO   FileInfo,
    IN BOOLEAN          System
    )
{
    PUDF_FILE_INFO*         PassedList = NULL;
    ULONG                   PassedListSize = 0;
    PUDF_FILE_INFO*         FoundList = NULL;
    ULONG                   FoundListSize = 0;
    NTSTATUS                RC;
    ULONG                   i;
    _SEH2_VOLATILE BOOLEAN  ResAcq = FALSE;
    _SEH2_VOLATILE BOOLEAN  AcquiredVcb = FALSE;
    UDFNTRequiredFCB*       NtReqFcb;
    PUDF_FILE_INFO          CurFileInfo;
    PFE_LIST_ENTRY          CurListPtr;
    PFE_LIST_ENTRY*         ListPtrArray = NULL;

    _SEH2_TRY {

        UDFPrint(("    UDFCloseAllXXXDelayedInDir(): Acquire DelayedCloseResource\n"));
        // Acquire DelayedCloseResource
        UDFAcquireResourceExclusive(&(UDFGlobalData.DelayedCloseResource), TRUE);
        ResAcq = TRUE;

        UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
        AcquiredVcb = TRUE;

        RC = UDFBuildTreeItemsList(Vcb, FileInfo,
                System ? UDFIsLastClose : UDFIsInDelayedCloseQueue,
                &PassedList, &PassedListSize, &FoundList, &FoundListSize);

        if(!NT_SUCCESS(RC)) {
            UDFPrint(("    UDFBuildTreeItemsList(): error %x\n", RC));
            try_return(RC);
        }

        if(!FoundList || !FoundListSize) {
            try_return(RC = STATUS_SUCCESS);
        }

        // build array of referenced pointers
        ListPtrArray = (PFE_LIST_ENTRY*)(MyAllocatePool__(NonPagedPool, FoundListSize*sizeof(PFE_LIST_ENTRY)));
        if(!ListPtrArray) {
            UDFPrint(("    Can't alloc ListPtrArray for %x items\n", FoundListSize));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }

        for(i=0;i<FoundListSize;i++) {

            _SEH2_TRY {

                CurFileInfo = FoundList[i];
                if(!CurFileInfo->ListPtr) {
                    CurFileInfo->ListPtr = (PFE_LIST_ENTRY)(MyAllocatePool__(NonPagedPool, sizeof(FE_LIST_ENTRY)));
                    if(!CurFileInfo->ListPtr) {
                        UDFPrint(("    Can't alloc ListPtrEntry for items %x\n", i));
                        try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                    }
                    CurFileInfo->ListPtr->FileInfo = CurFileInfo;
                    CurFileInfo->ListPtr->EntryRefCount = 0;
                }
                CurFileInfo->ListPtr->EntryRefCount++;
                ListPtrArray[i] = CurFileInfo->ListPtr;

            } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
                BrutePoint();
            } _SEH2_END;
        }

        UDFReleaseResource(&(Vcb->VCBResource));
        AcquiredVcb = FALSE;

        if(System) {
            // Remove from system queue
            PtrUDFFCB Fcb;
            IO_STATUS_BLOCK IoStatus;
            BOOLEAN NoDelayed = (Vcb->VCBFlags & UDF_VCB_FLAGS_NO_DELAYED_CLOSE) ?
                                     TRUE : FALSE;

            Vcb->VCBFlags |= UDF_VCB_FLAGS_NO_DELAYED_CLOSE;
            for(i=FoundListSize;i>0;i--) {
                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                AcquiredVcb = TRUE;
                _SEH2_TRY {

                    CurListPtr = ListPtrArray[i-1];
                    CurFileInfo = CurListPtr->FileInfo;
                    if(CurFileInfo &&
                       (Fcb = CurFileInfo->Fcb)) {
                        NtReqFcb = Fcb->NTRequiredFCB;
                        ASSERT((ULONG_PTR)NtReqFcb > 0x1000);
//                            ASSERT((ULONG)(NtReqFcb->SectionObject) > 0x1000);
                        if(!(NtReqFcb->NtReqFCBFlags & UDF_NTREQ_FCB_DELETED) &&
                            (NtReqFcb->NtReqFCBFlags & UDF_NTREQ_FCB_MODIFIED)) {
                            MmPrint(("    CcFlushCache()\n"));
                            CcFlushCache(&(NtReqFcb->SectionObject), NULL, 0, &IoStatus);
                        }
                        if(NtReqFcb->SectionObject.ImageSectionObject) {
                            MmPrint(("    MmFlushImageSection()\n"));
                            MmFlushImageSection(&(NtReqFcb->SectionObject), MmFlushForWrite);
                        }
                        if(NtReqFcb->SectionObject.DataSectionObject) {
                            MmPrint(("    CcPurgeCacheSection()\n"));
                            CcPurgeCacheSection( &(NtReqFcb->SectionObject), NULL, 0, FALSE );
                        }
                    } else {
                        MmPrint(("    Skip item: deleted\n"));
                    }
                    CurListPtr->EntryRefCount--;
                    if(!CurListPtr->EntryRefCount) {
                        if(CurListPtr->FileInfo)
                            CurListPtr->FileInfo->ListPtr = NULL;
                        MyFreePool__(CurListPtr);
                    }
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                    BrutePoint();
                } _SEH2_END;
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
            if(!NoDelayed)
                Vcb->VCBFlags &= ~UDF_VCB_FLAGS_NO_DELAYED_CLOSE;
        } else {
            // Remove from internal queue
            PtrUDFIrpContextLite NextIrpContextLite;

            for(i=FoundListSize;i>0;i--) {

                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                AcquiredVcb = TRUE;

                CurListPtr = ListPtrArray[i-1];
                CurFileInfo = CurListPtr->FileInfo;

                if(CurFileInfo &&
                   CurFileInfo->Fcb &&
                    (NextIrpContextLite = CurFileInfo->Fcb->IrpContextLite)) {
                    RemoveEntryList( &(NextIrpContextLite->DelayedCloseLinks) );
                    if (NextIrpContextLite->Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
//                            BrutePoint();
                        UDFGlobalData.DirDelayedCloseCount--;
                    } else {
                        UDFGlobalData.DelayedCloseCount--;
                    }
                    UDFDoDelayedClose(NextIrpContextLite);
                }
                CurListPtr->EntryRefCount--;
                if(!CurListPtr->EntryRefCount) {
                    if(CurListPtr->FileInfo)
                        CurListPtr->FileInfo->ListPtr = NULL;
                    MyFreePool__(CurListPtr);
                }
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
        }
        RC = STATUS_SUCCESS;

try_exit: NOTHING;

    } _SEH2_FINALLY {
        // release Vcb
        if(AcquiredVcb)
            UDFReleaseResource(&(Vcb->VCBResource));
        // Release DelayedCloseResource
        if(ResAcq)
            UDFReleaseResource(&(UDFGlobalData.DelayedCloseResource));

        if(ListPtrArray)
            MyFreePool__(ListPtrArray);
        if(PassedList)
            MyFreePool__(PassedList);
        if(FoundList)
            MyFreePool__(FoundList);
    } _SEH2_END;

    return RC;
} // end UDFCloseAllXXXDelayedInDir(


/*
    This routine adds request to Delayed Close queue.
    If number of queued requests exceeds higher threshold it fires
    UDFDelayedClose()
 */
NTSTATUS
UDFQueueDelayedClose(
    PtrUDFIrpContext    IrpContext,
    PtrUDFFCB           Fcb
    )
{
    PtrUDFIrpContextLite    IrpContextLite;
    BOOLEAN                 StartWorker = FALSE;
    _SEH2_VOLATILE BOOLEAN  AcquiredVcb = FALSE;
    NTSTATUS                RC;

    AdPrint(("  UDFQueueDelayedClose\n"));

    _SEH2_TRY {
        // Acquire DelayedCloseResource
        UDFAcquireResourceExclusive(&(UDFGlobalData.DelayedCloseResource), TRUE);

        UDFAcquireResourceShared(&(Fcb->Vcb->VCBResource), TRUE);
        AcquiredVcb = TRUE;

        if(Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE) {
            try_return(RC = STATUS_DELETE_PENDING);
        }

        if(Fcb->IrpContextLite ||
           Fcb->FCBFlags & UDF_FCB_POSTED_RENAME) {
//            BrutePoint();
            try_return(RC = STATUS_UNSUCCESSFUL);
        }

        if(!NT_SUCCESS(RC = UDFInitializeIrpContextLite(&IrpContextLite,IrpContext,Fcb))) {
            try_return(RC);
        }

        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            InsertTailList( &UDFGlobalData.DirDelayedCloseQueue,
                            &IrpContextLite->DelayedCloseLinks );
            UDFGlobalData.DirDelayedCloseCount++;
        } else {
            InsertTailList( &UDFGlobalData.DelayedCloseQueue,
                            &IrpContextLite->DelayedCloseLinks );
            UDFGlobalData.DelayedCloseCount++;
        }
        Fcb->IrpContextLite = IrpContextLite;

        //  If we are above our threshold then start the delayed
        //  close operation.
        if(UDFGlobalData.DelayedCloseCount > UDFGlobalData.MaxDelayedCloseCount) {

            UDFGlobalData.ReduceDelayedClose = TRUE;

            if(!UDFGlobalData.FspCloseActive) {

                UDFGlobalData.FspCloseActive = TRUE;
                StartWorker = TRUE;
            }
        }
        //  If we are above our threshold then start the delayed
        //  close operation.
        if(UDFGlobalData.DirDelayedCloseCount > UDFGlobalData.MaxDirDelayedCloseCount) {

            UDFGlobalData.ReduceDirDelayedClose = TRUE;

            if(!UDFGlobalData.FspCloseActive) {

                UDFGlobalData.FspCloseActive = TRUE;
                StartWorker = TRUE;
            }
        }
        // Start the FspClose thread if we need to.
        if(StartWorker) {
            ExQueueWorkItem( &UDFGlobalData.CloseItem, CriticalWorkQueue );
        }
        RC = STATUS_SUCCESS;

try_exit:    NOTHING;

    } _SEH2_FINALLY {

        if(!NT_SUCCESS(RC)) {
            Fcb->FCBFlags &= ~UDF_FCB_DELAY_CLOSE;
        }
        if(AcquiredVcb) {
            UDFReleaseResource(&(Fcb->Vcb->VCBResource));
        }
        // Release DelayedCloseResource
        UDFReleaseResource(&(UDFGlobalData.DelayedCloseResource));
    } _SEH2_END;
    return RC;
} // end UDFQueueDelayedClose()

