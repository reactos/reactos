/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    class.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#define CLASS_INIT_GUID 0
#include "classp.h"
#include "debug.h"

#ifdef DEBUG_USE_WPP
#include "create.tmh"
#endif

ULONG BreakOnClose = 0;

const PCSZ LockTypeStrings[] = {
    "Simple",
    "Secure",
    "Internal"
};


VOID
ClasspCleanupDisableMcn(
    IN PFILE_OBJECT_EXTENSION FsContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassCreateClose)
#pragma alloc_text(PAGE, ClasspCreateClose)
#pragma alloc_text(PAGE, ClasspCleanupProtectedLocks)
#pragma alloc_text(PAGE, ClasspEjectionControl)
#pragma alloc_text(PAGE, ClasspCleanupDisableMcn)
#pragma alloc_text(PAGE, ClassGetFsContext)
#endif

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    SCSI class driver create and close routine.  This is called by the I/O system
    when the device is opened or closed.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp - IRP involved.

Return Value:

    Device-specific drivers return value or STATUS_SUCCESS.

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    ULONG removeState;
    NTSTATUS status;

    PAGED_CODE();

    //
    // If we're getting a close request then we know the device object hasn't
    // been completely destroyed.  Let the driver cleanup if necessary.
    //

    removeState = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // Invoke the device-specific routine, if one exists. Otherwise complete
    // with SUCCESS
    //

    if((removeState == NO_REMOVE) ||
       IS_CLEANUP_REQUEST(IoGetCurrentIrpStackLocation(Irp)->MajorFunction)) {

        status = ClasspCreateClose(DeviceObject, Irp);

        if((NT_SUCCESS(status)) &&
           (commonExtension->DevInfo->ClassCreateClose)) {

            return commonExtension->DevInfo->ClassCreateClose(DeviceObject, Irp);
        }

    } else {
        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    Irp->IoStatus.Status = status;
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
ClasspCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine will handle create/close operations for a given classpnp
    device if the class driver doesn't supply it's own handler.  If there
    is a file object supplied for our driver (if it's a FO_DIRECT_DEVICE_OPEN
    file object) then it will initialize a file extension on create or destroy
    the extension on a close.

Arguments:

    DeviceObject - the device object being opened or closed.

    Irp - the create/close irp

Return Value:

    status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    PFILE_OBJECT fileObject = irpStack->FileObject;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();


    //
    // ISSUE-2000/3/28-henrygab - if lower stack fails create/close, we end up
    // in an inconsistent state.  re-write to verify all args and allocate all
    // required resources, then pass the irp down, then complete the
    // transaction.  this is because we also cannot forward the irp, then fail
    // it after it has succeeded a lower-level driver.
    //

    if(irpStack->MajorFunction == IRP_MJ_CREATE) {

        PIO_SECURITY_CONTEXT securityContext =
            irpStack->Parameters.Create.SecurityContext;
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "ClasspCREATEClose: create received for device %p\n",
                    DeviceObject));
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "ClasspCREATEClose: desired access %lx\n",
                    securityContext->DesiredAccess));
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "ClasspCREATEClose: file object %p\n",
                    irpStack->FileObject));

        NT_ASSERT(BreakOnClose == FALSE);

        if(irpStack->FileObject != NULL) {

            PFILE_OBJECT_EXTENSION fsContext;

            //
            // Allocate our own file object extension for this device object.
            //

            status = AllocateDictionaryEntry(
                        &commonExtension->FileObjectDictionary,
                        (ULONGLONG) irpStack->FileObject,
                        sizeof(FILE_OBJECT_EXTENSION),
                        CLASS_TAG_FILE_OBJECT_EXTENSION,
                        (PVOID *)&fsContext);

            if(NT_SUCCESS(status)) {

                RtlZeroMemory(fsContext,
                              sizeof(FILE_OBJECT_EXTENSION));

                fsContext->FileObject = irpStack->FileObject;
                fsContext->DeviceObject = DeviceObject;
            } else if (status == STATUS_OBJECT_NAME_COLLISION) {
                status = STATUS_SUCCESS;
            }
        }

    } else {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "ClasspCreateCLOSE: close received for device %p\n",
                    DeviceObject));
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "ClasspCreateCLOSE: file object %p\n",
                    fileObject));

        if(irpStack->FileObject != NULL) {

            PFILE_OBJECT_EXTENSION fsContext =
                ClassGetFsContext(commonExtension, irpStack->FileObject);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "ClasspCreateCLOSE: file extension %p\n",
                        fsContext));

            if(fsContext != NULL) {

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                            "ClasspCreateCLOSE: extension is ours - "
                            "freeing\n"));
                NT_ASSERT(BreakOnClose == FALSE);

                ClasspCleanupProtectedLocks(fsContext);

                ClasspCleanupDisableMcn(fsContext);

                FreeDictionaryEntry(&(commonExtension->FileObjectDictionary),
                                    fsContext);
            }
        }
    }

    //
    // Notify the lower levels about the create or close operation - give them
    // a chance to cleanup too.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "ClasspCreateClose: %s for devobj %p\n",
                (NT_SUCCESS(status) ? "Success" : "FAILED"),
                DeviceObject));


    if(NT_SUCCESS(status)) {

        KEVENT event;

        //
        // Set up the event to wait on
        //

        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine( Irp, ClassSignalCompletion, &event,
                                TRUE, TRUE, TRUE);

        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

        if(status == STATUS_PENDING) {
            (VOID)KeWaitForSingleObject(&event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);
            status = Irp->IoStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,
                        "ClasspCreateClose: Lower driver failed, but we "
                        "succeeded.  This is a problem, lock counts will be "
                        "out of sync between levels.\n"));
        }

    }


    return status;
}


VOID
ClasspCleanupProtectedLocks(
    IN PFILE_OBJECT_EXTENSION FsContext
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension =
        FsContext->DeviceObject->DeviceExtension;

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
        commonExtension->PartitionZeroExtension;

    ULONG newDeviceLockCount = 1;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "ClasspCleanupProtectedLocks called for %p\n",
                FsContext->DeviceObject));
    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "ClasspCleanupProtectedLocks - FsContext %p is locked "
                "%d times\n", FsContext, FsContext->LockCount));

    NT_ASSERT(BreakOnClose == FALSE);

    //
    // Synchronize with ejection and ejection control requests.
    //

    KeEnterCriticalRegion();
    (VOID)KeWaitForSingleObject(&(fdoExtension->EjectSynchronizationEvent),
                                UserRequest,
                                KernelMode,
                                FALSE,
                          NULL);

    //
    // For each secure lock on this handle decrement the secured lock count
    // for the FDO.  Keep track of the new value.
    //

    if (FsContext->LockCount != 0) {

        do {

            InterlockedDecrement((volatile LONG *)&FsContext->LockCount);

            newDeviceLockCount =
                InterlockedDecrement(&fdoExtension->ProtectedLockCount);

        } while (FsContext->LockCount > 0);

        //
        // If the new lock count has been dropped to zero then issue a lock
        // command to the device.
        //

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "ClasspCleanupProtectedLocks: FDO secured lock count = %d "
                    "lock count = %d\n",
                    fdoExtension->ProtectedLockCount,
                    fdoExtension->LockCount));

        if ((newDeviceLockCount == 0) && (fdoExtension->LockCount == 0)) {

            SCSI_REQUEST_BLOCK srb = {0};
            UCHAR srbExBuffer[CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE] = {0};
            PSTORAGE_REQUEST_BLOCK srbEx = (PSTORAGE_REQUEST_BLOCK)srbExBuffer;
            PCDB cdb = NULL;
            NTSTATUS status;
            PSCSI_REQUEST_BLOCK srbPtr;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "ClasspCleanupProtectedLocks: FDO lock count dropped "
                        "to zero\n"));

            if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
#ifdef _MSC_VER
                #pragma prefast(suppress:26015, "InitializeStorageRequestBlock ensures buffer access is bounded")
#endif
                status = InitializeStorageRequestBlock(srbEx,
                                                       STORAGE_ADDRESS_TYPE_BTL8,
                                                       sizeof(srbExBuffer),
                                                       1,
                                                       SrbExDataTypeScsiCdb16);
                if (NT_SUCCESS(status)) {
                    srbEx->TimeOutValue = fdoExtension->TimeOutValue;
                    SrbSetCdbLength(srbEx, 6);
                    cdb = SrbGetCdb(srbEx);
                    srbPtr = (PSCSI_REQUEST_BLOCK)srbEx;
                } else {
                    //
                    // Should not happen. Revert to legacy SRB.
                    //
                    NT_ASSERT(FALSE);
                    srb.TimeOutValue = fdoExtension->TimeOutValue;
                    srb.CdbLength = 6;
                    cdb = (PCDB) &(srb.Cdb);
                    srbPtr = &srb;
                }

            } else {

                srb.TimeOutValue = fdoExtension->TimeOutValue;
                srb.CdbLength = 6;
                cdb = (PCDB) &(srb.Cdb);
                srbPtr = &srb;

            }

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            //
            // TRUE - prevent media removal.
            // FALSE - allow media removal.
            //

            cdb->MEDIA_REMOVAL.Prevent = FALSE;

            status = ClassSendSrbSynchronous(fdoExtension->DeviceObject,
                                             srbPtr,
                                             NULL,
                                             0,
                                             FALSE);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "ClasspCleanupProtectedLocks: unlock request to drive "
                        "returned status %lx\n", status));
        }
    }

    KeSetEvent(&fdoExtension->EjectSynchronizationEvent,
               IO_NO_INCREMENT,
               FALSE);
    KeLeaveCriticalRegion();
    return;
}


VOID
ClasspCleanupDisableMcn(
    IN PFILE_OBJECT_EXTENSION FsContext
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension =
        FsContext->DeviceObject->DeviceExtension;

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
        commonExtension->PartitionZeroExtension;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_MCN,
                "ClasspCleanupDisableMcn called for %p\n",
                FsContext->DeviceObject));
    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_MCN,
                "ClasspCleanupDisableMcn - FsContext %p is disabled "
                "%d times\n", FsContext, FsContext->McnDisableCount));

    //
    // For each secure lock on this handle decrement the secured lock count
    // for the FDO.  Keep track of the new value.
    //

    while(FsContext->McnDisableCount != 0) {
        FsContext->McnDisableCount--;
        ClassEnableMediaChangeDetection(fdoExtension);
    }

    return;
}


#if 1
/*
 *  ISSUE: REMOVE this old function implementation as soon as the
 *                  boottime pagefile problems with the new one (below)
 *                  are resolved.
 */
NTSTATUS
ClasspEjectionControl(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN MEDIA_LOCK_TYPE LockType,
    IN BOOLEAN Lock
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension =
        (PCOMMON_DEVICE_EXTENSION) FdoExtension;

    PFILE_OBJECT_EXTENSION fsContext = NULL;
    NTSTATUS status;
    PSCSI_REQUEST_BLOCK srb = NULL;
    BOOLEAN countChanged = FALSE;

    PAGED_CODE();

    /*
     *  Ensure that the user thread is not suspended while we are holding EjectSynchronizationEvent.
     */
    KeEnterCriticalRegion();

    status = KeWaitForSingleObject(
                &(FdoExtension->EjectSynchronizationEvent),
                UserRequest,
                KernelMode,
                FALSE,
                NULL);

    NT_ASSERT(status == STATUS_SUCCESS);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                "ClasspEjectionControl: "
                "Received request for %s lock type\n",
                LockTypeStrings[LockType]
                ));

    _SEH2_TRY {
        PCDB cdb = NULL;

        //
        // Determine if this is a "secured" request.
        //

        if (LockType == SecureMediaLock) {

            PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
            PFILE_OBJECT fileObject = irpStack->FileObject;

            //
            // Make sure that the file object we are supplied has a
            // proper FsContext before we try doing a secured lock.
            //

            if (fileObject != NULL) {
                fsContext = ClassGetFsContext(commonExtension, fileObject);
            }

            if (fsContext == NULL) {

                //
                // This handle isn't setup correctly.  We can't let the
                // operation go.
                //

                status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        }

        if (Lock) {

            //
            // This is a lock command.  Reissue the command in case bus or
            // device was reset and the lock was cleared.
            // note: may need to decrement count if actual lock operation
            //       failed....
            //

            switch (LockType) {

                case SimpleMediaLock: {
                    FdoExtension->LockCount++;
                    countChanged = TRUE;
                    break;
                }

                case SecureMediaLock: {
                    fsContext->LockCount++;
                    FdoExtension->ProtectedLockCount++;
                    countChanged = TRUE;
                    break;
                }

                case InternalMediaLock: {
                    FdoExtension->InternalLockCount++;
                    countChanged = TRUE;
                    break;
                }
            }

        } else {

            //
            // This is an unlock command.  If it's a secured one then make sure
            // the caller has a lock outstanding or return an error.
            // note: may need to re-increment the count if actual unlock
            //       operation fails....
            //

            switch (LockType) {

                case SimpleMediaLock: {
                    if(FdoExtension->LockCount != 0) {
                        FdoExtension->LockCount--;
                        countChanged = TRUE;
                    }
                    break;
                }

                case SecureMediaLock: {
                    if(fsContext->LockCount == 0) {
                        status = STATUS_INVALID_DEVICE_STATE;
                        _SEH2_LEAVE;
                    }
                    fsContext->LockCount--;
                    FdoExtension->ProtectedLockCount--;
                    countChanged = TRUE;
                    break;
                }

                case InternalMediaLock: {
                    NT_ASSERT(FdoExtension->InternalLockCount != 0);
                    FdoExtension->InternalLockCount--;
                    countChanged = TRUE;
                    break;
                }
            }

            //
            // We only send an unlock command to the drive if both the
            // secured and unsecured lock counts have dropped to zero.
            //

            if ((FdoExtension->ProtectedLockCount != 0) ||
                (FdoExtension->InternalLockCount != 0) ||
                (FdoExtension->LockCount != 0)) {

                status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
        }

        status = STATUS_SUCCESS;
        if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {

            srb = (PSCSI_REQUEST_BLOCK)ClasspAllocateSrb(FdoExtension);

            if (srb == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

                //
                // NOTE - this is based on size used in ClasspAllocateSrb
                //

                status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srb,
                                                       STORAGE_ADDRESS_TYPE_BTL8,
                                                       CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                       1,
                                                       SrbExDataTypeScsiCdb16);
                if (!NT_SUCCESS(status)) {
                    NT_ASSERT(FALSE);
                    _SEH2_LEAVE;
                }

            } else {
                RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
            }

            SrbSetCdbLength(srb, 6);
            cdb = SrbGetCdb(srb);
            NT_ASSERT(cdb != NULL);

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            //
            // TRUE - prevent media removal.
            // FALSE - allow media removal.
            //

            cdb->MEDIA_REMOVAL.Prevent = Lock;

            //
            // Set timeout value.
            //

            SrbSetTimeOutValue(srb, FdoExtension->TimeOutValue);

            //
            // The actual lock operation on the device isn't so important
            // as the internal lock counts.  Ignore failures.
            //

            status = ClassSendSrbSynchronous(FdoExtension->DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             FALSE);
        }

    } _SEH2_FINALLY {

        if (!NT_SUCCESS(status)) {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "ClasspEjectionControl: FAILED status %x -- "
                        "reverting lock counts\n", status));

            if (countChanged) {

                //
                // have to revert to previous counts if the
                // lock/unlock operation actually failed.
                //

                if (Lock) {

                    switch (LockType) {

                        case SimpleMediaLock: {
                            FdoExtension->LockCount--;
                            break;
                        }

                        case SecureMediaLock: {
                            fsContext->LockCount--;
                            FdoExtension->ProtectedLockCount--;
                            break;
                        }

                        case InternalMediaLock: {
                            FdoExtension->InternalLockCount--;
                            break;
                        }
                    }

                } else {

                    switch (LockType) {

                        case SimpleMediaLock: {
                            FdoExtension->LockCount++;
                            break;
                        }

                        case SecureMediaLock: {
                            fsContext->LockCount++;
                            FdoExtension->ProtectedLockCount++;
                            break;
                        }

                        case InternalMediaLock: {
                            FdoExtension->InternalLockCount++;
                            break;
                        }
                    }
                }

            }

        } else {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "ClasspEjectionControl: Succeeded\n"));

        }

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "ClasspEjectionControl: "
                    "Current Counts: Internal: %x  Secure: %x  Simple: %x\n",
                    FdoExtension->InternalLockCount,
                    FdoExtension->ProtectedLockCount,
                    FdoExtension->LockCount
                    ));

        KeSetEvent(&(FdoExtension->EjectSynchronizationEvent),
                   IO_NO_INCREMENT,
                   FALSE);
        KeLeaveCriticalRegion();

        if (srb) {
            ClassFreeOrReuseSrb(FdoExtension, srb);
        }

    } _SEH2_END;
    return status;
}

#else

/*
 *  ISSUE:  RESTORE this (see above)
 *      This is a new implementation of the function that doesn't thrash memory
 *      or depend on the srbLookasideList.
 *      HOWEVER, it seems to cause pagefile initialization to fail during boot
 *      for some reason.  Need to resolve this before switching to this function.
 */
NTSTATUS
ClasspEjectionControl(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN MEDIA_LOCK_TYPE LockType,
    IN BOOLEAN Lock
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PFILE_OBJECT_EXTENSION fsContext;
    BOOLEAN fileHandleOk = TRUE;
    BOOLEAN countChanged = FALSE;
    NTSTATUS status;

    PAGED_CODE();

    status = KeWaitForSingleObject(
                &fdoExt->EjectSynchronizationEvent,
                UserRequest,
                KernelMode,
                FALSE,
                NULL);
    NT_ASSERT(status == STATUS_SUCCESS);

    /*
     *  If this is a "secured" request, we have to make sure
     *  that the file handle is valid.
     */
    if (LockType == SecureMediaLock){
        PIO_STACK_LOCATION thisSp = IoGetCurrentIrpStackLocation(Irp);

        /*
         *  Make sure that the file object we are supplied has a
         *  proper FsContext before we try doing a secured lock.
         */
        if (thisSp->FileObject){
            PCOMMON_DEVICE_EXTENSION commonExt = (PCOMMON_DEVICE_EXTENSION)fdoExt;
            fsContext = ClassGetFsContext(commonExt, thisSp->FileObject);
        }
        else {
            fsContext = NULL;
        }

        if (!fsContext){
            NT_ASSERT(fsContext);
            fileHandleOk = FALSE;
        }
    }

    if (fileHandleOk){

        /*
         *  Adjust the lock counts and make sure they make sense.
         */
        status = STATUS_SUCCESS;
        if (Lock){
            switch(LockType) {
                case SimpleMediaLock:
                    fdoExt->LockCount++;
                    countChanged = TRUE;
                    break;
                case SecureMediaLock:
                    fsContext->LockCount++;
                    fdoExt->ProtectedLockCount++;
                    countChanged = TRUE;
                    break;
                case InternalMediaLock:
                    fdoExt->InternalLockCount++;
                    countChanged = TRUE;
                    break;
            }
        }
        else {
            /*
             *  This is an unlock command.  If it's a secured one then make sure
             *  the caller has a lock outstanding or return an error.
             */
            switch (LockType){
                case SimpleMediaLock:
                    if (fdoExt->LockCount > 0){
                        fdoExt->LockCount--;
                        countChanged = TRUE;
                    }
                    else {
                        NT_ASSERT(fdoExt->LockCount > 0);
                        status = STATUS_INTERNAL_ERROR;
                    }
                    break;
                case SecureMediaLock:
                    if (fsContext->LockCount > 0){
                        NT_ASSERT(fdoExt->ProtectedLockCount > 0);
                        fsContext->LockCount--;
                        fdoExt->ProtectedLockCount--;
                        countChanged = TRUE;
                    }
                    else {
                        NT_ASSERT(fsContext->LockCount > 0);
                        status = STATUS_INVALID_DEVICE_STATE;
                    }
                    break;
                case InternalMediaLock:
                    NT_ASSERT(fdoExt->InternalLockCount > 0);
                    fdoExt->InternalLockCount--;
                    countChanged = TRUE;
                    break;
            }
        }

        if (NT_SUCCESS(status)){
            /*
             *  We only send an unlock command to the drive if
             *  all the lock counts have dropped to zero.
             */
            if (!Lock &&
               (fdoExt->ProtectedLockCount ||
                fdoExt->InternalLockCount ||
                fdoExt->LockCount)){

                /*
                 *  The lock count is still positive, so don't unlock yet.
                 */
                status = STATUS_SUCCESS;
            }
            else if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {
                /*
                 *  The device isn't removable media.  don't send a cmd.
                 */
                status  = STATUS_SUCCESS;
            }
            else {
                TRANSFER_PACKET *pkt;

                pkt = DequeueFreeTransferPacket(Fdo, TRUE);
                if (pkt){
                    KEVENT event;

                    /*
                     *  Store the number of packets servicing the irp (one)
                     *  inside the original IRP.  It will be used to counted down
                     *  to zero when the packet completes.
                     *  Initialize the original IRP's status to success.
                     *  If the packet fails, we will set it to the error status.
                     */
                    Irp->Tail.Overlay.DriverContext[0] = LongToPtr(1);
                    Irp->IoStatus.Status = STATUS_SUCCESS;

                    /*
                     *  Set this up as a SYNCHRONOUS transfer, submit it,
                     *  and wait for the packet to complete.  The result
                     *  status will be written to the original irp.
                     */
                    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
                    SetupEjectionTransferPacket(pkt, Lock, &event, Irp);
                    SubmitTransferPacket(pkt);
                    (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                    status = Irp->IoStatus.Status;
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
    }
    else {
        status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(status) && countChanged) {

        //
        // have to revert to previous counts if the
        // lock/unlock operation actually failed.
        //

        if(Lock) {

            switch(LockType) {

                case SimpleMediaLock: {
                    FdoExtension->LockCount--;
                    break;
                }

                case SecureMediaLock: {
                    fsContext->LockCount--;
                    FdoExtension->ProtectedLockCount--;
                    break;
                }

                case InternalMediaLock: {
                    FdoExtension->InternalLockCount--;
                    break;
                }
            }

        } else {

            switch(LockType) {

                case SimpleMediaLock: {
                    FdoExtension->LockCount++;
                    break;
                }

                case SecureMediaLock: {
                    fsContext->LockCount++;
                    FdoExtension->ProtectedLockCount++;
                    break;
                }

                case InternalMediaLock: {
                    FdoExtension->InternalLockCount++;
                    break;
                }
            }
        }
    }



    KeSetEvent(&fdoExt->EjectSynchronizationEvent, IO_NO_INCREMENT, FALSE);

    return status;
}
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
PFILE_OBJECT_EXTENSION
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassGetFsContext(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExtension,
    _In_ PFILE_OBJECT FileObject
    )
{
    PAGED_CODE();
    return GetDictionaryEntry(&(CommonExtension->FileObjectDictionary),
                              (ULONGLONG) FileObject);
}
