/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    scratch.c

Abstract:

    Functions for using common scratch buffer

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "stddef.h"
#include "string.h"

#include "ntddk.h"
#include "ntddstor.h"
#include "cdrom.h"
#include "ioctl.h"
#include "scratch.h"
#include "mmc.h"

#ifdef DEBUG_USE_WPP
#include "scratch.tmh"
#endif

// Forward declarations
EVT_WDF_REQUEST_COMPLETION_ROUTINE  ScratchBuffer_ReadWriteCompletionRoutine;

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, ScratchBuffer_Deallocate)
#pragma alloc_text(PAGE, ScratchBuffer_Allocate)
#pragma alloc_text(PAGE, ScratchBuffer_SetupSrb)
#pragma alloc_text(PAGE, ScratchBuffer_ExecuteCdbEx)

#endif

_IRQL_requires_max_(APC_LEVEL)
VOID
ScratchBuffer_Deallocate(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    release all resources allocated for scratch.

Arguments:

    DeviceExtension - device extension

Return Value:

    none

--*/
{
    PAGED_CODE ();

    NT_ASSERT(DeviceExtension->ScratchContext.ScratchInUse == 0);

    if (DeviceExtension->ScratchContext.ScratchHistory != NULL)
    {
        ExFreePool(DeviceExtension->ScratchContext.ScratchHistory);
        DeviceExtension->ScratchContext.ScratchHistory = NULL;
    }
    if (DeviceExtension->ScratchContext.ScratchSense != NULL)
    {
        ExFreePool(DeviceExtension->ScratchContext.ScratchSense);
        DeviceExtension->ScratchContext.ScratchSense = NULL;
    }
    if (DeviceExtension->ScratchContext.ScratchSrb != NULL)
    {
        ExFreePool(DeviceExtension->ScratchContext.ScratchSrb);
        DeviceExtension->ScratchContext.ScratchSrb = NULL;
    }
    if (DeviceExtension->ScratchContext.ScratchBufferSize != 0)
    {
        DeviceExtension->ScratchContext.ScratchBufferSize = 0;
    }
    if (DeviceExtension->ScratchContext.ScratchBufferMdl != NULL)
    {
        IoFreeMdl(DeviceExtension->ScratchContext.ScratchBufferMdl);
        DeviceExtension->ScratchContext.ScratchBufferMdl = NULL;
    }
    if (DeviceExtension->ScratchContext.ScratchBuffer != NULL)
    {
        ExFreePool(DeviceExtension->ScratchContext.ScratchBuffer);
        DeviceExtension->ScratchContext.ScratchBuffer = NULL;
    }

    if (DeviceExtension->ScratchContext.PartialMdl != NULL)
    {
        IoFreeMdl(DeviceExtension->ScratchContext.PartialMdl);
        DeviceExtension->ScratchContext.PartialMdl = NULL;
    }

    if (DeviceExtension->ScratchContext.ScratchRequest != NULL)
    {
        PIRP irp = WdfRequestWdmGetIrp(DeviceExtension->ScratchContext.ScratchRequest);
        if (irp->MdlAddress)
        {
            irp->MdlAddress = NULL;
        }
        WdfObjectDelete(DeviceExtension->ScratchContext.ScratchRequest);
        DeviceExtension->ScratchContext.ScratchRequest = NULL;
    }

    return;
}

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
ScratchBuffer_Allocate(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    allocate resources allocated for scratch.

Arguments:

    DeviceExtension - device extension

Return Value:

    none

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE ();

    NT_ASSERT(DeviceExtension->ScratchContext.ScratchInUse == 0);

    // quick-exit if already allocated
    if ((DeviceExtension->ScratchContext.ScratchBuffer     != NULL) &&
        (DeviceExtension->ScratchContext.ScratchBufferMdl  != NULL) &&
        (DeviceExtension->ScratchContext.ScratchBufferSize != 0)    &&
        (DeviceExtension->ScratchContext.ScratchRequest    != NULL) &&
        (DeviceExtension->ScratchContext.ScratchSrb        != NULL) &&
        (DeviceExtension->ScratchContext.ScratchHistory    != NULL) &&
        (DeviceExtension->ScratchContext.PartialMdl  != NULL)
        )
    {
        return TRUE;
    }

    // validate max transfer already determined
    NT_ASSERT(DeviceExtension->DeviceAdditionalData.MaxPageAlignedTransferBytes != 0);

    // validate no partially-saved state
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchBuffer     == NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchBufferMdl  == NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchBufferSize == 0);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchRequest    == NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.PartialMdl == NULL);

    // limit the scratch buffer to between 4k and 64k (so data length fits into USHORT -- req'd for many commands)
    DeviceExtension->ScratchContext.ScratchBufferSize = min(DeviceExtension->DeviceAdditionalData.MaxPageAlignedTransferBytes, (64*1024));

    // allocate the buffer
    if (NT_SUCCESS(status))
    {
        DeviceExtension->ScratchContext.ScratchBuffer = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                              DeviceExtension->ScratchContext.ScratchBufferSize,
                                                                              CDROM_TAG_SCRATCH);
        if (DeviceExtension->ScratchContext.ScratchBuffer == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to allocate scratch buffer of %x bytes\n",
                        DeviceExtension->ScratchContext.ScratchBufferSize
                        ));
        }
        else if (BYTE_OFFSET(DeviceExtension->ScratchContext.ScratchBuffer) != 0)
        {
            status = STATUS_INTERNAL_ERROR;
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_INIT,
                        "Allocation of %x bytes non-paged pool was not "
                        "allocated on page boundary?  STATUS_INTERNAL_ERROR\n",
                        DeviceExtension->ScratchContext.ScratchBufferSize
                        ));
        }
    }

    // allocate the MDL
    if (NT_SUCCESS(status))
    {
        DeviceExtension->ScratchContext.ScratchBufferMdl = IoAllocateMdl(DeviceExtension->ScratchContext.ScratchBuffer,
                                                                         DeviceExtension->ScratchContext.ScratchBufferSize,
                                                                         FALSE, FALSE, NULL);
        if (DeviceExtension->ScratchContext.ScratchBufferMdl == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to allocate MDL for %x byte buffer\n",
                        DeviceExtension->ScratchContext.ScratchBufferSize
                        ));
        }
        else
        {
            MmBuildMdlForNonPagedPool(DeviceExtension->ScratchContext.ScratchBufferMdl);
        }
    }

    // create the request
    if (NT_SUCCESS(status))
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                                CDROM_REQUEST_CONTEXT);

        status =  WdfRequestCreate(&attributes,
                                   DeviceExtension->IoTarget,
                                   &DeviceExtension->ScratchContext.ScratchRequest);

        if ((!NT_SUCCESS(status)) ||
            (DeviceExtension->ScratchContext.ScratchRequest == NULL))
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to allocate scratch MDL \n"));
        }
    }

    // allocate the srb
    if (NT_SUCCESS(status))
    {
        DeviceExtension->ScratchContext.ScratchSrb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                           sizeof(SCSI_REQUEST_BLOCK),
                                                                           CDROM_TAG_SCRATCH);

        if (DeviceExtension->ScratchContext.ScratchSrb == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to allocate scratch SRB\n"));
        }
    }

    // allocate the sense buffer
    if (NT_SUCCESS(status))
    {
        DeviceExtension->ScratchContext.ScratchSense = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                             sizeof(SENSE_DATA),
                                                                             CDROM_TAG_SCRATCH);

        if (DeviceExtension->ScratchContext.ScratchSense == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to allocate scratch sense data\n"
                        ));
        }
    }

    // allocate the SRB history data
    if (NT_SUCCESS(status))
    {
        size_t allocationSize = sizeof(SRB_HISTORY) - sizeof(SRB_HISTORY_ITEM);
        allocationSize += 20 * sizeof(SRB_HISTORY_ITEM);

        DeviceExtension->ScratchContext.ScratchHistory = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                               allocationSize,
                                                                               CDROM_TAG_SCRATCH);
        if (DeviceExtension->ScratchContext.ScratchHistory == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to allocate scratch history buffer\n"
                        ));
        }
        else
        {
            // must be initialized here...
            RtlZeroMemory(DeviceExtension->ScratchContext.ScratchHistory, allocationSize);
            DeviceExtension->ScratchContext.ScratchHistory->TotalHistoryCount = 20;
        }
    }

    // allocate the MDL
    if (NT_SUCCESS(status))
    {
        ULONG transferLength = 0;

        status = RtlULongAdd(DeviceExtension->DeviceAdditionalData.MaxPageAlignedTransferBytes, PAGE_SIZE, &transferLength);
        if (NT_SUCCESS(status))
        {
            DeviceExtension->ScratchContext.PartialMdlIsBuilt = FALSE;
            DeviceExtension->ScratchContext.PartialMdl = IoAllocateMdl(NULL,
                                                                       transferLength,
                                                                       FALSE,
                                                                       FALSE,
                                                                       NULL);
            if (DeviceExtension->ScratchContext.PartialMdl == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                            "Failed to allocate MDL for %x byte buffer\n",
                            DeviceExtension->ScratchContext.ScratchBufferSize
                            ));
            }
            else
            {
                NT_ASSERT(DeviceExtension->ScratchContext.PartialMdl->Size >=
                       (CSHORT)(sizeof(MDL) + BYTES_TO_PAGES(DeviceExtension->DeviceAdditionalData.MaxPageAlignedTransferBytes) * sizeof(PFN_NUMBER)));
            }
        }
        else
        {
            status = STATUS_INTEGER_OVERFLOW;
        }
    }

    // cleanup on failure
    if (!NT_SUCCESS(status))
    {
        ScratchBuffer_Deallocate(DeviceExtension);
    }

    return NT_SUCCESS(status);
}


VOID
ScratchBuffer_ResetItems(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                    ResetRequestHistory
    )
/*++

Routine Description:

    reset scratch items for reuse.

Arguments:

    DeviceExtension - device extension
    ResetRequestHistory - reset history fields or not

Return Value:

    none

--*/
{
    NTSTATUS                 status = STATUS_SUCCESS;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    PIRP                     irp = NULL;

    NT_ASSERT(DeviceExtension->ScratchContext.ScratchHistory    != NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchSense      != NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchSrb        != NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchRequest    != NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchBufferSize != 0);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchBuffer     != NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchBufferMdl  != NULL);
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchInUse      != 0);

    irp = WdfRequestWdmGetIrp(DeviceExtension->ScratchContext.ScratchRequest);

    if (ResetRequestHistory)
    {
        PSRB_HISTORY history = DeviceExtension->ScratchContext.ScratchHistory;
        RtlZeroMemory(history->History, sizeof(SRB_HISTORY_ITEM) * history->TotalHistoryCount);
        history->ClassDriverUse[0] = 0;
        history->ClassDriverUse[1] = 0;
        history->ClassDriverUse[2] = 0;
        history->ClassDriverUse[3] = 0;
        history->UsedHistoryCount  = 0;
    }

    // re-use the KMDF request object

    // deassign the MdlAddress, this is the value we assign explicitly.
    // this is to prevent WdfRequestReuse to release the Mdl unexpectly.
    if (irp->MdlAddress)
    {
        irp->MdlAddress = NULL;
    }

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    status = WdfRequestReuse(DeviceExtension->ScratchContext.ScratchRequest, &reuseParams);
    // WDF request to format the request befor sending it
    if (NT_SUCCESS(status))
    {
        // clean up completion routine.
        WdfRequestSetCompletionRoutine(DeviceExtension->ScratchContext.ScratchRequest, NULL, NULL);

        status = WdfIoTargetFormatRequestForInternalIoctlOthers(DeviceExtension->IoTarget,
                                                                DeviceExtension->ScratchContext.ScratchRequest,
                                                                IOCTL_SCSI_EXECUTE_IN,
                                                                NULL, NULL,
                                                                NULL, NULL,
                                                                NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                       "ScratchBuffer_ResetItems: WdfIoTargetFormatRequestForInternalIoctlOthers failed, %!STATUS!\n",
                       status));
        }
    }

    RtlZeroMemory(DeviceExtension->ScratchContext.ScratchSense, sizeof(SENSE_DATA));
    RtlZeroMemory(DeviceExtension->ScratchContext.ScratchSrb, sizeof(SCSI_REQUEST_BLOCK));

    return;
}


NTSTATUS
ScratchBuffer_PerformNextReadWrite(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ BOOLEAN                  FirstTry
    )
/*++

Routine Description:

    This function asynchronously sends the next read/write SRB down the stack.

Arguments:

    DeviceExtension - Device extension

Return Value:

    none

--*/
{
    PCDROM_SCRATCH_READ_WRITE_CONTEXT   readWriteContext = &DeviceExtension->ScratchContext.ScratchReadWriteContext;
    PCDROM_REQUEST_CONTEXT              requestContext = RequestGetContext(DeviceExtension->ScratchContext.ScratchRequest);
    WDFREQUEST                          originalRequest = requestContext->OriginalRequest;
    NTSTATUS                            status = STATUS_SUCCESS;

    ULONG       transferSize;
    BOOLEAN     usePartialMdl;

    transferSize = min((readWriteContext->EntireXferLen - readWriteContext->TransferedBytes), readWriteContext->MaxLength);

    if (FirstTry)
    {
        DeviceExtension->ScratchContext.NumRetries = 0;
    }

    ScratchBuffer_ResetItems(DeviceExtension, FALSE);

    usePartialMdl = (readWriteContext->PacketsCount > 1 || readWriteContext->TransferedBytes > 0);

    ScratchBuffer_SetupReadWriteSrb(DeviceExtension,
                                    originalRequest,
                                    readWriteContext->StartingOffset,
                                    transferSize,
                                    readWriteContext->DataBuffer,
                                    readWriteContext->IsRead,
                                    usePartialMdl
                                    );

    WdfRequestSetCompletionRoutine(DeviceExtension->ScratchContext.ScratchRequest,
            ScratchBuffer_ReadWriteCompletionRoutine, DeviceExtension);

    status = ScratchBuffer_SendSrb(DeviceExtension, FALSE, (FirstTry ? &readWriteContext->SrbHistoryItem : NULL));

    return status;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ScratchBuffer_ReadWriteTimerRoutine(
    struct _KDPC *Dpc,
    PVOID         DeferredContext,
    PVOID         SystemArgument1,
    PVOID         SystemArgument2
    )
/*++

Routine Description:

    Timer routine for retrying read and write requests.

Arguments:

    Timer - WDF timer

Return Value:

    none

--*/
{
    PCDROM_DEVICE_EXTENSION             deviceExtension = NULL;
    PCDROM_SCRATCH_READ_WRITE_CONTEXT   readWriteContext = NULL;
    WDFREQUEST                          originalRequest = NULL;
    PCDROM_REQUEST_CONTEXT              requestContext = NULL;
    NTSTATUS                            status = STATUS_SUCCESS;
    KIRQL                               oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (DeferredContext == NULL)
    {
        // This is impossible, but definition of KDEFERRED_ROUTINE allows optional argument,
        // and thus OACR will complain.

        return;
    }

    originalRequest = (WDFREQUEST) DeferredContext;
    requestContext = RequestGetContext(originalRequest);

    KeAcquireSpinLock(&requestContext->ReadWriteCancelSpinLock, &oldIrql);

    if (!requestContext->ReadWriteIsCompleted)
    {
        // As the first step, unregister the cancellation routine
        status = WdfRequestUnmarkCancelable(originalRequest);
    }
    else
    {
        status = STATUS_CANCELLED;
    }

    KeReleaseSpinLock(&requestContext->ReadWriteCancelSpinLock, oldIrql);

    if (status != STATUS_CANCELLED)
    {
        deviceExtension = requestContext->DeviceExtension;
        readWriteContext = &deviceExtension->ScratchContext.ScratchReadWriteContext;

        // We use timer only for retries, that's why the second parameter is always FALSE
        status = ScratchBuffer_PerformNextReadWrite(deviceExtension, FALSE);

        if (!NT_SUCCESS(status))
        {
            ScratchBuffer_EndUse(deviceExtension);
            RequestCompletion(deviceExtension, originalRequest, status, readWriteContext->TransferedBytes);
        }
    }

    //
    // Drop the extra reference
    //
    WdfObjectDereference(originalRequest);
}


EVT_WDF_REQUEST_CANCEL  ScratchBuffer_ReadWriteEvtRequestCancel;

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ScratchBuffer_ReadWriteEvtRequestCancel(
    _In_ WDFREQUEST  Request
    )
/*++

Routine Description:

    Cancels a request waiting for the read/write timer to expire. This function does not
    support cancellation of requests that have already been sent down.

Arguments:

    Request - WDF request

Return Value:

    none

--*/
{
    PCDROM_REQUEST_CONTEXT             requestContext = RequestGetContext(Request);
    PCDROM_DEVICE_EXTENSION            deviceExtension = requestContext->DeviceExtension;
    PCDROM_SCRATCH_READ_WRITE_CONTEXT  readWriteContext = &deviceExtension->ScratchContext.ScratchReadWriteContext;
    KIRQL                              oldIrql;

    KeAcquireSpinLock(&requestContext->ReadWriteCancelSpinLock, &oldIrql);

    if (KeCancelTimer(&requestContext->ReadWriteTimer))
    {
       //
       // Timer is canceled, we own the request.  Drop the reference we took before
       // queueing the timer.
       //
       WdfObjectDereference(Request);
    }
    else
    {
        //
        // Timer will run and drop the reference but it won't complete the request
        // because we set IsCompleted to TRUE
        //
    }

    requestContext->ReadWriteIsCompleted = TRUE;

    KeReleaseSpinLock(&requestContext->ReadWriteCancelSpinLock, oldIrql);

    ScratchBuffer_EndUse(deviceExtension);

    // If WdfTimerStop returned TRUE, it means this request was scheduled for a retry
    // and the retry has not happened yet. We just need to cancel it and release the scratch buffer.
    RequestCompletion(deviceExtension, Request, STATUS_CANCELLED, readWriteContext->TransferedBytes);
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ScratchBuffer_ReadWriteCompletionRoutine(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++

Routine Description:

    Read/write request completion routine.

Arguments:
    Request - WDF request
    Target - The IO target the request was completed by.
    Params - the request completion parameters
    Context - context

Return Value:

    none

--*/
{
    PCDROM_DEVICE_EXTENSION             deviceExtension = (PCDROM_DEVICE_EXTENSION) Context;
    PCDROM_SCRATCH_READ_WRITE_CONTEXT   readWriteContext = &deviceExtension->ScratchContext.ScratchReadWriteContext;
    NTSTATUS                            status = STATUS_SUCCESS;
    PCDROM_REQUEST_CONTEXT              requestContext = RequestGetContext(deviceExtension->ScratchContext.ScratchRequest);
    WDFREQUEST                          originalRequest = requestContext->OriginalRequest;

    if (!NT_SUCCESS(WdfRequestGetStatus(Request)))
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                   "WdfRequestSend: %lx\n",
                    WdfRequestGetStatus(Request)
                    ));
    }

    UNREFERENCED_PARAMETER(Params);
    UNREFERENCED_PARAMETER(Target);

    // We are not calling ScratchBuffer_BeginUse / ScratchBuffer_EndUse in this function, because we already own
    // the scratch buffer if this function is being called.

    if ((deviceExtension->ScratchContext.ScratchSrb->SrbStatus == SRB_STATUS_ABORTED) &&
        (deviceExtension->ScratchContext.ScratchSrb->InternalStatus == STATUS_CANCELLED))
    {
        // The request has been cancelled, just need to complete it
    }
    else if (SRB_STATUS(deviceExtension->ScratchContext.ScratchSrb->SrbStatus) != SRB_STATUS_SUCCESS)
    {
        // The SCSI command that we sent down has failed, retry it if necessary
        BOOLEAN shouldRetry = TRUE;
        LONGLONG retryIn100nsUnits = 0;

        shouldRetry = RequestSenseInfoInterpretForScratchBuffer(deviceExtension,
                                                                deviceExtension->ScratchContext.NumRetries,
                                                                &status,
                                                                &retryIn100nsUnits);

        if (shouldRetry)
        {
            deviceExtension->ScratchContext.NumRetries++;

            if (retryIn100nsUnits == 0)
            {
                // We take a shortcut here by calling ScratchBuffer_PerformNextReadWrite directly:
                // this helps to avoid unnecessary context switch.
                status = ScratchBuffer_PerformNextReadWrite(deviceExtension, FALSE);

                if (NT_SUCCESS(status))
                {
                    // We're not done with the request yet, no need to complete it now
                    return;
                }
            }
            else
            {
                PCDROM_REQUEST_CONTEXT originalRequestContext = RequestGetContext(originalRequest);
                KIRQL                  oldIrql;

                //
                // Initialize the spin lock and timer local to the original request.
                //
                if (!originalRequestContext->ReadWriteRetryInitialized)
                {
                    KeInitializeSpinLock(&originalRequestContext->ReadWriteCancelSpinLock);
                    KeInitializeTimer(&originalRequestContext->ReadWriteTimer);
                    KeInitializeDpc(&originalRequestContext->ReadWriteDpc, ScratchBuffer_ReadWriteTimerRoutine, originalRequest);
                    originalRequestContext->ReadWriteRetryInitialized = TRUE;
                }

                KeAcquireSpinLock(&requestContext->ReadWriteCancelSpinLock, &oldIrql);

                status = WdfRequestMarkCancelableEx(originalRequest, ScratchBuffer_ReadWriteEvtRequestCancel);

                if (status == STATUS_CANCELLED)
                {
                    requestContext->ReadWriteIsCompleted = TRUE;

                    KeReleaseSpinLock(&requestContext->ReadWriteCancelSpinLock, oldIrql);
                }
                else
                {
                    LARGE_INTEGER t;

                    t.QuadPart = -retryIn100nsUnits;

                    WdfObjectReference(originalRequest);

                    // Use negative time to indicate that we want a relative delay
                    KeSetTimer(&originalRequestContext->ReadWriteTimer,
                               t,
                               &originalRequestContext->ReadWriteDpc
                               );

                    KeReleaseSpinLock(&requestContext->ReadWriteCancelSpinLock, oldIrql);

                    return;
                }
            }
        }
    }
    else
    {
        // The SCSI command has succeeded
        readWriteContext->DataBuffer += deviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
        readWriteContext->StartingOffset.QuadPart += deviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
        readWriteContext->TransferedBytes += deviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
        readWriteContext->PacketsCount--;

        // Update the SRB history item
        if (readWriteContext->SrbHistoryItem)
        {
            ULONG senseSize;

            // Query the tick count and store in the history
            KeQueryTickCount(&readWriteContext->SrbHistoryItem->TickCountCompleted);

            // Copy the SRB Status...
            readWriteContext->SrbHistoryItem->SrbStatus = deviceExtension->ScratchContext.ScratchSrb->SrbStatus;

            // Determine the amount of valid sense data
            if (deviceExtension->ScratchContext.ScratchSrb->SenseInfoBufferLength >=
                    RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength))
            {
                PSENSE_DATA sense = (PSENSE_DATA)deviceExtension->ScratchContext.ScratchSrb->SenseInfoBuffer;
                senseSize = RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength) +
                            sense->AdditionalSenseLength;
                senseSize = min(senseSize, sizeof(SENSE_DATA));
            }
            else
            {
                senseSize = deviceExtension->ScratchContext.ScratchSrb->SenseInfoBufferLength;
            }

            // Normalize the sense data copy in the history
            RtlZeroMemory(&(readWriteContext->SrbHistoryItem->NormalizedSenseData), sizeof(SENSE_DATA));
            RtlCopyMemory(&(readWriteContext->SrbHistoryItem->NormalizedSenseData),
                    deviceExtension->ScratchContext.ScratchSrb->SenseInfoBuffer, senseSize);
        }

        // Check whether we need to send more SCSI commands to complete the request
        if (readWriteContext->PacketsCount > 0)
        {
            status = ScratchBuffer_PerformNextReadWrite(deviceExtension, TRUE);

            if (NT_SUCCESS(status))
            {
                // We're not done with the request yet, no need to complete it now
                return;
            }
        }
    }

    ScratchBuffer_EndUse(deviceExtension);


    RequestCompletion(deviceExtension, originalRequest, status, readWriteContext->TransferedBytes);
}

_IRQL_requires_max_(APC_LEVEL)
VOID
ScratchBuffer_SetupSrb(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ WDFREQUEST             OriginalRequest,
    _In_ ULONG                      MaximumTransferLength,
    _In_ BOOLEAN                    GetDataFromDevice
    )
/*++

Routine Description:

    setup scratch SRB for sending out.

Arguments:

    DeviceExtension - device extension
    OriginalRequest - original request delivered by WDF
    MaximumTransferLength - transfer length
    GetDataFromDevice - TRUE (get data from device); FALSE (send data to device)

Return Value:

    none

--*/
{
    WDFREQUEST              request = DeviceExtension->ScratchContext.ScratchRequest;
    PIRP                    irp = WdfRequestWdmGetIrp(request);
    PSCSI_REQUEST_BLOCK     srb = DeviceExtension->ScratchContext.ScratchSrb;
    PIO_STACK_LOCATION      irpStack = NULL;
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(request);

    PAGED_CODE ();

    requestContext->OriginalRequest = OriginalRequest;

    // set to use the full scratch buffer via the scratch SRB
    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    if (MaximumTransferLength == 0)
    {
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_NONE;
    }
    else if (GetDataFromDevice)
    {
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
    }
    else
    {
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_OUT;
    }
    irpStack->Parameters.Scsi.Srb = srb;

    if (MaximumTransferLength > 0)
    {
        // the Irp must show the MDL's address for the transfer
        irp->MdlAddress = DeviceExtension->ScratchContext.ScratchBufferMdl;

        srb->DataBuffer = DeviceExtension->ScratchContext.ScratchBuffer;
    }

    // prepare the SRB with default values
    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->SrbStatus = 0;
    srb->ScsiStatus = 0;
    srb->NextSrb = NULL;
    srb->OriginalRequest = irp;
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    srb->SenseInfoBuffer = DeviceExtension->ScratchContext.ScratchSense;

    srb->CdbLength = 16; // to cause failures if not set correctly -- CD devices limited to 12 bytes for now...

    srb->DataTransferLength = min(DeviceExtension->ScratchContext.ScratchBufferSize, MaximumTransferLength);
    srb->TimeOutValue = DeviceExtension->TimeOutValue;
    srb->SrbFlags = DeviceExtension->SrbFlags;
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);

    if (MaximumTransferLength == 0)
    {
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);
    }
    else if (GetDataFromDevice)
    {
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
    }
    else
    {
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_OUT);
    }
}


NTSTATUS
ScratchBuffer_SendSrb(
    _Inout_     PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_        BOOLEAN                 SynchronousSrb,
    _When_(SynchronousSrb, _Pre_null_)
    _When_(!SynchronousSrb, _In_opt_)
                PSRB_HISTORY_ITEM       *SrbHistoryItem
    )
/*++

Routine Description:

    Send the command from the scratch SRB to lower driver and retry if necessary.

Arguments:

    DeviceExtension - device extension
    SynchronousSrb - indicates whether the SRB needs to be sent synchronously or nor
    SrbHistoryItem - storage for SRB history item, if this is an asynchronous request

Return Value:

    none

--*/
{
    NTSTATUS                 status  = STATUS_SUCCESS;
    PSCSI_REQUEST_BLOCK      srb = DeviceExtension->ScratchContext.ScratchSrb;
    PSRB_HISTORY             history = DeviceExtension->ScratchContext.ScratchHistory;
    PSRB_HISTORY_ITEM        item = NULL;
    BOOLEAN                  requestCancelled = FALSE;

    srb->InternalStatus = 0;
    srb->SrbStatus = 0;

    // allocate/update history pre-command, if it is a synchronous request or we were supplied
    // with a storage for the history item
    if (SynchronousSrb || SrbHistoryItem != NULL)
    {
        // sending a packet implies a new history unit is to be used.
        NT_ASSERT( history->UsedHistoryCount <= history->TotalHistoryCount );

        // if already all used up, remove at least one history unit
        if (history->UsedHistoryCount == history->TotalHistoryCount )
        {
            CompressSrbHistoryData(history);
            NT_ASSERT( history->UsedHistoryCount < history->TotalHistoryCount );
        }

        // thus, since we are about to increment the count, it must now be less...
        NT_ASSERT( history->UsedHistoryCount < history->TotalHistoryCount );

        // increment the number of history units in use
        history->UsedHistoryCount++;

        // determine index to use
        item = &( history->History[ history->UsedHistoryCount-1 ] );

        if (SrbHistoryItem != NULL)
        {
            *SrbHistoryItem = item;
        }

        // zero out the history item
        RtlZeroMemory(item, sizeof(SRB_HISTORY_ITEM));

        // Query the tick count and store in the history
        KeQueryTickCount(&item->TickCountSent);
    }

    // get cancellation status;
    {
        PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(DeviceExtension->ScratchContext.ScratchRequest);

        if (requestContext->OriginalRequest != NULL)
        {
            requestCancelled = WdfRequestIsCanceled(requestContext->OriginalRequest);
        }
    }

    if (!requestCancelled)
    {
        status = RequestSend(DeviceExtension,
                             DeviceExtension->ScratchContext.ScratchRequest,
                             DeviceExtension->IoTarget,
                             SynchronousSrb ? WDF_REQUEST_SEND_OPTION_SYNCHRONOUS : 0,
                             NULL);

        // If this is a synchronous request, update the history item immediately, including "normalized" sense data
        if (SynchronousSrb)
        {
            ULONG senseSize;

            // Query the tick count and store in the history
            KeQueryTickCount(&item->TickCountCompleted);

            // Copy the SRB Status
            item->SrbStatus = srb->SrbStatus;

            // Determine the amount of valid sense data
            if (srb->SenseInfoBufferLength >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength))
            {
                PSENSE_DATA sense = (PSENSE_DATA)srb->SenseInfoBuffer;
                senseSize = RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength) +
                            sense->AdditionalSenseLength;
                senseSize = min(senseSize, sizeof(SENSE_DATA));
            }
            else
            {
                senseSize = srb->SenseInfoBufferLength;
            }

            // Normalize the sense data copy in the history
            RtlZeroMemory(&(item->NormalizedSenseData), sizeof(SENSE_DATA));
            RtlCopyMemory(&(item->NormalizedSenseData), srb->SenseInfoBuffer, senseSize);
        }
    }
    else
    {
        DeviceExtension->ScratchContext.ScratchSrb->SrbStatus = SRB_STATUS_ABORTED;
        DeviceExtension->ScratchContext.ScratchSrb->InternalStatus = (ULONG)STATUS_CANCELLED;
        status = STATUS_CANCELLED;
    }

    return status;
}

VOID
CompressSrbHistoryData(
    _Inout_  PSRB_HISTORY   RequestHistory
    )
/*++

Routine Description:

    compress the SRB history data.

Arguments:

    RequestHistory - SRB history data

Return Value:

    RequestHistory - compressed history data

--*/
{
    ULONG i;
    NT_ASSERT( RequestHistory->UsedHistoryCount == RequestHistory->TotalHistoryCount );
    ValidateSrbHistoryDataPresumptions(RequestHistory);

    for (i=0; i < RequestHistory->UsedHistoryCount; i++)
    {
        // for each item...
        PSRB_HISTORY_ITEM toMatch = &( RequestHistory->History[i] );
        // hint: read const qualifiers backwards.  i.e. srbstatus is a const UCHAR
        // so, "UCHAR const * const x" is read "x is a const pointer to a const UCHAR"
        // unfortunately, "const UCHAR" is equivalent to "UCHAR const", which causes
        // people no end of confusion due to its widespread use.
        UCHAR const srbStatus = toMatch->SrbStatus;
        UCHAR const sense     = toMatch->NormalizedSenseData.SenseKey;
        UCHAR const asc       = toMatch->NormalizedSenseData.AdditionalSenseCode;
        UCHAR const ascq      = toMatch->NormalizedSenseData.AdditionalSenseCodeQualifier;
        ULONG j;

        // see if there are any at higher indices with identical Sense/ASC/ASCQ
        for (j = i+1; (toMatch->ClassDriverUse != 0xFF) && (j < RequestHistory->UsedHistoryCount); j++)
        {
            PSRB_HISTORY_ITEM found = &( RequestHistory->History[j] );
            // close enough match?
            if ((srbStatus == found->SrbStatus) &&
                (sense     == found->NormalizedSenseData.SenseKey) &&
                (asc       == found->NormalizedSenseData.AdditionalSenseCode) &&
                (ascq      == found->NormalizedSenseData.AdditionalSenseCodeQualifier)) {

                // add the fields to keep reasonable track of delay times.
                if (toMatch->MillisecondsDelayOnRetry + found->MillisecondsDelayOnRetry < toMatch->MillisecondsDelayOnRetry) {
                    toMatch->MillisecondsDelayOnRetry = MAXULONG;
                } else {
                    toMatch->MillisecondsDelayOnRetry += found->MillisecondsDelayOnRetry;
                }

                // this found item cannot contain any compressed entries because
                // the first entry with a given set of sense/asc/ascq will always
                // either be full (0xFF) or be the only partially-full entry with
                // that sense/asc/ascq.
                NT_ASSERT(found->ClassDriverUse == 0);
                // add the counts so we still know how many retries total
                toMatch->ClassDriverUse++;


                // if not the last entry, need to move later entries earlier in the array
                if (j != RequestHistory->UsedHistoryCount-1) {
                    // how many entries remain?
                    SIZE_T remainingBytes = RequestHistory->UsedHistoryCount - 1 - j;
                    remainingBytes *= sizeof(SRB_HISTORY_ITEM);

                    // note that MOVE is required due to overlapping entries
                    RtlMoveMemory(found, found+1, remainingBytes);

                    // Finally, decrement the number of used history count and
                    // decrement j to rescan the current location again
                    --RequestHistory->UsedHistoryCount;
                    --j;
                } // end moving of array elements around
            } // end of close enough match
        } // end j loop
    } // end i loop

    // unable to compress duplicate sense/asc/ascq, so just lose the most recent data
    if (RequestHistory->UsedHistoryCount == RequestHistory->TotalHistoryCount)
    {
        PSRB_HISTORY_ITEM item = &( RequestHistory->History[ RequestHistory->TotalHistoryCount-1 ] );
        RequestHistory->ClassDriverUse[0] += item->ClassDriverUse; // how many did we "lose"?
        RequestHistory->UsedHistoryCount--;
    }

    // finally, zero any that are no longer in use
    NT_ASSERT( RequestHistory->UsedHistoryCount != RequestHistory->TotalHistoryCount);
    {
        SIZE_T bytesToZero = RequestHistory->TotalHistoryCount - RequestHistory->UsedHistoryCount;
        bytesToZero *= sizeof(SRB_HISTORY_ITEM);
        RtlZeroMemory(&(RequestHistory->History[RequestHistory->UsedHistoryCount]), bytesToZero);
    }

    ValidateSrbHistoryDataPresumptions(RequestHistory);
    return;
}

VOID
ValidateSrbHistoryDataPresumptions(
    _In_     SRB_HISTORY const * RequestHistory
    )
{
#if DBG
    // validate that all fully-compressed items are before any non-fully-compressed items of any particular sense/asc/ascq
    // validate that there is at most one partially-compressed item of any particular sense/asc/ascq
    // validate that all items of any particular sense/asc/ascq that are uncompressed are at the end
    // THUS: A(255) A(255) A( 40) A(  0) A(  0) is legal for all types with A as sense/asc/ascq
    //       A(0)   B(255) A(  0) B( 17) B(  0) is also legal because A/B are different types of error

    ULONG i;
    for (i = 0; i < RequestHistory->UsedHistoryCount; i++)
    {
        SRB_HISTORY_ITEM const * toMatch = &( RequestHistory->History[i] );
        UCHAR const srbStatus = toMatch->SrbStatus;
        UCHAR const sense     = toMatch->NormalizedSenseData.SenseKey;
        UCHAR const asc       = toMatch->NormalizedSenseData.AdditionalSenseCode;
        UCHAR const ascq      = toMatch->NormalizedSenseData.AdditionalSenseCodeQualifier;
        ULONG j;

        BOOLEAN foundPartiallyCompressedItem =
            (toMatch->ClassDriverUse !=    0) &&
            (toMatch->ClassDriverUse != 0xFF) ;
        BOOLEAN foundUncompressedItem =
            (toMatch->ClassDriverUse ==    0) ;

        for (j = i+1; j < RequestHistory->UsedHistoryCount; j++)
        {
            SRB_HISTORY_ITEM const * found = &( RequestHistory->History[j] );
            if ((srbStatus == found->SrbStatus) &&
                (sense     == found->NormalizedSenseData.SenseKey) &&
                (asc       == found->NormalizedSenseData.AdditionalSenseCode) &&
                (ascq      == found->NormalizedSenseData.AdditionalSenseCodeQualifier)
                )
            {
                // found a matching type, so validate ordering rules
                if (foundUncompressedItem && (found->ClassDriverUse != 0))
                {
                    DbgPrintEx(DPFLTR_CDROM_ID, DPFLTR_ERROR_LEVEL,
                               "History data has compressed history following uncompressed history "
                               "for srbstatus/sense/asc/ascq of %02x/%02x/%02x/%02x at indices %d (%08x) and %d (%08x)\n",
                               srbStatus, sense, asc, ascq,
                               i,i, j,j
                               );
                    NT_ASSERT(FALSE);
                }
                else if (foundPartiallyCompressedItem && (found->ClassDriverUse == 0xFF))
                {
                    DbgPrintEx(DPFLTR_CDROM_ID, DPFLTR_ERROR_LEVEL,
                               "History data has fully compressed history following partially compressed history "
                               "for srbstatus/sense/asc/ascq of %02x/%02x/%02x/%02x at indices %d (%08x) and %d (%08x)\n",
                               srbStatus, sense, asc, ascq,
                               i,i, j,j
                               );
                    NT_ASSERT(FALSE);
                }

                // update if we have now found partially compressed and/or uncompressed items
                if (found->ClassDriverUse == 0)
                {
                    foundUncompressedItem = TRUE;
                }
                else if (found->ClassDriverUse != 0xFF)
                {
                    foundPartiallyCompressedItem = TRUE;
                }
            } // end match of (toMatch,found)
        } // end loop j
    } // end loop i
#else
    UNREFERENCED_PARAMETER(RequestHistory);
#endif
    return;
}

VOID
ScratchBuffer_SetupReadWriteSrb(
    _Inout_ PCDROM_DEVICE_EXTENSION     DeviceExtension,
    _In_    WDFREQUEST                  OriginalRequest,
    _In_    LARGE_INTEGER               StartingOffset,
    _In_    ULONG                       RequiredLength,
    _Inout_updates_bytes_(RequiredLength) UCHAR* DataBuffer,
    _In_    BOOLEAN                     IsReadRequest,
    _In_    BOOLEAN                     UsePartialMdl
    )
/*++

Routine Description:

    setup SRB for read/write request.

Arguments:

    DeviceExtension - device extension
    OriginalRequest - read/write request
    StartingOffset - read/write starting offset
    DataBuffer - buffer for read/write
    IsReadRequest - TRUE (read); FALSE (write)

Return Value:

    none

--*/
{
    //NOTE: R/W request not use the ScratchBuffer, instead, it uses the buffer associated with IRP.

    PSCSI_REQUEST_BLOCK srb = DeviceExtension->ScratchContext.ScratchSrb;
    PCDB                cdb = (PCDB)srb->Cdb;
    LARGE_INTEGER       logicalBlockAddr;
    ULONG               numTransferBlocks;

    PIRP                originalIrp = WdfRequestWdmGetIrp(OriginalRequest);

    PIRP                irp = WdfRequestWdmGetIrp(DeviceExtension->ScratchContext.ScratchRequest);
    PIO_STACK_LOCATION  irpStack = NULL;

    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(DeviceExtension->ScratchContext.ScratchRequest);

    requestContext->OriginalRequest = OriginalRequest;


    logicalBlockAddr.QuadPart = Int64ShrlMod32(StartingOffset.QuadPart, DeviceExtension->SectorShift);
    numTransferBlocks = RequiredLength >> DeviceExtension->SectorShift;

    // set to use the full scratch buffer via the scratch SRB
    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    if (IsReadRequest)
    {
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
    }
    else
    {
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_OUT;
    }
    irpStack->Parameters.Scsi.Srb = srb;

    // prepare the SRB with default values
    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->SrbStatus = 0;
    srb->ScsiStatus = 0;
    srb->NextSrb = NULL;
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    srb->SenseInfoBuffer = DeviceExtension->ScratchContext.ScratchSense;

    srb->DataBuffer = DataBuffer;
    srb->DataTransferLength = RequiredLength;

    srb->QueueSortKey = logicalBlockAddr.LowPart;
    if (logicalBlockAddr.QuadPart > 0xFFFFFFFF)
    {
        //
        // If the requested LBA is more than max ULONG set the
        // QueueSortKey to the maximum value, so that these
        // requests can be added towards the end of the queue.
        //
        srb->QueueSortKey = 0xFFFFFFFF;
    }

    srb->OriginalRequest = irp;
    srb->TimeOutValue = DeviceExtension->TimeOutValue;

    if (RequestIsRealtimeStreaming(OriginalRequest, IsReadRequest) &&
        !TEST_FLAG(DeviceExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_STREAMING))
    {
        if (IsReadRequest)
        {
            RtlZeroMemory(&cdb->READ12, sizeof(cdb->READ12));
            REVERSE_BYTES(&cdb->READ12.LogicalBlock, &logicalBlockAddr.LowPart);
            REVERSE_BYTES(&cdb->READ12.TransferLength, &numTransferBlocks);
            cdb->READ12.Streaming = 1;
            cdb->READ12.OperationCode = SCSIOP_READ12;
            srb->CdbLength = sizeof(cdb->READ12);
        }
        else
        {
            RtlZeroMemory(&cdb->WRITE12, sizeof(cdb->WRITE12));
            REVERSE_BYTES(&cdb->WRITE12.LogicalBlock, &logicalBlockAddr.LowPart);
            REVERSE_BYTES(&cdb->WRITE12.TransferLength, &numTransferBlocks);
            cdb->WRITE12.Streaming = 1;
            cdb->WRITE12.OperationCode = SCSIOP_WRITE12;
            srb->CdbLength = sizeof(cdb->WRITE12);
        }
    }
    else
    {
        RtlZeroMemory(&cdb->CDB10, sizeof(cdb->CDB10));
        cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte3;
        cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte2;
        cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte1;
        cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte0;
        cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&numTransferBlocks)->Byte1;
        cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&numTransferBlocks)->Byte0;
        cdb->CDB10.OperationCode = (IsReadRequest) ? SCSIOP_READ : SCSIOP_WRITE;
        srb->CdbLength = sizeof(cdb->CDB10);
    }

    //  Set SRB and IRP flags
    srb->SrbFlags = DeviceExtension->SrbFlags;
    if (TEST_FLAG(originalIrp->Flags, IRP_PAGING_IO) ||
        TEST_FLAG(originalIrp->Flags, IRP_SYNCHRONOUS_PAGING_IO))
    {
        SET_FLAG(srb->SrbFlags, SRB_CLASS_FLAGS_PAGING);
    }

    SET_FLAG(srb->SrbFlags, (IsReadRequest) ? SRB_FLAGS_DATA_IN : SRB_FLAGS_DATA_OUT);
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_ADAPTER_CACHE_ENABLE);

    //
    // If the request is not split, we can use the original IRP MDL.  If the
    // request needs to be split, we need to use a partial MDL.  The partial MDL
    // is needed because more than one driver might be mapping the same MDL
    // and this causes problems.
    //
    if (UsePartialMdl == FALSE)
    {
        irp->MdlAddress = originalIrp->MdlAddress;
    }
    else
    {
        if (DeviceExtension->ScratchContext.PartialMdlIsBuilt != FALSE)
        {
            MmPrepareMdlForReuse(DeviceExtension->ScratchContext.PartialMdl);
        }

        IoBuildPartialMdl(originalIrp->MdlAddress, DeviceExtension->ScratchContext.PartialMdl, srb->DataBuffer, srb->DataTransferLength);
        DeviceExtension->ScratchContext.PartialMdlIsBuilt = TRUE;
        irp->MdlAddress = DeviceExtension->ScratchContext.PartialMdl;
    }

    //DBGLOGSENDPACKET(Pkt);
    //HISTORYLOGSENDPACKET(Pkt);

    //
    // Set the original irp here for SFIO.
    //
    srb->SrbExtension = (PVOID)(originalIrp);

    return;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
ScratchBuffer_ExecuteCdbEx(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ WDFREQUEST             OriginalRequest,
    _In_ ULONG                      TransferSize,
    _In_ BOOLEAN                    GetDataFromDevice,
    _In_ PCDB                       Cdb,
    _In_ UCHAR                      OprationLength,
    _In_ ULONG                      TimeoutValue
    )
/*++

Routine Description:

    Use Scratch buffer to send the Cdb, check error and retry if necessary.

Arguments:

    DeviceExtension - device context
    OriginalRequest - original request that requires this CDB operation
    TransferSize - Data transfer size required
    GetFromDevice - TRUE if getting data from device.
    Cdb - SCSI command
    OprationLength - SCSI command length: 6, 10 or 12
    TimeoutValue - if > 0, use it as timeout value for command
                   if 0, use the default device timeout value

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PSCSI_REQUEST_BLOCK srb = DeviceExtension->ScratchContext.ScratchSrb;
    PCDB                cdb = (PCDB)(srb->Cdb);

    BOOLEAN             shouldRetry = TRUE;
    ULONG               timesAlreadyRetried = 0;
    LONGLONG            retryIn100nsUnits = 0;

    PAGED_CODE ();

    while (shouldRetry)
    {
        ScratchBuffer_SetupSrb(DeviceExtension, OriginalRequest, TransferSize, GetDataFromDevice);

        // Set up the SRB/CDB
        RtlCopyMemory(cdb, Cdb, sizeof(CDB));

        srb->CdbLength = OprationLength;

        if (TimeoutValue > 0)
        {
            srb->TimeOutValue = TimeoutValue;
        }

        ScratchBuffer_SendSrb(DeviceExtension, TRUE, NULL);

        if ((DeviceExtension->ScratchContext.ScratchSrb->SrbStatus == SRB_STATUS_ABORTED) &&
            (DeviceExtension->ScratchContext.ScratchSrb->InternalStatus == STATUS_CANCELLED))
        {
            shouldRetry = FALSE;
            status = STATUS_CANCELLED;
        }
        else
        {
            shouldRetry = RequestSenseInfoInterpretForScratchBuffer(DeviceExtension,
                                                                    timesAlreadyRetried,
                                                                    &status,
                                                                    &retryIn100nsUnits);
            if (shouldRetry)
            {
                LARGE_INTEGER t;
                t.QuadPart = -retryIn100nsUnits;
                timesAlreadyRetried++;
                KeDelayExecutionThread(KernelMode, FALSE, &t);
                // keep items clean
                ScratchBuffer_ResetItems(DeviceExtension, FALSE);
            }
        }
    }

    return status;
}


