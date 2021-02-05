/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    power.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#ifndef __REACTOS__
#include "stddef.h"
#include "ntddk.h"
#include "scsi.h"
#endif
#include "classp.h"

// #include <stdarg.h> __REACTOS__

#ifdef DEBUG_USE_WPP
#include "power.tmh"
#endif

#define CLASS_TAG_POWER     'WLcS'

// constants for power transition process. (UNIT: seconds)
#define DEFAULT_POWER_IRP_TIMEOUT_VALUE 10*60
#define TIME_LEFT_FOR_LOWER_DRIVERS     30
#define TIME_LEFT_FOR_UPPER_DRIVERS     5
#define DEFAULT_IO_TIMEOUT_VALUE        10
#define MINIMUM_STOP_UNIT_TIMEOUT_VALUE 2

//
// MINIMAL value is one that has some slack and is the value to use
// if there is a shortened POWER IRP timeout value. If time remaining
// is less than MINIMAL, we will use the MINIMUM value. Both values
// are in the same unit as above (seconds).
//
#define MINIMAL_START_UNIT_TIMEOUT_VALUE 60
#define MINIMUM_START_UNIT_TIMEOUT_VALUE 30

// PoQueryWatchdogTime was introduced in Windows 7.
// Returns TRUE if a watchdog-enabled power IRP is found, otherwise FALSE.
#if (NTDDI_VERSION < NTDDI_WIN7)
#define PoQueryWatchdogTime(A, B) FALSE
#endif

IO_COMPLETION_ROUTINE ClasspPowerDownCompletion;

IO_COMPLETION_ROUTINE ClasspPowerUpCompletion;

IO_COMPLETION_ROUTINE ClasspStartNextPowerIrpCompletion;
IO_COMPLETION_ROUTINE ClasspDeviceLockFailurePowerIrpCompletion;

NTSTATUS
ClasspPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN CLASS_POWER_OPTIONS Options
    );

VOID
RetryPowerRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PCLASS_POWER_CONTEXT Context
    );

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ClasspPowerSettingCallback)
#endif

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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

    //
    // If device is added but not yet started, we need to send the Power
    // request down the stack.  If device is started and then stopped,
    // we have enough state to process the power request.
    //

    if (!commonExtension->IsInitialized) {

        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(commonExtension->LowerDeviceObject, Irp);
    }

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    if (isRemoved) {
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

    Irp - Context->Irp: original power irp; fdoExtension->PrivateFdoData->PowerProcessIrp: power process irp

    Context - Class power context used to perform port/class operations.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED or
    STATUS_SUCCESS

--*/
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspPowerUpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PCLASS_POWER_CONTEXT PowerContext = (PCLASS_POWER_CONTEXT)Context;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PIRP OriginalIrp;
    PIO_STACK_LOCATION currentStack;
    PIO_STACK_LOCATION nextStack;

    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;
    PSTORAGE_REQUEST_BLOCK_HEADER srbHeader;
    ULONG srbFlags;
    BOOLEAN FailurePredictionEnabled = FALSE;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (PowerContext == NULL) {
        NT_ASSERT(PowerContext != NULL);
        return STATUS_INVALID_PARAMETER;
    }

    commonExtension = PowerContext->DeviceObject->DeviceExtension;
    fdoExtension = PowerContext->DeviceObject->DeviceExtension;
    OriginalIrp = PowerContext->Irp;

    // currentStack - from original power irp
    // nextStack - from power process irp
    currentStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    nextStack = IoGetNextIrpStackLocation(fdoExtension->PrivateFdoData->PowerProcessIrp);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "ClasspPowerUpCompletion: Device Object %p, Irp %p, "
                   "Context %p\n",
                PowerContext->DeviceObject, Irp, Context));

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->PrivateFdoData->PowerSrb.SrbEx);

        //
        // Check if reverted to using legacy SRB.
        //
        if (PowerContext->Srb.Length == sizeof(SCSI_REQUEST_BLOCK)) {
            srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
        }
    } else {
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
    }

    srbFlags = SrbGetSrbFlags(srbHeader);
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
    NT_ASSERT(PowerContext->Options.PowerDown == FALSE);
    NT_ASSERT(PowerContext->Options.HandleSpinUp);

    if ((Irp == OriginalIrp) && (Irp->PendingReturned)) {
        // only for original power irp
        IoMarkIrpPending(Irp);
    }

    PowerContext->PowerChangeState.PowerUp++;

    switch (PowerContext->PowerChangeState.PowerUp) {

        case PowerUpDeviceLocked: {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously sent power lock\n", Irp));

            //
            // Lock Queue operation has been sent.
            // Now, send the original power irp down to get lower driver and device ready.
            //

            IoCopyCurrentIrpStackLocationToNext(OriginalIrp);

            if ((PowerContext->Options.LockQueue == TRUE) &&
                (!NT_SUCCESS(Irp->IoStatus.Status))) {

                //
                // Lock was not successful:
                // Issue the original power request to the lower driver and next power irp will be started in completion routine.
                //


                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIrp status was %lx\n",
                            Irp, Irp->IoStatus.Status));
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSrb status was %lx\n",
                            Irp, srbHeader->SrbStatus));

                IoSetCompletionRoutine(OriginalIrp,
                                       ClasspDeviceLockFailurePowerIrpCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                PoCallDriver(commonExtension->LowerDeviceObject, OriginalIrp);

                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {
                PowerContext->QueueLocked = (UCHAR)PowerContext->Options.LockQueue;
            }

            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

            PowerContext->PowerChangeState.PowerUp = PowerUpDeviceLocked;

            IoSetCompletionRoutine(OriginalIrp,
                                   ClasspPowerUpCompletion,
                                   PowerContext,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            status = PoCallDriver(commonExtension->LowerDeviceObject, OriginalIrp);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n", OriginalIrp, status));
            break;
        }

        case PowerUpDeviceOn: {

            //
            // Original power irp has been completed by lower driver.
            //

            if (NT_SUCCESS(Irp->IoStatus.Status)) {
                //
                // If power irp succeeded, START UNIT command will be sent.
                //
                PCDB cdb;
                ULONG secondsRemaining = 0;
                ULONG timeoutValue = 0;
                ULONG startUnitTimeout;

                if (PoQueryWatchdogTime(fdoExtension->LowerPdo, &secondsRemaining)) {

                    // do not exceed DEFAULT_POWER_IRP_TIMEOUT_VALUE.
                    secondsRemaining = min(secondsRemaining, DEFAULT_POWER_IRP_TIMEOUT_VALUE);

                    //
                    // It's possible for POWER IRP timeout value to be smaller than default of
                    // START_UNIT_TIMEOUT. If this is the case, use a smaller timeout value.
                    //
                    if (secondsRemaining >= START_UNIT_TIMEOUT) {
                        startUnitTimeout = START_UNIT_TIMEOUT;
                    } else {
                        startUnitTimeout = MINIMAL_START_UNIT_TIMEOUT_VALUE;
                    }

                    // plan to leave (TIME_LEFT_FOR_UPPER_DRIVERS) seconds to upper level drivers
                    // for processing original power irp.
                    if (secondsRemaining >= (TIME_LEFT_FOR_UPPER_DRIVERS + startUnitTimeout)) {
                        fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount =
                            (secondsRemaining - TIME_LEFT_FOR_UPPER_DRIVERS) / startUnitTimeout;

                        // * No 'short' timeouts
                        //
                        //
                        // timeoutValue = (secondsRemaining - TIME_LEFT_FOR_UPPER_DRIVERS) %
                        //                startUnitTimeout;
                        //

                        if (--fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount)
                        {
                            timeoutValue = startUnitTimeout;
                        } else {
                            timeoutValue = secondsRemaining - TIME_LEFT_FOR_UPPER_DRIVERS;
                        }
                    } else {
                        // issue the command with minimum timeout value and do not retry on it.
                        // case of (secondsRemaining < DEFAULT_IO_TIMEOUT_VALUE) is ignored as it should not happen.
                        NT_ASSERT(secondsRemaining >= DEFAULT_IO_TIMEOUT_VALUE);

                        fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = 0;
                        timeoutValue = MINIMUM_START_UNIT_TIMEOUT_VALUE; // use the minimum value for this corner case.
                    }

                } else {
                    // don't know how long left, do not exceed DEFAULT_POWER_IRP_TIMEOUT_VALUE.
                    fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount =
                        DEFAULT_POWER_IRP_TIMEOUT_VALUE / START_UNIT_TIMEOUT - 1;
                    timeoutValue = START_UNIT_TIMEOUT;
                }


                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSending start unit to device\n", Irp));

                //
                // Issue the start unit command to the device.
                //

                PowerContext->RetryCount = fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount;

                if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                    status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                                            STORAGE_ADDRESS_TYPE_BTL8,
                                                            CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                            1,
                                                            SrbExDataTypeScsiCdb16);
                    if (NT_SUCCESS(status)) {
                        ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;

                        //
                        // Set length field in Power Context SRB so we know legacy SRB is not being used.
                        //
                        PowerContext->Srb.Length = 0;

                    } else {
                        //
                        // Should not happen. Revert to legacy SRB.
                        //
                        NT_ASSERT(FALSE);
                        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                        RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                        srbHeader->Function = SRB_FUNCTION_EXECUTE_SCSI;
                    }

                } else {
                    RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                    srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                    srbHeader->Function = SRB_FUNCTION_EXECUTE_SCSI;
                }

                SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);
                SrbSetSenseInfoBuffer(srbHeader, commonExtension->PartitionZeroExtension->SenseData);
                SrbSetSenseInfoBufferLength(srbHeader, GET_FDO_EXTENSON_SENSE_DATA_LENGTH(commonExtension->PartitionZeroExtension));

                SrbSetTimeOutValue(srbHeader, timeoutValue);
                SrbAssignSrbFlags(srbHeader,
                                     (SRB_FLAGS_NO_DATA_TRANSFER |
                                      SRB_FLAGS_DISABLE_AUTOSENSE |
                                      SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                      SRB_FLAGS_NO_QUEUE_FREEZE));

                if (PowerContext->Options.LockQueue) {
                    SrbSetSrbFlags(srbHeader, SRB_FLAGS_BYPASS_LOCKED_QUEUE);
                }

                SrbSetCdbLength(srbHeader, 6);

                cdb = SrbGetCdb(srbHeader);
                RtlZeroMemory(cdb, sizeof(CDB));

                cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
                cdb->START_STOP.Start = 1;

                PowerContext->PowerChangeState.PowerUp = PowerUpDeviceOn;

                IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                                       ClasspPowerUpCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                nextStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
                nextStack->MajorFunction = IRP_MJ_SCSI;

                status = IoCallDriver(commonExtension->LowerDeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp);

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n", fdoExtension->PrivateFdoData->PowerProcessIrp, status));

            } else {

                //
                // power irp is failed by lower driver. we're done.
                //

                PowerContext->FinalStatus = Irp->IoStatus.Status;
                goto ClasspPowerUpCompletionFailure;
            }

            break;
        }

        case PowerUpDeviceStarted: { // 3

            //
            // First deal with an error if one occurred.
            //

            if (SRB_STATUS(srbHeader->SrbStatus) != SRB_STATUS_SUCCESS) {

                BOOLEAN retry;
                LONGLONG delta100nsUnits = 0;
                ULONG secondsRemaining = 0;
                ULONG startUnitTimeout = START_UNIT_TIMEOUT;

                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_POWER, "%p\tError occured when issuing START_UNIT "
                            "command to device. Srb %p, Status %x\n",
                            Irp,
                            srbHeader,
                            srbHeader->SrbStatus));

                NT_ASSERT(!(TEST_FLAG(srbHeader->SrbStatus, SRB_STATUS_QUEUE_FROZEN)));
                NT_ASSERT((srbHeader->Function == SRB_FUNCTION_EXECUTE_SCSI) ||
                          (((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction == SRB_FUNCTION_EXECUTE_SCSI));

                PowerContext->RetryInterval = 0;
                retry = InterpretSenseInfoWithoutHistory(
                            fdoExtension->DeviceObject,
                            Irp,
                            (PSCSI_REQUEST_BLOCK)srbHeader,
                            IRP_MJ_SCSI,
                            IRP_MJ_POWER,
                            fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount - PowerContext->RetryCount,
                            &status,
                            &delta100nsUnits);

                // NOTE: Power context is a public structure, and thus cannot be
                //       updated to use 100ns units.  Therefore, must store the
                //       one-second equivalent.  Round up to ensure minimum delay
                //       requirements have been met.
                delta100nsUnits += (10*1000*1000) - 1;
                delta100nsUnits /= (10*1000*1000);
                // guaranteed not to have high bits set per SAL annotations
                PowerContext->RetryInterval = (ULONG)(delta100nsUnits);


                if ((retry == TRUE) && (PowerContext->RetryCount-- != 0)) {

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tRetrying failed request\n", Irp));

                    //
                    // Decrement the state so we come back through here the
                    // next time.
                    //

                    PowerContext->PowerChangeState.PowerUp--;

                    //
                    // Adjust start unit timeout based on remaining time if needed.
                    //
                    if (PoQueryWatchdogTime(fdoExtension->LowerPdo, &secondsRemaining)) {

                        if (secondsRemaining >= TIME_LEFT_FOR_UPPER_DRIVERS) {
                            secondsRemaining -= TIME_LEFT_FOR_UPPER_DRIVERS;
                        }

                        if (secondsRemaining < MINIMAL_START_UNIT_TIMEOUT_VALUE) {
                            startUnitTimeout = MINIMUM_START_UNIT_TIMEOUT_VALUE;
                        } else if (secondsRemaining < START_UNIT_TIMEOUT) {
                            startUnitTimeout = MINIMAL_START_UNIT_TIMEOUT_VALUE;
                        }
                    }

                    SrbSetTimeOutValue(srbHeader, startUnitTimeout);

                    RetryPowerRequest(commonExtension->DeviceObject,
                                      Irp,
                                      PowerContext);

                    break;

                }

                // reset retry count for UNLOCK command.
                fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
                PowerContext->RetryCount = MAXIMUM_RETRIES;
            }

ClasspPowerUpCompletionFailure:

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously spun device up\n", Irp));

            if (PowerContext->QueueLocked) {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tUnlocking queue\n", Irp));

                if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                    //
                    // Will reuse SRB for a non-SCSI SRB.
                    //
                    status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                                           STORAGE_ADDRESS_TYPE_BTL8,
                                                           CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE,
                                                           0);
                    if (NT_SUCCESS(status)) {
                        ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_UNLOCK_QUEUE;

                        //
                        // Set length field in Power Context SRB so we know legacy SRB is not being used.
                        //
                        PowerContext->Srb.Length = 0;

                    } else {
                        //
                        // Should not occur. Revert to legacy SRB.
                        NT_ASSERT(FALSE);
                        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                        RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                        srbHeader->Function = SRB_FUNCTION_UNLOCK_QUEUE;
                    }
                } else {
                    RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                    srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                    srbHeader->Function = SRB_FUNCTION_UNLOCK_QUEUE;
                }
                SrbAssignSrbFlags(srbHeader, SRB_FLAGS_BYPASS_LOCKED_QUEUE);
                SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);

                nextStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
                nextStack->MajorFunction = IRP_MJ_SCSI;

                PowerContext->PowerChangeState.PowerUp = PowerUpDeviceStarted;

                IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                                       ClasspPowerUpCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = IoCallDriver(commonExtension->LowerDeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp);
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n",
                            fdoExtension->PrivateFdoData->PowerProcessIrp, status));
                break;
            }

            // Fall-through to next case...

        }

        case PowerUpDeviceUnlocked: {

            //
            // This is the end of the dance.
            // We're ignoring possible intermediate error conditions ....
            //

            if (PowerContext->QueueLocked) {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously unlocked queue\n", OriginalIrp));

                //
                // If the lower device is being removed, the IRP's status may be STATUS_DELETE_PENDING or
                // STATUS_DEVICE_DOES_NOT_EXIST.
                //
                if((NT_SUCCESS(Irp->IoStatus.Status) == FALSE) &&
                   (Irp->IoStatus.Status != STATUS_DELETE_PENDING) &&
                   (Irp->IoStatus.Status != STATUS_DEVICE_DOES_NOT_EXIST)) {


                    NT_ASSERT(FALSE);
                }

            } else {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tFall-through (queue not locked)\n", OriginalIrp));
            }

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tFreeing srb and completing\n", OriginalIrp));

            status = PowerContext->FinalStatus;
            OriginalIrp->IoStatus.Status = status;

            //
            // Set the new power state
            //

            if (NT_SUCCESS(status)) {
                fdoExtension->DevicePowerState = currentStack->Parameters.Power.State.DeviceState;
            }

            //
            // Check whether failure detection is enabled
            //

            if ((fdoExtension->FailurePredictionInfo != NULL) &&
                (fdoExtension->FailurePredictionInfo->Method != FailurePredictionNone)) {
                 FailurePredictionEnabled = TRUE;
            }

            //
            // Enable tick timer at end of D0 processing if it was previously enabled.
            //

            if ((commonExtension->DriverExtension->InitData.ClassTick != NULL) ||
                ((fdoExtension->MediaChangeDetectionInfo != NULL) &&
                 (fdoExtension->FunctionSupportInfo != NULL) &&
                 (fdoExtension->FunctionSupportInfo->AsynchronousNotificationSupported == FALSE)) ||
                (FailurePredictionEnabled)) {


                //
                // If failure prediction is turned on and we've been powered
                // off longer than the failure prediction query period then
                // force the query on the next timer tick.
                //

                if ((FailurePredictionEnabled) && (ClasspFailurePredictionPeriodMissed(fdoExtension))) {
                     fdoExtension->FailurePredictionInfo->CountDown = 1;
                }

                //
                // Finally, enable the timer.
                //

                ClasspEnableTimer(fdoExtension);
            }

            //
            // Indicate to Po that we've been successfully powered up so
            // it can do it's notification stuff.
            //

            PoSetPowerState(PowerContext->DeviceObject,
                            currentStack->Parameters.Power.Type,
                            currentStack->Parameters.Power.State);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tStarting next power irp\n", OriginalIrp));

            ClassReleaseRemoveLock(PowerContext->DeviceObject, OriginalIrp);

            PowerContext->InUse = FALSE;

            PoStartNextPowerIrp(OriginalIrp);

            // prevent from completing the irp allocated by ourselves
            if ((fdoExtension->PrivateFdoData) && (Irp == fdoExtension->PrivateFdoData->PowerProcessIrp)) {
                // complete original irp if we are processing powerprocess irp,
                // otherwise, by returning status other than STATUS_MORE_PROCESSING_REQUIRED, IO manager will complete it.
                ClassCompleteRequest(commonExtension->DeviceObject, OriginalIrp, IO_NO_INCREMENT);
                status = STATUS_MORE_PROCESSING_REQUIRED;
            }

            return status;
        }
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
} // end ClasspPowerUpCompletion()

/*++////////////////////////////////////////////////////////////////////////////

ClasspPowerDownCompletion()

Routine Description:

    This routine is used for intermediate completion of a power down request.
    PowerDown performs the following sequence to power down the device.

        1. The queue(s) in the lower stack is/are "power locked" to ensure new
           requests are held until the power-down process is complete.

        2. A request to the lower layers to wait for all outstanding IO to
           complete ("quiescence") is sent.  This ensures we don't power down
           the device while it's in the middle of handling IO.

        3. A request to flush the device's cache is sent.  The device may lose
           power when we forward the D-IRP so any data in volatile storage must
           be committed to non-volatile storage first.

        4. A "stop unit" request is sent to the device to notify it that it
           is about to be powered down.

        5. The D-IRP is forwarded down the stack.  If D3Cold is supported and
           enabled via ACPI, the ACPI filter driver may power off the device.

        6. Once the D-IRP is completed by the lower stack, we will "power
           unlock" the queue(s).  (It is the lower stack's responsibility to
           continue to queue any IO that requires hardware access until the
           device is powered up again.)

Arguments:

    DeviceObject - the device object being powered down

    Irp - the IO_REQUEST_PACKET containing the power request

    Context - the class power context used to perform port/class operations.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED or
    STATUS_SUCCESS

--*/
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspPowerDownCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PCLASS_POWER_CONTEXT PowerContext = (PCLASS_POWER_CONTEXT)Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = PowerContext->DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = PowerContext->DeviceObject->DeviceExtension;
    PIRP OriginalIrp = PowerContext->Irp;

    // currentStack is for original power irp
    // nextStack is for power process irp
    PIO_STACK_LOCATION currentStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(fdoExtension->PrivateFdoData->PowerProcessIrp);

    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;
    PSTORAGE_REQUEST_BLOCK_HEADER srbHeader;
    ULONG srbFlags;

    UNREFERENCED_PARAMETER(DeviceObject);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "ClasspPowerDownCompletion: Device Object %p, "
                   "Irp %p, Context %p\n",
                PowerContext->DeviceObject, Irp, Context));

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->PrivateFdoData->PowerSrb.SrbEx);

        //
        // Check if reverted to using legacy SRB.
        //
        if (PowerContext->Srb.Length == sizeof(SCSI_REQUEST_BLOCK)) {
            srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
        }
    } else {
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
    }

    srbFlags = SrbGetSrbFlags(srbHeader);
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
    NT_ASSERT(PowerContext->Options.PowerDown == TRUE);
    NT_ASSERT(PowerContext->Options.HandleSpinDown);

    if ((Irp == OriginalIrp) && (Irp->PendingReturned)) {
        // only for original power irp
        IoMarkIrpPending(Irp);
    }

    PowerContext->PowerChangeState.PowerDown3++;

    switch(PowerContext->PowerChangeState.PowerDown3) {

        case PowerDownDeviceLocked3: {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously sent power lock\n", Irp));

            if ((PowerContext->Options.LockQueue == TRUE) &&
                (!NT_SUCCESS(Irp->IoStatus.Status))) {

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIrp status was %lx\n",
                            Irp,
                            Irp->IoStatus.Status));
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSrb status was %lx\n",
                            Irp,
                            srbHeader->SrbStatus));



                //
                // Lock was not successful - throw down the power IRP
                // by itself and don't try to spin down the drive or unlock
                // the queue.
                //

                //
                // Set the new power state
                //

                fdoExtension->DevicePowerState =
                    currentStack->Parameters.Power.State.DeviceState;

                //
                // Indicate to Po that we've been successfully powered down
                // so it can do it's notification stuff.
                //

                IoCopyCurrentIrpStackLocationToNext(OriginalIrp);
                IoSetCompletionRoutine(OriginalIrp,
                                       ClasspStartNextPowerIrpCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                PoSetPowerState(PowerContext->DeviceObject,
                                currentStack->Parameters.Power.Type,
                                currentStack->Parameters.Power.State);

                fdoExtension->PowerDownInProgress = FALSE;

                ClassReleaseRemoveLock(commonExtension->DeviceObject,
                                       OriginalIrp);

                PoCallDriver(commonExtension->LowerDeviceObject, OriginalIrp);

                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {
                //
                // Lock the device queue succeeded. Now wait for all outstanding IO to complete.
                // To do this, Srb with SRB_FUNCTION_QUIESCE_DEVICE will be sent down with default timeout value.
                // We need to tolerant failure of this request, no retry will be made.
                //
                PowerContext->QueueLocked = (UCHAR) PowerContext->Options.LockQueue;

                //
                // No retry on device quiescence reqeust
                //
                fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = 0;
                PowerContext->RetryCount = 0;

                if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                    srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->PrivateFdoData->PowerSrb.SrbEx);

                    //
                    // Initialize extended SRB for a SRB_FUNCTION_LOCK_QUEUE
                    //
                    status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                                           STORAGE_ADDRESS_TYPE_BTL8,
                                                           CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE,
                                                           0);
                    if (NT_SUCCESS(status)) {
                        ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_QUIESCE_DEVICE;
                    } else {
                        //
                        // Should not happen. Revert to legacy SRB.
                        //
                        NT_ASSERT(FALSE);
                        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                        srbHeader->Function = SRB_FUNCTION_QUIESCE_DEVICE;
                    }
                } else {
                    srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                    srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                    srbHeader->Function = SRB_FUNCTION_QUIESCE_DEVICE;
                }

                SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);
                SrbSetTimeOutValue(srbHeader, fdoExtension->TimeOutValue);

                SrbAssignSrbFlags(srbHeader,
                                     (SRB_FLAGS_NO_DATA_TRANSFER |
                                      SRB_FLAGS_DISABLE_AUTOSENSE |
                                      SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                      SRB_FLAGS_NO_QUEUE_FREEZE |
                                      SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                      SRB_FLAGS_D3_PROCESSING));

                IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                                       ClasspPowerDownCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                nextStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
                nextStack->MajorFunction = IRP_MJ_SCSI;

                status = IoCallDriver(commonExtension->LowerDeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp);

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n", fdoExtension->PrivateFdoData->PowerProcessIrp, status));
                break;
            }

        }

        case PowerDownDeviceQuiesced3: {

            PCDB cdb;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously sent device quiesce\n", Irp));

            //
            // don't care the result of device quiesce, we've made the effort.
            // continue on sending other SCSI commands anyway.
            //


            if (!TEST_FLAG(fdoExtension->PrivateFdoData->HackFlags,
                           FDO_HACK_NO_SYNC_CACHE)) {

                //
                // send SCSIOP_SYNCHRONIZE_CACHE
                //

                fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
                PowerContext->RetryCount = MAXIMUM_RETRIES;

                if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                    status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                                            STORAGE_ADDRESS_TYPE_BTL8,
                                                            CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                            1,
                                                            SrbExDataTypeScsiCdb16);
                    if (NT_SUCCESS(status)) {
                        ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;

                        //
                        // Set length field in Power Context SRB so we know legacy SRB is not being used.
                        //
                        PowerContext->Srb.Length = 0;

                    } else {
                        //
                        // Should not occur. Revert to legacy SRB.
                        NT_ASSERT(FALSE);
                        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                        RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                        srbHeader->Function = SRB_FUNCTION_EXECUTE_SCSI;
                    }

                } else {
                    RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                    srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                    srbHeader->Function = SRB_FUNCTION_EXECUTE_SCSI;
                }


                SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);
                SrbSetSenseInfoBuffer(srbHeader, commonExtension->PartitionZeroExtension->SenseData);
                SrbSetSenseInfoBufferLength(srbHeader, GET_FDO_EXTENSON_SENSE_DATA_LENGTH(commonExtension->PartitionZeroExtension));
                SrbSetTimeOutValue(srbHeader, fdoExtension->TimeOutValue);

                SrbAssignSrbFlags(srbHeader,
                                     (SRB_FLAGS_NO_DATA_TRANSFER |
                                      SRB_FLAGS_DISABLE_AUTOSENSE |
                                      SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                      SRB_FLAGS_NO_QUEUE_FREEZE |
                                      SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                      SRB_FLAGS_D3_PROCESSING));

                SrbSetCdbLength(srbHeader, 10);

                cdb = SrbGetCdb(srbHeader);

                RtlZeroMemory(cdb, sizeof(CDB));
                cdb->SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;

                IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                                       ClasspPowerDownCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                nextStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
                nextStack->MajorFunction = IRP_MJ_SCSI;

                status = IoCallDriver(commonExtension->LowerDeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp);

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n", fdoExtension->PrivateFdoData->PowerProcessIrp, status));
                break;

            } else {

                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_POWER, "(%p)\tPower Down: not sending SYNCH_CACHE\n",
                            PowerContext->DeviceObject));
                PowerContext->PowerChangeState.PowerDown3++;
                srbHeader->SrbStatus = SRB_STATUS_SUCCESS;
                // and fall through....
            }
            // no break in case the device doesn't like synch_cache commands

        }

        case PowerDownDeviceFlushed3: {

            PCDB cdb;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously send SCSIOP_SYNCHRONIZE_CACHE\n",
                        Irp));

            //
            // SCSIOP_SYNCHRONIZE_CACHE was sent
            //

            if (SRB_STATUS(srbHeader->SrbStatus) != SRB_STATUS_SUCCESS) {

                BOOLEAN retry;
                LONGLONG delta100nsUnits = 0;

                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_POWER, "(%p)\tError occured when issuing "
                            "SYNCHRONIZE_CACHE command to device. "
                            "Srb %p, Status %lx\n",
                            Irp,
                            srbHeader,
                            srbHeader->SrbStatus));

                NT_ASSERT(!(TEST_FLAG(srbHeader->SrbStatus, SRB_STATUS_QUEUE_FROZEN)));
                NT_ASSERT((srbHeader->Function == SRB_FUNCTION_EXECUTE_SCSI) ||
                          (((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction == SRB_FUNCTION_EXECUTE_SCSI));

                PowerContext->RetryInterval = 0;
                retry = InterpretSenseInfoWithoutHistory(
                            fdoExtension->DeviceObject,
                            Irp,
                            (PSCSI_REQUEST_BLOCK)srbHeader,
                            IRP_MJ_SCSI,
                            IRP_MJ_POWER,
                            fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount - PowerContext->RetryCount,
                            &status,
                            &delta100nsUnits);

                // NOTE: Power context is a public structure, and thus cannot be
                //       updated to use 100ns units.  Therefore, must store the
                //       one-second equivalent.  Round up to ensure minimum delay
                //       requirements have been met.
                delta100nsUnits += (10*1000*1000) - 1;
                delta100nsUnits /= (10*1000*1000);
                // guaranteed not to have high bits set per SAL annotations
                PowerContext->RetryInterval = (ULONG)(delta100nsUnits);


                if ((retry == TRUE) && (PowerContext->RetryCount-- != 0)) {

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tRetrying failed request\n", Irp));

                    //
                    // decrement the state so we come back through here
                    // the next time.
                    //

                    PowerContext->PowerChangeState.PowerDown3--;
                    RetryPowerRequest(commonExtension->DeviceObject,
                                      Irp,
                                      PowerContext);
                    break;
                }

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSYNCHRONIZE_CACHE not retried\n", Irp));
                fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
                PowerContext->RetryCount = MAXIMUM_RETRIES;
            } // end !SRB_STATUS_SUCCESS

            //
            // note: we are purposefully ignoring any errors.  if the drive
            //       doesn't support a synch_cache, then we're up a creek
            //       anyways.
            //

            if ((currentStack->Parameters.Power.State.DeviceState == PowerDeviceD3) &&
                (currentStack->Parameters.Power.ShutdownType == PowerActionHibernate) &&
                (commonExtension->HibernationPathCount != 0)) {

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPower Down: not sending SPIN DOWN due to hibernation path\n",
                            PowerContext->DeviceObject));

                PowerContext->PowerChangeState.PowerDown3++;
                srbHeader->SrbStatus = SRB_STATUS_SUCCESS;
                status = STATUS_SUCCESS;

                // Fall through to next case...

            } else {
                // Send STOP UNIT command. As "Imme" bit is set to '1', this command should be completed in short time.
                // This command is at low importance, failure of this command has very small impact.

                ULONG secondsRemaining;
                ULONG timeoutValue;

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSending stop unit to device\n", Irp));

                if (PoQueryWatchdogTime(fdoExtension->LowerPdo, &secondsRemaining)) {
                    // plan to leave some time (TIME_LEFT_FOR_LOWER_DRIVERS) to lower level drivers
                    // for processing the original power irp.
                    if (secondsRemaining >= (TIME_LEFT_FOR_LOWER_DRIVERS + DEFAULT_IO_TIMEOUT_VALUE)) {
                        fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount =
                            (secondsRemaining - TIME_LEFT_FOR_LOWER_DRIVERS) / DEFAULT_IO_TIMEOUT_VALUE;

                        // * No 'short' timeouts
                        //
                        // timeoutValue = (secondsRemaining - TIME_LEFT_FOR_LOWER_DRIVERS) %
                        //                DEFAULT_IO_TIMEOUT_VALUE;
                        // if (timeoutValue < MINIMUM_STOP_UNIT_TIMEOUT_VALUE)
                        // {
                        if (--fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount)
                        {
                            timeoutValue = DEFAULT_IO_TIMEOUT_VALUE;
                        } else {
                            timeoutValue = secondsRemaining - TIME_LEFT_FOR_LOWER_DRIVERS;
                        }
                        // }

                        // Limit to maximum retry count.
                        if (fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount > MAXIMUM_RETRIES) {
                            fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
                        }
                    } else {
                        // issue the command with minimum timeout value and do not retry on it.
                        fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = 0;

                        // minimum as MINIMUM_STOP_UNIT_TIMEOUT_VALUE.
                        if (secondsRemaining > 2 * MINIMUM_STOP_UNIT_TIMEOUT_VALUE) {
                            timeoutValue = secondsRemaining - MINIMUM_STOP_UNIT_TIMEOUT_VALUE;
                        } else {
                            timeoutValue = MINIMUM_STOP_UNIT_TIMEOUT_VALUE;
                        }

                    }

                } else {
                    // do not know how long, use default values.
                    fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
                    timeoutValue = DEFAULT_IO_TIMEOUT_VALUE;
                }

                //
                // Issue STOP UNIT command to the device.
                //

                PowerContext->RetryCount = fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount;

                if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                    status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                                            STORAGE_ADDRESS_TYPE_BTL8,
                                                            CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                            1,
                                                            SrbExDataTypeScsiCdb16);
                    if (NT_SUCCESS(status)) {
                        ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;

                        //
                        // Set length field in Power Context SRB so we know legacy SRB is not being used.
                        //
                        PowerContext->Srb.Length = 0;

                    } else {
                        //
                        // Should not occur. Revert to legacy SRB.
                        //
                        NT_ASSERT(FALSE);
                        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                        RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                        srbHeader->Function = SRB_FUNCTION_EXECUTE_SCSI;
                    }

                } else {
                    RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                    srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                    srbHeader->Function = SRB_FUNCTION_EXECUTE_SCSI;
                }

                SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);
                SrbSetSenseInfoBuffer(srbHeader, commonExtension->PartitionZeroExtension->SenseData);
                SrbSetSenseInfoBufferLength(srbHeader, GET_FDO_EXTENSON_SENSE_DATA_LENGTH(commonExtension->PartitionZeroExtension));
                SrbSetTimeOutValue(srbHeader, timeoutValue);


                SrbAssignSrbFlags(srbHeader,
                                     (SRB_FLAGS_NO_DATA_TRANSFER |
                                      SRB_FLAGS_DISABLE_AUTOSENSE |
                                      SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                      SRB_FLAGS_NO_QUEUE_FREEZE |
                                      SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                      SRB_FLAGS_D3_PROCESSING));

                SrbSetCdbLength(srbHeader, 6);

                cdb = SrbGetCdb(srbHeader);
                RtlZeroMemory(cdb, sizeof(CDB));

                cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
                cdb->START_STOP.Start = 0;
                cdb->START_STOP.Immediate = 1;

                IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                                       ClasspPowerDownCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                nextStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
                nextStack->MajorFunction = IRP_MJ_SCSI;

                status = IoCallDriver(commonExtension->LowerDeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp);

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n", fdoExtension->PrivateFdoData->PowerProcessIrp, status));
                break;
            }
        }

        case PowerDownDeviceStopped3: {

            BOOLEAN ignoreError = TRUE;

            //
            // stop was sent
            //

            if (SRB_STATUS(srbHeader->SrbStatus) != SRB_STATUS_SUCCESS) {

                BOOLEAN retry;
                LONGLONG delta100nsUnits = 0;

                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_POWER, "(%p)\tError occured when issueing STOP_UNIT "
                            "command to device. Srb %p, Status %lx\n",
                            Irp,
                            srbHeader,
                            srbHeader->SrbStatus));

                NT_ASSERT(!(TEST_FLAG(srbHeader->SrbStatus, SRB_STATUS_QUEUE_FROZEN)));
                NT_ASSERT((srbHeader->Function == SRB_FUNCTION_EXECUTE_SCSI) ||
                          (((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction == SRB_FUNCTION_EXECUTE_SCSI));

                PowerContext->RetryInterval = 0;
                retry = InterpretSenseInfoWithoutHistory(
                            fdoExtension->DeviceObject,
                            Irp,
                            (PSCSI_REQUEST_BLOCK)srbHeader,
                            IRP_MJ_SCSI,
                            IRP_MJ_POWER,
                            fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount - PowerContext->RetryCount,
                            &status,
                            &delta100nsUnits);

                // NOTE: Power context is a public structure, and thus cannot be
                //       updated to use 100ns units.  Therefore, must store the
                //       one-second equivalent.  Round up to ensure minimum delay
                //       requirements have been met.
                delta100nsUnits += (10*1000*1000) - 1;
                delta100nsUnits /= (10*1000*1000);
                // guaranteed not to have high bits set per SAL annotations
                PowerContext->RetryInterval = (ULONG)(delta100nsUnits);


                if ((retry == TRUE) && (PowerContext->RetryCount-- != 0)) {

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tRetrying failed request\n", Irp));

                    //
                    // decrement the state so we come back through here
                    // the next time.
                    //

                    PowerContext->PowerChangeState.PowerDown3--;

                    SrbSetTimeOutValue(srbHeader, DEFAULT_IO_TIMEOUT_VALUE);

                    RetryPowerRequest(commonExtension->DeviceObject,
                                      Irp,
                                      PowerContext);
                    break;
                }

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSTOP_UNIT not retried\n", Irp));
                fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
                PowerContext->RetryCount = MAXIMUM_RETRIES;

            } // end !SRB_STATUS_SUCCESS


            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously sent stop unit\n", Irp));

            //
            // some operations, such as a physical format in progress,
            // should not be ignored and should fail the power operation.
            //

            if (!NT_SUCCESS(status)) {

                PVOID senseBuffer = SrbGetSenseInfoBuffer(srbHeader);

                if (TEST_FLAG(srbHeader->SrbStatus, SRB_STATUS_AUTOSENSE_VALID) &&
                    (senseBuffer != NULL)) {

                    BOOLEAN validSense = FALSE;
                    UCHAR senseKey = 0;
                    UCHAR additionalSenseCode = 0;
                    UCHAR additionalSenseCodeQualifier = 0;

                    validSense = ScsiGetSenseKeyAndCodes(senseBuffer,
                                                         SrbGetSenseInfoBufferLength(srbHeader),
                                                         SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED,
                                                         &senseKey,
                                                         &additionalSenseCode,
                                                         &additionalSenseCodeQualifier);

                    if (validSense) {
                        if ((senseKey == SCSI_SENSE_NOT_READY) &&
                            (additionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
                            (additionalSenseCodeQualifier == SCSI_SENSEQ_FORMAT_IN_PROGRESS)) {

                            ignoreError = FALSE;
                            PowerContext->FinalStatus = STATUS_DEVICE_BUSY;
                            status = PowerContext->FinalStatus;
                        }
                    }
                }
            }

            if (NT_SUCCESS(status) || ignoreError) {

                //
                // Issue the original power request to the lower driver.
                //

                IoCopyCurrentIrpStackLocationToNext(OriginalIrp);

                IoSetCompletionRoutine(OriginalIrp,
                                       ClasspPowerDownCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = PoCallDriver(commonExtension->LowerDeviceObject, OriginalIrp);

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPoCallDriver returned %lx\n", OriginalIrp, status));
                break;
            }

            // else fall through w/o sending the power irp, since the device
            // is reporting an error that would be "really bad" to power down
            // during.

        }

        case PowerDownDeviceOff3: {

            //
            // SpinDown request completed ... whether it succeeded or not is
            // another matter entirely.
            //

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously sent power irp\n", OriginalIrp));

            if (PowerContext->QueueLocked) {

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tUnlocking queue\n", OriginalIrp));

                if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                    //
                    // Will reuse SRB for a non-SCSI SRB.
                    //
                    status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                                           STORAGE_ADDRESS_TYPE_BTL8,
                                                           CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE,
                                                           0);
                    if (NT_SUCCESS(status)) {
                        ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_UNLOCK_QUEUE;

                        //
                        // Set length field in Power Context SRB so we know legacy SRB is not being used.
                        //
                        PowerContext->Srb.Length = 0;

                    } else {
                        //
                        // Should not occur. Revert to legacy SRB.
                        //
                        NT_ASSERT(FALSE);
                        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(PowerContext->Srb);
                        RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                        srbHeader->Function = SRB_FUNCTION_UNLOCK_QUEUE;
                    }
                } else {
                    RtlZeroMemory(srbHeader, sizeof(SCSI_REQUEST_BLOCK));
                    srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
                    srbHeader->Function = SRB_FUNCTION_UNLOCK_QUEUE;
                }

                SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);
                SrbAssignSrbFlags(srbHeader, (SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                              SRB_FLAGS_D3_PROCESSING));

                nextStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
                nextStack->MajorFunction = IRP_MJ_SCSI;

                IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                                       ClasspPowerDownCompletion,
                                       PowerContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = IoCallDriver(commonExtension->LowerDeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp);
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIoCallDriver returned %lx\n",
                            fdoExtension->PrivateFdoData->PowerProcessIrp,
                            status));
                break;
            }

        }

        case PowerDownDeviceUnlocked3: {

            //
            // This is the end of the dance.
            // We're ignoring possible intermediate error conditions ....
            //

            if (PowerContext->QueueLocked == FALSE) {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tFall through (queue not locked)\n", OriginalIrp));
            } else {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPreviously unlocked queue\n", OriginalIrp));
                NT_ASSERT(NT_SUCCESS(Irp->IoStatus.Status));
                NT_ASSERT(srbHeader->SrbStatus == SRB_STATUS_SUCCESS);

                if (NT_SUCCESS(Irp->IoStatus.Status)) {
                    PowerContext->QueueLocked = FALSE;
                }
            }

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tFreeing srb and completing\n", OriginalIrp));
            status = PowerContext->FinalStatus; // allow failure to propogate

            OriginalIrp->IoStatus.Status = status;
            OriginalIrp->IoStatus.Information = 0;

            if (NT_SUCCESS(status)) {

                //
                // Set the new power state
                //

                fdoExtension->DevicePowerState =
                    currentStack->Parameters.Power.State.DeviceState;

            }

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tStarting next power irp\n", OriginalIrp));

            ClassReleaseRemoveLock(PowerContext->DeviceObject, OriginalIrp);

            PowerContext->InUse = FALSE;

            PoStartNextPowerIrp(OriginalIrp);

            fdoExtension->PowerDownInProgress = FALSE;

            // prevent from completing the irp allocated by ourselves
            if (Irp == fdoExtension->PrivateFdoData->PowerProcessIrp) {
                // complete original irp if we are processing powerprocess irp,
                // otherwise, by returning status other than STATUS_MORE_PROCESSING_REQUIRED, IO manager will complete it.
                ClassCompleteRequest(commonExtension->DeviceObject, OriginalIrp, IO_NO_INCREMENT);
                status = STATUS_MORE_PROCESSING_REQUIRED;
            }

            return status;
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
    PSTORAGE_REQUEST_BLOCK_HEADER srbHeader;
    ULONG srbFlags;
    NTSTATUS status;

    _Analysis_assume_(fdoExtension);
    _Analysis_assume_(fdoExtension->PrivateFdoData);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "ClasspPowerHandler: Power irp %p to %s %p\n",
                Irp, (commonExtension->IsFdo ? "fdo" : "pdo"), DeviceObject));

    if (!commonExtension->IsFdo) {

        //
        // certain assumptions are made here,
        // particularly: having the fdoExtension
        //

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_POWER, "ClasspPowerHandler: Called for PDO %p???\n",
                    DeviceObject));
        NT_ASSERT(!"PDO using ClasspPowerHandler");

        ClassReleaseRemoveLock(DeviceObject, Irp);
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        PoStartNextPowerIrp(Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }

    switch (irpStack->MinorFunction) {

        case IRP_MN_SET_POWER: {
            PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIRP_MN_SET_POWER\n", Irp));

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSetting %s state to %d\n",
                        Irp,
                        (irpStack->Parameters.Power.Type == SystemPowerState ?
                            "System" : "Device"),
                        irpStack->Parameters.Power.State.SystemState));

            switch (irpStack->Parameters.Power.ShutdownType){

                case PowerActionNone:

                    //
                    // Skip if device doesn't need volume verification during idle power
                    // transitions.
                    //
                    if ((fdoExtension->FunctionSupportInfo) &&
                        (fdoExtension->FunctionSupportInfo->IdlePower.NoVerifyDuringIdlePower)) {
                        break;
                    }

                case PowerActionSleep:
                case PowerActionHibernate:
                    if (fdoData->HotplugInfo.MediaRemovable || fdoData->HotplugInfo.MediaHotplug) {
                        /*
                            *  We are suspending device and this drive is either hot-pluggable
                            *  or contains removeable media.
                            *  Set the media dirty bit, since the media may change while
                            *  we are suspended.
                            */
                        SET_FLAG(DeviceObject->Flags, DO_VERIFY_VOLUME);

                        //
                        // Bumping the media  change count  will force the
                        // file system to verify the volume when we resume
                        //

                        InterlockedIncrement((volatile LONG *)&fdoExtension->MediaChangeCount);
                    }

                    break;
                }

            break;
        }

        default: {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tIrp minor code = %#x\n",
                        Irp, irpStack->MinorFunction));
            break;
        }
    }

    if (irpStack->Parameters.Power.Type != DevicePowerState ||
        irpStack->MinorFunction != IRP_MN_SET_POWER) {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tSending to lower device\n", Irp));

        goto ClasspPowerHandlerCleanup;

    }

    //
    // already in exact same state, don't work to transition to it.
    //

    if (irpStack->Parameters.Power.State.DeviceState ==
        fdoExtension->DevicePowerState) {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tAlready in device state %x\n",
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
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tAlready powered down to %x???\n",
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

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tNot handling spinup to state %x\n",
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

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tNot handling spindown to state %x\n",
                    Irp, fdoExtension->DevicePowerState));
        fdoExtension->DevicePowerState =
            irpStack->Parameters.Power.State.DeviceState;
        goto ClasspPowerHandlerCleanup;

    }

    //
    // validation completed, start the real work.
    //

    IoReuseIrp(fdoExtension->PrivateFdoData->PowerProcessIrp, STATUS_SUCCESS);
    IoSetNextIrpStackLocation(fdoExtension->PrivateFdoData->PowerProcessIrp);
    nextIrpStack = IoGetNextIrpStackLocation(fdoExtension->PrivateFdoData->PowerProcessIrp);

    context = &(fdoExtension->PowerContext);

    NT_ASSERT(context->InUse == FALSE);

    RtlZeroMemory(context, sizeof(CLASS_POWER_CONTEXT));
    context->InUse = TRUE;

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->PrivateFdoData->PowerSrb.SrbEx);

        //
        // Initialize extended SRB for a SRB_FUNCTION_LOCK_QUEUE
        //
        status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbHeader,
                                               STORAGE_ADDRESS_TYPE_BTL8,
                                               CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE,
                                               0);
        if (NT_SUCCESS(status)) {
            ((PSTORAGE_REQUEST_BLOCK)srbHeader)->SrbFunction = SRB_FUNCTION_LOCK_QUEUE;
        } else {
            //
            // Should not happen. Revert to legacy SRB.
            //
            NT_ASSERT(FALSE);
            srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(context->Srb);
            srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
            srbHeader->Function = SRB_FUNCTION_LOCK_QUEUE;
        }
    } else {
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&(context->Srb);
        srbHeader->Length = sizeof(SCSI_REQUEST_BLOCK);
        srbHeader->Function = SRB_FUNCTION_LOCK_QUEUE;
    }
    nextIrpStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srbHeader;
    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    context->FinalStatus = STATUS_SUCCESS;

    SrbSetOriginalRequest(srbHeader, fdoExtension->PrivateFdoData->PowerProcessIrp);
    SrbSetSrbFlags(srbHeader, (SRB_FLAGS_BYPASS_LOCKED_QUEUE | SRB_FLAGS_NO_QUEUE_FREEZE));

    fdoExtension->PrivateFdoData->MaxPowerOperationRetryCount = MAXIMUM_RETRIES;
    context->RetryCount = MAXIMUM_RETRIES;

    context->Options = Options;
    context->DeviceObject = DeviceObject;
    context->Irp = Irp;

    if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0) {

        NT_ASSERT(Options.HandleSpinUp);

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tpower up - locking queue\n", Irp));

        //
        // We need to issue a queue lock request so that we
        // can spin the drive back up after the power is restored
        // but before any requests are processed.
        //

        context->Options.PowerDown = FALSE;
        context->PowerChangeState.PowerUp = PowerUpDeviceInitial;
        context->CompletionRoutine = ClasspPowerUpCompletion;

    } else {

        NT_ASSERT(Options.HandleSpinDown);

        fdoExtension->PowerDownInProgress = TRUE;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tPowering down - locking queue\n", Irp));

        //
        // Disable tick timer at beginning of D3 processing if running.
        //
        if ((fdoExtension->PrivateFdoData->TickTimerEnabled)) {
            ClasspDisableTimer(fdoExtension);
        }

        PoSetPowerState(DeviceObject,
                        irpStack->Parameters.Power.Type,
                        irpStack->Parameters.Power.State);

        context->Options.PowerDown = TRUE;
        context->PowerChangeState.PowerDown3 = PowerDownDeviceInitial3;
        context->CompletionRoutine = ClasspPowerDownCompletion;

    }

    //
    // we are not dealing with port-allocated sense in these routines.
    //

    srbFlags = SrbGetSrbFlags(srbHeader);
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));

    //
    // Mark the original power irp pending.
    //

    IoMarkIrpPending(Irp);

    if (Options.LockQueue) {

        //
        // Send the lock irp down.
        //

        IoSetCompletionRoutine(fdoExtension->PrivateFdoData->PowerProcessIrp,
                               context->CompletionRoutine,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);

        IoCallDriver(lowerDevice, fdoExtension->PrivateFdoData->PowerProcessIrp);

    } else {

        //
        // Call the completion routine directly.  It won't care what the
        // status of the "lock" was - it will just go and do the next
        // step of the operation.
        //

        context->CompletionRoutine(DeviceObject, fdoExtension->PrivateFdoData->PowerProcessIrp, context);
    }

    return STATUS_PENDING;

ClasspPowerHandlerCleanup:

    //
    // Send the original power irp down, we will start the next power irp in completion routine.
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tStarting next power irp\n", Irp));
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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

    switch (irpStack->MinorFunction)
    {
        case IRP_MN_SET_POWER:
        {
            switch (irpStack->Parameters.Power.ShutdownType)
            {
                case PowerActionNone:
                case PowerActionSleep:
                case PowerActionHibernate:
                {
                    if (TEST_FLAG(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA))
                    {
                        if ((ClassGetVpb(DeviceObject) != NULL) && (ClassGetVpb(DeviceObject)->Flags & VPB_MOUNTED))
                        {
                            //
                            // This flag will cause the filesystem to verify the
                            // volume when coming out of hibernation or standby or runtime power
                            //
                            SET_FLAG(DeviceObject->Flags, DO_VERIFY_VOLUME);
                        }
                    }
                }
                break;
            }
        }

        //
        // Fall through
        //

        case IRP_MN_QUERY_POWER:
        {
            if (!commonExtension->IsFdo)
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;
            }
        }
        break;
    }

    if (commonExtension->IsFdo)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = PoCallDriver(commonExtension->LowerDeviceObject, Irp);
    }
    else
    {
        status = Irp->IoStatus.Status;
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    }

    return status;
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
__control_entrypoint(DeviceDriver)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSpinDownPowerHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    CLASS_POWER_OPTIONS options = {0};

    fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

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

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "ClasspPowerHandler: Devobj %p\n"
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassStopUnitPowerHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;

    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_POWER, "ClassStopUnitPowerHandler - Devobj %p using outdated call\n"
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

    This routine reinitalizes the necessary fields, and sends the request
    to the lower driver.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - Supplies the request to be retried.

    Context - Supplies a pointer to the power up context for this request.

Return Value:

    None

--*/
VOID
RetryPowerRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PCLASS_POWER_CONTEXT Context
    )
{
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
        (PFUNCTIONAL_DEVICE_EXTENSION)Context->DeviceObject->DeviceExtension;
    PSTORAGE_REQUEST_BLOCK_HEADER srb;
    LONGLONG dueTime;
    ULONG srbFlags;
    ULONG srbFunction;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tDelaying retry by queueing DPC\n", Irp));

    //NT_ASSERT(Context->Irp == Irp);
    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srb = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->PrivateFdoData->PowerSrb.SrbEx);

        //
        // Check if reverted to using legacy SRB.
        //
        if (Context->Srb.Length == sizeof(SCSI_REQUEST_BLOCK)) {
            srb = (PSTORAGE_REQUEST_BLOCK_HEADER)&(Context->Srb);
            srbFunction = srb->Function;
        } else {
            srbFunction = ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFunction;
        }
    } else {
        srb = (PSTORAGE_REQUEST_BLOCK_HEADER)&(Context->Srb);
        srbFunction = srb->Function;
    }

    NT_ASSERT(Context->DeviceObject == DeviceObject);
    srbFlags = SrbGetSrbFlags(srb);
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));

    if (Context->RetryInterval == 0) {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tDelaying minimum time (.2 sec)\n", Irp));
        dueTime = (LONGLONG)1000000 * 2;

    } else {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER, "(%p)\tDelaying %x seconds\n",
                    Irp, Context->RetryInterval));
        dueTime = (LONGLONG)1000000 * 10 * Context->RetryInterval;

    }

    //
    // reset the retry interval
    //

    Context->RetryInterval = 0;

    //
    // Reset byte count of transfer in SRB Extension.
    //

    SrbSetDataTransferLength(srb, 0);

    //
    // Zero SRB statuses.
    //

    srb->SrbStatus = 0;
    if (srbFunction == SRB_FUNCTION_EXECUTE_SCSI) {
        SrbSetScsiStatus(srb, 0);
    }

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srb;

    //
    // Set the completion routine up again.
    //

    IoSetCompletionRoutine(Irp, Context->CompletionRoutine, Context,
                           TRUE, TRUE, TRUE);

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspStartNextPowerIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PCLASS_POWER_CONTEXT PowerContext = (PCLASS_POWER_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (PowerContext != NULL)
    {
        PowerContext->InUse = FALSE;
    }


    PoStartNextPowerIrp(Irp);
    return STATUS_SUCCESS;
} // end ClasspStartNextPowerIrpCompletion()

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspDeviceLockFailurePowerIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PCLASS_POWER_CONTEXT PowerContext = (PCLASS_POWER_CONTEXT)Context;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PIO_STACK_LOCATION currentStack;
    BOOLEAN FailurePredictionEnabled = FALSE;

    UNREFERENCED_PARAMETER(DeviceObject);

    commonExtension = PowerContext->DeviceObject->DeviceExtension;
    fdoExtension = PowerContext->DeviceObject->DeviceExtension;

    currentStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Set the new power state
    //

    fdoExtension->DevicePowerState = currentStack->Parameters.Power.State.DeviceState;

    //
    // We reach here becasue LockQueue operation was not successful.
    // However, media change detection would not happen in case of resume becasue we
    // had disabled the timer while going into lower power state.
    // So, if the device goes into D0 then enable the tick timer.
    //

    if (fdoExtension->DevicePowerState == PowerDeviceD0) {
        //
        // Check whether failure detection is enabled
        //

        if ((fdoExtension->FailurePredictionInfo != NULL) &&
            (fdoExtension->FailurePredictionInfo->Method != FailurePredictionNone)) {
             FailurePredictionEnabled = TRUE;
        }

        //
        // Enable tick timer at end of D0 processing if it was previously enabled.
        //

        if ((commonExtension->DriverExtension->InitData.ClassTick != NULL) ||
            ((fdoExtension->MediaChangeDetectionInfo != NULL) &&
             (fdoExtension->FunctionSupportInfo != NULL) &&
             (fdoExtension->FunctionSupportInfo->AsynchronousNotificationSupported == FALSE)) ||
            (FailurePredictionEnabled)) {

            //
            // If failure prediction is turned on and we've been powered
            // off longer than the failure prediction query period then
            // force the query on the next timer tick.
            //

            if ((FailurePredictionEnabled) && (ClasspFailurePredictionPeriodMissed(fdoExtension))) {
                 fdoExtension->FailurePredictionInfo->CountDown = 1;
            }

            //
            // Finally, enable the timer.
            //

            ClasspEnableTimer(fdoExtension);
        }
    }

    //
    // Indicate to Po that we've been successfully powered up so
    // it can do it's notification stuff.
    //

    PoSetPowerState(PowerContext->DeviceObject,
                    currentStack->Parameters.Power.Type,
                    currentStack->Parameters.Power.State);

    PowerContext->InUse = FALSE;


    ClassReleaseRemoveLock(commonExtension->DeviceObject, Irp);

    //
    // Start the next power IRP
    //

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    PoStartNextPowerIrp(Irp);

    return STATUS_SUCCESS;
}


_IRQL_requires_same_
NTSTATUS
ClasspSendEnableIdlePowerIoctl(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*++
Description:

    This function is used to send IOCTL_STORAGE_ENABLE_IDLE_POWER to the port
    driver.  It pulls the relevant idle power management properties from the
    FDO's device extension.

Arguments:

    DeviceObject - The class FDO.

Return Value:

    The NTSTATUS code returned from the port driver.  STATUS_SUCCESS indicates
    this device is now enabled for idle (runtime) power management.

--*/
{
    NTSTATUS status;
    STORAGE_IDLE_POWER idlePower = {0};
    IO_STATUS_BLOCK ioStatus = {0};
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = &(fdoExtension->CommonExtension);

    idlePower.Version = 1;
    idlePower.Size = sizeof(STORAGE_IDLE_POWER);
    idlePower.WakeCapableHint = fdoExtension->FunctionSupportInfo->IdlePower.DeviceWakeable;
    idlePower.D3ColdSupported = fdoExtension->FunctionSupportInfo->IdlePower.D3ColdSupported;
    idlePower.D3IdleTimeout = fdoExtension->FunctionSupportInfo->IdlePower.D3IdleTimeout;

    ClassSendDeviceIoControlSynchronous(
        IOCTL_STORAGE_ENABLE_IDLE_POWER,
        commonExtension->LowerDeviceObject,
        &idlePower,
        sizeof(STORAGE_IDLE_POWER),
        0,
        FALSE,
        &ioStatus
        );

    status = ioStatus.Status;

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_POWER,
                "ClasspSendEnableIdlePowerIoctl: Port driver returned status (%x) for FDO (%p)\n"
                "\tWakeCapableHint: %u\n"
                "\tD3ColdSupported: %u\n"
                "\tD3IdleTimeout: %u (ms)",
                status,
                DeviceObject,
                idlePower.WakeCapableHint,
                idlePower.D3ColdSupported,
                idlePower.D3IdleTimeout));

    return status;
}

_Function_class_(POWER_SETTING_CALLBACK)
_IRQL_requires_same_
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspPowerSettingCallback(
    _In_ LPCGUID SettingGuid,
    _In_reads_bytes_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength,
    _Inout_opt_ PVOID Context
)
/*++
Description:

    This function is the callback for power setting notifications (registered
    when ClasspGetD3IdleTimeout() is called for the first time).

    Currently, this function is used to get the disk idle timeout value from
    the system power settings.

    This function is guaranteed to be called at PASSIVE_LEVEL.

Arguments:

    SettingGuid - The power setting GUID.
    Value - Pointer to the power setting value.
    ValueLength - Size of the Value buffer.
    Context - The FDO's device extension.

Return Value:

    STATUS_SUCCESS

--*/
{
    PIDLE_POWER_FDO_LIST_ENTRY fdoEntry = NULL;

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer to PIRP for this use case
#endif
    PIRP removeLockTag = (PIRP)&ClasspPowerSettingCallback;

    UNREFERENCED_PARAMETER(Context);

    PAGED_CODE();

    if (IsEqualGUID(SettingGuid, &GUID_DISK_IDLE_TIMEOUT)) {
        if (ValueLength != sizeof(ULONG) || Value == NULL) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // The value supplied by this GUID is already in milliseconds.
        //
        DiskIdleTimeoutInMS = *((PULONG)Value);

        //
        // For each FDO on the idle power list, grab the remove lock and send
        // IOCTL_STORAGE_ENABLE_IDLE_POWER to the port driver to update the
        // idle timeout value.
        //
        KeAcquireGuardedMutex(&IdlePowerFDOListMutex);
        fdoEntry = (PIDLE_POWER_FDO_LIST_ENTRY)IdlePowerFDOList.Flink;
        while ((PLIST_ENTRY)fdoEntry != &IdlePowerFDOList) {

            ULONG isRemoved = ClassAcquireRemoveLock(fdoEntry->Fdo, removeLockTag);

            if (!isRemoved) {
                PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)fdoEntry->Fdo->DeviceExtension;

                //
                // Apply the new timeout if the user hasn't overridden it via the registry.
                //
                if (!fdoExtension->FunctionSupportInfo->IdlePower.D3IdleTimeoutOverridden) {
                    fdoExtension->FunctionSupportInfo->IdlePower.D3IdleTimeout = DiskIdleTimeoutInMS;
                    ClasspSendEnableIdlePowerIoctl(fdoEntry->Fdo);
                }
            }

            ClassReleaseRemoveLock(fdoEntry->Fdo, removeLockTag);

            fdoEntry = (PIDLE_POWER_FDO_LIST_ENTRY)fdoEntry->ListEntry.Flink;
        }
        KeReleaseGuardedMutex(&IdlePowerFDOListMutex);

    } else if (IsEqualGUID(SettingGuid, &GUID_CONSOLE_DISPLAY_STATE)) {

        //
        // If monitor is off, change media change requests to not
        // keep device active. This allows removable media devices to
        // go to sleep if there are no other active requests. Otherwise,
        // let media change requests keep the device active.
        //
        if ((ValueLength == sizeof(ULONG)) && (Value != NULL)) {
            if (*((PULONG)Value) == PowerMonitorOff) {
                ClasspScreenOff = TRUE;
            } else {
                ClasspScreenOff = FALSE;
            }

            KeAcquireGuardedMutex(&IdlePowerFDOListMutex);
            fdoEntry = (PIDLE_POWER_FDO_LIST_ENTRY)IdlePowerFDOList.Flink;
            while ((PLIST_ENTRY)fdoEntry != &IdlePowerFDOList) {

                ULONG isRemoved = ClassAcquireRemoveLock(fdoEntry->Fdo, removeLockTag);
                if (!isRemoved) {
                    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)fdoEntry->Fdo->DeviceExtension;

                    if (ClasspScreenOff == FALSE) {
                        //
                        // Now that the screen is on, we may need to check for media
                        // for devices that are not in D0 and may have removable media.
                        // This is because the media change polling has been disabled
                        // for devices in D3 and now that the screen is on the user may
                        // have inserted some media that they want to interact with.
                        //
                        if ((fdoExtension->DevicePowerState != PowerDeviceD0) &&
                            (fdoExtension->MediaChangeDetectionInfo != NULL) &&
                            (fdoExtension->FunctionSupportInfo->AsynchronousNotificationSupported == FALSE)) {
                            ClassCheckMediaState(fdoExtension);
                        }

                        //
                        // We disabled failure prediction polling during screen-off
                        // so now check to see if we missed a failure prediction
                        // period and if so, force the IOCTL to be sent now.
                        //
                        if ((fdoExtension->FailurePredictionInfo != NULL) &&
                            (fdoExtension->FailurePredictionInfo->Method != FailurePredictionNone)) {
                            if (ClasspFailurePredictionPeriodMissed(fdoExtension)) {
                                fdoExtension->FailurePredictionInfo->CountDown = 1;
                            }
                        }
                    }

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
                    //
                    // Screen state has changed so attempt to update the tick
                    // timer's no-wake tolerance accordingly.
                    //
                    ClasspUpdateTimerNoWakeTolerance(fdoExtension);
#endif
                }
                ClassReleaseRemoveLock(fdoEntry->Fdo, removeLockTag);

                fdoEntry = (PIDLE_POWER_FDO_LIST_ENTRY)fdoEntry->ListEntry.Flink;
            }
            KeReleaseGuardedMutex(&IdlePowerFDOListMutex);
        }

    }

    return STATUS_SUCCESS;
}


_IRQL_requires_same_
NTSTATUS
ClasspEnableIdlePower(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*++
Description:

    This function is used to enable idle (runtime) power management for the
    device.  It will do the work to determine D3Cold support, idle timeout,
    etc. and then notify the port driver that it wants to enable idle power
    management.

    This function may modify some of the idle power fields in the FDO's device
    extension.

Arguments:

    DeviceObject - The class FDO.

Return Value:

    An NTSTATUS code indicating the status of the operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG d3ColdDisabledByUser = FALSE;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ULONG idleTimeoutOverrideInSeconds = 0;

    //
    // This function should only be called once.
    //
    NT_ASSERT(fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled == FALSE);

    ClassGetDeviceParameter(fdoExtension,
                        CLASSP_REG_SUBKEY_NAME,
                        CLASSP_REG_DISABLE_D3COLD,
                        &d3ColdDisabledByUser);

    //
    // If the device is hot-pluggable or the user has explicitly
    // disabled D3Cold, do not enable D3Cold for this device.
    //
    if (d3ColdDisabledByUser || fdoExtension->PrivateFdoData->HotplugInfo.DeviceHotplug) {
        fdoExtension->FunctionSupportInfo->IdlePower.D3ColdSupported = 0;
    }

    ClassGetDeviceParameter(fdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_IDLE_TIMEOUT_IN_SECONDS,
                            &idleTimeoutOverrideInSeconds);

    //
    // Set the idle timeout.  If the user has not specified an override value,
    // this will either be a default value or will have been updated by the
    // power setting notification callback.
    //
    if (idleTimeoutOverrideInSeconds != 0) {
        fdoExtension->FunctionSupportInfo->IdlePower.D3IdleTimeout = (idleTimeoutOverrideInSeconds * 1000);
        fdoExtension->FunctionSupportInfo->IdlePower.D3IdleTimeoutOverridden = TRUE;
    } else {
        fdoExtension->FunctionSupportInfo->IdlePower.D3IdleTimeout = DiskIdleTimeoutInMS;
    }

    //
    // We don't allow disks to be wakeable.
    //
    fdoExtension->FunctionSupportInfo->IdlePower.DeviceWakeable = FALSE;

    //
    // Send IOCTL_STORAGE_ENABLE_IDLE_POWER to the port driver to enable idle
    // power management by the port driver.
    //
    status = ClasspSendEnableIdlePowerIoctl(DeviceObject);

    if (NT_SUCCESS(status)) {
        PIDLE_POWER_FDO_LIST_ENTRY fdoEntry = NULL;

        //
        // Put this FDO on the list of devices that are idle power managed.
        //
        fdoEntry = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(IDLE_POWER_FDO_LIST_ENTRY), CLASS_TAG_POWER);
        if (fdoEntry) {

            fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled = TRUE;

            fdoEntry->Fdo = DeviceObject;

            KeAcquireGuardedMutex(&IdlePowerFDOListMutex);
            InsertHeadList(&IdlePowerFDOList, &(fdoEntry->ListEntry));
            KeReleaseGuardedMutex(&IdlePowerFDOListMutex);

            //
            // If not registered already, register for disk idle timeout power
            // setting notifications.  The power manager will call our power
            // setting callback very soon to set the idle timeout to the actual
            // value.
            //
            if (PowerSettingNotificationHandle == NULL) {
                PoRegisterPowerSettingCallback(DeviceObject,
                                                &GUID_DISK_IDLE_TIMEOUT,
                                                &ClasspPowerSettingCallback,
                                                NULL,
                                                &(PowerSettingNotificationHandle));
            }
        } else {
            fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled = FALSE;
            status = STATUS_UNSUCCESSFUL;
        }
    }

    return status;
}

