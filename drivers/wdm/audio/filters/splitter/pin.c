/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/filters/splitter/pin.c
 * PURPOSE:         Pin Context Handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

NTSTATUS
NTAPI
PinCreate(
    IN PKSPIN  Pin,
    IN PIRP  Irp)
{
    PKSFILTER Filter;
    PKSPIN FirstPin;
    PPIN_CONTEXT PinContext;

    /* first get the parent filter */
    Filter = KsPinGetParentFilter(Pin);

    /* now get first child pin */
    FirstPin = KsFilterGetFirstChildPin(Filter, Pin->Id);

    /* sanity check */
    ASSERT(FirstPin);

    if (FirstPin != Pin)
    {
        /* a previous pin already exists */
        if (RtlCompareMemory(FirstPin->ConnectionFormat, Pin->ConnectionFormat, Pin->ConnectionFormat->FormatSize) != Pin->ConnectionFormat->FormatSize)
        {
            /* each instantiated pin must have the same connection format */
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* allocate pin context */
    PinContext = ExAllocatePool(NonPagedPool, sizeof(PIN_CONTEXT));
    if (!PinContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* store pin context */
    Pin->Context = PinContext;

    /* clear pin context */
    RtlZeroMemory(PinContext, sizeof(PIN_CONTEXT));

    /* FIXME
     * check allocator framing and apply to all pins
     */

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PinClose(
    IN PKSPIN  Pin,
    IN PIRP  Irp)
{
    /* is there a context */
    if (Pin->Context)
    {
        /* free pin context */
        ExFreePool(Pin->Context);
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
PinReset(
    IN PKSPIN  Pin)
{
    PKSFILTER Filter;

    /* sanity check */
    ASSERT(Pin->Context);

    /* clear pin context */
    RtlZeroMemory(Pin->Context, sizeof(PIN_CONTEXT));

    /* get parent filter */
    Filter = KsPinGetParentFilter(Pin);

    /* sanity check */
    ASSERT(Filter);

    /* attempt processing */
    KsFilterAttemptProcessing(Filter, TRUE);
}

NTSTATUS
NTAPI
PinState(
    IN PKSPIN  Pin,
    IN KSSTATE  ToState,
    IN KSSTATE  FromState)
{
    PKSFILTER Filter;

    /* should the pin stop */
    if (ToState == KSSTATE_STOP)
    {
        /* clear pin context */
        RtlZeroMemory(Pin->Context, sizeof(PIN_CONTEXT));
    }

    /* get parent filter */
    Filter = KsPinGetParentFilter(Pin);

    /* sanity check */
    ASSERT(Filter);

    /* attempt processing */
    KsFilterAttemptProcessing(Filter, TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AudioPositionPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PKSFILTER Filter;
    PKSPIN Pin, FirstPin;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    ULONG BytesReturned;

    /* first get the pin */
    Pin = KsGetPinFromIrp(Irp);

    /* sanity check */
    ASSERT(Pin);

    /* get parent filter */
    Filter = KsPinGetParentFilter(Pin);

    /* acquire filter control mutex */
    KsFilterAcquireControl(Filter);

    /* get first pin */
    FirstPin = KsFilterGetFirstChildPin(Filter, Pin->Id);

    /* get connected pin of first pin */
    FileObject = KsPinGetConnectedPinFileObject(FirstPin);

    if (!FileObject)
    {
        /* no pin connected */
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* perform request */
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)Request, sizeof(KSPROPERTY), Data, sizeof(KSAUDIO_POSITION), &BytesReturned);

        /* store result size */
        Irp->IoStatus.Information = sizeof(KSAUDIO_POSITION);
    }

    /* release control */
    KsFilterReleaseControl(Filter);

    /* done */
    return Status;
}

NTSTATUS
NTAPI
PinIntersectHandler(
    IN PVOID Context,
    IN PIRP Irp,
    IN PKSP_PIN Pin,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize)
{
    PKSPIN FirstPin;
    NTSTATUS Status;

    /* get first pin */
    FirstPin = KsFilterGetFirstChildPin((PKSFILTER)Context, Pin->PinId);

    /* sanity check */
    ASSERT(FirstPin);

    /* check for matching dataformat */
    if (!IsEqualGUIDAligned(&FirstPin->ConnectionFormat->SubFormat, &DataRange->SubFormat) ||
        !IsEqualGUIDAligned(&FirstPin->ConnectionFormat->Specifier, &DataRange->Specifier) ||
        !IsEqualGUIDAligned(&GUID_NULL, &DataRange->SubFormat) ||
        !IsEqualGUIDAligned(&GUID_NULL, &DataRange->Specifier))
    {
        /* no match */
        return STATUS_NO_MATCH;
    }


    if (DataBufferSize)
    {
        /* there is output buffer */
        if (DataBufferSize >= FirstPin->ConnectionFormat->FormatSize)
        {
            /* copy dataformat */
            RtlMoveMemory(Data, FirstPin->ConnectionFormat, FirstPin->ConnectionFormat->FormatSize);

            /* store output length */
            *DataSize = FirstPin->ConnectionFormat->FormatSize;

            Status = STATUS_SUCCESS;
        }
        else
        {
            /* buffer too small */
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* store output length */
        *DataSize = FirstPin->ConnectionFormat->FormatSize;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    /* done */
    return Status;
}

