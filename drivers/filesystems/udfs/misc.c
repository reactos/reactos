////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 File: Misc.cpp

 Module: UDF File System Driver (Kernel mode execution only)

 Description:
   This file contains some miscellaneous support routines.

*/

#include            "udffs.h"
// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_MISC

//  The following constant is the maximum number of ExWorkerThreads that we
//  will allow to be servicing a particular target device at any one time.

#define FSP_PER_DEVICE_THRESHOLD         (2)

#define UDFAllocateIoContext()                       \
    FsRtlAllocatePoolWithTag(NonPagedPool,           \
                            sizeof(UDF_IO_CONTEXT),  \
                            TAG_IO_CONTEXT)

#define UDFFreeIoContext(IO)     ExFreePool( (IO) )

/*************************************************************************
*
* Function: UDFExceptionFilter()
*
* Description:
*   This routines allows the driver to determine whether the exception
*   is an "allowed" exception i.e. one we should not-so-quietly consume
*   ourselves, or one which should be propagated onwards in which case
*   we will most likely bring down the machine.
*
*   This routine employs the services of FsRtlIsNtstatusExpected(). This
*   routine returns a BOOLEAN result. A RC of FALSE will cause us to return
*   EXCEPTION_CONTINUE_SEARCH which will probably cause a panic.
*   The FsRtl.. routine returns FALSE iff exception values are (currently) :
*       STATUS_DATATYPE_MISALIGNMENT    ||  STATUS_ACCESS_VIOLATION ||
*       STATUS_ILLEGAL_INSTRUCTION  ||  STATUS_INSTRUCTION_MISALIGNMENT
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: EXCEPTION_EXECUTE_HANDLER/EXECEPTION_CONTINUE_SEARCH
*
*************************************************************************/
LONG
UDFExceptionFilter(
    PIRP_CONTEXT IrpContext,
    PEXCEPTION_POINTERS ExceptionPointer
    )
{
    NTSTATUS ExceptionCode;

    ASSERT_OPTIONAL_IRP_CONTEXT(IrpContext);

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode =
            (NTSTATUS)ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    if (IrpContext) {
        IrpContext->ExceptionStatus = ExceptionCode;
    }

    // check if we should propagate this exception or not
    if (!(FsRtlIsNtstatusExpected(ExceptionCode))) {

        // better free up the IrpContext now ...
        if (IrpContext) {
            UDFPrint(("    UDF Driver internal error\n"));
            BrutePoint();
        } else {
            // we are not ok, propagate this exception.
            //  NOTE: we will bring down the machine ...
            //ReturnCode = EXCEPTION_CONTINUE_SEARCH;
            BrutePoint();
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
} // end UDFExceptionFilter()


/*************************************************************************
*
* Function: UDFExceptionHandler()
*
* Description:
*   One of the routines in the FSD or in the modules we invoked encountered
*   an exception. We have decided that we will "handle" the exception.
*   Therefore we will prevent the machine from a panic ...
*   You can do pretty much anything you choose to in your commercial
*   driver at this point to ensure a graceful exit. In the UDF
*   driver, We shall simply free up the IrpContext (if any), set the
*   error code in the IRP and complete the IRP at this time ...
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: Error code
*
*************************************************************************/
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFProcessException(
    _In_opt_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ NTSTATUS ExceptionCode
    )
{
    PDEVICE_OBJECT Device;
    PVPB Vpb;
    PETHREAD Thread;

    ASSERT_OPTIONAL_IRP_CONTEXT(IrpContext);
    ASSERT_IRP(Irp);

    // If there is not an irp context, then complete the request with the
    // current status code.

    if (!ARGUMENT_PRESENT(IrpContext)) {

        UDFCompleteRequest(NULL, Irp, ExceptionCode);
        return ExceptionCode;
    }

    // Get the real exception status from the IrpContext.

    ExceptionCode = IrpContext->ExceptionStatus;

    // If this is an Mdl write request, then take care of the Mdl
    // here so that things get cleaned up properly.  Cc now leaves
    // the MDL in place so a filesystem can retry after clearing an
    // internal condition (FAT does not).

    if ((IrpContext->MajorFunction == IRP_MJ_WRITE) &&
        (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE_MDL) == IRP_MN_COMPLETE_MDL) &&
        (Irp->MdlAddress != NULL)) {

        PIO_STACK_LOCATION LocalIrpSp = IoGetCurrentIrpStackLocation(Irp);

        CcMdlWriteAbort(LocalIrpSp->FileObject, Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    // Check if we are posting this request.  One of the following must be true
    // if we are to post a request.
    //
    //     - Status code is STATUS_CANT_WAIT and the request is asynchronous
    //         or we are forcing this to be posted.
    //
    //     - Status code is STATUS_VERIFY_REQUIRED and we are at APC level
    //         or higher, or within a guarded region.  Can't wait for IO in 
    //         the verify path in this case.
    //
    // Set the MORE_PROCESSING flag in the IrpContext to keep if from being
    // deleted if this is a retryable condition.

    // Note that (children of) UDFFsdPostRequest can raise (Mdl allocation).

    _SEH2_TRY {

        if (ExceptionCode == STATUS_CANT_WAIT) {

            if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST)) {

                ExceptionCode = UDFFsdPostRequest(IrpContext, Irp);
            }
        } 
        else if ((ExceptionCode == STATUS_VERIFY_REQUIRED) &&
                 FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL) &&
                 (KeGetCurrentIrql() >= APC_LEVEL)) {

            ExceptionCode = UDFFsdPostRequest(IrpContext, Irp);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {
    
        ExceptionCode = GetExceptionCode();
    } _SEH2_END;

    // If we posted the request or our caller will retry then just return here.

    if ((ExceptionCode == STATUS_PENDING) ||
        (ExceptionCode == STATUS_CANT_WAIT)) {

        return ExceptionCode;
    }

    ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING);

    // Store this error into the Irp for posting back to the Io system.

    Irp->IoStatus.Status = ExceptionCode;

    if (IoIsErrorUserInduced(ExceptionCode)) {

        // Check for the various error conditions that can be caused by,
        // and possibly resolved my the user.

        if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            // Now we are at the top level file system entry point.
            //
            // If we have already posted this request then the device to
            // verify is in the original thread.  Find this via the Irp.

            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

            //  If there is no device in that location then check in the
            //  current thread.
            if (Device == NULL) {

                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

                ASSERT( Device != NULL );

                //  Let's not BugCheck just because the driver screwed up.
                if (Device == NULL) {

                    ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;

                    UDFCompleteRequest(IrpContext, Irp, ExceptionCode);

                    return ExceptionCode;
                }
            }

            UDFPrint(("  use UDFPerformVerify()\n"));
            //  UDFPerformVerify() will do the right thing with the Irp.
            //  If we return STATUS_CANT_WAIT then the current thread
            //  can retry the request.
            return UDFPerformVerify( IrpContext, Irp, Device );
        }

        // The other user induced conditions generate an error unless
        // they have been disabled for this request.

        if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS)) {

            UDFPrint(("  DISABLE_POPUPS, complete Irp and return\n"));

            UDFCompleteRequest(IrpContext, Irp, ExceptionCode);
            return ExceptionCode;

        } else {

            //  Generate a pop-up
            if (IoGetCurrentIrpStackLocation( Irp )->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation( Irp )->FileObject->Vpb;
            } else {

                Vpb = NULL;
            }
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            Thread = Irp->Tail.Overlay.Thread;
            Device = IoGetDeviceToVerify( Thread );

            if (Device == NULL) {

                Thread = PsGetCurrentThread();
                Device = IoGetDeviceToVerify( Thread );
                ASSERT( Device != NULL );

                //  Let's not BugCheck just because the driver screwed up.
                if (Device == NULL) {

                    UDFCompleteRequest(IrpContext, Irp, ExceptionCode);

                    return ExceptionCode;
                }
            }

            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.

            IoMarkIrpPending(Irp);
            IoRaiseHardError(Irp, Vpb, Device);

            // We will be handing control back to the caller here, so
            // reset the saved device object.

            IoSetDeviceToVerify(Thread, NULL);

            // The Irp will be completed by Io or resubmitted.  In either
            // case we must clean up the IrpContext here.

            UDFCompleteRequest(IrpContext, NULL, STATUS_SUCCESS);
            return STATUS_PENDING;
        }
    }

    // If it was a normal request from IOManager then complete it

    UDFCompleteRequest(IrpContext, Irp, ExceptionCode);

    UDFPrint(("  return from exception handler with code %x\n", ExceptionCode));
    return(ExceptionCode);
} // end UDFExceptionHandler()

/*************************************************************************
*
* Function: UDFAllocateObjectName()
*
* Description:
*   Allocate a new ObjectName structure to represent an open on-disk object.
*   Also initialize the ObjectName structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the ObjectName structure OR NULL.
*
*************************************************************************/
PtrUDFObjectName
UDFAllocateObjectName(VOID)
{
    PtrUDFObjectName NewObjectName = NULL;

    NewObjectName = (PtrUDFObjectName)ExAllocateFromNPagedLookasideList(&UdfData.ObjectNameLookasideList);

    if (!NewObjectName) {
        return NULL;
    }

    // zero out the allocated memory block
    RtlZeroMemory(NewObjectName, sizeof(struct UDFObjectName));

    // set up some fields ...
    NewObjectName->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_OBJECT_NAME;
    NewObjectName->NodeIdentifier.NodeByteSize = sizeof(struct UDFObjectName);

    return NewObjectName;
} // end UDFAllocateObjectName()


/*************************************************************************
*
* Function: UDFReleaseObjectName()
*
* Description:
*   Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
UDFReleaseObjectName(
    PtrUDFObjectName ObjectName)
{
    ASSERT(ObjectName);

    ExFreeToNPagedLookasideList(&UdfData.ObjectNameLookasideList, ObjectName);

    return;
} // end UDFReleaseObjectName()


/*************************************************************************
*
* Function: UDFCreateCcb()
*
* Description:
*   Allocate a new CCB structure to represent an open on-disk object.
*   Also initialize the CCB structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the CCB structure OR NULL.
*
*************************************************************************/
PCCB
UDFCreateCcb()
{
    PCCB NewCcb = NULL;

    NewCcb = (PCCB)ExAllocateFromPagedLookasideList(&UdfData.CcbLookasideList);

    if (!NewCcb) {
        return NULL;
    }

    // zero out the allocated memory block
    RtlZeroMemory(NewCcb, sizeof(struct CCB));

    // set up some fields ...
    NewCcb->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_CCB;
    NewCcb->NodeIdentifier.NodeByteSize = sizeof(struct CCB);

    return NewCcb;
} // end UDFCreateCcb()

/*************************************************************************
*
* Function: UDFCreateIrpContext()
*
* Description:
*   The UDF FSD creates an IRP context for each request received. This
*   routine simply allocates (and initializes to NULL) a UDFIrpContext
*   structure.
*   Most of the fields in the context structure are then initialized here.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the IrpContext structure OR NULL.
*
*************************************************************************/
_Ret_maybenull_ PIRP_CONTEXT
UDFCreateIrpContext(
    _In_ PIRP Irp,
    _In_ BOOLEAN Wait
    )
{
    ASSERT(Irp);

    PIRP_CONTEXT NewIrpContext = NULL;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //  The only operations a filesystem device object should ever receive
    //  are create/teardown of fsdo handles and operations which do not
    //  occur in the context of fileobjects (i.e., mount).

    if (UdfDeviceIsFsdo(IrpSp->DeviceObject)) {

        if (IrpSp->FileObject != NULL &&
            IrpSp->MajorFunction != IRP_MJ_CREATE &&
            IrpSp->MajorFunction != IRP_MJ_CLEANUP &&
            IrpSp->MajorFunction != IRP_MJ_CLOSE) {

            ExRaiseStatus(STATUS_INVALID_DEVICE_REQUEST);
        }

        NT_ASSERT( IrpSp->FileObject != NULL ||

                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST &&
                 IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_INVALIDATE_VOLUMES) ||

                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME ) ||

                IrpSp->MajorFunction == IRP_MJ_SHUTDOWN );
    }

    NewIrpContext = (PIRP_CONTEXT)ExAllocateFromNPagedLookasideList(&UdfData.IrpContextLookasideList);

    if (NewIrpContext == NULL) {
        return NULL;
    }

    // zero out the allocated memory block
    RtlZeroMemory(NewIrpContext, sizeof(struct IRP_CONTEXT));

    // Set the proper node type code and node byte size
    NewIrpContext->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_IRP_CONTEXT;
    NewIrpContext->NodeIdentifier.NodeByteSize = sizeof(struct IRP_CONTEXT);

    // Set the originating Irp field
    NewIrpContext->Irp = Irp;

    // Copy RealDevice for workque algorithms.  We will update this in the Mount or
    // Verify since they have no file objects to use here.

    if (IrpSp->FileObject != NULL) {

        NewIrpContext->RealDevice = IrpSp->FileObject->DeviceObject;
    }

    // Carry write-through through the filesystem's own lower write IRPs.
    // A write-through file object also marks ordinary NtWriteFile requests;
    // a kernel caller can request it on a noncached write IRP directly.
    if (IrpSp->MajorFunction == IRP_MJ_WRITE && IrpSp->FileObject != NULL &&
        (BooleanFlagOn(IrpSp->FileObject->Flags, FO_WRITE_THROUGH) ||
         (BooleanFlagOn(Irp->Flags, IRP_NOCACHE) &&
          BooleanFlagOn(IrpSp->Flags, SL_WRITE_THROUGH)))) {
        SetFlag(NewIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
    }

    if (!UdfDeviceIsFsdo(IrpSp->DeviceObject)) {

        NewIrpContext->Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;
    }

    //  Major/Minor Function codes
    NewIrpContext->MajorFunction = IrpSp->MajorFunction;
    NewIrpContext->MinorFunction = IrpSp->MinorFunction;

    // Set the wait parameter

    if (Wait) {

        SetFlag(NewIrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    } else {

        SetFlag(NewIrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST);
    }

    // Are we top-level ? This information is used by the dispatching code
    // later (and also by the FSD dispatch routine)
    if (IoGetTopLevelIrp() != Irp) {
        // We are not top-level. Note this fact in the context structure
        SetFlag(NewIrpContext->Flags, UDF_IRP_CONTEXT_NOT_TOP_LEVEL);
    }

    return NewIrpContext;
} // end UDFCreateIrpContext()

VOID
UDFCleanupIrpContext(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ BOOLEAN Post
    )

/*++

Routine Description:

    This routine is called to cleanup and possibly deallocate the Irp Context.
    If the request is being posted or this Irp Context is possibly on the
    stack then we only cleanup any auxilary structures.

Arguments:

    Post - TRUE if we are posting this request, FALSE if we are deleting
        or retrying this in the current thread.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    // If we aren't doing more processing then deallocate this as appropriate.

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING)) {

        // If this context is the top level UDFS context then we need to
        // restore the top level thread context.

        if (IrpContext->ThreadContext != NULL) {

            UDFRestoreThreadContext(IrpContext);
        }

        // Deallocate the Io context if allocated.

        if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO) &&
            IrpContext->IoContext != NULL) {

            UDFFreeIoContext(IrpContext->IoContext);
        }

        // Deallocate the IrpContext if not from the stack.

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK )) {

            ExFreeToNPagedLookasideList(&UdfData.IrpContextLookasideList, IrpContext);
        }

    // Clear the appropriate flags.

    } else if (Post) {

        // If this context is the top level CDFS context then we need to
        // restore the top level thread context.

        if (IrpContext->ThreadContext != NULL) {

            UDFRestoreThreadContext(IrpContext);
        }

        ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST);

    } else {

        ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY);
    }

    return;
}

_When_(RaiseOnError || return, _At_(Fcb->FileLock, _Post_notnull_))
_When_(RaiseOnError, _At_(IrpContext, _Pre_notnull_))
BOOLEAN
UDFCreateFileLock (
    _In_opt_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine is called when we want to attach a file lock structure to the
    given Fcb.  It is possible the file lock is already attached.

    This routine is sometimes called from the fast path and sometimes in the
    Irp-based path.  We don't want to raise in the fast path, just return FALSE.

Arguments:

    Fcb - This is the Fcb to create the file lock for.

    RaiseOnError - If TRUE, we will raise on an allocation failure.  Otherwise we
        return FALSE on an allocation failure.

Return Value:

    BOOLEAN - TRUE if the Fcb has a filelock, FALSE otherwise.

--*/

{
    BOOLEAN Result = TRUE;
    PFILE_LOCK FileLock;

    PAGED_CODE();

    ASSERT(RaiseOnError == FALSE);

    //  Lock the Fcb and check if there is really any work to do.

    UDFLockFcb( IrpContext, Fcb );

    if (Fcb->FileLock != NULL) {

        UDFUnlockFcb( IrpContext, Fcb );
        return TRUE;
    }

    Fcb->FileLock = FileLock = FsRtlAllocateFileLock(NULL, NULL);

    UDFUnlockFcb( IrpContext, Fcb );

    //  Return or raise as appropriate.
    if (FileLock == NULL) {
         
        if (RaiseOnError) {

            NT_ASSERT(ARGUMENT_PRESENT(IrpContext));

            UDFRaiseStatus(IrpContext, STATUS_INSUFFICIENT_RESOURCES);
        }

        Result = FALSE;
    }

    return Result;
}

_Requires_lock_held_(_Global_critical_region_)
VOID
UDFPrePostIrp(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine performs any neccessary work before STATUS_PENDING is
    returned with the Fsd thread.  This routine is called within the
    filesystem and by the oplock package.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN RemovedFcb;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_IRP(Irp);

    // Case on the type of the operation.

    switch (IrpContext->MajorFunction) {

    case IRP_MJ_CREATE :

        // If called from the oplock package then there is an
        // Fcb to possibly teardown.  We will call the teardown
        // routine and release the Fcb if still present.  The cleanup
        // code in create will know not to release this Fcb because
        // we will clear the pointer.

        if ((IrpContext->TeardownFcb != NULL) &&
            *(IrpContext->TeardownFcb) != NULL) {

            //TODO: Impl
            ASSERT(FALSE);
            RemovedFcb = FALSE; //TODO: temp init

            //UDFTeardownStructures( IrpContext, *(IrpContext->TeardownFcb), &RemovedFcb );

            if (!RemovedFcb) {

                _Analysis_assume_lock_held_((*IrpContext->TeardownFcb)->FcbNonpaged->FcbResource);
                UDFReleaseFcb(IrpContext, *(IrpContext->TeardownFcb));
            }

            *(IrpContext->TeardownFcb) = NULL;
            IrpContext->TeardownFcb = NULL;
        }

        break;

    // We need to lock the user's buffer, unless this is an MDL read/write,
    // in which case there is no user buffer.

    case IRP_MJ_READ :

        if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL)) {

            UDFLockUserBuffer(IrpContext, IrpSp->Parameters.Read.Length, IoWriteAccess);
        }

        break;

    case IRP_MJ_WRITE :

        if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

            UDFLockUserBuffer(IrpContext, IrpSp->Parameters.Read.Length, IoReadAccess);
        }

        break;

    // We also need to check whether this is a query file operation.

    case IRP_MJ_DIRECTORY_CONTROL :

        if (IrpContext->MinorFunction == IRP_MN_QUERY_DIRECTORY) {

            UDFLockUserBuffer(IrpContext, IrpSp->Parameters.QueryDirectory.Length, IoWriteAccess);
        }

        break;
    }

    // Cleanup the IrpContext for the post.

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING);
    UDFCleanupIrpContext(IrpContext, TRUE);

    // Mark the Irp to show that we've already returned pending to the user.

    IoMarkIrpPending(Irp);

    return;
}

/*************************************************************************
*
* Function: UDFAddToWorkque()
*
* Description:
*   Queue up a request for deferred processing (in the context of a system
*   worker thread). The caller must have locked the user buffer (if required)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
UDFAddToWorkque(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP             Irp
    )
{
    PVOLUME_DEVICE_OBJECT Vdo;
    KIRQL SavedIrql;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    // Check if this request has an associated file object, and thus volume
    // device object.

    if (IrpSp->FileObject != NULL) {

        Vdo = CONTAINING_RECORD(IrpSp->DeviceObject,
            struct VOLUME_DEVICE_OBJECT,
            DeviceObject);

        // Check to see if this request should be sent to the overflow
        // queue.  If not, then send it off to an exworker thread.

        KeAcquireSpinLock(&Vdo->OverflowQueueSpinLock, &SavedIrql);

        if (Vdo->PostedRequestCount > FSP_PER_DEVICE_THRESHOLD) {

            // We cannot currently respond to this IRP so we'll just enqueue it
            // to the overflow queue on the volume.

            InsertTailList(&Vdo->OverflowQueue,
                           &IrpContext->WorkQueueItem.List);

            Vdo->OverflowQueueCount += 1;

            KeReleaseSpinLock(&Vdo->OverflowQueueSpinLock, SavedIrql);

            return;

        }
        else {

            // We are going to send this Irp to an ex worker thread so up
            // the count.

            Vdo->PostedRequestCount += 1;

            KeReleaseSpinLock(&Vdo->OverflowQueueSpinLock, SavedIrql);
        }
    }

    // Send it off.....

#pragma prefast(suppress:28155, "the function prototype is correct")
    ExInitializeWorkItem(&IrpContext->WorkQueueItem,
                         UDFFspDispatch,
                         IrpContext);

#pragma prefast(suppress: 28159, "prefast believes this routine is obsolete, but it is ok for CDFS to continue using it")
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);

    return;
} // end UDFAddToWorkque()

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFFsdPostRequest(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine enqueues the request packet specified by IrpContext to the
    work queue associated with the FileSystemDeviceObject.  This is a FSD
    routine.

Arguments:

    IrpContext - Pointer to the IrpContext to be queued to the Fsp.

    Irp - I/O Request Packet.

Return Value:

    STATUS_PENDING

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_IRP(Irp);

    // Posting is a three step operation.  First lock down any buffers
    // in the Irp.  Next cleanup the IrpContext for the post and finally
    // add this to a workque.

    UDFPrePostIrp(IrpContext, Irp);

    UDFAddToWorkque(IrpContext, Irp);

    // And return to our caller

    return STATUS_PENDING;
}

/*************************************************************************
*
* Function: UDFFspDispatch()
*
* Description:
*   The common dispatch routine invoked in the context of a system worker
*   thread. All we do here is pretty much case off the major function
*   code and invoke the appropriate FSD dispatch routine for further
*   processing.
*
* Expected Interrupt Level (for execution) :
*
*   IRQL PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
NTAPI
UDFFspDispatch(
    IN PVOID Context   // actually is a pointer to IRPContext structure
    )
{
    struct THREAD_CONTEXT ThreadContext = { 0 };
    PIRP_CONTEXT IrpContext = (PIRP_CONTEXT)Context;
    NTSTATUS Status;

    PIRP Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PVOLUME_DEVICE_OBJECT VolDo = NULL;

    // If this request has an associated volume device object, remember it.

    if (IrpSp->FileObject != NULL) {

        VolDo = CONTAINING_RECORD(IrpSp->DeviceObject,
                                  struct VOLUME_DEVICE_OBJECT,
                                  DeviceObject);
    }

    // Now case on the function code.  For each major function code,
    // either call the appropriate worker routine.  This routine that
    // we call is responsible for completing the IRP, and not us.
    // That way the routine can complete the IRP and then continue
    // post processing as required.  For example, a read can be
    // satisfied right away and then read can be done.
    //
    // We'll do all of the work within an exception handler that
    // will be invoked if ever some underlying operation gets into
    // trouble.

    while (TRUE) {

        // Set all the flags indicating we are in the Fsp.

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FSP_FLAGS);

        FsRtlEnterFileSystem();

        UDFSetThreadContext(IrpContext, &ThreadContext);

        while (TRUE) {

            _SEH2_TRY {

                // Reinitialize for the next try at completing this
                // request.

                Status =
                IrpContext->ExceptionStatus = STATUS_SUCCESS;

                // Initialize the Io status field in the Irp.

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;

                // Case on the major irp code.

                switch (IrpContext->MajorFunction) {

                case IRP_MJ_CREATE:

                    Status = UDFCommonCreate(IrpContext, Irp);
                    break;

                case IRP_MJ_DEVICE_CONTROL:

                    Status = UDFCommonDevControl(IrpContext, Irp);
                    break;

                case IRP_MJ_READ:

                    Status = UDFCommonRead(IrpContext, Irp);
                    break;

                case IRP_MJ_WRITE:

                    Status = UDFCommonWrite(IrpContext, Irp);
                    break;

                case IRP_MJ_CLEANUP:

                    Status = UDFCommonCleanup(IrpContext, Irp);
                    break;

                case IRP_MJ_CLOSE:

                    NT_ASSERT(FALSE);
                    break;

                case IRP_MJ_DIRECTORY_CONTROL:

                    Status = UDFCommonDirControl(IrpContext, Irp);
                    break;

                case IRP_MJ_FILE_SYSTEM_CONTROL:

                    Status = UDFCommonFsControl(IrpContext, Irp);
                    break;

                case IRP_MJ_LOCK_CONTROL:

                    Status = UDFCommonLockControl(IrpContext, Irp);
                    break;

                case IRP_MJ_PNP:

                    NT_ASSERT(FALSE);
                    Status = UDFCommonPnp(IrpContext, Irp);
                    break;

                case IRP_MJ_QUERY_INFORMATION:

                    Status = UDFCommonQueryInfo(IrpContext, Irp);
                    break;

                case IRP_MJ_SET_INFORMATION:

                    Status = UDFCommonSetInfo(IrpContext, Irp);
                    break;

                case IRP_MJ_QUERY_VOLUME_INFORMATION:

                    Status = UDFCommonQueryVolInfo(IrpContext, Irp);
                    break;

                case IRP_MJ_SET_VOLUME_INFORMATION:

                    Status = UDFCommonSetVolInfo(IrpContext, Irp);
                    break;

                default:

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    UDFCompleteRequest(IrpContext, Irp, Status);
                }

            } _SEH2_EXCEPT(UDFExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

                Status = UDFProcessException(IrpContext, Irp, _SEH2_GetExceptionCode());
            } _SEH2_END;

            // Break out of the loop if we didn't get CANT_WAIT.

            if (Status != STATUS_CANT_WAIT) { break; }

            // We are retrying this request.  Cleanup the IrpContext for the retry.

            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING);
            UDFCleanupIrpContext(IrpContext, FALSE);
        }

        FsRtlExitFileSystem();

        //  If there are any entries on this volume's overflow queue, service
        //  them.

        if (VolDo != NULL) {

            KIRQL SavedIrql;
            PVOID Entry = NULL;

            //
            //  We have a volume device object so see if there is any work
            //  left to do in its overflow queue.
            //

            KeAcquireSpinLock(&VolDo->OverflowQueueSpinLock, &SavedIrql);

            if (VolDo->OverflowQueueCount > 0) {

                //
                //  There is overflow work to do in this volume so we'll
                //  decrement the Overflow count, dequeue the IRP, and release
                //  the Event
                //

                VolDo->OverflowQueueCount -= 1;

                Entry = RemoveHeadList(&VolDo->OverflowQueue);

            }
            else {

                VolDo->PostedRequestCount -= 1;

                Entry = NULL;
            }

            KeReleaseSpinLock(&VolDo->OverflowQueueSpinLock, SavedIrql);

            //
            //  There wasn't an entry, break out of the loop and return to
            //  the Ex Worker thread.
            //

            if (Entry == NULL) {

                break;
            }

            //
            //  Extract the IrpContext , Irp, set wait to TRUE, and loop.
            //

            IrpContext = CONTAINING_RECORD(Entry,
                                           struct IRP_CONTEXT,
                                           WorkQueueItem.List);

            Irp = IrpContext->Irp;
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            __analysis_assert(IrpSp != 0);

            continue;
        }

        break;
    }

    return;
} // end UDFFspDispatch()

typedef ULONG
(*ptrUDFGetParameter)(
    IN PVCB Vcb,
    IN PCWSTR Name,
    IN ULONG DefValue
    );

VOID
UDFUpdateCompatOption(
    PVCB Vcb,
    BOOLEAN Update,
    BOOLEAN UseCfg,
    PCWSTR Name,
    ULONG Flag,
    BOOLEAN Default
    )
{
    ptrUDFGetParameter UDFGetParameter = UDFGetRegParameter;

    if (UDFGetParameter(Vcb, Name, Update ? ((Vcb->CompatFlags & Flag) ? TRUE : FALSE) : Default)) {
        Vcb->CompatFlags |= Flag;
    } else {
        Vcb->CompatFlags &= ~Flag;
    }
} // end UDFUpdateCompatOption()

VOID
UDFReadRegKeys(
    PVCB Vcb,
    BOOLEAN Update,
    BOOLEAN UseCfg
    )
{
    ptrUDFGetParameter UDFGetParameter = UDFGetRegParameter;

    // Timeouts for FreeSpaceBitMap & TheWholeDirTree flushes
    Vcb->BM_FlushPriod = UDFGetParameter(Vcb, UDF_BM_FLUSH_PERIOD_NAME,
        Update ? Vcb->BM_FlushPriod : 0);
    if (!Vcb->BM_FlushPriod) {
        Vcb->BM_FlushPriod = UDF_DEFAULT_BM_FLUSH_TIMEOUT;
    } else
    if (Vcb->BM_FlushPriod == (ULONG)-1) {
        Vcb->BM_FlushPriod = 0;
    }
    Vcb->Tree_FlushPriod = UDFGetParameter(Vcb, UDF_TREE_FLUSH_PERIOD_NAME,
        Update ? Vcb->Tree_FlushPriod : 0);
    if (!Vcb->Tree_FlushPriod) {
        Vcb->Tree_FlushPriod = UDF_DEFAULT_TREE_FLUSH_TIMEOUT;
    } else
    if (Vcb->Tree_FlushPriod == (ULONG)-1) {
        Vcb->Tree_FlushPriod = 0;
    }
    Vcb->SkipCountLimit = UDFGetParameter(Vcb, UDF_NO_UPDATE_PERIOD_NAME,
        Update ? Vcb->SkipCountLimit : 0);
    if (!Vcb->SkipCountLimit)
        Vcb->SkipCountLimit = -1;

    // The mimimum FileSize increment when we'll decide not to allocate
    // on-disk space.
    Vcb->SparseThreshold = UDFGetParameter(Vcb, UDF_SPARSE_THRESHOLD_NAME,
        Update ? Vcb->SparseThreshold : 0);
    if (!Vcb->SparseThreshold)
        Vcb->SparseThreshold = UDF_DEFAULT_SPARSE_THRESHOLD;

    // Should we update AttrFileTime on Attr changes
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_TIMES_ATTR, UDF_VCB_IC_UPDATE_ATTR_TIME, FALSE);
    // Windows build tools use LastWriteTime to decide whether an output is
    // newer than its inputs.  This driver does not currently load per-volume
    // registry options (UDFGetRegParameter returns the default), so leaving
    // this disabled makes in-place writes retain their old timestamp.
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_TIMES_MOD, UDF_VCB_IC_UPDATE_MODIFY_TIME, TRUE);
    // Should we update AccessFileTime on Exec & so on.
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_TIMES_ACCS, UDF_VCB_IC_UPDATE_ACCESS_TIME, FALSE);
    // Should we update Archive bit
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_ATTR_ARCH, UDF_VCB_IC_UPDATE_ARCH_BIT, FALSE);
    // Should we update Dir's Times & Attrs on Modify
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_DIR_TIMES_ATTR_W, UDF_VCB_IC_UPDATE_DIR_WRITE, FALSE);
    // Should we update Dir's Times & Attrs on Access
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_DIR_TIMES_ATTR_R, UDF_VCB_IC_UPDATE_DIR_READ, FALSE);
    // Should we allow user to change Access Time for unchanged Directory
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_ALLOW_UPDATE_TIMES_ACCS_UCHG_DIR, UDF_VCB_IC_UPDATE_UCHG_DIR_ACCESS_TIME, FALSE);
    // Should we read LONG_ADs with invalid PartitionReferenceNumber (generated by Nero Instant Burner)
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_INSTANT_COMPAT_ALLOC_DESCS, UDF_VCB_IC_INSTANT_COMPAT_ALLOC_DESCS, TRUE);

    // Should we ignore FO_SEQUENTIAL_ONLY
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_IGNORE_SEQUENTIAL_IO, UDF_VCB_IC_IGNORE_SEQUENTIAL_IO, FALSE);

    if (!Update)  {

        Vcb->NoFreeRelocationSpaceVolumeAction = (UCHAR)UDFGetParameter(Vcb, UDF_NO_SPARE_BEHAVIOR, UDF_PART_DAMAGED_RW);
        if (Vcb->NoFreeRelocationSpaceVolumeAction > 1) {
            Vcb->NoFreeRelocationSpaceVolumeAction = UDF_PART_DAMAGED_RW;
        }

        // Set dirty volume mount mode
        if (UDFGetParameter(Vcb, UDF_DIRTY_VOLUME_BEHAVIOR, UDF_PART_DAMAGED_RO)) {
            Vcb->CompatFlags |= UDF_VCB_IC_DIRTY_RO;
        }
    }
    return;
} // end UDFReadRegKeys()

ULONG
UDFGetRegParameter(
    IN PVCB Vcb,
    IN PCWSTR Name,
    IN ULONG DefValue
    )
{
    return DefValue;
} // end UDFGetRegParameter()

VOID
UDFDeleteVCB(
    PIRP_CONTEXT IrpContext,
    PVCB  Vcb
    )
{
    LARGE_INTEGER delay;
    UDFPrint(("UDFDeleteVCB\n"));

    PVOLUME_DEVICE_OBJECT Vdo = (PVOLUME_DEVICE_OBJECT)CONTAINING_RECORD(Vcb,
                                                                         struct VOLUME_DEVICE_OBJECT,
                                                                         Vcb);

    delay.QuadPart = -500000; // 0.05 sec
    while(Vdo->PostedRequestCount) {
        UDFPrint(("UDFDeleteVCB: PostedRequestCount = %d\n", Vdo->PostedRequestCount));
        // spin until all queues IRPs are processed
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
        delay.QuadPart -= 500000; // grow delay 0.05 sec
    }

#ifdef UDF_DBG
    _SEH2_TRY {
        if (!ExIsResourceAcquiredShared(&UdfData.GlobalDataResource)) {
            UDFPrint(("UDF: attempt to access to not protected data\n"));
            UDFPrint(("UDF: UDFGlobalData\n"));
            BrutePoint();
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
#endif

    _SEH2_TRY {
        RemoveEntryList(&(Vcb->VcbLinks));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    _SEH2_TRY {
        UDFPrint(("UDF: Delete resources\n"));
        UDFDeleteResource(&(Vcb->VcbResource));
        UDFDeleteResource(&(Vcb->BitMapResource1));
        UDFDeleteResource(&(Vcb->MetadataGrowResource));

        UDFDeleteResource(&(Vcb->DlocResource));
        UDFDeleteResource(&(Vcb->DlocResource2));
        UDFDeleteResource(&(Vcb->FlushResource));
        UDFDeleteResource(&(Vcb->PreallocResource));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    _SEH2_TRY {
        UDFPrint(("UDF: Cleanup VCB\n"));
        ASSERT(IsListEmpty(&(Vcb->NextNotifyIRP)));
        FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
        UDFCleanupVCB(Vcb);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    // Chuck the backpocket Vpb we kept just in case.
    UDFFreePool((PVOID*)&Vcb->SwapVpb);

    // If there is a Vpb then we must delete it ourselves.
    UDFFreePool((PVOID*)&Vcb->Vpb);

    IoDeleteDevice((PDEVICE_OBJECT)CONTAINING_RECORD(Vcb,
                                                     struct VOLUME_DEVICE_OBJECT,
                                                     Vcb));

} // end UDFDeleteVCB()

/*
Routine Description:
    This routine is called to initialize an IrpContext for the current
    UDFFS request.  The IrpContext is on the stack and we need to initialize
    it for the current request.  The request is a close operation.

Arguments:

    IrpContext - IrpContext to initialize.

    IrpContextLite - source for initialization

Return Value:

    None

*/
VOID
UDFInitializeStackIrpContextFromLite(
    OUT PIRP_CONTEXT IrpContext,
    IN PIRP_CONTEXT_LITE IrpContextLite
    )
{
    ASSERT(IrpContextLite->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_IRP_CONTEXT_LITE);
    ASSERT(IrpContextLite->NodeIdentifier.NodeByteSize == sizeof(struct IRP_CONTEXT_LITE));

    // Zero and then initialize the structure.
    RtlZeroMemory(IrpContext, sizeof(struct IRP_CONTEXT));

    // Set the proper node type code and node byte size
    IrpContext->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_IRP_CONTEXT;
    IrpContext->NodeIdentifier.NodeByteSize = sizeof(struct IRP_CONTEXT);

    //  Major/Minor Function codes
    IrpContext->MajorFunction = IRP_MJ_CLOSE;
    IrpContext->Vcb = IrpContextLite->Fcb->Vcb;
    IrpContext->Fcb = IrpContextLite->Fcb;
    IrpContext->RealDevice = IrpContextLite->RealDevice;

    // Note that this is from the stack.
    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK);

    // Set the wait parameter
    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

} // end UDFInitializeStackIrpContextFromLite()

ULONG
UDFIsResourceAcquired(
    IN PERESOURCE Resource
    )
{
    ULONG ReAcqRes =
        ExIsResourceAcquiredExclusiveLite(Resource) ? 1 :
        (ExIsResourceAcquiredSharedLite(Resource) ? 2 : 0);
    return ReAcqRes;
} // end UDFIsResourceAcquired()

BOOLEAN
UDFAcquireResourceExclusiveWithCheck(
    IN PERESOURCE Resource
    )
{
    ULONG ReAcqRes =
        ExIsResourceAcquiredExclusiveLite(Resource) ? 1 :
        (ExIsResourceAcquiredSharedLite(Resource) ? 2 : 0);
    if (ReAcqRes) {
        UDFPrint(("UDFAcquireResourceExclusiveWithCheck: ReAcqRes, %x\n", ReAcqRes));
    } else {
//        BrutePoint();
    }

    if (ReAcqRes == 1) {
        // OK
    } else
    if (ReAcqRes == 2) {
        UDFPrint(("UDFAcquireResourceExclusiveWithCheck: !!! Shared !!!\n"));
        //BrutePoint();
    } else {
        UDFAcquireResourceExclusive(Resource, TRUE);
        return TRUE;
    }
    return FALSE;
} // end UDFAcquireResourceExclusiveWithCheck()

BOOLEAN
UDFAcquireResourceSharedWithCheck(
    IN PERESOURCE Resource
    )
{
    ULONG ReAcqRes =
        ExIsResourceAcquiredExclusiveLite(Resource) ? 1 :
        (ExIsResourceAcquiredSharedLite(Resource) ? 2 : 0);
    if (ReAcqRes) {
        UDFPrint(("UDFAcquireResourceSharedWithCheck: ReAcqRes, %x\n", ReAcqRes));
/*    } else {
        BrutePoint();*/
    }

    if (ReAcqRes == 2) {
        // OK
    } else
    if (ReAcqRes == 1) {
        UDFPrint(("UDFAcquireResourceSharedWithCheck: Exclusive\n"));
        //BrutePoint();
    } else {
        UDFAcquireResourceShared(Resource, TRUE);
        return TRUE;
    }
    return FALSE;
} // end UDFAcquireResourceSharedWithCheck()

VOID
UDFSetModified(
    IN PVCB        Vcb
    )
{
    if (InterlockedIncrement((PLONG) & (Vcb->Modified)) & 0x80000000)
        Vcb->Modified = 2;
} // end UDFSetModified()

VOID
UDFPreClrModified(
    IN PVCB Vcb
    )
{
    Vcb->Modified = 1;
} // end UDFPreClrModified()

VOID
UDFClrModified(
    IN PVCB        Vcb
    )
{
    UDFPrint(("ClrModified\n"));
    InterlockedDecrement((PLONG)&Vcb->Modified);
} // end UDFClrModified()

NTSTATUS
UDFToggleMediaEjectDisable (
    IN PVCB Vcb,
    IN BOOLEAN PreventRemoval
    )
{
    PREVENT_MEDIA_REMOVAL Prevent;

    //  If PreventRemoval is the same as UDF_VCB_FLAGS_MEDIA_LOCKED,
    //  no-op this call, otherwise toggle the state of the flag.

    if ((PreventRemoval ^ BooleanFlagOn(Vcb->VcbState, UDF_VCB_FLAGS_MEDIA_LOCKED)) == 0) {

        return STATUS_SUCCESS;

    } else {

        Vcb->VcbState ^= UDF_VCB_FLAGS_MEDIA_LOCKED;
    }

    Prevent.PreventMediaRemoval = PreventRemoval;

    return UDFPerformDevIoCtrl(IOCTL_DISK_MEDIA_REMOVAL,
                          Vcb->TargetDeviceObject,
                          &Prevent,
                          sizeof(Prevent),
                          NULL,
                          0,
                          FALSE,
                          NULL);
}

/*++

Routine Description:

    This routine completes a Irp and cleans up the IrpContext.  Either or
    both of these may not be specified.

Arguments:

    Irp - Supplies the Irp being processed.

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/
VOID
UDFCompleteRequest (
    _Inout_opt_ PIRP_CONTEXT IrpContext OPTIONAL,
    _Inout_opt_ PIRP Irp OPTIONAL,
    _In_ NTSTATUS Status
    )
{
    ASSERT_OPTIONAL_IRP_CONTEXT(IrpContext);
    ASSERT_OPTIONAL_IRP(Irp);

    //  Cleanup the IrpContext if passed in here.

    if (ARGUMENT_PRESENT(IrpContext)) {

        UDFCleanupIrpContext(IrpContext, FALSE);
    }

    //  If we have an Irp then complete the irp.

    if (ARGUMENT_PRESENT(Irp)) {

        //  Clear the information field in case we have used this Irp
        //  internally.

        if (NT_ERROR( Status ) &&
            FlagOn(Irp->Flags, IRP_INPUT_OPERATION)) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;

        AssertVerifyDeviceIrp(Irp);

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    return;
}

VOID
UDFSetThreadContext(
    _Inout_ PIRP_CONTEXT IrpContext,
    _In_ PTHREAD_CONTEXT ThreadContext
    )

/*++

Routine Description:

    This routine is called at each Fsd/Fsp entry point set up the IrpContext
    and thread local storage to track top level requests.  If there is
    not a Udfs context in the thread local storage then we use the input one.
    Otherwise we use the one already there.  This routine also updates the
    IrpContext based on the state of the top-level context.

    If the TOP_LEVEL flag in the IrpContext is already set when we are called
    then we force this request to appear top level.

Arguments:

    ThreadContext - Address on stack for local storage if not already present.

    ForceTopLevel - We force this request to appear top level regardless of
        any previous stack value.

Return Value:

    None

--*/

{
    PTHREAD_CONTEXT CurrentThreadContext;
#ifdef __REACTOS__
    ULONG_PTR StackTop;
    ULONG_PTR StackBottom;
#endif

    PAGED_CODE();

    ASSERT_IRP_CONTEXT(IrpContext);

    //  Get the current top-level irp out of the thread storage.
    //  If NULL then this is the top-level request.

    CurrentThreadContext = (PTHREAD_CONTEXT) IoGetTopLevelIrp();

    if (CurrentThreadContext == NULL) {

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL);
    }

    // Initialize the input context unless we are using the current
    // thread context block.  We use the new block if our caller
    // specified this or the existing block is invalid.
    //
    // The following must be true for the current to be a valid Cdfs context.
    //
    //      Structure must lie within current stack.
    //      Address must be ULONG aligned.
    //      Cdfs signature must be present.
    //
    // If this is not a valid Cdfs context then use the input thread
    // context and store it in the top level context.

#ifdef __REACTOS__
    IoGetStackLimits( &StackTop, &StackBottom);
#endif

#pragma warning(suppress: 6011) // Bug in PREFast around bitflag operations
    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL ) ||
#ifndef __REACTOS__
        (!IoWithinStackLimits( (ULONG_PTR)CurrentThreadContext, sizeof(struct THREAD_CONTEXT) ) ||
#else
        (((ULONG_PTR) CurrentThreadContext > StackBottom - sizeof( struct THREAD_CONTEXT )) ||
         ((ULONG_PTR) CurrentThreadContext <= StackTop) ||
#endif
         FlagOn( (ULONG_PTR) CurrentThreadContext, 0x3 ) ||
         (CurrentThreadContext->Udfs != 0x53464444))) {

        ThreadContext->Udfs = 0x53464444;
        ThreadContext->SavedTopLevelIrp = (PIRP) CurrentThreadContext;
        ThreadContext->TopLevelIrpContext = IrpContext;
        IoSetTopLevelIrp((PIRP)ThreadContext);

        IrpContext->TopLevel = IrpContext;
        IrpContext->ThreadContext = ThreadContext;

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS);

    //  Otherwise use the IrpContext in the thread context.

    } else {

        IrpContext->TopLevel = CurrentThreadContext->TopLevelIrpContext;
    }

    return;
}


_Requires_lock_held_(_Global_critical_region_)
_When_(Type == AcquireExclusive && return != FALSE, _Acquires_exclusive_lock_(*Resource))
_When_(Type == AcquireShared && return != FALSE, _Acquires_shared_lock_(*Resource))
_When_(Type == AcquireSharedStarveExclusive && return != FALSE, _Acquires_shared_lock_(*Resource))
_When_(IgnoreWait == FALSE, _Post_satisfies_(return == TRUE))
BOOLEAN
UDFAcquireResource(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PERESOURCE Resource,
    _In_ BOOLEAN IgnoreWait,
    _In_ enum TYPE_OF_ACQUIRE Type
    )

/*++

Routine Description:

    This is the single routine used to acquire file system resources.  It
    looks at the IgnoreWait flag to determine whether to try to acquire the
    resource without waiting.  Returning TRUE/FALSE to indicate success or
    failure.  Otherwise it is driven by the WAIT flag in the IrpContext and
    will raise CANT_WAIT on a failure.

Arguments:

    Resource - This is the resource to try and acquire.

    IgnoreWait - If TRUE then this routine will not wait to acquire the
        resource and will return a boolean indicating whether the resource was
        acquired.  Otherwise we use the flag in the IrpContext and raise
        if the resource is not acquired.

    Type - Indicates how we should try to get the resource.

Return Value:

    BOOLEAN - TRUE if the resource is acquired.  FALSE if not acquired and
        IgnoreWait is specified.  Otherwise we raise CANT_WAIT.

--*/

{
    BOOLEAN Wait = FALSE;
    BOOLEAN Acquired;
    PAGED_CODE();

    //  We look first at the IgnoreWait flag, next at the flag in the Irp
    // Context to decide how to acquire this resource.

    if (!IgnoreWait && FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Wait = TRUE;
    }

    // Attempt to acquire the resource either shared or exclusively.

    switch (Type) {
        case AcquireExclusive:

#pragma prefast( suppress:28137, "prefast believes Wait should be a constant, but this is ok for CDFS" )
            Acquired = ExAcquireResourceExclusiveLite( Resource, Wait );
            break;

        case AcquireShared:

            Acquired = ExAcquireResourceSharedLite( Resource, Wait );
            break;

        case AcquireSharedStarveExclusive:

            Acquired = ExAcquireSharedStarveExclusive( Resource, Wait );
            break;

        default:
            Acquired = FALSE;
            NT_ASSERT( FALSE );
    }

    // If not acquired and the user didn't specifiy IgnoreWait then
    // raise CANT_WAIT.

    if (!Acquired && !IgnoreWait) {

        UDFRaiseStatus(IrpContext, STATUS_CANT_WAIT);
    }

    return Acquired;
}

VOID
UDFFinishIoAtEof(
    IN PFCB Fcb
    )
{
    PEOF_WAIT_BLOCK EofWaitBlock;

    PAGED_CODE();

    // Check if list is empty

    if (IsListEmpty(&Fcb->EofListHead)) {

        // No waiters - clear the EOF advance flag

        ClearFlag(Fcb->Header.Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE);

    } else {

        // Remove first waiter from list

        EofWaitBlock = (PEOF_WAIT_BLOCK)RemoveHeadList(&Fcb->EofListHead);

        // Signal the waiter's event

        KeSetEvent(&EofWaitBlock->Event, 0, FALSE);
    }
}

BOOLEAN
UDFWaitForIoAtEof(
    IN PFCB Fcb,
    IN LONGLONG FileOffset,
    IN ULONG Length
    )
{
    EOF_WAIT_BLOCK WaitBlock;

    PAGED_CODE();

    ASSERT(Fcb->Header.FileSize.QuadPart >= Fcb->Header.ValidDataLength.QuadPart);

    // Initialize wait block

    RtlZeroMemory(&WaitBlock, sizeof(EOF_WAIT_BLOCK));

    // Initialize event as synchronization event (NotificationEvent = FALSE)

    KeInitializeEvent(&WaitBlock.Event, SynchronizationEvent, FALSE);

    // Insert wait block at end of list

    InsertTailList(&Fcb->EofListHead, &WaitBlock.EofWaitLinks);

    ExReleaseFastMutex(Fcb->Header.FastMutex);

    // Wait for event to be signaled

    KeWaitForSingleObject(&WaitBlock.Event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ExAcquireFastMutex(Fcb->Header.FastMutex);

    // Check if we still need to extend EOF

    if ((FileOffset >= 0) &&
        ((FileOffset + Length) <= Fcb->Header.ValidDataLength.QuadPart)) {

        UDFFinishIoAtEof(Fcb);

        return FALSE;
    }

    return TRUE;
}

#include "Include/regtools.c"

