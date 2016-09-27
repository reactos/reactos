/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/pin.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "usbaudio.h"

NTSTATUS
NTAPI
USBAudioPinCreate(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    PKSFILTER Filter;
    PFILTER_CONTEXT FilterContext;
    PPIN_CONTEXT PinContext;

    Filter = KsPinGetParentFilter(Pin);
    if (Filter == NULL)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* get filter context */
    FilterContext = Filter->Context;

    /* allocate pin context */
    PinContext = AllocFunction(sizeof(PIN_CONTEXT));
    if (!PinContext)
    {
        /* no memory*/
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init pin context */
    PinContext->DeviceExtension = FilterContext->DeviceExtension;
    PinContext->LowerDevice = FilterContext->LowerDevice;

    /* store pin context*/
    Pin->Context = PinContext;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBAudioPinClose(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
USBAudioPinProcess(
    _In_ PKSPIN Pin)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}


VOID
NTAPI
USBAudioPinReset(
    _In_ PKSPIN Pin)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
USBAudioPinSetDataFormat(
    _In_ PKSPIN Pin,
    _In_opt_ PKSDATAFORMAT OldFormat,
    _In_opt_ PKSMULTIPLE_ITEM OldAttributeList,
    _In_ const KSDATARANGE* DataRange,
    _In_opt_ const KSATTRIBUTE_LIST* AttributeRange)
{
    PURB Urb;
    PUCHAR SampleRateBuffer;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatEx;

    /* allocate sample rate buffer */
    SampleRateBuffer = AllocFunction(sizeof(ULONG));
    if (!SampleRateBuffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (IsEqualGUIDAligned(&Pin->ConnectionFormat->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO) && 
        IsEqualGUIDAligned(&Pin->ConnectionFormat->SubFormat,  &KSDATAFORMAT_SUBTYPE_PCM) &&
        IsEqualGUIDAligned(&Pin->ConnectionFormat->Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        WaveFormatEx = (PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat;
        SampleRateBuffer[0] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 16) & 0xFF;
        SampleRateBuffer[1] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 8) & 0xFF;
        SampleRateBuffer[2] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 0) & 0xFF;
    }
    else
    {
        /* not supported yet*/
        UNIMPLEMENTED;
        FreeFunction(SampleRateBuffer);
        return STATUS_INVALID_PARAMETER;
    }

    /* allocate urb */
    Urb = AllocFunction(sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        /* no memory */
        FreeFunction(SampleRateBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* format urb */
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_ENDPOINT,
                          sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                          USBD_TRANSFER_DIRECTION_OUT,
                          0,
                          0x01,
                          0x100,
                          0x81, //bEndpointAddress
                          SampleRateBuffer,
                          NULL,
                          3,
                          NULL);

    /* get pin context */
    PinContext = Pin->Context;
    DbgBreakPoint();
    /* submit urb */
    Status = SubmitUrbSync(PinContext->LowerDevice, Urb);

    DPRINT1("USBAudioPinSetDataFormat Pin %p Status %x\n", Pin, Status);
    FreeFunction(Urb);
    FreeFunction(SampleRateBuffer);
    return Status;
}

NTSTATUS
NTAPI
USBAudioPinSetDeviceState(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}
