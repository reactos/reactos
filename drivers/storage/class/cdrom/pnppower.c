/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    pnppower.c

Abstract:

    Functions to handle PnP and Power IRPs.

Environment:

    kernel mode only

Notes:
    optical devices do not need to issue SPIN UP when power up.
    The device itself should SPIN UP to process commands.


Revision History:

--*/


#include "ntddk.h"
#include "wdfcore.h"

#include "cdrom.h"
#include "ioctl.h"
#include "scratch.h"
#include "mmc.h"

#ifdef DEBUG_USE_WPP
#include "pnppower.tmh"
#endif


NTSTATUS
DeviceScratchSyncCache(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
DeviceScratchPreventMediaRemoval(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 Prevent
    );

NTSTATUS
RequestIssueShutdownFlush(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP                    Irp
    );

IO_COMPLETION_ROUTINE RequestProcessPowerIrpCompletion;

EVT_WDF_REQUEST_COMPLETION_ROUTINE RequestUnlockQueueCompletion;

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DevicePowerSettingCallback)

#endif

#pragma warning(push)
#pragma warning(disable:4152) // nonstandard extension, function/data pointer conversion in expression

#pragma warning(disable:28118)  // Dispatch routines for IRP_MJ_SHUTDOWN, IRP_MJ_FLUSH_BUFFERS occur at PASSIVE_LEVEL.
                                // WDF defines EVT_WDFDEVICE_WDM_IRP_PREPROCESS with _IRQL_requires_max_(DISPATCH_LEVEL),
                                // triggering a false positive for this warning.

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestProcessShutdownFlush(
    WDFDEVICE  Device,
    PIRP       Irp
    )
/*++

Routine Description:

    process IRP: IRP_MJ_SHUTDOWN, IRP_MJ_FLUSH_BUFFERS

Arguments:

    Device - device object
    Irp - the irp

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PIO_STACK_LOCATION      currentStack = NULL;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);

    //add trace info

    // acquire the shutdown/flush lock
    WdfWaitLockAcquire(deviceExtension->ShutdownFlushWaitLock, NULL);

    currentStack = IoGetCurrentIrpStackLocation(Irp);

    // finish all current requests
    WdfIoQueueStopSynchronously(deviceExtension->SerialIOQueue);

    // sync cache
    if (NT_SUCCESS(status))
    {
        // safe to use scratch srb to send the request.
        status = DeviceScratchSyncCache(deviceExtension);
    }

    // For SHUTDOWN, allow media removal.
    if (NT_SUCCESS(status))
    {
        if (currentStack->MajorFunction == IRP_MJ_SHUTDOWN)
        {
            // safe to use scratch srb to send the request.
            status = DeviceScratchPreventMediaRemoval(deviceExtension, FALSE);
        }
    }

    // Use original IRP, send SRB_FUNCTION_SHUTDOWN or SRB_FUNCTION_FLUSH (no retry)
    if (NT_SUCCESS(status))
    {
        status = RequestIssueShutdownFlush(deviceExtension, Irp);
    }

    // restart queue to allow processing further requests.
    WdfIoQueueStart(deviceExtension->SerialIOQueue);

    // release the shutdown/flush lock
    WdfWaitLockRelease(deviceExtension->ShutdownFlushWaitLock);

    // 6. complete the irp
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, 0);

    return status;
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestProcessSetPower(
    WDFDEVICE  Device,
    PIRP       Irp
    )
/*++

Routine Description:

    process IRP: IRP_MJ_POWER

Arguments:

    Device - device object
    Irp - the irp

Return Value:

    NTSTATUS

--*/
{
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PIO_STACK_LOCATION      currentStack;
    NTSTATUS                status;
    BOOLEAN                 IrpMarkedPending = FALSE;

    currentStack = IoGetCurrentIrpStackLocation(Irp);

    if ((currentStack->Parameters.Power.Type == DevicePowerState) &&
        (currentStack->Parameters.Power.State.DeviceState != PowerDeviceD0))
    {
        // We need to unlock the device queue in D3 postprocessing.
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               RequestProcessPowerIrpCompletion,
                               deviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);

        // Mark the Irp pending as we'll defer the I/O completion.
        IoMarkIrpPending(Irp);
        IrpMarkedPending = TRUE;
    }
    else {

        IoSkipCurrentIrpStackLocation(Irp);
    }

#pragma warning(push)
#pragma warning(disable: 28193) // OACR will complain that the status variable is not examined.

    //
    // Deliver the IRP back to the framework.
    //

    status = WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);

    if (IrpMarkedPending)
    {
        UNREFERENCED_PARAMETER(status);
        return STATUS_PENDING;
    }

#pragma warning(pop)

    return status;
}

// use scratch SRB to issue SYNC CACHE command.
NTSTATUS
DeviceScratchSyncCache(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    use scratch buffer to send SYNC CACHE command

Arguments:

    DeviceExtension - device context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       transferSize = 0;
    CDB         cdb;

    ScratchBuffer_BeginUse(DeviceExtension);

    RtlZeroMemory(&cdb, sizeof(CDB));
    // Set up the CDB
    cdb.SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;
    //srb->QueueTag = SP_UNTAGGED;
    //srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    status = ScratchBuffer_ExecuteCdbEx(DeviceExtension, NULL, transferSize, FALSE, &cdb, 10, TimeOutValueGetCapValue(DeviceExtension->TimeOutValue, 4));

    ScratchBuffer_EndUse(DeviceExtension);

    return status;
}

NTSTATUS
DeviceScratchPreventMediaRemoval(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 Prevent
    )
/*++

Routine Description:

    use scratch SRB to issue ALLOW/PREVENT MEDIA REMOVAL command.

Arguments:

    DeviceExtension - device context
    Prevent - TRUE (prevent media removal); FALSE (allow media removal)

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       transferSize = 0;
    CDB         cdb;

    ScratchBuffer_BeginUse(DeviceExtension);

    RtlZeroMemory(&cdb, sizeof(CDB));
    // Set up the CDB
    cdb.MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
    cdb.MEDIA_REMOVAL.Prevent = Prevent;
    //srb->QueueTag = SP_UNTAGGED;
    //srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    status = ScratchBuffer_ExecuteCdb(DeviceExtension, NULL, transferSize, FALSE, &cdb, 6);

    ScratchBuffer_EndUse(DeviceExtension);

    return status;
}

NTSTATUS
RequestIssueShutdownFlush(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP                    Irp
    )
/*++

Routine Description:

    issue SRB function Flush/Shutdown command.

Arguments:

    DeviceExtension - device context
    Irp - the irp

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PSCSI_REQUEST_BLOCK srb = DeviceExtension->ScratchContext.ScratchSrb;
    PIO_STACK_LOCATION  currentStack = NULL;

    ULONG       transferSize = 0;
    BOOLEAN     shouldRetry = TRUE;
    ULONG       timesAlreadyRetried = 0;
    LONGLONG    retryIn100nsUnits = 0;


    currentStack = IoGetCurrentIrpStackLocation(Irp);


    ScratchBuffer_BeginUse(DeviceExtension);

    // no retry needed.
    {
        ScratchBuffer_SetupSrb(DeviceExtension, NULL, transferSize, FALSE);

        // Set up the SRB/CDB
        srb->QueueTag = SP_UNTAGGED;
        srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
        srb->TimeOutValue = TimeOutValueGetCapValue(DeviceExtension->TimeOutValue, 4);
        srb->CdbLength = 0;

        if (currentStack->MajorFunction == IRP_MJ_SHUTDOWN)
        {
            srb->Function = SRB_FUNCTION_SHUTDOWN;
        }
        else
        {
            srb->Function = SRB_FUNCTION_FLUSH;
        }

        ScratchBuffer_SendSrb(DeviceExtension, TRUE, NULL);

        shouldRetry = RequestSenseInfoInterpretForScratchBuffer(DeviceExtension,
                                                                timesAlreadyRetried,
                                                                &status,
                                                                &retryIn100nsUnits);
        UNREFERENCED_PARAMETER(shouldRetry); //defensive coding, avoid PREFAST warning.
        UNREFERENCED_PARAMETER(status); //defensive coding, avoid PREFAST warning.

        // retrieve the real status from the request.
        status = WdfRequestGetStatus(DeviceExtension->ScratchContext.ScratchRequest);
    }

    ScratchBuffer_EndUse(DeviceExtension);


    return status;
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestUnlockQueueCompletion (
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
    )
{
    PIRP                    Irp = Context;
    WDFDEVICE               device = WdfIoTargetGetDevice(Target);
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(device);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Params);

    deviceExtension->PowerContext.Options.LockQueue = FALSE;

    PowerContextEndUse(deviceExtension);

    // Complete the original power irp
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestProcessPowerIrpCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
/*++

Routine Description:

    Free the Irp.

Arguments:

    DeviceObject - device that the completion routine fires on.

    Irp - The irp to be completed.

    Context - IRP context

Return Value:
    NTSTATUS

--*/
{
    PCDROM_DEVICE_EXTENSION deviceExtension = Context;
    PIO_STACK_LOCATION      currentStack;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
    {
        IoMarkIrpPending( Irp );
    }

    currentStack = IoGetCurrentIrpStackLocation(Irp);

    NT_ASSERT(currentStack->Parameters.Power.Type == DevicePowerState);
    NT_ASSERT(currentStack->Parameters.Power.State.DeviceState != PowerDeviceD0);

    _Analysis_assume_(deviceExtension != NULL);

    deviceExtension->PowerContext.PowerChangeState.PowerDown++;

    // Step 5. UNLOCK QUEUE
    if (deviceExtension->PowerContext.Options.LockQueue)
    {
        (VOID)DeviceSendPowerDownProcessRequest(deviceExtension,
                                                   RequestUnlockQueueCompletion,
                                                   Irp);

        // Let the completion routine complete the Irp
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // Release the power context if it wasn't already done as part of D0Exit handling
    if (deviceExtension->PowerContext.InUse)
    {
        PowerContextEndUse(deviceExtension);
    }

    return STATUS_CONTINUE_COMPLETION;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PowerContextReuseRequest(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    reset fields for the request.

Arguments:

    DeviceExtension - device context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                 status = STATUS_SUCCESS;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    PIRP                     irp = NULL;

    RtlZeroMemory(&(DeviceExtension->PowerContext.SenseData), sizeof(DeviceExtension->PowerContext.SenseData));
    RtlZeroMemory(&(DeviceExtension->PowerContext.Srb), sizeof(DeviceExtension->PowerContext.Srb));

    irp = WdfRequestWdmGetIrp(DeviceExtension->PowerContext.PowerRequest);

    // Re-use the previously created PowerRequest object and format it
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    status = WdfRequestReuse(DeviceExtension->PowerContext.PowerRequest, &reuseParams);
    if (NT_SUCCESS(status))
    {
        // This request was preformated during initialization so this call should never fail.
        status = WdfIoTargetFormatRequestForInternalIoctlOthers(DeviceExtension->IoTarget,
                                                                DeviceExtension->PowerContext.PowerRequest,
                                                                IOCTL_SCSI_EXECUTE_IN,
                                                                NULL, NULL,
                                                                NULL, NULL,
                                                                NULL, NULL);

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "PowerContextReuseRequest: WdfIoTargetFormatRequestForInternalIoctlOthers failed, %!STATUS!\n",
                       status));
        }
    }

    // Do some basic initialization of the PowerRequest, the rest will be done by the caller
    // of this function
    if (NT_SUCCESS(status))
    {
        PIO_STACK_LOCATION  nextStack = NULL;

        nextStack = IoGetNextIrpStackLocation(irp);

        nextStack->MajorFunction = IRP_MJ_SCSI;
        nextStack->Parameters.Scsi.Srb = &(DeviceExtension->PowerContext.Srb);

        DeviceExtension->PowerContext.Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
        DeviceExtension->PowerContext.Srb.OriginalRequest = irp;

        DeviceExtension->PowerContext.Srb.SenseInfoBuffer = &(DeviceExtension->PowerContext.SenseData);
        DeviceExtension->PowerContext.Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PowerContextBeginUse(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    initialize fields in power context

Arguments:

    DeviceExtension - device context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    NT_ASSERT(!DeviceExtension->PowerContext.InUse);

    DeviceExtension->PowerContext.InUse = TRUE;
    DeviceExtension->PowerContext.RetryCount = MAXIMUM_RETRIES;
    DeviceExtension->PowerContext.RetryIntervalIn100ns = 0;

    KeQueryTickCount(&DeviceExtension->PowerContext.StartTime);

    RtlZeroMemory(&(DeviceExtension->PowerContext.Options), sizeof(DeviceExtension->PowerContext.Options));
    RtlZeroMemory(&(DeviceExtension->PowerContext.PowerChangeState), sizeof(DeviceExtension->PowerContext.PowerChangeState));

    status = PowerContextReuseRequest(DeviceExtension);

    RequestClearSendTime(DeviceExtension->PowerContext.PowerRequest);

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PowerContextEndUse(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    inidate that power context using is finished.

Arguments:

    DeviceExtension - device context

Return Value:

    NTSTATUS

--*/
{
    NT_ASSERT(DeviceExtension->PowerContext.InUse);

    DeviceExtension->PowerContext.InUse = FALSE;

    KeQueryTickCount(&DeviceExtension->PowerContext.CompleteTime);

    return STATUS_SUCCESS;
}


_Function_class_(POWER_SETTING_CALLBACK)
_IRQL_requires_same_
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DevicePowerSettingCallback(
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
    PCDROM_DEVICE_EXTENSION DeviceExtension = Context;
    MONITOR_DISPLAY_STATE DisplayState;

    PAGED_CODE();

    if (IsEqualGUID(SettingGuid, &GUID_CONSOLE_DISPLAY_STATE)) {

        if ((ValueLength == sizeof(ULONG)) && (Value != NULL)) {

            DisplayState = *((PULONG)Value);

            _Analysis_assume_(DeviceExtension != NULL);

            //
            // Power setting callbacks are asynchronous so make sure the device
            // is completely initialized before taking any actions.
            //
            if (DeviceExtension->IsInitialized) {

                //
                // If monitor is off, change media change requests to not keep device active.
                // This allows the devices to go to sleep if there are no other active requests.
                //

                if (DisplayState == PowerMonitorOff) {

                    //
                    // Mark the device inactive so that it can enter a low power state.
                    //

                    DeviceMarkActive(DeviceExtension, FALSE, TRUE);
                    SET_FLAG(DeviceExtension->MediaChangeDetectionInfo->SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE);
                }
                else
                {
                    CLEAR_FLAG(DeviceExtension->MediaChangeDetectionInfo->SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE);
                    DeviceMarkActive(DeviceExtension, TRUE, TRUE);
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

#pragma warning(pop) // un-sets any local warning changes

