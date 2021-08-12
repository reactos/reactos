/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    common.c

Abstract:

    shared private routines for cdrom.sys

Environment:

    kernel mode only

Notes:


Revision History:

--*/


#include "ntddk.h"
#include "ntddstor.h"
#include "ntstrsafe.h"

#include "cdrom.h"
#include "scratch.h"


#ifdef DEBUG_USE_WPP
#include "common.tmh"
#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DeviceGetParameter)
#pragma alloc_text(PAGE, DeviceSetParameter)
#pragma alloc_text(PAGE, DeviceSendSrbSynchronously)
#pragma alloc_text(PAGE, DevicePickDvdRegion)
#pragma alloc_text(PAGE, StringsAreMatched)
#pragma alloc_text(PAGE, PerformEjectionControl)
#pragma alloc_text(PAGE, DeviceFindFeaturePage)
#pragma alloc_text(PAGE, DevicePrintAllFeaturePages)
#pragma alloc_text(PAGE, DeviceRegisterInterface)
#pragma alloc_text(PAGE, DeviceRestoreDefaultSpeed)
#pragma alloc_text(PAGE, DeviceSendRequestSynchronously)
#pragma alloc_text(PAGE, MediaReadCapacity)
#pragma alloc_text(PAGE, MediaReadCapacityDataInterpret)
#pragma alloc_text(PAGE, DeviceRetrieveModeSenseUsingScratch)
#pragma alloc_text(PAGE, ModeSenseFindSpecificPage)
#pragma alloc_text(PAGE, DeviceUnlockExclusive)

#endif

LPCSTR LockTypeStrings[] = {"Simple",
                            "Secure",
                            "Internal"
                            };

VOID
RequestSetReceivedTime(
    _In_ WDFREQUEST Request
    )
{
    PCDROM_REQUEST_CONTEXT requestContext = RequestGetContext(Request);
    LARGE_INTEGER          temp;

    KeQueryTickCount(&temp);

    requestContext->TimeReceived = temp;

    return;
}

VOID
RequestSetSentTime(
    _In_ WDFREQUEST Request
    )
{
    PCDROM_REQUEST_CONTEXT requestContext = RequestGetContext(Request);
    LARGE_INTEGER          temp;

    KeQueryTickCount(&temp);

    if (requestContext->TimeSentDownFirstTime.QuadPart == 0)
    {
        requestContext->TimeSentDownFirstTime = temp;
    }

    requestContext->TimeSentDownLasttTime = temp;

    if (requestContext->OriginalRequest != NULL)
    {
        PCDROM_REQUEST_CONTEXT originalRequestContext = RequestGetContext(requestContext->OriginalRequest);

        if (originalRequestContext->TimeSentDownFirstTime.QuadPart == 0)
        {
            originalRequestContext->TimeSentDownFirstTime = temp;
        }

        originalRequestContext->TimeSentDownLasttTime = temp;
    }

    return;
}

VOID
RequestClearSendTime(
    _In_ WDFREQUEST Request
    )
/*
Routine Description:

    This function is used to clean SentTime fields in reusable request context.

Arguments:
    Request -

Return Value:
    N/A

*/
{
    PCDROM_REQUEST_CONTEXT requestContext = RequestGetContext(Request);

    requestContext->TimeSentDownFirstTime.QuadPart = 0;
    requestContext->TimeSentDownLasttTime.QuadPart = 0;

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceGetParameter(
    _In_ PCDROM_DEVICE_EXTENSION    DeviceExtension,
    _In_opt_ PWSTR                  SubkeyName,
    _In_ PWSTR                      ParameterName,
    _Inout_ PULONG                  ParameterValue  // also default value
    )
/*++
Routine Description:

    retrieve device parameter from registry.

Arguments:

    DeviceExtension - device context.

    SubkeyName - name of subkey

    ParameterName - the registry parameter to be retrieved

Return Value:

    ParameterValue - registry value retrieved

--*/
{
    NTSTATUS        status;
    WDFKEY          rootKey = NULL;
    WDFKEY          subKey = NULL;
    UNICODE_STRING  registrySubKeyName;
    UNICODE_STRING  registryValueName;
    ULONG           defaultParameterValue;

    PAGED_CODE();

    RtlInitUnicodeString(&registryValueName, ParameterName);

    if (SubkeyName != NULL)
    {
        RtlInitUnicodeString(&registrySubKeyName, SubkeyName);
    }

    // open the hardware key
    status = WdfDeviceOpenRegistryKey(DeviceExtension->Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_READ,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &rootKey);

    // open the sub key
    if (NT_SUCCESS(status) && (SubkeyName != NULL))
    {
        status = WdfRegistryOpenKey(rootKey,
                                    &registrySubKeyName,
                                    KEY_READ,
                                    WDF_NO_OBJECT_ATTRIBUTES,
                                    &subKey);

        if (!NT_SUCCESS(status))
        {
            WdfRegistryClose(rootKey);
            rootKey = NULL;
        }
    }

    if (NT_SUCCESS(status) && (rootKey != NULL))
    {
        defaultParameterValue = *ParameterValue;

        status = WdfRegistryQueryULong((subKey != NULL) ? subKey : rootKey,
                                       &registryValueName,
                                       ParameterValue);

        if (!NT_SUCCESS(status))
        {
            *ParameterValue = defaultParameterValue; // use default value
        }
    }

    // close what we open
    if (subKey != NULL)
    {
        WdfRegistryClose(subKey);
        subKey = NULL;
    }

    if (rootKey != NULL)
    {
        WdfRegistryClose(rootKey);
        rootKey = NULL;
    }

    // Windows 2000 SP3 uses the driver-specific key, so look in there
    if (!NT_SUCCESS(status))
    {
        // open the software key
        status = WdfDeviceOpenRegistryKey(DeviceExtension->Device,
                                          PLUGPLAY_REGKEY_DRIVER,
                                          KEY_READ,
                                          WDF_NO_OBJECT_ATTRIBUTES,
                                          &rootKey);

        // open the sub key
        if (NT_SUCCESS(status) && (SubkeyName != NULL))
        {
            status = WdfRegistryOpenKey(rootKey,
                                        &registrySubKeyName,
                                        KEY_READ,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &subKey);

            if (!NT_SUCCESS(status))
            {
                WdfRegistryClose(rootKey);
                rootKey = NULL;
            }
        }

        if (NT_SUCCESS(status) && (rootKey != NULL))
        {
            defaultParameterValue = *ParameterValue;

            status = WdfRegistryQueryULong((subKey != NULL) ? subKey : rootKey,
                                           &registryValueName,
                                           ParameterValue);

            if (!NT_SUCCESS(status))
            {
                *ParameterValue = defaultParameterValue; // use default value
            }
            else
            {
                // Migrate the value over to the device-specific key
                DeviceSetParameter(DeviceExtension, SubkeyName, ParameterName, *ParameterValue);
            }
        }

        // close what we open
        if (subKey != NULL)
        {
            WdfRegistryClose(subKey);
            subKey = NULL;
        }

        if (rootKey != NULL)
        {
            WdfRegistryClose(rootKey);
            rootKey = NULL;
        }
    }

    return;

} // end DeviceetParameter()


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceSetParameter(
    _In_ PCDROM_DEVICE_EXTENSION    DeviceExtension,
    _In_opt_z_ PWSTR                SubkeyName,
    _In_ PWSTR                      ParameterName,
    _In_ ULONG                      ParameterValue
    )
/*++
Routine Description:

    set parameter to registry.

Arguments:

    DeviceExtension - device context.

    SubkeyName - name of subkey

    ParameterName - the registry parameter to be retrieved

    ParameterValue - registry value to be set

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS        status;
    WDFKEY          rootKey = NULL;
    WDFKEY          subKey = NULL;
    UNICODE_STRING  registrySubKeyName;
    UNICODE_STRING  registryValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&registryValueName, ParameterName);

    if (SubkeyName != NULL)
    {
        RtlInitUnicodeString(&registrySubKeyName, SubkeyName);
    }

    // open the hardware key
    status = WdfDeviceOpenRegistryKey(DeviceExtension->Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_READ | KEY_WRITE,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &rootKey);

    // open the sub key
    if (NT_SUCCESS(status) && (SubkeyName != NULL))
    {
        status = WdfRegistryOpenKey(rootKey,
                                    &registrySubKeyName,
                                    KEY_READ | KEY_WRITE,
                                    WDF_NO_OBJECT_ATTRIBUTES,
                                    &subKey);

        if (!NT_SUCCESS(status))
        {
            WdfRegistryClose(rootKey);
            rootKey = NULL;
        }
    }

    if (NT_SUCCESS(status) && (rootKey != NULL))
    {
        status = WdfRegistryAssignULong((subKey != NULL) ? subKey : rootKey,
                                        &registryValueName,
                                        ParameterValue);
    }

    // close what we open
    if (subKey != NULL)
    {
        WdfRegistryClose(subKey);
        subKey = NULL;
    }

    if (rootKey != NULL)
    {
        WdfRegistryClose(rootKey);
        rootKey = NULL;
    }

    return status;

} // end DeviceSetParameter()


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceSendRequestSynchronously(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request,
    _In_ BOOLEAN      RequestFormated
    )
/*++
Routine Description:

    send a request to lower driver synchronously.

Arguments:

    Device - device object.

    Request - request object

    RequestFormated - if the request is already formatted, will no do it in this function

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);
    BOOLEAN                     requestCancelled = FALSE;
    PCDROM_REQUEST_CONTEXT      requestContext = RequestGetContext(Request);

    PAGED_CODE();

    if (!RequestFormated)
    {
        // set request up for sending down
        WdfRequestFormatRequestUsingCurrentType(Request);
    }

    // get cancellation status for the original request
    if (requestContext->OriginalRequest != NULL)
    {
        requestCancelled = WdfRequestIsCanceled(requestContext->OriginalRequest);
    }

    if (!requestCancelled)
    {
        status = RequestSend(deviceExtension,
                             Request,
                             deviceExtension->IoTarget,
                             WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
                             NULL);
    }
    else
    {
        status = STATUS_CANCELLED;
    }

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceSendSrbSynchronously(
    _In_ WDFDEVICE           Device,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_opt_ PVOID           BufferAddress,
    _In_ ULONG               BufferLength,
    _In_ BOOLEAN             WriteToDevice,
    _In_opt_ WDFREQUEST      OriginalRequest
    )
/*++
Routine Description:

    Send a SRB structure to lower driver synchronously.

    Process of this function:
    1. Allocate SenseBuffer; Create Request; Allocate MDL
    2. Do following loop if necessary
       2.1 Reuse Request
       2.2 Format Srb, Irp
       2.3 Send Request
       2.4 Error Intepret and retry decision making.
    3. Release all allocated resosurces.

Arguments:

    Device - device object.

    Request - request object

    RequestFormated - if the request is already formatted, will no do it in this function

Return Value:
    NTSTATUS

NOTE:
The caller needs to setup following fields before calling this routine.
        srb.CdbLength
        srb.TimeOutValue
        cdb

BufferLength and WriteToDevice to control the data direction of the device
    BufferLength = 0: No data transfer
    BufferLenth != 0 && !WriteToDevice: get data from device
    BufferLenth != 0 && WriteToDevice: send data to device
--*/
{
    NTSTATUS                status;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_PRIVATE_FDO_DATA fdoData = deviceExtension->PrivateFdoData;
    PUCHAR                  senseInfoBuffer = NULL;
    ULONG                   retryCount = 0;
    BOOLEAN                 retry = FALSE;
    ULONG                   ioctlCode = 0;
    WDFREQUEST              request = NULL;
    PIRP                    irp = NULL;
    PIO_STACK_LOCATION      nextStack = NULL;
    PMDL                    mdlAddress = NULL;
    BOOLEAN                 memoryLocked = FALSE;
    WDF_OBJECT_ATTRIBUTES   attributes;
    PZERO_POWER_ODD_INFO    zpoddInfo = deviceExtension->ZeroPowerODDInfo;

    PAGED_CODE();

    // NOTE: This code is only pagable because we are not freezing
    //       the queue.  Allowing the queue to be frozen from a pagable
    //       routine could leave the queue frozen as we try to page in
    //       the code to unfreeze the queue.  The result would be a nice
    //       case of deadlock.  Therefore, since we are unfreezing the
    //       queue regardless of the result, just set the NO_FREEZE_QUEUE
    //       flag in the SRB.
    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //1. allocate SenseBuffer and initiate Srb common fields
    //   these fields will not be changed by lower driver.
    {
        // Write length to SRB.
        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        // Sense buffer is in aligned nonpaged pool.
        senseInfoBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                                SENSE_BUFFER_SIZE,
                                                CDROM_TAG_SENSE_INFO);

        if (senseInfoBuffer == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "DeviceSendSrbSynchronously: Can't allocate MDL\n"));

            goto Exit;
        }

        Srb->SenseInfoBuffer = senseInfoBuffer;
        Srb->DataBuffer = BufferAddress;

        // set timeout value to default value if it's not specifically set by caller.
        if (Srb->TimeOutValue == 0)
        {
            Srb->TimeOutValue = deviceExtension->TimeOutValue;
        }
    }

    //2. Create Request object
    {
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                                CDROM_REQUEST_CONTEXT);

        status = WdfRequestCreate(&attributes,
                                  deviceExtension->IoTarget,
                                  &request);

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "DeviceSendSrbSynchronously: Can't create request: %lx\n",
                       status));

            goto Exit;
        }

        irp = WdfRequestWdmGetIrp(request);
    }

    // 3. Build an MDL for the data buffer and stick it into the irp.
    if (BufferAddress != NULL)
    {
        mdlAddress = IoAllocateMdl( BufferAddress,
                                    BufferLength,
                                    FALSE,
                                    FALSE,
                                    irp );
        if (mdlAddress == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "DeviceSendSrbSynchronously: Can't allocate MDL\n"));

            goto Exit;
        }

        _SEH2_TRY
        {
            MmProbeAndLockPages(mdlAddress,
                                KernelMode,
                                (WriteToDevice ? IoReadAccess : IoWriteAccess));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            status = _SEH2_GetExceptionCode();

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "DeviceSendSrbSynchronously: Exception %lx locking buffer\n", status));

            _SEH2_YIELD(goto Exit);
        }
        _SEH2_END;

        memoryLocked = TRUE;
    }

    // 4. Format Srb, Irp; Send request and retry when necessary
    do
    {
        // clear the control variable.
        retry = FALSE;

        // 4.1 reuse the request object; set originalRequest field.
        {
            WDF_REQUEST_REUSE_PARAMS    params;
            PCDROM_REQUEST_CONTEXT      requestContext = NULL;

            // deassign the MdlAddress, this is the value we assign explicitly.
            // doing this can prevent WdfRequestReuse to release the Mdl unexpectly.
            if (irp->MdlAddress)
            {
                irp->MdlAddress = NULL;
            }

            WDF_REQUEST_REUSE_PARAMS_INIT(&params,
                                          WDF_REQUEST_REUSE_NO_FLAGS,
                                          STATUS_SUCCESS);

            status = WdfRequestReuse(request, &params);

            if (!NT_SUCCESS(status))
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                           "DeviceSendSrbSynchronously: WdfRequestReuse failed, %!STATUS!\n",
                           status));
                // exit the loop.
                break;
            }

            // WDF requests to format the request befor sending it
            status = WdfIoTargetFormatRequestForInternalIoctlOthers(deviceExtension->IoTarget,
                                                                    request,
                                                                    ioctlCode,
                                                                    NULL, NULL,
                                                                    NULL, NULL,
                                                                    NULL, NULL);

            if (!NT_SUCCESS(status))
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                           "DeviceSendSrbSynchronously: WdfIoTargetFormatRequestForInternalIoctlOthers failed, %!STATUS!\n",
                           status));
                // exit the loop.
                break;
            }

            requestContext = RequestGetContext(request);
            requestContext->OriginalRequest = OriginalRequest;
        }

        // 4.2 Format Srb and Irp
        {
            Srb->OriginalRequest = irp;
            Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
            Srb->DataTransferLength = BufferLength;
            Srb->SrbFlags = deviceExtension->SrbFlags;

            // Disable synchronous transfer for these requests.
            SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(Srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);

            if (BufferAddress != NULL)
            {
                if (WriteToDevice)
                {
                    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_OUT);
                    ioctlCode = IOCTL_SCSI_EXECUTE_OUT;
                }
                else
                {
                    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_IN);
                    ioctlCode = IOCTL_SCSI_EXECUTE_IN;
                }
            }
            else
            {
                ioctlCode = IOCTL_SCSI_EXECUTE_NONE;
            }


            // Zero out status.
            Srb->ScsiStatus = 0;
            Srb->SrbStatus = 0;
            Srb->NextSrb = NULL;

            // irp related fields
            irp->MdlAddress = mdlAddress;

            nextStack = IoGetNextIrpStackLocation(irp);

            nextStack->MajorFunction = IRP_MJ_SCSI;
            nextStack->Parameters.DeviceIoControl.IoControlCode = ioctlCode;
            nextStack->Parameters.Scsi.Srb = Srb;
        }

        // 4.3 send Request to lower driver.
        status = DeviceSendRequestSynchronously(Device, request, TRUE);

        if (status != STATUS_CANCELLED)
        {
            NT_ASSERT(SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_PENDING);
            NT_ASSERT(status != STATUS_PENDING);
            NT_ASSERT(!(Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN));

            // 4.4 error process.
            if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS)
            {
                LONGLONG    retryIntervalIn100ns = 0;

                // Update status and determine if request should be retried.
                retry = RequestSenseInfoInterpret(deviceExtension,
                                                  request,
                                                  Srb,
                                                  retryCount,
                                                  &status,
                                                  &retryIntervalIn100ns);

                if (retry)
                {
                    LARGE_INTEGER t;
                    t.QuadPart = -retryIntervalIn100ns;
                    retryCount++;
                    KeDelayExecutionThread(KernelMode, FALSE, &t);
                }
            }
            else
            {
                // Request succeeded.
                fdoData->LoggedTURFailureSinceLastIO = FALSE;
                status = STATUS_SUCCESS;
                retry = FALSE;
            }
        }
    } while(retry);

    if ((zpoddInfo != NULL) &&
        (zpoddInfo->MonitorStartStopUnit != FALSE) &&
        (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceSendSrbSynchronously: soft eject detected, device marked as active\n"));

        DeviceMarkActive(deviceExtension, TRUE, FALSE);
    }

    // 5. Release all allocated resources.

    // required even though we allocated our own, since the port driver may
    // have allocated one also
    if (PORT_ALLOCATED_SENSE(deviceExtension, Srb))
    {
        FREE_PORT_ALLOCATED_SENSE_BUFFER(deviceExtension, Srb);
    }

Exit:

    if (senseInfoBuffer != NULL)
    {
        FREE_POOL(senseInfoBuffer);
    }

    Srb->SenseInfoBuffer = NULL;
    Srb->SenseInfoBufferLength = 0;

    if (mdlAddress)
    {
        if (memoryLocked)
        {
            MmUnlockPages(mdlAddress);
            memoryLocked = FALSE;
        }

        IoFreeMdl(mdlAddress);
        irp->MdlAddress = NULL;
    }

    if (request)
    {
        WdfObjectDelete(request);
    }

    return status;
}


VOID
DeviceSendNotification(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ const GUID*              Guid,
    _In_ ULONG                    ExtraDataSize,
    _In_opt_ PVOID                ExtraData
    )
/*++
Routine Description:

    send notification to other components

Arguments:

    DeviceExtension - device context.

    Guid - GUID for the notification

    ExtraDataSize - data size along with notification

    ExtraData - data buffer send with notification

Return Value:
    None

--*/
{
    PTARGET_DEVICE_CUSTOM_NOTIFICATION  notification;
    ULONG                               requiredSize;
    NTSTATUS                            status;

    status = RtlULongAdd((sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION) - sizeof(UCHAR)),
                         ExtraDataSize,
                         &requiredSize);

    if (!(NT_SUCCESS(status)) || (requiredSize > 0x0000ffff))
    {
        // MAX_USHORT, max total size for these events!
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_MCN,
                   "Error sending event: size too large! (%x)\n",
                   requiredSize));
        return;
    }

    notification = ExAllocatePoolWithTag(NonPagedPoolNx,
                                         requiredSize,
                                         CDROM_TAG_NOTIFICATION);

    // if none allocated, exit
    if (notification == NULL)
    {
        return;
    }

    // Prepare and send the request!
    RtlZeroMemory(notification, requiredSize);
    notification->Version = 1;
    notification->Size = (USHORT)(requiredSize);
    notification->FileObject = NULL;
    notification->NameBufferOffset = -1;
    notification->Event = *Guid;

    if (ExtraData != NULL)
    {
        RtlCopyMemory(notification->CustomDataBuffer, ExtraData, ExtraDataSize);
    }

    IoReportTargetDeviceChangeAsynchronous(DeviceExtension->LowerPdo,
                                           notification,
                                           NULL,
                                           NULL);

    FREE_POOL(notification);

    return;
}


VOID
DeviceSendStartUnit(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    Send command to SCSI unit to start or power up.
    Because this command is issued asynchronounsly, that is, without
    waiting on it to complete, the IMMEDIATE flag is not set. This
    means that the CDB will not return until the drive has powered up.
    This should keep subsequent requests from being submitted to the
    device before it has completely spun up.

    This routine is called from the InterpretSense routine, when a
    request sense returns data indicating that a drive must be
    powered up.

    This routine may also be called from a class driver's error handler,
    or anytime a non-critical start device should be sent to the device.

Arguments:

    Device - The device object.

Return Value:

    None.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = NULL;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFREQUEST              startUnitRequest = NULL;
    WDFMEMORY               inputMemory = NULL;

    PCOMPLETION_CONTEXT     context = NULL;
    PSCSI_REQUEST_BLOCK     srb = NULL;
    PCDB                    cdb = NULL;

    deviceExtension = DeviceGetExtension(Device);

    if (NT_SUCCESS(status))
    {
        // Allocate Srb from nonpaged pool.
        context = ExAllocatePoolWithTag(NonPagedPoolNx,
                                        sizeof(COMPLETION_CONTEXT),
                                        CDROM_TAG_COMPLETION_CONTEXT);

        if (context == NULL)
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "DeviceSendStartUnit: Failed to allocate completion context\n"));

            status = STATUS_INTERNAL_ERROR;
        }
    }

    if (NT_SUCCESS(status))
    {
        // Save the device object in the context for use by the completion
        // routine.
        context->Device = Device;
        srb = &context->Srb;

        // Zero out srb.
        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        // setup SRB structure.
        srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->TimeOutValue = START_UNIT_TIMEOUT;

        srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                        SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        // setup CDB
        srb->CdbLength = 6;
        cdb = (PCDB)srb->Cdb;

        cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
        cdb->START_STOP.Start = 1;
        cdb->START_STOP.Immediate = 0;
        cdb->START_STOP.LogicalUnitNumber = srb->Lun;

        //Create Request for sending down to port driver
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                                CDROM_REQUEST_CONTEXT);
        attributes.ParentObject = deviceExtension->IoTarget;

        status = WdfRequestCreate(&attributes,
                                  deviceExtension->IoTarget,
                                  &startUnitRequest);
    }

    if (NT_SUCCESS(status))
    {
        srb->OriginalRequest = WdfRequestWdmGetIrp(startUnitRequest);
        NT_ASSERT(srb->OriginalRequest != NULL);

        //Prepare the request
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = startUnitRequest;

        status = WdfMemoryCreatePreallocated(&attributes,
                                             (PVOID)srb,
                                             sizeof(SCSI_REQUEST_BLOCK),
                                             &inputMemory);
    }

    if (NT_SUCCESS(status))
    {
        status = WdfIoTargetFormatRequestForInternalIoctlOthers(deviceExtension->IoTarget,
                                                                startUnitRequest,
                                                                IOCTL_SCSI_EXECUTE_NONE,
                                                                inputMemory,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        // Set a CompletionRoutine callback function.
        WdfRequestSetCompletionRoutine(startUnitRequest,
                                       DeviceAsynchronousCompletion,
                                       context);

        status = RequestSend(deviceExtension,
                             startUnitRequest,
                             deviceExtension->IoTarget,
                             0,
                             NULL);
    }

    // release resources when failed.
    if (!NT_SUCCESS(status))
    {
        FREE_POOL(context);
        if (startUnitRequest != NULL)
        {
            WdfObjectDelete(startUnitRequest);
        }
    }

    return;
} // end StartUnit()


VOID
DeviceSendIoctlAsynchronously(
    _In_ PCDROM_DEVICE_EXTENSION    DeviceExtension,
    _In_ ULONG                      IoControlCode,
    _In_ PDEVICE_OBJECT             TargetDeviceObject
    )
/*++

Routine Description:

    Send an IOCTL asynchronously

Arguments:

    DeviceExtension - device context.
    IoControlCode - IOCTL code.
    TargetDeviceObject - target device object.

Return Value:

    None.

--*/
{
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  nextIrpStack = NULL;

    irp = IoAllocateIrp(DeviceExtension->DeviceObject->StackSize, FALSE);

    if (irp != NULL)
    {
        nextIrpStack = IoGetNextIrpStackLocation(irp);

        nextIrpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;

        nextIrpStack->Parameters.DeviceIoControl.OutputBufferLength = 0;
        nextIrpStack->Parameters.DeviceIoControl.InputBufferLength = 0;
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
        nextIrpStack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        IoSetCompletionRoutine(irp,
                               RequestAsynchronousIrpCompletion,
                               DeviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);

        (VOID) IoCallDriver(TargetDeviceObject, irp);
    }
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestAsynchronousIrpCompletion(
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
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceAsynchronousCompletion(
    _In_ WDFREQUEST                       Request,
    _In_ WDFIOTARGET                      Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS   Params,
    _In_ WDFCONTEXT                       Context
    )
/*++

Routine Description:

    This routine is called when an asynchronous I/O request
    which was issused by the class driver completes.  Examples of such requests
    are release queue or START UNIT. This routine releases the queue if
    necessary.  It then frees the context and the IRP.

Arguments:

    DeviceObject - The device object for the logical unit; however since this
        is the top stack location the value is NULL.

    Irp - Supplies a pointer to the Irp to be processed.

    Context - Supplies the context to be used to process this request.

Return Value:

    None.

--*/
{
    PCOMPLETION_CONTEXT     context = (PCOMPLETION_CONTEXT)Context;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(context->Device);

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    // If this is an execute srb, then check the return status and make sure.
    // the queue is not frozen.
    if (context->Srb.Function == SRB_FUNCTION_EXECUTE_SCSI)
    {
        // Check for a frozen queue.
        if (context->Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            // Unfreeze the queue getting the device object from the context.
            DeviceReleaseQueue(context->Device);
        }
    }

    // free port-allocated sense buffer if we can detect
    //
    if (PORT_ALLOCATED_SENSE(deviceExtension, &context->Srb))
    {
        FREE_PORT_ALLOCATED_SENSE_BUFFER(deviceExtension, &context->Srb);
    }

    FREE_POOL(context);

    WdfObjectDelete(Request);

} // end DeviceAsynchronousCompletion()


VOID
DeviceReleaseQueue(
    _In_ WDFDEVICE    Device
    )
/*++

Routine Description:

    This routine issues an internal device control command
    to the port driver to release a frozen queue. The call
    is issued asynchronously as DeviceReleaseQueue will be invoked
    from the IO completion DPC (and will have no context to
    wait for a synchronous call to complete).

    This routine must be called with the remove lock held.

Arguments:

    Device - The functional device object for the device with the frozen queue.

Return Value:

    None.

--*/
{
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);
    PSCSI_REQUEST_BLOCK         srb = NULL;
    KIRQL                       currentIrql;

    // we raise irql seperately so we're not swapped out or suspended
    // while holding the release queue irp in this routine.  this lets
    // us release the spin lock before lowering irql.
    KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

    WdfSpinLockAcquire(deviceExtension->ReleaseQueueSpinLock);

    if (deviceExtension->ReleaseQueueInProgress)
    {
        // Someone is already doing this work - just set the flag to indicate that
        // we need to release the queue again.
        deviceExtension->ReleaseQueueNeeded = TRUE;
        WdfSpinLockRelease(deviceExtension->ReleaseQueueSpinLock);
        KeLowerIrql(currentIrql);

        return;
    }

    // Mark that there is a release queue in progress and drop the spinlock.
    deviceExtension->ReleaseQueueInProgress = TRUE;

    WdfSpinLockRelease(deviceExtension->ReleaseQueueSpinLock);

    srb = &(deviceExtension->ReleaseQueueSrb);

    // Optical media are removable, so we just flush the queue.  This will also release it.
    srb->Function = SRB_FUNCTION_FLUSH_QUEUE;

    srb->OriginalRequest = WdfRequestWdmGetIrp(deviceExtension->ReleaseQueueRequest);

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine(deviceExtension->ReleaseQueueRequest,
                                   DeviceReleaseQueueCompletion,
                                   Device);
    // Send the request. If an error occurs, complete the request.
    RequestSend(deviceExtension,
                deviceExtension->ReleaseQueueRequest,
                deviceExtension->IoTarget,
                WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE,
                NULL);

    KeLowerIrql(currentIrql);

    return;

} // end DeviceReleaseQueue()

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceReleaseQueueCompletion(
    _In_ WDFREQUEST                       Request,
    _In_ WDFIOTARGET                      Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS   Params,
    _In_ WDFCONTEXT                       Context
    )
/*++

Routine Description:

    This routine is called when an asynchronous release queue request which
    was issused in DeviceReleaseQueue completes. This routine prepares for
    the next release queue request and resends it if necessary.

Arguments:

    Request - The completed request.

    Target - IoTarget object

    Params - Completion parameters

    Context - WDFDEVICE object handle.

Return Value:

    None.

--*/
{
    NTSTATUS                    status;
    WDFDEVICE                   device = Context;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(device);

    BOOLEAN                     releaseQueueNeeded = FALSE;
    WDF_REQUEST_REUSE_PARAMS    params = {0};

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    WDF_REQUEST_REUSE_PARAMS_INIT(&params,
                                  WDF_REQUEST_REUSE_NO_FLAGS,
                                  STATUS_SUCCESS);

    // Grab the spinlock and clear the release queue in progress flag so others
    // can run. Save (and clear) the state of the release queue needed flag
    // so that we can issue a new release queue outside the spinlock.
    WdfSpinLockAcquire(deviceExtension->ReleaseQueueSpinLock);

    releaseQueueNeeded = deviceExtension->ReleaseQueueNeeded;

    deviceExtension->ReleaseQueueNeeded = FALSE;
    deviceExtension->ReleaseQueueInProgress = FALSE;

    // Reuse the ReleaseQueueRequest for the next time.
    status = WdfRequestReuse(Request,&params);

    if (NT_SUCCESS(status))
    {
        // Preformat the ReleaseQueueRequest for the next time.
        // This should always succeed because it was already preformatted once during device initialization
        status = WdfIoTargetFormatRequestForInternalIoctlOthers(deviceExtension->IoTarget,
                                                                Request,
                                                                IOCTL_SCSI_EXECUTE_NONE,
                                                                deviceExtension->ReleaseQueueInputMemory,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL);
    }

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                   "DeviceReleaseQueueCompletion: WdfIoTargetFormatRequestForInternalIoctlOthers failed, %!STATUS!\n",
                   status));
    }

    RequestClearSendTime(Request);

    WdfSpinLockRelease(deviceExtension->ReleaseQueueSpinLock);

    // If we need a release queue then issue one now.  Another processor may
    // have already started one in which case we'll try to issue this one after
    // it is done - but we should never recurse more than one deep.
    if (releaseQueueNeeded)
    {
        DeviceReleaseQueue(device);
    }

    return;

} // DeviceReleaseQueueCompletion()


//
// In order to provide better performance without the need to reboot,
// we need to implement a self-adjusting method to set and clear the
// srb flags based upon current performance.
//
// whenever there is an error, immediately grab the spin lock.  the
// MP perf hit here is acceptable, since we're in an error path.  this
// is also neccessary because we are guaranteed to be modifying the
// SRB flags here, setting SuccessfulIO to zero, and incrementing the
// actual error count (which is always done within this spinlock).
//
// whenever there is no error, increment a counter.  if there have been
// errors on the device, and we've enabled dynamic perf, *and* we've
// just crossed the perf threshhold, then grab the spin lock and
// double check that the threshhold has, indeed been hit(*). then
// decrement the error count, and if it's dropped sufficiently, undo
// some of the safety changes made in the SRB flags due to the errors.
//
// * this works in all cases.  even if lots of ios occur after the
//   previous guy went in and cleared the successfulio counter, that
//   just means that we've hit the threshhold again, and so it's proper
//   to run the inner loop again.
//

VOID
DevicePerfIncrementErrorCount(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
{
    PCDROM_PRIVATE_FDO_DATA fdoData = DeviceExtension->PrivateFdoData;
    KIRQL                   oldIrql;
    ULONG                   errors;

    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

    fdoData->Perf.SuccessfulIO = 0; // implicit interlock
    errors = InterlockedIncrement((PLONG)&DeviceExtension->ErrorCount);

    if (errors >= CLASS_ERROR_LEVEL_1)
    {
        // If the error count has exceeded the error limit, then disable
        // any tagged queuing, multiple requests per lu queueing
        // and sychronous data transfers.
        //
        // Clearing the no queue freeze flag prevents the port driver
        // from sending multiple requests per logical unit.
        CLEAR_FLAG(DeviceExtension->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
        CLEAR_FLAG(DeviceExtension->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);

        SET_FLAG(DeviceExtension->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                   "PerfIncrementErrorCount: Too many errors; disabling tagged queuing and "
                   "synchronous data tranfers.\n"));
    }

    if (errors >= CLASS_ERROR_LEVEL_2)
    {
        // If a second threshold is reached, disable disconnects.
        SET_FLAG(DeviceExtension->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT);
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                   "PerfIncrementErrorCount: Too many errors; disabling disconnects.\n"));
    }

    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    return;
}


_IRQL_requires_max_(APC_LEVEL)
PVOID
DeviceFindFeaturePage(
    _In_reads_bytes_(Length) PGET_CONFIGURATION_HEADER   FeatureBuffer,
    _In_ ULONG const                                Length,
    _In_ FEATURE_NUMBER const                       Feature
    )
/*++
Routine Description:

    find the specific feature page in the buffer

Arguments:

    FeatureBuffer - buffer contains the device feature set.

    Length - buffer length

    Feature - the feature number looking for.

Return Value:

    PVOID - pointer to the starting location of the specific feature in buffer.

--*/
{
    PUCHAR buffer;
    PUCHAR limit;
    ULONG  validLength;

    PAGED_CODE();

    if (Length < sizeof(GET_CONFIGURATION_HEADER) + sizeof(FEATURE_HEADER))
    {
        return NULL;
    }

    // Calculate the length of valid data available in the
    // capabilities buffer from the DataLength field
    REVERSE_BYTES(&validLength, FeatureBuffer->DataLength);

    validLength += RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);

    // set limit to point to first illegal address
    limit  = (PUCHAR)FeatureBuffer;
    limit += min(Length, validLength);

    // set buffer to point to first page
    buffer = FeatureBuffer->Data;

    // loop through each page until we find the requested one, or
    // until it's not safe to access the entire feature header
    // (if equal, have exactly enough for the feature header)
    while (buffer + sizeof(FEATURE_HEADER) <= limit)
    {
        PFEATURE_HEADER header = (PFEATURE_HEADER)buffer;
        FEATURE_NUMBER  thisFeature;

        thisFeature  = (header->FeatureCode[0] << 8) |
                       (header->FeatureCode[1]);

        if (thisFeature == Feature)
        {
            PUCHAR temp;

            // if don't have enough memory to safely access all the feature
            // information, return NULL
            temp = buffer;
            temp += sizeof(FEATURE_HEADER);
            temp += header->AdditionalLength;

            if (temp > limit)
            {
                // this means the transfer was cut-off, an insufficiently
                // small buffer was given, or other arbitrary error.  since
                // it's not safe to view the amount of data (even though
                // the header is safe) in this feature, pretend it wasn't
                // transferred at all...
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                           "Feature %x exists, but not safe to access all its data.  returning NULL\n",
                           Feature));
                return NULL;
            }
            else
            {
                return buffer;
            }
        }

        if ((header->AdditionalLength % 4) &&
            !(Feature >= 0xff00 && Feature <= 0xffff))
        {
            return NULL;
        }

        buffer += sizeof(FEATURE_HEADER);
        buffer += header->AdditionalLength;
    }

    return NULL;
}


_IRQL_requires_max_(APC_LEVEL)
VOID
DevicePrintAllFeaturePages(
    _In_reads_bytes_(Usable) PGET_CONFIGURATION_HEADER   Buffer,
    _In_ ULONG const                                Usable
    )
/*++
Routine Description:

    print out all feature pages in the buffer

Arguments:

    Buffer - buffer contains the device feature set.

    Usable -

Return Value:

    none

--*/
{
#if DBG
    PFEATURE_HEADER header;

    PAGED_CODE();

    ////////////////////////////////////////////////////////////////////////////////
    // items expected to ALWAYS be current if they exist
    ////////////////////////////////////////////////////////////////////////////////

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureProfileList);
    if (header != NULL) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: CurrentProfile %x "
                   "with %x bytes of data at %p\n",
                   Buffer->CurrentProfile[0] << 8 |
                   Buffer->CurrentProfile[1],
                   Usable, Buffer));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureCore);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CORE Features"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureMorphing);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Morphing"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureRemovableMedium);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Removable Medium"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeaturePowerManagement);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Power Management"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureEmbeddedChanger);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Embedded Changer"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureMicrocodeUpgrade);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Microcode Update"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureTimeout);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Timeouts"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureLogicalUnitSerialNumber);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "LUN Serial Number"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureFirmwareDate);
    if (header) {

        ULONG featureSize = header->AdditionalLength;
        featureSize += RTL_SIZEOF_THROUGH_FIELD(FEATURE_HEADER, AdditionalLength);

        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                   "Currently supports" : "Is able to support"),
                   "Firmware Date"
                   ));

        if (featureSize >= RTL_SIZEOF_THROUGH_FIELD(FEATURE_DATA_FIRMWARE_DATE, Minute))
        {
            PFEATURE_DATA_FIRMWARE_DATE date = (PFEATURE_DATA_FIRMWARE_DATE)header;
            // show date as "YYYY/MM/DD hh:mm", which is 18 chars (17+NULL)
            UCHAR dateString[18] = { 0 };
            dateString[ 0] = date->Year[0];
            dateString[ 1] = date->Year[1];
            dateString[ 2] = date->Year[2];
            dateString[ 3] = date->Year[3];
            dateString[ 4] = '/';
            dateString[ 5] = date->Month[0];
            dateString[ 6] = date->Month[1];
            dateString[ 7] = '/';
            dateString[ 8] = date->Day[0];
            dateString[ 9] = date->Day[1];
            dateString[10] = ' ';
            dateString[11] = ' ';
            dateString[12] = date->Hour[0];
            dateString[13] = date->Hour[1];
            dateString[14] = ':';
            dateString[15] = date->Minute[0];
            dateString[16] = date->Minute[1];
            dateString[17] = 0;
            // SECONDS IS NOT AVAILABLE ON EARLY IMPLEMENTATIONS -- ignore it
            //dateString[17] = ':';
            //dateString[18] = date->Seconds[0];
            //dateString[19] = date->Seconds[1];
            //dateString[20] = 0;
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                        "CdromGetConfiguration: Firmware Date/Time %s (UTC)\n",
                        (PCSTR)dateString
                        ));
        }
    }

////////////////////////////////////////////////////////////////////////////////
// items expected not to always be current
////////////////////////////////////////////////////////////////////////////////


    header = DeviceFindFeaturePage(Buffer, Usable, FeatureWriteProtect);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Software Write Protect"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureRandomReadable);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Random Reads"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureMultiRead);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Multi-Read"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureCdRead);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "reading from CD-ROM/R/RW"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDvdRead);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD Structure Reads"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureRandomWritable);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Random Writes"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureIncrementalStreamingWritable);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Incremental Streaming Writing"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureSectorErasable);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Sector Erasable Media"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureFormattable);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Formatting"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDefectManagement);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "defect management"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureWriteOnce);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Write Once Media"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureRestrictedOverwrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Restricted Overwrites"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureCdrwCAVWrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CD-RW CAV recording"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureMrw);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Mount Rainier media"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureEnhancedDefectReporting);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Enhanced Defect Reporting"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDvdPlusRW);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD+RW media"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureRigidRestrictedOverwrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Rigid Restricted Overwrite"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureCdTrackAtOnce);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CD Recording (Track At Once)"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureCdMastering);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CD Recording (Mastering)"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDvdRecordableWrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD Recording (Mastering)"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDDCDRead);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DD CD Reading"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDDCDRWrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DD CD-R Writing"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDDCDRWWrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DD CD-RW Writing"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureLayerJumpRecording);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Layer Jump Recording"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureHDDVDRead);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "HD-DVD Reading"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureHDDVDWrite);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "HD-DVD Writing"
                   ));
    }


    header = DeviceFindFeaturePage(Buffer, Usable, FeatureSMART);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "S.M.A.R.T."
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureCDAudioAnalogPlay);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Analogue CD Audio Operations"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDvdCSS);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD CSS"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureRealTimeStreaming);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Real-time Streaming Reads"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDiscControlBlocks);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD Disc Control Blocks"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureDvdCPRM);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD CPRM"
                   ));
    }

    header = DeviceFindFeaturePage(Buffer, Usable, FeatureAACS);
    if (header) {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "AACS"
                   ));
    }

#else
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Usable);
    UNREFERENCED_PARAMETER(Buffer);

#endif // DBG
    return;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
MediaReadCapacity(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    Get media capacity

Arguments:

    Device - the device that owns the media

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    SCSI_REQUEST_BLOCK      srb;
    PCDB                    cdb = NULL;
    READ_CAPACITY_DATA      capacityData;

    PAGED_CODE();

    RtlZeroMemory(&srb, sizeof(srb));
    RtlZeroMemory(&capacityData, sizeof(capacityData));

    cdb = (PCDB)(&srb.Cdb);

    //Prepare SCSI command fields
    srb.CdbLength = 10;
    srb.TimeOutValue = CDROM_READ_CAPACITY_TIMEOUT;
    cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;

    status = DeviceSendSrbSynchronously(Device,
                                        &srb,
                                        &capacityData,
                                        sizeof(READ_CAPACITY_DATA),
                                        FALSE,
                                        NULL);

    //Remember the result
    if (!NT_SUCCESS(status))
    {
        //Set the BytesPerBlock to zero, this is for safe as if error happens this field should stay zero (no change).
        //it will be treated as error case in MediaReadCapacityDataInterpret()
        capacityData.BytesPerBlock = 0;
    }

    MediaReadCapacityDataInterpret(Device, &capacityData);

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
VOID
MediaReadCapacityDataInterpret(
    _In_ WDFDEVICE            Device,
    _In_ PREAD_CAPACITY_DATA  ReadCapacityBuffer
    )
/*++
Routine Description:

    Interpret media capacity and set corresponding fields in device context

Arguments:

    Device - the device that owns the media

    ReadCapacityBuffer - data buffer of capacity

Return Value:

    none

--*/
{
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    ULONG                   lastSector = 0;
    ULONG                   bps = 0;
    ULONG                   lastBit = 0;
    ULONG                   bytesPerBlock = 0;
    BOOLEAN                 errorHappened = FALSE;

    PAGED_CODE();

    NT_ASSERT(ReadCapacityBuffer);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                "MediaReadCapacityDataInterpret: Entering\n"));

    // Swizzle bytes from Read Capacity and translate into
    // the necessary geometry information in the device extension.
    bytesPerBlock = ReadCapacityBuffer->BytesPerBlock;

    ((PFOUR_BYTE)&bps)->Byte0 = ((PFOUR_BYTE)&bytesPerBlock)->Byte3;
    ((PFOUR_BYTE)&bps)->Byte1 = ((PFOUR_BYTE)&bytesPerBlock)->Byte2;
    ((PFOUR_BYTE)&bps)->Byte2 = ((PFOUR_BYTE)&bytesPerBlock)->Byte1;
    ((PFOUR_BYTE)&bps)->Byte3 = ((PFOUR_BYTE)&bytesPerBlock)->Byte0;

    // Insure that bps is a power of 2.
    // This corrects a problem with the HP 4020i CDR where it
    // returns an incorrect number for bytes per sector.
    if (!bps)
    {
        // Set disk geometry to default values (per ISO 9660).
        bps = 2048;
        errorHappened = TRUE;
    }
    else
    {
        lastBit = (ULONG)(-1);
        while (bps)
        {
            lastBit++;
            bps = (bps >> 1);
        }
        bps = (1 << lastBit);
    }

    deviceExtension->DiskGeometry.BytesPerSector = bps;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
               "MediaReadCapacityDataInterpret: Calculated bps %#x\n",
                deviceExtension->DiskGeometry.BytesPerSector));

    // Copy last sector in reverse byte order.
    bytesPerBlock = ReadCapacityBuffer->LogicalBlockAddress;

    ((PFOUR_BYTE)&lastSector)->Byte0 = ((PFOUR_BYTE)&bytesPerBlock)->Byte3;
    ((PFOUR_BYTE)&lastSector)->Byte1 = ((PFOUR_BYTE)&bytesPerBlock)->Byte2;
    ((PFOUR_BYTE)&lastSector)->Byte2 = ((PFOUR_BYTE)&bytesPerBlock)->Byte1;
    ((PFOUR_BYTE)&lastSector)->Byte3 = ((PFOUR_BYTE)&bytesPerBlock)->Byte0;

    // Calculate sector to byte shift.
    WHICH_BIT(bps, deviceExtension->SectorShift);

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL,
               "MediaReadCapacityDataInterpret: Sector size is %d\n",
               deviceExtension->DiskGeometry.BytesPerSector));

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
               "MediaReadCapacityDataInterpret: Number of Sectors is %d\n",
               lastSector + 1));

    // Calculate media capacity in bytes.
    if (errorHappened)
    {
        // Set disk geometry to default values (per ISO 9660).
        deviceExtension->PartitionLength.QuadPart = (LONGLONG)(0x7fffffff);
    }
    else
    {
        deviceExtension->PartitionLength.QuadPart = (LONGLONG)(lastSector + 1);
        deviceExtension->PartitionLength.QuadPart =
                    (deviceExtension->PartitionLength.QuadPart << deviceExtension->SectorShift);
    }

    // we've defaulted to 32/64 forever.  don't want to change this now...
    deviceExtension->DiskGeometry.TracksPerCylinder = 0x40;
    deviceExtension->DiskGeometry.SectorsPerTrack = 0x20;

    // Calculate number of cylinders.
    deviceExtension->DiskGeometry.Cylinders.QuadPart = (LONGLONG)((lastSector + 1) / (32 * 64));

    deviceExtension->DiskGeometry.MediaType = RemovableMedia;

    return;
}


_IRQL_requires_max_(APC_LEVEL)
VOID
DevicePickDvdRegion(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    pick a default dvd region

Arguments:

    Device - Device Object

Return Value:

    none

--*/
{
    NTSTATUS                status;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);

    // these five pointers all point to dvdReadStructure or part of
    // its data, so don't deallocate them more than once!
    PDVD_READ_STRUCTURE         dvdReadStructure;
    PDVD_COPY_PROTECT_KEY       copyProtectKey;
    PDVD_COPYRIGHT_DESCRIPTOR   dvdCopyRight;
    PDVD_RPC_KEY                rpcKey;
    PDVD_SET_RPC_KEY            dvdRpcKey;

    size_t          bytesReturned = 0;
    ULONG           bufferLen = 0;
    UCHAR           mediaRegion = 0;
    ULONG           pickDvdRegion = 0;
    ULONG           defaultDvdRegion = 0;
    ULONG           dvdRegion = 0;
    WDFKEY          registryKey = NULL;

    DECLARE_CONST_UNICODE_STRING(registryValueName, DVD_DEFAULT_REGION);

    PAGED_CODE();

    if ((pickDvdRegion = InterlockedExchange((PLONG)&deviceExtension->DeviceAdditionalData.PickDvdRegion, 0)) == 0)
    {
        // it was non-zero, so either another thread will do this, or
        // we no longer need to pick a region
        return;
    }

    bufferLen = max(
                    max(sizeof(DVD_DESCRIPTOR_HEADER) +
                            sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                        sizeof(DVD_READ_STRUCTURE)
                        ),
                    max(DVD_RPC_KEY_LENGTH,
                        DVD_SET_RPC_KEY_LENGTH
                        )
                    );

    dvdReadStructure = (PDVD_READ_STRUCTURE)
                        ExAllocatePoolWithTag(PagedPool, bufferLen, DVD_TAG_DVD_REGION);

    if (dvdReadStructure == NULL)
    {
        InterlockedExchange((PLONG)&deviceExtension->DeviceAdditionalData.PickDvdRegion, pickDvdRegion);
        return;
    }

    copyProtectKey = (PDVD_COPY_PROTECT_KEY)dvdReadStructure;

    dvdCopyRight = (PDVD_COPYRIGHT_DESCRIPTOR)
                        ((PDVD_DESCRIPTOR_HEADER)dvdReadStructure)->Data;

    // get the media region
    RtlZeroMemory (dvdReadStructure, bufferLen);
    dvdReadStructure->Format = DvdCopyrightDescriptor;

    // Build and send a request for READ_KEY
    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "DevicePickDvdRegion (%p): Getting Copyright Descriptor\n",
                Device));

    status = ReadDvdStructure(deviceExtension,
                              NULL,
                              dvdReadStructure,
                              sizeof(DVD_READ_STRUCTURE),
                              dvdReadStructure,
                              sizeof(DVD_DESCRIPTOR_HEADER) + sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                              &bytesReturned);

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "DevicePickDvdRegion (%p): Got Copyright Descriptor %x\n",
                Device, status));

    if ((NT_SUCCESS(status)) &&
        (dvdCopyRight->CopyrightProtectionType == 0x01))
    {
        // keep the media region bitmap around
        // a 1 means ok to play
        if (dvdCopyRight->RegionManagementInformation == 0xff)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                      "DevicePickDvdRegion (%p): RegionManagementInformation "
                      "is set to dis-allow playback for all regions.  This is "
                      "most likely a poorly authored disc.  defaulting to all "
                      "region disc for purpose of choosing initial region\n",
                      Device));
            dvdCopyRight->RegionManagementInformation = 0;
        }

        mediaRegion = ~dvdCopyRight->RegionManagementInformation;
    }
    else
    {
        // can't automatically pick a default region on a drive without media, so just exit
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): failed to auto-choose a region due to status %x getting copyright descriptor\n",
                    Device, status));
        goto getout;
    }

    // get the device region
    RtlZeroMemory (copyProtectKey, bufferLen);
    copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdGetRpcKey;

    // Build and send a request for READ_KEY for RPC key
    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "DevicePickDvdRegion (%p): Getting RpcKey\n",
                Device));
    status = DvdStartSessionReadKey(deviceExtension,
                                    IOCTL_DVD_READ_KEY,
                                    NULL,
                                    copyProtectKey,
                                    DVD_RPC_KEY_LENGTH,
                                    copyProtectKey,
                                    DVD_RPC_KEY_LENGTH,
                                    &bytesReturned);

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "DevicePickDvdRegion (%p): Got RpcKey %x\n",
                Device, status));

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): failed to get RpcKey from "
                    "a DVD Device\n", Device));
        goto getout;
    }

    // so we now have what we can get for the media region and the
    // drive region.  we will not set a region if the drive has one
    // set already (mask is not all 1's), nor will we set a region
    // if there are no more user resets available.
    rpcKey = (PDVD_RPC_KEY)copyProtectKey->KeyData;

    if (rpcKey->RegionMask != 0xff)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): not picking a region since "
                    "it is already chosen\n", Device));
        goto getout;
    }

    if (rpcKey->UserResetsAvailable <= 1)
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): not picking a region since "
                    "only one change remains\n", Device));
        goto getout;
    }

    // OOBE sets this key based upon the system locale
    status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(),
                                                KEY_READ,
                                                WDF_NO_OBJECT_ATTRIBUTES,
                                                &registryKey);

    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryULong(registryKey,
                                       &registryValueName,
                                       &defaultDvdRegion);

        WdfRegistryClose(registryKey);
    }

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): failed to read registry value due to status %x\n",
                    Device, status));

        // by default the default Dvd region is 0
        defaultDvdRegion = 0;
        status = STATUS_SUCCESS;
    }

    if (defaultDvdRegion > DVD_MAX_REGION)
    {
        // the registry has a bogus default
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): registry has a bogus default "
                    "region value of %x\n", Device, defaultDvdRegion));

        defaultDvdRegion = 0;
    }

    // if defaultDvdRegion == 0, it means no default.

    // we will select the initial dvd region for the user

    if ((defaultDvdRegion != 0) &&
        (mediaRegion & (1 << (defaultDvdRegion - 1))))
    {
        // first choice:
        // the media has region that matches
        // the default dvd region.
        dvdRegion = (1 << (defaultDvdRegion - 1));

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): Choice #1: media matches "
                    "drive's default, chose region %x\n", Device, dvdRegion));
    }
    else if (mediaRegion)
    {
        // second choice:
        // pick the lowest region number from the media
        UCHAR mask = 1;
        dvdRegion = 0;

        while (mediaRegion && !dvdRegion)
        {
            // pick the lowest bit
            dvdRegion = mediaRegion & mask;
            mask <<= 1;
        }

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): Choice #2: choosing lowest "
                    "media region %x\n", Device, dvdRegion));
    }
    else if (defaultDvdRegion)
    {
        // third choice:
        // default dvd region from the dvd class installer
        dvdRegion = (1 << (defaultDvdRegion - 1));
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): Choice #3: using default "
                    "region for this install %x\n", Device, dvdRegion));
    }
    else
    {
        // unable to pick one for the user -- this should rarely
        // happen, since the proppage dvd class installer sets
        // the key based upon the system locale
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "DevicePickDvdRegion (%p): Choice #4: failed to choose "
                    "a media region\n", Device));
        goto getout;
    }

    // now that we've chosen a region, set the region by sending the
    // appropriate request to the drive
    RtlZeroMemory (copyProtectKey, bufferLen);
    copyProtectKey->KeyLength = DVD_SET_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdSetRpcKey;
    dvdRpcKey = (PDVD_SET_RPC_KEY)copyProtectKey->KeyData;
    dvdRpcKey->PreferredDriveRegionCode = (UCHAR)~dvdRegion;

    // Build and send request for SEND_KEY
    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "DevicePickDvdRegion (%p): Sending new Rpc Key to region %x\n",
                Device, dvdRegion));

    status = DvdSendKey(deviceExtension,
                        NULL,
                        copyProtectKey,
                        DVD_SET_RPC_KEY_LENGTH,
                        &bytesReturned);

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "DevicePickDvdRegion (%p): Sent new Rpc Key %x\n",
                Device, status));

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL, "DevicePickDvdRegion (%p): unable to set dvd initial "
                     " region code (%x)\n", Device, status));
    }
    else
    {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL, "DevicePickDvdRegion (%p): Successfully set dvd "
                     "initial region\n", Device));

        pickDvdRegion = 0;
    }

getout:
    if (dvdReadStructure)
    {
        FREE_POOL(dvdReadStructure);
    }

    // update the new PickDvdRegion value
    InterlockedExchange((PLONG)&deviceExtension->DeviceAdditionalData.PickDvdRegion, pickDvdRegion);

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRegisterInterface(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ CDROM_DEVICE_INTERFACES InterfaceType
    )
/*++
Routine Description:

    used to register device class interface or mount device interface

Arguments:

    DeviceExtension - device context

    InterfaceType - interface type to be registered.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    WDFSTRING       string = NULL;
    GUID*           interfaceGuid = NULL;
    PUNICODE_STRING savingString = NULL;
    BOOLEAN         setRestricted = FALSE;
    UNICODE_STRING  localString;

    PAGED_CODE();

    //Get parameters
    switch(InterfaceType)
    {
    case CdRomDeviceInterface:
        interfaceGuid = (LPGUID)&GUID_DEVINTERFACE_CDROM;
        setRestricted = TRUE;
        savingString = &localString;
        break;
    case MountedDeviceInterface:
        interfaceGuid = (LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID;
        savingString = &(DeviceExtension->MountedDeviceInterfaceName);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    status = WdfDeviceCreateDeviceInterface(DeviceExtension->Device,
                                            interfaceGuid,
                                            NULL);

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                    "DeviceRegisterInterface: Unable to register cdrom "
                    "DCA for fdo %p type: %s [%lx]\n",
                    DeviceExtension->Device,
                    (InterfaceType == CdRomDeviceInterface)? "CdRom Interface" : "Mounted Device Interface",
                    status));
    }

    // Retrieve interface string
    if (NT_SUCCESS(status))
    {
        // The string object will be released when its parent object is released.
        WDF_OBJECT_ATTRIBUTES  attributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = DeviceExtension->Device;

        status = WdfStringCreate(WDF_NO_OBJECT_ATTRIBUTES,
                                 NULL,
                                 &string);
    }

    if (NT_SUCCESS(status))
    {
        status = WdfDeviceRetrieveDeviceInterfaceString(DeviceExtension->Device,
                                                        interfaceGuid,
                                                        NULL,
                                                        string);
    }

    if (NT_SUCCESS(status))
    {
        WdfStringGetUnicodeString(string, savingString);

        if (setRestricted) {


            WdfObjectDelete(string);
        }
    }

    return status;
} // end DeviceRegisterInterface()


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceRestoreDefaultSpeed(
    _In_ WDFWORKITEM  WorkItem
    )
/*++

Routine Description:

    This workitem is called on a media change when the CDROM device
    speed should be restored to the default value.

Arguments:

    Fdo      - Supplies the device object for the CDROM device.
    WorkItem - Supplies the pointer to the workitem.

Return Value:

    None

--*/
{
    NTSTATUS                status;
    WDFDEVICE               device = WdfWorkItemGetParentObject(WorkItem);
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(device);
    PPERFORMANCE_DESCRIPTOR perfDescriptor;
    ULONG                   transferLength = sizeof(PERFORMANCE_DESCRIPTOR);
    SCSI_REQUEST_BLOCK      srb = {0};
    PCDB                    cdb = (PCDB)srb.Cdb;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DeviceRestoreDefaultSpeed: Restore device speed for %p\n", device));

    perfDescriptor = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                           transferLength,
                                           CDROM_TAG_STREAM);
    if (perfDescriptor == NULL)
    {
        return;
    }

    RtlZeroMemory(perfDescriptor, transferLength);

    perfDescriptor->RestoreDefaults = TRUE;

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    srb.CdbLength = 12;
    cdb->SET_STREAMING.OperationCode = SCSIOP_SET_STREAMING;
    REVERSE_BYTES_SHORT(&cdb->SET_STREAMING.ParameterListLength, &transferLength);

    status = DeviceSendSrbSynchronously(device,
                                        &srb,
                                        perfDescriptor,
                                        transferLength,
                                        TRUE,
                                        NULL);
    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
               "DeviceRestoreDefaultSpeed: Set Streaming command completed with status: 0x%X\n", status));

    FREE_POOL(perfDescriptor);
    WdfObjectDelete(WorkItem);

    return;
}

// custom string match -- careful!
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
StringsAreMatched(
    _In_opt_z_ PCHAR StringToMatch,
    _In_z_     PCHAR TargetString
    )
/*++

Routine Description:

    compares if two strings are identical

Arguments:

    StringToMatch - source string.
    TargetString - target string.

Return Value:

    BOOLEAN - TRUE (identical); FALSE (not match)

--*/
{
    size_t length;

    PAGED_CODE();

    NT_ASSERT(TargetString);

    // if no match requested, return TRUE
    if (StringToMatch == NULL)
    {
        return TRUE;
    }

    // cache the string length for efficiency
    length = strlen(StringToMatch);

    // ZERO-length strings may only match zero-length strings
    if (length == 0)
    {
        return (strlen(TargetString) == 0);
    }

    // strncmp returns zero if the strings match
    return (strncmp(StringToMatch, TargetString, length) == 0);
}


NTSTATUS
RequestSetContextFields(
    _In_ WDFREQUEST    Request,
    _In_ PSYNC_HANDLER Handler
    )
/*++

Routine Description:

    set the request object context fields

Arguments:

    Request - request object.
    Handler - the function that finally handles this request.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(Request);
    PKEVENT                 syncEvent = NULL;

    syncEvent = ExAllocatePoolWithTag(NonPagedPoolNx,
                                      sizeof(KEVENT),
                                      CDROM_TAG_SYNC_EVENT);

    if (syncEvent == NULL)
    {
        // memory allocation failed.
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        // now, put the special synchronization information into the context
        requestContext->SyncRequired = TRUE;
        requestContext->SyncEvent = syncEvent;
        requestContext->SyncCallback = Handler;

        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
RequestDuidGetDeviceIdProperty(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ WDFREQUEST              Request,
    _In_ WDF_REQUEST_PARAMETERS  RequestParameters,
    _Out_ size_t *               DataLength
    )
/*++

Routine Description:



Arguments:

    DeviceExtension - device context
    Request - request object.
    RequestParameters - request parameter
    DataLength - transferred data length.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PSTORAGE_DEVICE_ID_DESCRIPTOR   deviceIdDescriptor = NULL;
    PSTORAGE_DESCRIPTOR_HEADER      descHeader = NULL;
    STORAGE_PROPERTY_ID             propertyId = StorageDeviceIdProperty;

    *DataLength = 0;

    // Get the VPD page 83h data.
    status = DeviceRetrieveDescriptor(DeviceExtension->Device,
                                      &propertyId,
                                      (PSTORAGE_DESCRIPTOR_HEADER*)&deviceIdDescriptor);

    if (NT_SUCCESS(status) && (deviceIdDescriptor == NULL))
    {
        status = STATUS_NOT_FOUND;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &descHeader,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        PSTORAGE_DEVICE_UNIQUE_IDENTIFIER   storageDuid = NULL;
        ULONG                               offset = descHeader->Size;
        PUCHAR                              dest = (PUCHAR)descHeader + offset;
        size_t                              outputBufferSize;

        outputBufferSize = RequestParameters.Parameters.DeviceIoControl.OutputBufferLength;

        // Adjust required size and potential destination location.
        status = RtlULongAdd(descHeader->Size, deviceIdDescriptor->Size, &descHeader->Size);

        if (NT_SUCCESS(status) &&
            (outputBufferSize < descHeader->Size))
        {
            // Output buffer is too small.  Return error and make sure
            // the caller gets info about required buffer size.
            *DataLength = descHeader->Size;
            status = STATUS_BUFFER_OVERFLOW;
        }

        if (NT_SUCCESS(status))
        {
            storageDuid = (PSTORAGE_DEVICE_UNIQUE_IDENTIFIER)descHeader;
            storageDuid->StorageDeviceIdOffset = offset;

            RtlCopyMemory(dest,
                          deviceIdDescriptor,
                          deviceIdDescriptor->Size);

            *DataLength = storageDuid->Size;
            status = STATUS_SUCCESS;
        }

        FREE_POOL(deviceIdDescriptor);
    }

    return status;
}

NTSTATUS
RequestDuidGetDeviceProperty(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ WDFREQUEST              Request,
    _In_ WDF_REQUEST_PARAMETERS  RequestParameters,
    _Out_ size_t *               DataLength
    )
/*++

Routine Description:



Arguments:

    DeviceExtension - device context
    Request - request object.
    RequestParameters - request parameter
    DataLength - transferred data length.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                          status = STATUS_SUCCESS;
    PSTORAGE_DEVICE_DESCRIPTOR        deviceDescriptor = DeviceExtension->DeviceDescriptor;
    PSTORAGE_DESCRIPTOR_HEADER        descHeader = NULL;
    PSTORAGE_DEVICE_UNIQUE_IDENTIFIER storageDuid;
    PUCHAR                            dest = NULL;

    if (deviceDescriptor == NULL)
    {
        status = STATUS_NOT_FOUND;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &descHeader,
                                                NULL);
    }

    if (NT_SUCCESS(status) &&
        (deviceDescriptor->SerialNumberOffset == 0))
    {
        status = STATUS_NOT_FOUND;
    }

    // Use this info only if serial number is available.
    if (NT_SUCCESS(status))
    {
        ULONG offset = descHeader->Size;
        size_t outputBufferSize = RequestParameters.Parameters.DeviceIoControl.OutputBufferLength;

        // Adjust required size and potential destination location.
        dest = (PUCHAR)descHeader + offset;

        status = RtlULongAdd(descHeader->Size, deviceDescriptor->Size, &descHeader->Size);

        if (NT_SUCCESS(status) &&
            (outputBufferSize < descHeader->Size))
        {
            // Output buffer is too small.  Return error and make sure
            // the caller get info about required buffer size.
            *DataLength = descHeader->Size;
            status = STATUS_BUFFER_OVERFLOW;
        }

        if (NT_SUCCESS(status))
        {
            storageDuid = (PSTORAGE_DEVICE_UNIQUE_IDENTIFIER)descHeader;
            storageDuid->StorageDeviceOffset = offset;

            RtlCopyMemory(dest,
                          deviceDescriptor,
                          deviceDescriptor->Size);

            *DataLength = storageDuid->Size;
            status = STATUS_SUCCESS;
        }
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
ULONG
DeviceRetrieveModeSenseUsingScratch(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_reads_bytes_(Length) PCHAR     ModeSenseBuffer,
    _In_ ULONG                    Length,
    _In_ UCHAR                    PageCode,
    _In_ UCHAR                    PageControl
    )
/*++

Routine Description:

    retrieve mode sense informaiton of the device

Arguments:

    DeviceExtension - device context
    ModeSenseBuffer - buffer to savee the mode sense info.
    Length - buffer length
    PageCode - .
    PageControl -

Return Value:

    ULONG - transferred data length

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       transferSize = min(Length, DeviceExtension->ScratchContext.ScratchBufferSize);
    CDB                         cdb;

    PAGED_CODE();

    ScratchBuffer_BeginUse(DeviceExtension);

    RtlZeroMemory(&cdb, sizeof(CDB));
    // Set up the CDB
    cdb.MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb.MODE_SENSE.PageCode = PageCode;
    cdb.MODE_SENSE.Pc = PageControl;
    cdb.MODE_SENSE.AllocationLength = (UCHAR)transferSize;

    status = ScratchBuffer_ExecuteCdb(DeviceExtension, NULL, transferSize, TRUE, &cdb, 6);

    if (NT_SUCCESS(status))
    {
        transferSize = min(Length, DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength);
        RtlCopyMemory(ModeSenseBuffer,
                      DeviceExtension->ScratchContext.ScratchBuffer,
                      transferSize);
    }

    ScratchBuffer_EndUse(DeviceExtension);

    return transferSize;
}

_IRQL_requires_max_(APC_LEVEL)
PVOID
ModeSenseFindSpecificPage(
    _In_reads_bytes_(Length) PCHAR     ModeSenseBuffer,
    _In_ size_t                   Length,
    _In_ UCHAR                    PageMode,
    _In_ BOOLEAN                  Use6BytesCdb
    )
/*++

Routine Description:

    This routine scans through the mode sense data and finds the requested
    mode sense page code.

Arguments:
    ModeSenseBuffer - Supplies a pointer to the mode sense data.

    Length - Indicates the length of valid data.

    PageMode - Supplies the page mode to be searched for.

    Use6BytesCdb - Indicates whether 6 or 10 byte mode sense was used.

Return Value:

    A pointer to the the requested mode page.  If the mode page was not found
    then NULL is return.

--*/
{
    PCHAR limit;
    ULONG  parameterHeaderLength;
    PVOID  result = NULL;

    PAGED_CODE();

    limit = ModeSenseBuffer + Length;
    parameterHeaderLength = (Use6BytesCdb)
                            ? sizeof(MODE_PARAMETER_HEADER)
                            : sizeof(MODE_PARAMETER_HEADER10);

    if (Length >= parameterHeaderLength)
    {
        PMODE_PARAMETER_HEADER10 modeParam10;
        ULONG                    blockDescriptorLength;

        //  Skip the mode select header and block descriptors.
        if (Use6BytesCdb)
        {
            blockDescriptorLength = ((PMODE_PARAMETER_HEADER)ModeSenseBuffer)->BlockDescriptorLength;
        }
        else
        {
            modeParam10 = (PMODE_PARAMETER_HEADER10) ModeSenseBuffer;
            blockDescriptorLength = modeParam10->BlockDescriptorLength[1];
        }

        ModeSenseBuffer += parameterHeaderLength + blockDescriptorLength;

        // ModeSenseBuffer now points at pages.  Walk the pages looking for the
        // requested page until the limit is reached.
        while (ModeSenseBuffer +
               RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength) < limit)
        {
            if (((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageCode == PageMode)
            {
                // found the mode page.  make sure it's safe to touch it all
                // before returning the pointer to caller
                if (ModeSenseBuffer + ((PMODE_DISCONNECT_PAGE)ModeSenseBuffer)->PageLength > limit)
                {
                    //  Return NULL since the page is not safe to access in full
                    result = NULL;
                }
                else
                {
                    result = ModeSenseBuffer;
                }
                break;
            }

            // Advance to the next page which is 4-byte-aligned offset after this page.
            ModeSenseBuffer += ((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageLength +
                                RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength);
        }
    }

    return result;
} // end ModeSenseFindSpecificPage()


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PerformEjectionControl(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ WDFREQUEST               Request,
    _In_ MEDIA_LOCK_TYPE          LockType,
    _In_ BOOLEAN                  Lock
    )
/*++

Routine Description:

    ejection control process

Arguments:

    DeviceExtension - device extension
    Request - WDF request to be used for communication with the device
    LockType - the type of lock
    Lock - if TRUE, lock the device; if FALSE, unlock it

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PFILE_OBJECT_CONTEXT    fileObjectContext = NULL;
    SCSI_REQUEST_BLOCK      srb;
    PCDB                    cdb = NULL;

    LONG                    newLockCount = 0;
    LONG                    newProtectedLockCount = 0;
    LONG                    newInternalLockCount = 0;
    LONG                    newFileLockCount = 0;
    BOOLEAN                 countChanged = FALSE;
    BOOLEAN                 previouslyLocked = FALSE;
    BOOLEAN                 nowLocked = FALSE;

    PAGED_CODE();

    // Prevent race conditions while working with lock counts
    status = WdfWaitLockAcquire(DeviceExtension->EjectSynchronizationLock, NULL);
    if (!NT_SUCCESS(status))
    {
        NT_ASSERT(FALSE);
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                "PerformEjectionControl: "
                "Received request for %s lock type\n",
                LockTypeStrings[LockType]
                ));


    // If this is a "secured" request, retrieve the file object context
    if (LockType == SecureMediaLock)
    {
        WDFFILEOBJECT fileObject = NULL;

        fileObject = WdfRequestGetFileObject(Request);

        if (fileObject == NULL)
        {
            status = STATUS_INVALID_HANDLE;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "FileObject does not match to the one in IRP_MJ_CREATE, KMDF returns NULL\n"));
            goto Exit;
        }

        fileObjectContext = FileObjectGetContext(fileObject);
        NT_ASSERT(fileObjectContext != NULL);
    }

    // Lock counts should never fall below 0
    NT_ASSERT(DeviceExtension->LockCount >= 0);
    NT_ASSERT(DeviceExtension->ProtectedLockCount >= 0);
    NT_ASSERT(DeviceExtension->InternalLockCount >= 0);

    // Get the current lock counts
    newLockCount = DeviceExtension->LockCount;
    newProtectedLockCount = DeviceExtension->ProtectedLockCount;
    newInternalLockCount = DeviceExtension->InternalLockCount;
    if (fileObjectContext)
    {
        // fileObjectContext->LockCount is ULONG and should always >= 0
        newFileLockCount = fileObjectContext->LockCount;
    }

    // Determine which lock counts need to be changed and how
    if (Lock && LockType == SimpleMediaLock)
    {
        newLockCount++;
        countChanged = TRUE;
    }
    else if (Lock && LockType == SecureMediaLock)
    {
        newFileLockCount++;
        newProtectedLockCount++;
        countChanged = TRUE;
    }
    else if (Lock && LockType == InternalMediaLock)
    {
        newInternalLockCount++;
        countChanged = TRUE;
    }
    else if (!Lock && LockType == SimpleMediaLock)
    {
        if (newLockCount != 0)
        {
            newLockCount--;
            countChanged = TRUE;
        }
    }
    else if (!Lock && LockType == SecureMediaLock)
    {
        if ( (newFileLockCount == 0) || (newProtectedLockCount == 0) )
        {
            status = STATUS_INVALID_DEVICE_STATE;
            goto Exit;
        }
        newFileLockCount--;
        newProtectedLockCount--;
        countChanged = TRUE;
    }
    else if (!Lock && LockType == InternalMediaLock)
    {
        NT_ASSERT(newInternalLockCount != 0);
        newInternalLockCount--;
        countChanged = TRUE;
    }

    if ( (DeviceExtension->LockCount != 0) ||
         (DeviceExtension->ProtectedLockCount != 0) ||
         (DeviceExtension->InternalLockCount != 0) )
    {
        previouslyLocked = TRUE;
    }
    if ( (newLockCount != 0) ||
         (newProtectedLockCount != 0) ||
         (newInternalLockCount != 0) )
    {
        nowLocked = TRUE;
    }

    // Only send command down to device when necessary
    if (previouslyLocked != nowLocked)
    {
        // Compose and send the PREVENT ALLOW MEDIA REMOVAL command.
        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        srb.CdbLength = 6;
        srb.TimeOutValue = DeviceExtension->TimeOutValue;

        cdb = (PCDB)&srb.Cdb;
        cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
        cdb->MEDIA_REMOVAL.Prevent = Lock;

        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            &srb,
                                            NULL,
                                            0,
                                            FALSE,
                                            Request);
    }

Exit:

    // Store the updated lock counts on success
    if (countChanged && NT_SUCCESS(status))
    {
        DeviceExtension->LockCount = newLockCount;
        DeviceExtension->ProtectedLockCount = newProtectedLockCount;
        DeviceExtension->InternalLockCount = newInternalLockCount;
        if (fileObjectContext)
        {
            fileObjectContext->LockCount = newFileLockCount;
        }
    }

    WdfWaitLockRelease(DeviceExtension->EjectSynchronizationLock);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                "PerformEjectionControl: %!STATUS!, "
                "Count Changed: %d, Command Sent: %d, "
                "Current Counts: Internal: %x  Secure: %x  Simple: %x\n",
                status,
                countChanged,
                previouslyLocked != nowLocked,
                DeviceExtension->InternalLockCount,
                DeviceExtension->ProtectedLockCount,
                DeviceExtension->LockCount
                ));

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceUnlockExclusive(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ WDFFILEOBJECT            FileObject,
    _In_ BOOLEAN                  IgnorePreviousMediaChanges
    )
/*++

Routine Description:

    to unlock the exclusive lock

Arguments:

    DeviceExtension - device context
    FileObject - file object that currently holds the lock
    IgnorePreviousMediaChanges - if TRUE, ignore previously accumulated media changes

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PCDROM_DATA                     cdData = &DeviceExtension->DeviceAdditionalData;
    PMEDIA_CHANGE_DETECTION_INFO    info = DeviceExtension->MediaChangeDetectionInfo;
    BOOLEAN                         ANPending = 0;
    LONG                            requestInUse = 0;

    PAGED_CODE();

    if (!EXCLUSIVE_MODE(cdData))
    {
        // Device is not locked for exclusive access.
        // Can not process unlock request.
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                    "RequestHandleExclusiveAccessUnlockDevice: Device not locked for exclusive access, can't unlock device.\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (!EXCLUSIVE_OWNER(cdData, FileObject))
    {
        // Request not from the exclusive owner, can't unlock the device.
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                   "RequestHandleExclusiveAccessUnlockDevice: Unable to unlock device, invalid file object\n"));

        status = STATUS_INVALID_HANDLE;
    }

    if (NT_SUCCESS(status))
    {
        // Unless we were explicitly requested not to do so, generate a media removal notification
        // followed by a media arrival notification similar to volume lock/unlock file system events.
        if (!IgnorePreviousMediaChanges)
        {
            MEDIA_CHANGE_DETECTION_STATE previousMediaState = MediaUnknown;

            // Change the media state to "unavailable", which will cause a removal notification if the media
            // was previously present. At the same time, store the previous state in previousMediaState.
            DeviceSetMediaChangeStateEx(DeviceExtension, MediaUnavailable, &previousMediaState);
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "DeviceUnlockExclusive: Changing the media state to MediaUnavailable\n"));

            // Restore the previous media state, which will cause a media arrival notification if the media
            // was originally present.
            DeviceSetMediaChangeStateEx(DeviceExtension, previousMediaState, NULL);
        }

        // Set DO_VERIFY_VOLUME so that the file system will remount on it.
        if (IsVolumeMounted(DeviceExtension->DeviceObject))
        {
            SET_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);
        }

        // Set MMC state to update required
        cdData->Mmc.WriteAllowed = FALSE;
        cdData->Mmc.UpdateState = CdromMmcUpdateRequired;

        // Send unlock notification
        DeviceSendNotification(DeviceExtension,
                               &GUID_IO_CDROM_EXCLUSIVE_UNLOCK,
                               0,
                               NULL);

        InterlockedExchangePointer((PVOID)&cdData->ExclusiveOwner,  NULL);

        if ((info != NULL) && (info->AsynchronousNotificationSupported != FALSE))
        {
            ANPending = info->ANSignalPendingDueToExclusiveLock;
            info->ANSignalPendingDueToExclusiveLock = FALSE;

            if ((ANPending != FALSE) && (info->MediaChangeDetectionDisableCount == 0))
            {
                // if the request is not in use, mark it as such.
                requestInUse = InterlockedCompareExchange((PLONG)&info->MediaChangeRequestInUse, 1, 0);

                if (requestInUse == 0)
                {
                    // The last MCN finished. ok to issue the new one.
                    RequestSetupMcnSyncIrp(DeviceExtension);

                    // The irp will go into KMDF framework and a request will be created there to represent it.
                    IoCallDriver(DeviceExtension->DeviceObject, info->MediaChangeSyncIrp);
                }
            }
        }
    }

    return status;
}


VOID
RequestCompletion(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ WDFREQUEST              Request,
    _In_ NTSTATUS                Status,
    _In_ ULONG_PTR               Information
    )
{
#ifdef DBG
    ULONG                   ioctlCode = 0;
    WDF_REQUEST_PARAMETERS  requestParameters;

    // Get the Request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    if (requestParameters.Type == WdfRequestTypeDeviceControl)
    {
        ioctlCode = requestParameters.Parameters.DeviceIoControl.IoControlCode;

        if (requestParameters.Parameters.DeviceIoControl.IoControlCode != IOCTL_MCN_SYNC_FAKE_IOCTL)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                       "Request complete - IOCTL - code: %X; Status: %X; Information: %X\n",
                       ioctlCode,
                       Status,
                       (ULONG)Information));
        }
        else
        {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL,
                       "Request complete - IOCTL - code: %X; Status: %X; Information: %X\n",
                       ioctlCode,
                       Status,
                       (ULONG)Information));
        }
    }
    else if (requestParameters.Type == WdfRequestTypeRead)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                   "Request complete - READ - Starting Offset: %X; Length: %X; Transferred Length: %X; Status: %X\n",
                   (ULONG)requestParameters.Parameters.Read.DeviceOffset,
                   (ULONG)requestParameters.Parameters.Read.Length,
                   (ULONG)Information,
                   Status));
    }
    else if (requestParameters.Type == WdfRequestTypeWrite)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                   "Request complete - WRITE - Starting Offset: %X; Length: %X; Transferred Length: %X; Status: %X\n",
                   (ULONG)requestParameters.Parameters.Write.DeviceOffset,
                   (ULONG)requestParameters.Parameters.Write.Length,
                   (ULONG)Information,
                   Status));
    }
#endif

    if (IoIsErrorUserInduced(Status))
    {
        PIRP irp = WdfRequestWdmGetIrp(Request);
        if (irp->Tail.Overlay.Thread)
        {
            IoSetHardErrorOrVerifyDevice(irp, DeviceExtension->DeviceObject);
        }
    }

    if (!NT_SUCCESS(Status) && DeviceExtension->SurpriseRemoved == TRUE)
    {
        // IMAPI expects ERROR_DEV_NOT_EXISTS if recorder has been surprised removed,
        // or it will retry WRITE commands for up to 3 minutes
        // CDROM behavior should be consistent for all requests, including SCSI pass-through
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    WdfRequestCompleteWithInformation(Request, Status, Information);

    return;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestDummyCompletionRoutine(
    _In_ WDFREQUEST                     Request,
    _In_ WDFIOTARGET                    Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT                     Context
    )
/*++

Routine Description:

    This is a dummy competion routine that simply calls WdfRequestComplete. We have to use
    this dummy competion routine instead of WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET, because
    the latter causes the framework to not check if the I/O target is closed or not.

Arguments:

    Request - completed request
    Target - the I/O target that completed the request
    Params - request parameters
    Context - not used

Return Value:

    none

--*/
{
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);
    UNREFERENCED_PARAMETER(Context);

    WdfRequestCompleteWithInformation(Request,
                                      WdfRequestGetStatus(Request),
                                      WdfRequestGetInformation(Request));
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
DeviceSendPowerDownProcessRequest(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ PVOID Context
    )
/*++

Routine Description:

    This function is called during processing power down request.
    It is used to send either SYNC CACHE command or STOP UNIT command.

    Caller should set proper value in deviceExtension->PowerContext.PowerChangeState.PowerDown
    to trigger the correct command be sent.

Arguments:

    DeviceExtension -

    CompletionRoutine - Completion routine that needs to be set for the request

    Context - Completion context associated with the completion routine

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status;
    BOOLEAN                     requestSent = FALSE;

    BOOLEAN                     shouldRetry = TRUE;
    PCDB                        cdb = (PCDB)DeviceExtension->PowerContext.Srb.Cdb;
    ULONG                       timeoutValue = DeviceExtension->TimeOutValue;
    ULONG                       retryCount = 1;

    // reset some fields.
    DeviceExtension->PowerContext.RetryIntervalIn100ns = 0;
    status = PowerContextReuseRequest(DeviceExtension);
    RequestClearSendTime(DeviceExtension->PowerContext.PowerRequest);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // set proper timeout value and max retry count.
    switch(DeviceExtension->PowerContext.PowerChangeState.PowerDown)
    {
    case PowerDownDeviceInitial:
    case PowerDownDeviceQuiesced:
    case PowerDownDeviceStopped:
        break;

    case PowerDownDeviceLocked:
        // Case of issuing SYNC CACHE command. Do not use power irp timeout remaining time in this case
        // as we want to give best try on SYNC CACHE command.
        retryCount = MAXIMUM_RETRIES;
        timeoutValue = DeviceExtension->TimeOutValue;
        break;

    case PowerDownDeviceFlushed:
    {
        // Case of issuing STOP UNIT command
        // As "Imme" bit is set to '1', this command should be completed in short time.
        // This command is at low importance, failure of this command has very small impact.
        ULONG secondsRemaining = 0;

#if (WINVER >= 0x0601)
        // this API is introduced in Windows7
        PoQueryWatchdogTime(DeviceExtension->LowerPdo, &secondsRemaining);
#endif

        if (secondsRemaining == 0)
        {
            // not able to retrieve remaining time from PoQueryWatchdogTime API, use default values.
            retryCount = MAXIMUM_RETRIES;
            timeoutValue = SCSI_CDROM_TIMEOUT;
        }
        else
        {
            // plan to leave about 30 seconds to lower level drivers if possible.
            if (secondsRemaining >= 32)
            {
                retryCount = (secondsRemaining - 30)/SCSI_CDROM_TIMEOUT + 1;
                timeoutValue = SCSI_CDROM_TIMEOUT;

                if (retryCount > MAXIMUM_RETRIES)
                {
                    retryCount = MAXIMUM_RETRIES;
                }

                if (retryCount == 1)
                {
                    timeoutValue = secondsRemaining - 30;
                }
            }
            else
            {
                // issue the command with minimal timeout value and do not retry on it.
                retryCount = 1;
                timeoutValue = 2;
            }
        }
    }
        break;
    default:
        NT_ASSERT( FALSE );
        status = STATUS_NOT_IMPLEMENTED;
        return status;
    }

    DeviceExtension->PowerContext.RetryCount = retryCount;

    // issue command.
    while (shouldRetry)
    {

        // set SRB fields.
        DeviceExtension->PowerContext.Srb.SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                                                     SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                                     SRB_FLAGS_NO_QUEUE_FREEZE |
                                                     SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                                     SRB_FLAGS_D3_PROCESSING;

        DeviceExtension->PowerContext.Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        DeviceExtension->PowerContext.Srb.TimeOutValue = timeoutValue;

        if (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceInitial)
        {
            DeviceExtension->PowerContext.Srb.Function = SRB_FUNCTION_LOCK_QUEUE;
        }
        else if (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceLocked)
        {
            DeviceExtension->PowerContext.Srb.Function = SRB_FUNCTION_QUIESCE_DEVICE;
        }
        else if (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceQuiesced)
        {
            // Case of issuing SYNC CACHE command.
            DeviceExtension->PowerContext.Srb.CdbLength = 10;
            cdb->SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;
        }
        else if (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceFlushed)
        {
            // Case of issuing STOP UNIT command.
            DeviceExtension->PowerContext.Srb.CdbLength = 6;
            cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->START_STOP.Start = 0;
            cdb->START_STOP.Immediate = 1;
        }
        else if (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceStopped)
        {
            DeviceExtension->PowerContext.Srb.Function = SRB_FUNCTION_UNLOCK_QUEUE;
        }

        // Set up completion routine and context if requested
        if (CompletionRoutine)
        {
            WdfRequestSetCompletionRoutine(DeviceExtension->PowerContext.PowerRequest,
                                           CompletionRoutine,
                                           Context);
        }

        status = RequestSend(DeviceExtension,
                             DeviceExtension->PowerContext.PowerRequest,
                             DeviceExtension->IoTarget,
                             CompletionRoutine ? 0 : WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
                             &requestSent);

        if (requestSent)
        {
            if ((CompletionRoutine == NULL) &&
                (SRB_STATUS(DeviceExtension->PowerContext.Srb.SrbStatus) != SRB_STATUS_SUCCESS))
            {
                TracePrint((TRACE_LEVEL_ERROR,
                            TRACE_FLAG_POWER,
                            "%p\tError occured when issuing %s command to device. Srb %p, Status %x\n",
                            DeviceExtension->PowerContext.PowerRequest,
                            (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceQuiesced) ? "SYNC CACHE" : "STOP UNIT",
                            &DeviceExtension->PowerContext.Srb,
                            DeviceExtension->PowerContext.Srb.SrbStatus));

                NT_ASSERT(!(TEST_FLAG(DeviceExtension->PowerContext.Srb.SrbStatus, SRB_STATUS_QUEUE_FROZEN)));

                shouldRetry = RequestSenseInfoInterpret(DeviceExtension,
                                                        DeviceExtension->PowerContext.PowerRequest,
                                                        &(DeviceExtension->PowerContext.Srb),
                                                        retryCount - DeviceExtension->PowerContext.RetryCount,
                                                        &status,
                                                        &(DeviceExtension->PowerContext.RetryIntervalIn100ns));

                if (shouldRetry && (DeviceExtension->PowerContext.RetryCount-- == 0))
                {
                    shouldRetry = FALSE;
                }
            }
            else
            {
                // succeeded, do not need to retry.
                shouldRetry = FALSE;
            }

        }
        else
        {
            // request failed to be sent
            shouldRetry = FALSE;
        }

        if (shouldRetry)
        {
            LARGE_INTEGER t;
            t.QuadPart = -DeviceExtension->PowerContext.RetryIntervalIn100ns;
            KeDelayExecutionThread(KernelMode, FALSE, &t);

            status = PowerContextReuseRequest(DeviceExtension);
            if (!NT_SUCCESS(status))
            {
                shouldRetry = FALSE;
            }
        }
    }

    if (DeviceExtension->PowerContext.PowerChangeState.PowerDown == PowerDownDeviceQuiesced)
    {
        // record SYNC CACHE command completion time stamp.
        KeQueryTickCount(&DeviceExtension->PowerContext.Step1CompleteTime);
    }

    return status;
}

NTSTATUS
RequestSend(
    _In_        PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_        WDFREQUEST              Request,
    _In_        WDFIOTARGET             IoTarget,
    _In_        ULONG                   Flags,
    _Out_opt_   PBOOLEAN                RequestSent
    )
/*++

Routine Description:

    Send the request to the target, wake up the device from Zero Power state if necessary.

Arguments:

    DeviceExtension - device extension
    Request - the request to be sent
    IoTarget - target of the above request
    Flags - flags for the operation
    RequestSent - optional, if the request was sent

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                 status  = STATUS_SUCCESS;
    BOOLEAN                  requestSent = FALSE;
    WDF_REQUEST_SEND_OPTIONS options;

    UNREFERENCED_PARAMETER(DeviceExtension);

    if ((DeviceExtension->ZeroPowerODDInfo != NULL) &&
        (DeviceExtension->ZeroPowerODDInfo->InZeroPowerState != FALSE))
    {
    }

    // Now send down the request
    if (NT_SUCCESS(status))
    {
        WDF_REQUEST_SEND_OPTIONS_INIT(&options, Flags);

        RequestSetSentTime(Request);

        // send request and check status

        // Disable SDV warning about infinitely waiting in caller's context:
        //   1. Some requests (such as SCSI_PASS_THROUGH, contains buffer from user space) need to be sent down in caller’s context.
        //      Consequently, these requests wait in caller’s context until they are allowed to be sent down.
        //   2. Considering the situation that during sleep, a request can be hold by storage port driver. When system resumes, any time out value (if we set using KMDF time out value) might be expires.
        //      This will cause the waiting request being failed (behavior change). We’d rather not set time out value.

        _Analysis_assume_(options.Timeout != 0);
        requestSent = WdfRequestSend(Request, IoTarget, &options);
        _Analysis_assume_(options.Timeout == 0);

        // If WdfRequestSend fails, or if the WDF_REQUEST_SEND_OPTION_SYNCHRONOUS flag is set,
        // the driver can call WdfRequestGetStatus immediately after calling WdfRequestSend.
        if ((requestSent == FALSE) ||
            (Flags & WDF_REQUEST_SEND_OPTION_SYNCHRONOUS))
        {
            status = WdfRequestGetStatus(Request);

            if (requestSent == FALSE)
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                            "WdfRequestSend failed: %lx\n",
                            status
                            ));
            }
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        if (RequestSent != NULL)
        {
            *RequestSent = requestSent;
        }
    }

    return status;
}

