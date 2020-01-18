/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    class.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"

#define CLASS_TAG_POWER     'WLcS'

NTSTATUS
NTAPI
ClasspPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN CLASS_POWER_OPTIONS Options
    );

IO_COMPLETION_ROUTINE ClasspPowerDownCompletion;

IO_COMPLETION_ROUTINE ClasspPowerUpCompletion;

VOID
NTAPI
RetryPowerRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PCLASS_POWER_CONTEXT Context
    );

NTSTATUS
NTAPI
ClasspStartNextPowerIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


/*++////////////////////////////////////////////////////////////////////////////

ClassDispatchPower()

Routine Description:

    This routine acquires the removelock for the irp and then calls the
    appropriate power callback.

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
NTSTATUS
NTAPI
ClassDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    ULONG isRemoved;

    //
    // NOTE: This code may be called at PASSIVE or DISPATCH, depending
    //       upon the device object it is being called for.
    //       don't do anything that would break under either circumstance.
    //

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    if(isRemoved) {
        ClassReleaseRemoveLock(DeviceObject, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        PoStartNextPowerIrp(Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    return commonExtension->DevInfo->ClassPowerDevice(DeviceObject, Irp);
} // end ClassDispatchPower()

/*++////////////////////////////////////////////////////////////////////////////

ClasspPowerUpCompletion()

Routine Description:

    This routine is used for intermediate completion of a power up request.
    PowerUp requires four requests to be sent to the lower driver in sequence.

        * The queue is "power locked" to ensure that the class driver power-up
          work can be done before request processing resumes.

        * The power irp is sent down the stack for any filter drivers and the
          port driver to return power and resume command processing for the
          device.  Since the queue is locked, no queued irps will be sent
          immediately.

        * A start unit command is issued to the device with appropriate flags
          to override the "power locked" queue.

        * The queue is "power unlocked" to start processing requests again.

    This routine uses the function in the srb which just completed to determine
    which state it is in.

Arguments:

    DeviceObject - the device object being powered up

    Irp - the IO_REQUEST_PACKET containing the power request

    Srb - the SRB used to perform port/class operations.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED or
    STATUS_SUCCESS
    
--*/
NTSTATUS
NTAPI
ClasspPowerUpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID CompletionContext
    )
{
    PCLASS_POWER_CONTEXT context = CompletionContext;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION currentStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);


    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;

    DebugPrint((1, "ClasspPowerUpCompletion: Device Object %p, Irp %p, "
                   "Context %p\n",
                DeviceObject, Irp, context));

    ASSERT(!TEST_FLAG(context->Srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    ASSERT(!TEST_FLAG(context->Srb.SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
    ASSERT(context->Options.PowerDown == FALSE);
    ASSERT(context->Options.HandleSpinUp);

    if(Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    context->PowerChangeState.PowerUp++;

    switch(context->PowerChangeState.PowerUp) {

        case PowerUpDeviceLocked: {

            DebugPrint((1, "(%p)\tPreviously sent power lock\n", Irp));

            //
            // Issue the actual power request to the lower driver.
            //

            IoCopyCurrentIrpStackLocationToNext(Irp);

            //
            // If the lock wasn't successful then just bail out on the power
            // request unless we can ignore failed locks
            //

            if ((context->Options.LockQueue != FALSE) &&
               (!NT_SUCCESS(Irp->IoStatus.Status))) {

                DebugPrint((1, "(%p)\tIrp status was %lx\n",
                            Irp, Irp->IoStatus.Status));
                DebugPrint((1, "(%p)\tSrb status was %lx\n",
                            Irp, context->Srb.SrbStatus));

                //
                // Lock was not successful - throw down the power IRP
                // by itself and don't try to spin up the drive or unlock
                // the queue.
                //

                context->InUse = FALSE;
                context = NULL;

                //
                // Set the new power state
                //

                fdoExtension->DevicePowerState =
                    currentStack->Parameters.Power.State.DeviceState;

                Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                IoCopyCurrentIrpStackLocationToNext(Irp);

                IoSetCompletionRoutine(Irp,
                                       ClasspStartNextPowerIrpCompletion,
                                       NULL,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                //
                // Indicate to Po that we've been successfully powered up so
                // it can do it's notification stuff.
                //

                PoSetPowerState(DeviceObject,
                                currentStack->Parameters.Power.Type,
                                currentStack->Parameters.Power.State);

                PoCallDriver(commonExtension->LowerDeviceObject, Irp);

                ClassReleaseRemoveLock(commonExtension->DeviceObject,
                                       Irp);

                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {
                context->QueueLocked = (UCHAR) context->Options.LockQueue;
            }

            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

            context->PowerChangeState.PowerUp = PowerUpDeviceLocked;

            IoSetCompletionRoutine(Irp,
                                   ClasspPowerUpCompletion,
                                   context,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            status = PoCallDriver(commonExtension->LowerDeviceObject, Irp);

            DebugPrint((2, "(%p)\tPoCallDriver returned %lx\n", Irp, status));
            break;
        }

        case PowerUpDeviceOn: {

            PCDB cdb;

            if(NT_SUCCESS(Irp->IoStatus.Status)) {

                DebugPrint((1, "(%p)\tSending start unit to device\n", Irp));

                //
                // Issue the start unit command to the device.
                //

                context->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
                context->Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;

                context->Srb.SrbStatus = context->Srb.ScsiStatus = 0;
                context->Srb.DataTransferLength = 0;

                context->Srb.TimeOutValue = START_UNIT_TIMEOUT;

                context->Srb.SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                                        SRB_FLAGS_DISABLE_AUTOSENSE |
                                        SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                        SRB_FLAGS_NO_QUEUE_FREEZE;

                if(context->Options.LockQueue) {
                    SET_FLAG(context->Srb.SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE);
                }

                context->Srb.CdbLength = 6;

                cdb = (PCDB) (context->Srb.Cdb);
                RtlZeroMemory(cdb, sizeof(CDB));


                cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
                cdb->START_STOP.Start = 1;

                context->PowerChangeState.PowerUp = PowerUpDeviceOn;

                IoSetCompletionRoutine(Irp,
                                       ClasspPowerUpCompletion,
                                       context,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                nextStack->Parameters.Scsi.Srb = &(context->Srb);
                nextStack->MajorFunction = IRP_MJ_SCSI;

                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

                DebugPrint((2, "(%p)\tIoCallDriver returned %lx\n", Irp, status));

            } else {

                //
                // we're done.
                //

                context->FinalStatus = Irp->IoStatus.Status;
                goto ClasspPowerUpCompletionFailure;
            }

            break;
        }

        case PowerUpDeviceStarted: { // 3

            //
            // First deal with an error if one occurred.
            //

            if(SRB_STATUS(context->Srb.SrbStatus) != SRB_STATUS_SUCCESS) {

                BOOLEAN retry;

                DebugPrint((1, "%p\tError occured when issuing START_UNIT "
                            "command to device. Srb %p, Status %x\n",
                            Irp,
                            &context->Srb,
                            context->Srb.SrbStatus));

                ASSERT(!(TEST_FLAG(context->Srb.SrbStatus,
                                   SRB_STATUS_QUEUE_FROZEN)));
                ASSERT(context->Srb.Function == SRB_FUNCTION_EXECUTE_SCSI);

                context->RetryInterval = 0;

                retry = ClassInterpretSenseInfo(
                            commonExtension->DeviceObject,
                            &context->Srb,
                            IRP_MJ_SCSI,
                            IRP_MJ_POWER,
                            MAXIMUM_RETRIES - context->RetryCount,
                            &status,
                            &context->RetryInterval);

                if ((retry != FALSE) && (context->RetryCount-- != 0)) {

                    DebugPrint((1, "(%p)\tRetrying failed request\n", Irp));

                    //
                    // Decrement the state so we come back through here the
                    // next time.
                    //

                    context->PowerChangeState.PowerUp--;

                    RetryPowerRequest(commonExtension->DeviceObject,
                                      Irp,
                                      context);

                    break;

                }

                // reset retries
                context->RetryCount = MAXIMUM_RETRIES;

            }

ClasspPowerUpCompletionFailure:

            DebugPrint((1, "(%p)\tPreviously spun device up\n", Irp));

            if (context->QueueLocked) {
                DebugPrint((1, "(%p)\tUnlocking queue\n", Irp));

                context->Srb.Function = SRB_FUNCTION_UNLOCK_QUEUE;
                context->Srb.SrbFlags = SRB_FLAGS_BYPASS_LOCKED_QUEUE;
                context->Srb.SrbStatus = context->Srb.ScsiStatus = 0;
                context->Srb.DataTransferLength = 0;

                nextStack->Parameters.Scsi.Srb = &(context->Srb);
                nextStack->MajorFunction = IRP_MJ_SCSI;

                context->PowerChangeState.PowerUp = PowerUpDeviceStarted;

                IoSetCompletionRoutine(Irp,
                                       ClasspPowerUpCompletion,
                                       context,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                DebugPrint((1, "(%p)\tIoCallDriver returned %lx\n",
                            Irp, status));
                break;
            }

            // Fall-through to next case...

        }

        case PowerUpDeviceUnlocked: {

            //
            // This is the end of the dance.  Free the srb and complete the
            // request finally.  We're ignoring possible intermediate
            // error conditions ....
            //

            if (context->QueueLocked) {
                DebugPrint((1, "(%p)\tPreviously unlocked queue\n", Irp));
                ASSERT(NT_SUCCESS(Irp->IoStatus.Status));
                ASSERT(context->Srb.SrbStatus == SRB_STATUS_SUCCESS);
            } else {
                DebugPrint((1, "(%p)\tFall-through (queue not locked)\n", Irp));
            }

            DebugPrint((1, "(%p)\tFreeing srb and completing\n", Irp));
            context->InUse = FALSE;

            status = context->FinalStatus;
            Irp->IoStatus.Status = status;

            context = NULL;

            //
            // Set the new power state
            //

            if(NT_SUCCESS(status)) {
                fdoExtension->DevicePowerState =
                    currentStack->Parameters.Power.State.DeviceState;
            }

            //
            // Indicate to Po that we've been successfully powered up so
            // it can do it's notification stuff.
            //
            
            PoSetPowerState(DeviceObject,
                            currentStack->Parameters.Power.Type,
                            currentStack->Parameters.Power.State);

            DebugPrint((1, "(%p)\tStarting next power irp\n", Irp));
            ClassReleaseRemoveLock(DeviceObject, Irp);
            PoStartNextPowerIrp(Irp);

            return status;
        }

        case PowerUpDeviceInitial: {
            NT_ASSERT(context->PowerChangeState.PowerUp != PowerUpDeviceInitial);
            break;
        }
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
} // end ClasspPowerUpCompletion()

/*++////////////////////////////////////////////////////////////////////////////

ClasspPowerDownCompletion()

Routine Description:

    This routine is used for intermediate completion of a power up request.
    PowerUp requires four requests to be sent to the lower driver in sequence.

        * The queue is "power locked" to ensure that the class driver power-up
          work can be done before request processing resumes.

        * The power irp is sent down the stack for any filter drivers and the
          port driver to return power and resume command processing for the
          device.  Since the queue is locked, no queued irps will be sent
          immediately.

        * A start unit command is issued to the device with appropriate flags
          to override the "power locked" queue.

        * The queue is "power unlocked" to start processing requests again.

    This routine uses the function in the srb which just completed to determine
    which state it is in.

Arguments:

    DeviceObject - the device object being powered up

    Irp - the IO_REQUEST_PACKET containing the power request

    Srb - the SRB used to perform port/class operations.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED or
    STATUS_SUCCESS

--*/
NTSTATUS
NTAPI
ClasspPowerDownCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID CompletionContext
    )
{
    PCLASS_POWER_CONTEXT context = CompletionContext;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION currentStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);

    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;

    DebugPrint((1, "ClasspPowerDownCompletion: Device Object %p, "
                   "Irp %p, Context %p\n",
                DeviceObject, Irp, context));

    ASSERT(!TEST_FLAG(context->Srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    ASSERT(!TEST_FLAG(context->Srb.SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
    ASSERT(context->Options.PowerDown == TRUE);
    ASSERT(context->Options.HandleSpinDown);

    if(Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    context->PowerChangeState.PowerDown2++;

    switch(context->PowerChangeState.PowerDown2) {

        case PowerDownDeviceLocked2: {

            PCDB cdb;

            DebugPrint((1, "(%p)\tPreviously sent power lock\n", Irp));

            if ((context->Options.LockQueue != FALSE) &&
               (!NT_SUCCESS(Irp->IoStatus.Status))) {

                DebugPrint((1, "(%p)\tIrp status was %lx\n",
                            Irp,
                            Irp->IoStatus.Status));
                DebugPrint((1, "(%p)\tSrb status was %lx\n",
                            Irp,
                            context->Srb.SrbStatus));

                Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                //
                // Lock was not successful - throw down the power IRP
                // by itself and don't try to spin down the drive or unlock
                // the queue.
                //

                context->InUse = FALSE;
                context = NULL;

                //
                // Set the new power state
                //

                fdoExtension->DevicePowerState =
                    currentStack->Parameters.Power.State.DeviceState;

                //
                // Indicate to Po that we've been successfully powered down
                // so it can do it's notification stuff.
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       ClasspStartNextPowerIrpCompletion,
                                       NULL,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                PoSetPowerState(DeviceObject,
                                currentStack->Parameters.Power.Type,
                                currentStack->Parameters.Power.State);

                fdoExtension->PowerDownInProgress = FALSE;

                PoCallDriver(commonExtension->LowerDeviceObject, Irp);

                ClassReleaseRemoveLock(commonExtension->DeviceObject,
                                       Irp);

                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {
                context->QueueLocked = (UCHAR) context->Options.LockQueue;
            }

            if (!TEST_FLAG(fdoExtension->PrivateFdoData->HackFlags,
                           FDO_HACK_NO_SYNC_CACHE)) {

                //
                // send SCSIOP_SYNCHRONIZE_CACHE
                //

                context->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
                context->Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;

                context->Srb.TimeOutValue = fdoExtension->TimeOutValue;

                context->Srb.SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                                        SRB_FLAGS_DISABLE_AUTOSENSE |
                                        SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                        SRB_FLAGS_NO_QUEUE_FREEZE |
                                        SRB_FLAGS_BYPASS_LOCKED_QUEUE;

                context->Srb.SrbStatus = context->Srb.ScsiStatus = 0;
                context->Srb.DataTransferLength = 0;

                context->Srb.CdbLength = 10;

                cdb = (PCDB) context->Srb.Cdb;

                RtlZeroMemory(cdb, sizeof(CDB));
                cdb->SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;

                IoSetCompletionRoutine(Irp,
                                       ClasspPowerDownCompletion,
                                       context,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                nextStack->Parameters.Scsi.Srb = &(context->Srb);
                nextStack->MajorFunction = IRP_MJ_SCSI;

                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

                DebugPrint((1, "(%p)\tIoCallDriver returned %lx\n", Irp, status));
                break;

            } else {

                DebugPrint((1, "(%p)\tPower Down: not sending SYNCH_CACHE\n",
                            DeviceObject));
                context->PowerChangeState.PowerDown2++;
                context->Srb.SrbStatus = SRB_STATUS_SUCCESS;
                // and fall through....
            }
            // no break in case the device doesn't like synch_cache commands

        }
         
        case PowerDownDeviceFlushed2: {

            PCDB cdb;

            DebugPrint((1, "(%p)\tPreviously send SCSIOP_SYNCHRONIZE_CACHE\n",
                        Irp));

            //
            // SCSIOP_SYNCHRONIZE_CACHE was sent
            //

            if(SRB_STATUS(context->Srb.SrbStatus) != SRB_STATUS_SUCCESS) {

                BOOLEAN retry;

                DebugPrint((1, "(%p)\tError occured when issuing "
                            "SYNCHRONIZE_CACHE command to device. "
                            "Srb %p, Status %lx\n",
                            Irp,
                            &context->Srb,
                            context->Srb.SrbStatus));

                ASSERT(!(TEST_FLAG(context->Srb.SrbStatus,
                                   SRB_STATUS_QUEUE_FROZEN)));
                ASSERT(context->Srb.Function == SRB_FUNCTION_EXECUTE_SCSI);

                context->RetryInterval = 0;
                retry = ClassInterpretSenseInfo(
                            commonExtension->DeviceObject,
                            &context->Srb,
                            IRP_MJ_SCSI,
                            IRP_MJ_POWER,
                            MAXIMUM_RETRIES - context->RetryCount,
                            &status,
                            &context->RetryInterval);

                if ((retry != FALSE) && (context->RetryCount-- != 0)) {

                        DebugPrint((1, "(%p)\tRetrying failed request\n", Irp));

                        //
                        // decrement the state so we come back through here
                        // the next time.
                        //

                        context->PowerChangeState.PowerDown2--;
                        RetryPowerRequest(commonExtension->DeviceObject,
                                          Irp,
                                          context);
                        break;
                }

                DebugPrint((1, "(%p)\tSYNCHRONIZE_CACHE not retried\n", Irp));
                context->RetryCount = MAXIMUM_RETRIES;

            } // end !SRB_STATUS_SUCCESS

            //
            // note: we are purposefully ignoring any errors.  if the drive
            //       doesn't support a synch_cache, then we're up a creek
            //       anyways.
            //

            DebugPrint((1, "(%p)\tSending stop unit to device\n", Irp));

            //
            // Issue the start unit command to the device.
            //

            context->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
            context->Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;

            context->Srb.TimeOutValue = START_UNIT_TIMEOUT;

            context->Srb.SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                                    SRB_FLAGS_DISABLE_AUTOSENSE |
                                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                    SRB_FLAGS_NO_QUEUE_FREEZE |
                                    SRB_FLAGS_BYPASS_LOCKED_QUEUE;

            context->Srb.SrbStatus = context->Srb.ScsiStatus = 0;
            context->Srb.DataTransferLength = 0;

            context->Srb.CdbLength = 6;

            cdb = (PCDB) context->Srb.Cdb;
            RtlZeroMemory(cdb, sizeof(CDB));

            cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->START_STOP.Start = 0;
            cdb->START_STOP.Immediate = 1;

            IoSetCompletionRoutine(Irp,
                                   ClasspPowerDownCompletion,
                                   context,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            nextStack->Parameters.Scsi.Srb = &(context->Srb);
            nextStack->MajorFunction = IRP_MJ_SCSI;

            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

            DebugPrint((1, "(%p)\tIoCallDriver returned %lx\n", Irp, status));
            break;

        }

        case PowerDownDeviceStopped2: {

            BOOLEAN ignoreError = TRUE;

            //
            // stop was sent
            //

            if(SRB_STATUS(context->Srb.SrbStatus) != SRB_STATUS_SUCCESS) {

                BOOLEAN retry;

                DebugPrint((1, "(%p)\tError occured when issuing STOP_UNIT "
                            "command to device. Srb %p, Status %lx\n",
                            Irp,
                            &context->Srb,
                            context->Srb.SrbStatus));

                ASSERT(!(TEST_FLAG(context->Srb.SrbStatus,
                                   SRB_STATUS_QUEUE_FROZEN)));
                ASSERT(context->Srb.Function == SRB_FUNCTION_EXECUTE_SCSI);

                context->RetryInterval = 0;
                retry = ClassInterpretSenseInfo(
                            commonExtension->DeviceObject,
                            &context->Srb,
                            IRP_MJ_SCSI,
                            IRP_MJ_POWER,
                            MAXIMUM_RETRIES - context->RetryCount,
                            &status,
                            &context->RetryInterval);

                if ((retry != FALSE) && (context->RetryCount-- != 0)) {

                        DebugPrint((1, "(%p)\tRetrying failed request\n", Irp));

                        //
                        // decrement the state so we come back through here
                        // the next time.
                        //

                        context->PowerChangeState.PowerDown2--;
                        RetryPowerRequest(commonExtension->DeviceObject,
                                          Irp,
                                          context);
                        break;
                }

                DebugPrint((1, "(%p)\tSTOP_UNIT not retried\n", Irp));
                context->RetryCount = MAXIMUM_RETRIES;

            } // end !SRB_STATUS_SUCCESS


            DebugPrint((1, "(%p)\tPreviously sent stop unit\n", Irp));

            //
            // some operations, such as a physical format in progress,
            // should not be ignored and should fail the power operation.
            //

            if (!NT_SUCCESS(status)) {

                PSENSE_DATA senseBuffer = context->Srb.SenseInfoBuffer;

                if (TEST_FLAG(context->Srb.SrbStatus,
                              SRB_STATUS_AUTOSENSE_VALID) &&
                    ((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_NOT_READY) &&
                    (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
                    (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_FORMAT_IN_PROGRESS)
                    ) {
                    ignoreError = FALSE;
                    context->FinalStatus = STATUS_DEVICE_BUSY;
                    status = context->FinalStatus;
                }

            }

            if (NT_SUCCESS(status) || ignoreError) {

                //
                // Issue the actual power request to the lower driver.
                //

                Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                IoCopyCurrentIrpStackLocationToNext(Irp);

                IoSetCompletionRoutine(Irp,
                                       ClasspPowerDownCompletion,
                                       context,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = PoCallDriver(commonExtension->LowerDeviceObject, Irp);

                DebugPrint((1, "(%p)\tPoCallDriver returned %lx\n", Irp, status));
                break;
            }
            
            // else fall through w/o sending the power irp, since the device
            // is reporting an error that would be "really bad" to power down
            // during.

        }

        case PowerDownDeviceOff2: {

            //
            // SpinDown request completed ... whether it succeeded or not is
            // another matter entirely.
            //

            DebugPrint((1, "(%p)\tPreviously sent power irp\n", Irp));

            if (context->QueueLocked) {

                DebugPrint((1, "(%p)\tUnlocking queue\n", Irp));

                context->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);

                context->Srb.SrbStatus = context->Srb.ScsiStatus = 0;
                context->Srb.DataTransferLength = 0;

                context->Srb.Function = SRB_FUNCTION_UNLOCK_QUEUE;
                context->Srb.SrbFlags = SRB_FLAGS_BYPASS_LOCKED_QUEUE;
                nextStack->Parameters.Scsi.Srb = &(context->Srb);
                nextStack->MajorFunction = IRP_MJ_SCSI;

                IoSetCompletionRoutine(Irp,
                                       ClasspPowerDownCompletion,
                                       context,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                DebugPrint((1, "(%p)\tIoCallDriver returned %lx\n",
                            Irp,
                            status));
                break;
            }

        }

        case PowerDownDeviceUnlocked2: {

            //
            // This is the end of the dance.  Free the srb and complete the
            // request finally.  We're ignoring possible intermediate
            // error conditions ....
            //

            if (context->QueueLocked == FALSE) {
                DebugPrint((1, "(%p)\tFall through (queue not locked)\n", Irp));
            } else {
                DebugPrint((1, "(%p)\tPreviously unlocked queue\n", Irp));
                ASSERT(NT_SUCCESS(Irp->IoStatus.Status));
                ASSERT(context->Srb.SrbStatus == SRB_STATUS_SUCCESS);
            }

            DebugPrint((1, "(%p)\tFreeing srb and completing\n", Irp));
            context->InUse = FALSE;
            status = context->FinalStatus; // allow failure to propagate
            context = NULL;

            if(Irp->PendingReturned) {
                IoMarkIrpPending(Irp);
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            if (NT_SUCCESS(status)) {
                
                //
                // Set the new power state
                //

                fdoExtension->DevicePowerState =
                    currentStack->Parameters.Power.State.DeviceState;

            }


            DebugPrint((1, "(%p)\tStarting next power irp\n", Irp));

            ClassReleaseRemoveLock(DeviceObject, Irp);
            PoStartNextPowerIrp(Irp);
            fdoExtension->PowerDownInProgress = FALSE;

            return status;
        }

        case PowerDownDeviceInitial2: {
            NT_ASSERT(context->PowerChangeState.PowerDown2 != PowerDownDeviceInitial2);
            break;
        }
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
} // end ClasspPowerDownCompletion()

/*++////////////////////////////////////////////////////////////////////////////

ClasspPowerHandler()

Routine Description:

    This routine reduces the number of useless spinups and spindown requests
    sent to a given device by ignoring transitions to power states we are
    currently in.
    
    ISSUE-2000/02/20-henrygab - by ignoring spin-up requests, we may be
          allowing the drive

Arguments:

    DeviceObject - the device object which is transitioning power states
    Irp - the power irp
    Options - a set of flags indicating what the device handles

Return Value:

--*/
NTSTATUS
NTAPI
ClasspPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN CLASS_POWER_OPTIONS Options  // ISSUE-2000/02/20-henrygab - pass pointer, not whole struct
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT lowerDevice = commonExtension->LowerDeviceObject;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCLASS_POWER_CONTEXT context;

    if (!commonExtension->IsFdo) {

        //
        // certain assumptions are made here,
        // particularly: having the fdoExtension
        //

        DebugPrint((0, "ClasspPowerHandler: Called for PDO %p???\n",
                    DeviceObject));
        ASSERT(!"PDO using ClasspPowerHandler");
        return STATUS_NOT_SUPPORTED;
    }

    DebugPrint((1, "ClasspPowerHandler: Power irp %p to %s %p\n",
                Irp, (commonExtension->IsFdo ? "fdo" : "pdo"), DeviceObject));

    switch(irpStack->MinorFunction) {

        case IRP_MN_SET_POWER: {
            PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;

            DebugPrint((1, "(%p)\tIRP_MN_SET_POWER\n", Irp));

            DebugPrint((1, "(%p)\tSetting %s state to %d\n",
                        Irp,
                        (irpStack->Parameters.Power.Type == SystemPowerState ?
                            "System" : "Device"),
                        irpStack->Parameters.Power.State.SystemState));

                switch (irpStack->Parameters.Power.ShutdownType){
                    
                    case PowerActionSleep:
                    case PowerActionHibernate:
                        if (fdoData->HotplugInfo.MediaRemovable || fdoData->HotplugInfo.MediaHotplug){
                            /*
                             *  We are suspending and this drive is either hot-pluggable
                             *  or contains removeable media.
                             *  Set the media dirty bit, since the media may change while
                             *  we are suspended.
                             */
                            SET_FLAG(DeviceObject->Flags, DO_VERIFY_VOLUME);
                        }
                        break;
                    default:
                        break;
                }
            
            break;
        }

        default: {

            DebugPrint((1, "(%p)\tIrp minor code = %#x\n",
                        Irp, irpStack->MinorFunction));
            break;
        }
    }

    if (irpStack->Parameters.Power.Type != DevicePowerState ||
        irpStack->MinorFunction != IRP_MN_SET_POWER) {

        DebugPrint((1, "(%p)\tSending to lower device\n", Irp));

        goto ClasspPowerHandlerCleanup;

    }

    nextIrpStack = IoGetNextIrpStackLocation(Irp);

    //
    // already in exact same state, don't work to transition to it.
    //

    if(irpStack->Parameters.Power.State.DeviceState ==
       fdoExtension->DevicePowerState) {

        DebugPrint((1, "(%p)\tAlready in device state %x\n",
                    Irp, fdoExtension->DevicePowerState));
        goto ClasspPowerHandlerCleanup;

    }

    //
    // or powering down from non-d0 state (device already stopped)
    // NOTE -- we're not sure whether this case can exist or not (the
    // power system may never send this sort of request) but it's trivial
    // to deal with.
    //

    if ((irpStack->Parameters.Power.State.DeviceState != PowerDeviceD0) &&
        (fdoExtension->DevicePowerState != PowerDeviceD0)) {
        DebugPrint((1, "(%p)\tAlready powered down to %x???\n",
                    Irp, fdoExtension->DevicePowerState));
        fdoExtension->DevicePowerState =
            irpStack->Parameters.Power.State.DeviceState;
        goto ClasspPowerHandlerCleanup;
    }

    //
    // or going into a hibernation state when we're in the hibernation path.
    // If the device is spinning then we should leave it spinning - if it's not
    // then the dump driver will start it up for us.
    //

    if((irpStack->Parameters.Power.State.DeviceState == PowerDeviceD3) &&
       (irpStack->Parameters.Power.ShutdownType == PowerActionHibernate) &&
       (commonExtension->HibernationPathCount != 0)) {

        DebugPrint((1, "(%p)\tdoing nothing for hibernation request for "
                       "state %x???\n",
                    Irp, fdoExtension->DevicePowerState));
        fdoExtension->DevicePowerState =
            irpStack->Parameters.Power.State.DeviceState;
        goto ClasspPowerHandlerCleanup;
    }
    //
    // or when not handling powering up and are powering up
    //

    if ((!Options.HandleSpinUp) &&
        (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0)) {

        DebugPrint((2, "(%p)\tNot handling spinup to state %x\n",
                    Irp, fdoExtension->DevicePowerState));
        fdoExtension->DevicePowerState =
            irpStack->Parameters.Power.State.DeviceState;
        goto ClasspPowerHandlerCleanup;

    }

    //
    // or when not handling powering down and are powering down
    //

    if ((!Options.HandleSpinDown) &&
        (irpStack->Parameters.Power.State.DeviceState != PowerDeviceD0)) {

        DebugPrint((2, "(%p)\tNot handling spindown to state %x\n",
                    Irp, fdoExtension->DevicePowerState));
        fdoExtension->DevicePowerState =
            irpStack->Parameters.Power.State.DeviceState;
        goto ClasspPowerHandlerCleanup;

    }

    context = &(fdoExtension->PowerContext);

#if DBG
    //
    // Mark the context as in use.  We should be synchronizing this but
    // since it's just for debugging purposes we don't worry too much.
    //

    ASSERT(context->InUse == FALSE);
#endif

    RtlZeroMemory(context, sizeof(CLASS_POWER_CONTEXT));
    context->InUse = TRUE;

    nextIrpStack->Parameters.Scsi.Srb = &(context->Srb);
    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    context->FinalStatus = STATUS_SUCCESS;

    context->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
    context->Srb.OriginalRequest = Irp;
    context->Srb.SrbFlags |= SRB_FLAGS_BYPASS_LOCKED_QUEUE
                          |  SRB_FLAGS_NO_QUEUE_FREEZE;
    context->Srb.Function = SRB_FUNCTION_LOCK_QUEUE;

    context->Srb.SenseInfoBuffer =
        commonExtension->PartitionZeroExtension->SenseData;
    context->Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    context->RetryCount = MAXIMUM_RETRIES;

    context->Options = Options;
    context->DeviceObject = DeviceObject;
    context->Irp = Irp;

    if(irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0) {

        ASSERT(Options.HandleSpinUp);

        DebugPrint((2, "(%p)\tpower up - locking queue\n", Irp));

        //
        // We need to issue a queue lock request so that we
        // can spin the drive back up after the power is restored
        // but before any requests are processed.
        //

        context->Options.PowerDown = FALSE;
        context->PowerChangeState.PowerUp = PowerUpDeviceInitial;
        context->CompletionRoutine = ClasspPowerUpCompletion;

    } else {

        ASSERT(Options.HandleSpinDown);

        fdoExtension->PowerDownInProgress = TRUE;

        DebugPrint((2, "(%p)\tPowering down - locking queue\n", Irp));

        PoSetPowerState(DeviceObject,
                        irpStack->Parameters.Power.Type,
                        irpStack->Parameters.Power.State);

        context->Options.PowerDown = TRUE;
        context->PowerChangeState.PowerDown2 = PowerDownDeviceInitial2;
        context->CompletionRoutine = ClasspPowerDownCompletion;

    }

    //
    // we are not dealing with port-allocated sense in these routines.
    //

    ASSERT(!TEST_FLAG(context->Srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    ASSERT(!TEST_FLAG(context->Srb.SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));

    //
    // we are always returning STATUS_PENDING, so we need to always
    // set the irp as pending.
    //

    IoMarkIrpPending(Irp);

    if(Options.LockQueue) {

        //
        // Send the lock irp down.
        //

        IoSetCompletionRoutine(Irp,
                               context->CompletionRoutine,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);

        IoCallDriver(lowerDevice, Irp);

    } else {

        //
        // Call the completion routine directly.  It won't care what the
        // status of the "lock" was - it will just go and do the next
        // step of the operation.
        //

        context->CompletionRoutine(DeviceObject, Irp, context);
    }

    return STATUS_PENDING;

ClasspPowerHandlerCleanup:

    ClassReleaseRemoveLock(DeviceObject, Irp);

    DebugPrint((1, "(%p)\tStarting next power irp\n", Irp));
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           ClasspStartNextPowerIrpCompletion,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);
    return PoCallDriver(lowerDevice, Irp);
} // end ClasspPowerHandler()

/*++////////////////////////////////////////////////////////////////////////////

ClassMinimalPowerHandler()

Routine Description:

    This routine is the minimum power handler for a storage driver.  It does
    the least amount of work possible.

--*/
NTSTATUS
NTAPI
ClassMinimalPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    PoStartNextPowerIrp(Irp);

    if(commonExtension->IsFdo) {

        if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {

            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = 
                DeviceObject->DeviceExtension;

            //
            // Check if the system is going to hibernate or standby.
            //
            if (irpStack->MinorFunction == IRP_MN_SET_POWER){
                PVPB vpb;
                
                switch (irpStack->Parameters.Power.ShutdownType){
                    
                    case PowerActionSleep:
                    case PowerActionHibernate:
                        //
                        // If the volume is mounted, set the verify bit so that
                        // the filesystem will be forced re-read the media
                        // after coming out of hibernation or standby.
                        //
                        vpb = ClassGetVpb(fdoExtension->DeviceObject);
                        if (vpb && (vpb->Flags & VPB_MOUNTED)){
                            SET_FLAG(fdoExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);  
                        } 
                        break;
                    default:
                        break;
                }
            } 
        }

        IoCopyCurrentIrpStackLocationToNext(Irp);
        return PoCallDriver(commonExtension->LowerDeviceObject, Irp);

    } else {

        if (irpStack->MinorFunction != IRP_MN_SET_POWER &&
            irpStack->MinorFunction != IRP_MN_QUERY_POWER) {
           
            NOTHING;

        } else {

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

        }
        status = Irp->IoStatus.Status;

        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return status;
    }
} // end ClassMinimalPowerHandler()

/*++////////////////////////////////////////////////////////////////////////////

ClassSpinDownPowerHandler()

Routine Description:

    This routine is a callback for disks and other things which require both
    a start and a stop to be sent to the device.  (actually the starts are
    almost always optional, since most device power themselves on to process
    commands, but i digress).
    
    Determines proper use of spinup, spindown, and queue locking based upon
    ScanForSpecialFlags in the FdoExtension.  This is the most common power
    handler passed into classpnp.sys

Arguments:

    DeviceObject - Supplies the functional device object

    Irp - Supplies the request to be retried.

Return Value:

    None

--*/
NTSTATUS
NTAPI
ClassSpinDownPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    CLASS_POWER_OPTIONS options;

    fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // this will set all options to FALSE
    //

    RtlZeroMemory(&options, sizeof(CLASS_POWER_OPTIONS));

    //
    // check the flags to see what options we need to worry about
    //

    if (!TEST_FLAG(fdoExtension->ScanForSpecialFlags,
                  CLASS_SPECIAL_DISABLE_SPIN_DOWN)) {
        options.HandleSpinDown = TRUE;
    }

    if (!TEST_FLAG(fdoExtension->ScanForSpecialFlags,
                  CLASS_SPECIAL_DISABLE_SPIN_UP)) {
        options.HandleSpinUp = TRUE;
    }

    if (!TEST_FLAG(fdoExtension->ScanForSpecialFlags,
                  CLASS_SPECIAL_NO_QUEUE_LOCK)) {
        options.LockQueue = TRUE;
    }

    DebugPrint((3, "ClasspPowerHandler: Devobj %p\n"
                "\t%shandling spin down\n"
                "\t%shandling spin up\n"
                "\t%slocking queue\n",
                DeviceObject,
                (options.HandleSpinDown ? "" : "not "),
                (options.HandleSpinUp   ? "" : "not "),
                (options.LockQueue      ? "" : "not ")
                ));

    //
    // do all the dirty work
    //

    return ClasspPowerHandler(DeviceObject, Irp, options);
} // end ClassSpinDownPowerHandler()

/*++////////////////////////////////////////////////////////////////////////////

ClassStopUnitPowerHandler()

Routine Description:

    This routine is an outdated call.  To achieve equivalent functionality,
    the driver should set the following flags in ScanForSpecialFlags in the
    FdoExtension:
        
        CLASS_SPECIAL_DISABLE_SPIN_UP
        CLASS_SPECIAL_NO_QUEUE_LOCK

--*/
NTSTATUS
NTAPI
ClassStopUnitPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;

    DebugPrint((0, "ClassStopUnitPowerHandler - Devobj %p using outdated call\n"
                "Drivers should set the following flags in ScanForSpecialFlags "
                " in the FDO extension:\n"
                "\tCLASS_SPECIAL_DISABLE_SPIN_UP\n"
                "\tCLASS_SPECIAL_NO_QUEUE_LOCK\n"
                "This will provide equivalent functionality if the power "
                "routine is then set to ClassSpinDownPowerHandler\n\n",
                DeviceObject));

    fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    SET_FLAG(fdoExtension->ScanForSpecialFlags,
             CLASS_SPECIAL_DISABLE_SPIN_UP);
    SET_FLAG(fdoExtension->ScanForSpecialFlags,
             CLASS_SPECIAL_NO_QUEUE_LOCK);

    return ClassSpinDownPowerHandler(DeviceObject, Irp);
} // end ClassStopUnitPowerHandler()

/*++////////////////////////////////////////////////////////////////////////////

RetryPowerRequest()

Routine Description:

    This routine reinitializes the necessary fields, and sends the request
    to the lower driver.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - Supplies the request to be retried.

    Context - Supplies a pointer to the power up context for this request.

Return Value:

    None

--*/
VOID
NTAPI
RetryPowerRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PCLASS_POWER_CONTEXT Context
    )
{
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = &(Context->Srb);
    LARGE_INTEGER dueTime;

    DebugPrint((1, "(%p)\tDelaying retry by queueing DPC\n", Irp));

    ASSERT(Context->Irp == Irp);
    ASSERT(Context->DeviceObject == DeviceObject);
    ASSERT(!TEST_FLAG(Context->Srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    ASSERT(!TEST_FLAG(Context->Srb.SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));

    //
    // reset the retry interval
    //

    Context->RetryInterval = 0;

    //
    // Reset byte count of transfer in SRB Extension.
    //

    srb->DataTransferLength = 0;

    //
    // Zero SRB statuses.
    //

    srb->SrbStatus = srb->ScsiStatus = 0;

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = srb;

    //
    // Set the completion routine up again.
    //

    IoSetCompletionRoutine(Irp, Context->CompletionRoutine, Context,
                           TRUE, TRUE, TRUE);


    if (Context->RetryInterval == 0) {

        DebugPrint((2, "(%p)\tDelaying minimum time (.2 sec)\n", Irp));
        dueTime.QuadPart = (LONGLONG)1000000 * 2;

    } else {

        DebugPrint((2, "(%p)\tDelaying %x seconds\n",
                    Irp, Context->RetryInterval));
        dueTime.QuadPart = (LONGLONG)1000000 * 10 * Context->RetryInterval;

    }

    ClassRetryRequest(DeviceObject, Irp, dueTime);

    return;

} // end RetryRequest()

/*++////////////////////////////////////////////////////////////////////////////

ClasspStartNextPowerIrpCompletion()

Routine Description:

    This routine guarantees that the next power irp (power up or down) is not
    sent until the previous one has fully completed.

--*/
NTSTATUS
NTAPI
ClasspStartNextPowerIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    if(Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    PoStartNextPowerIrp(Irp);
    return STATUS_SUCCESS;
} // end ClasspStartNextPowerIrpCompletion()
