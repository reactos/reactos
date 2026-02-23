/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_wavert.cpp
 * PURPOSE:         WaveRT IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortPinWaveRT : public CUnknownImpl<IPortPinWaveRT>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortPinWaveRT;
    CPortPinWaveRT(IUnknown *OuterUnknown){}
    virtual ~CPortPinWaveRT(){}

protected:

    IPortWaveRT * m_Port;
    IPortFilterWaveRT * m_Filter;
    KSPIN_DESCRIPTOR * m_KsPinDescriptor;
    PMINIPORTWAVERT m_Miniport;
    PMINIPORTWAVERTSTREAM m_Stream;
    PMINIPORTWAVERTSTREAMNOTIFICATION m_StreamNotification;
    PPORTWAVERTSTREAM m_PortStream;
    KSSTATE m_State;
    PKSDATAFORMAT m_Format;
    KSPIN_CONNECT * m_ConnectDetails;
    KSAUDIO_POSITION m_Position;

    KSRTAUDIO_HWLATENCY m_Latency;
    PKPROCESS m_UserProcess;
    PVOID m_UserAddress;
    PKEVENT m_UserEvent;

    PUCHAR m_CommonBuffer;
    ULONG m_CommonBufferSize;
    ULONG m_CommonBufferOffset;

    BOOL m_Capture;

    MEMORY_CACHING_TYPE m_CacheType;
    PMDL m_Mdl;

    PSUBDEVICE_DESCRIPTOR m_Descriptor;
    KSPIN_LOCK m_EventListLock;
    LIST_ENTRY m_EventList;

    NTSTATUS NTAPI HandleKsProperty(IN PIRP Irp);
    NTSTATUS NTAPI HandleKsStream(IN PIRP Irp);
    VOID NTAPI SetStreamState(IN KSSTATE State);
    friend VOID NTAPI SetStreamWorkerRoutine(IN PDEVICE_OBJECT  DeviceObject, IN PVOID  Context);
    friend VOID NTAPI CloseStreamRoutine(IN PDEVICE_OBJECT  DeviceObject, IN PVOID Context);
    friend VOID NTAPI WorkerStreamRoutine(IN PVOID Context);
    friend NTSTATUS NTAPI PinWaveRTAudioGetAudioBuffer(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveRTAudioGetHwLatency(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveRTAudioGetRTAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveRTAudioGetClockRegister(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveRTAudioGetBufferWithNotification(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveRTAudioRegisterNotificationEvent(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveRTAudioUnregisterNotificationEvent(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
};

typedef struct
{
    CPortPinWaveRT *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

NTSTATUS NTAPI PinWaveRTAudioGetAudioBuffer(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveRTAudioGetHwLatency(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveRTAudioGetRTAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveRTAudioGetClockRegister(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveRTAudioGetBufferWithNotification(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveRTAudioRegisterNotificationEvent(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveRTAudioUnregisterNotificationEvent(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

DEFINE_KSPROPERTY_RTAUDIOSET(
    PinWaveRTAudioSet,
    PinWaveRTAudioGetAudioBuffer,
    PinWaveRTAudioGetHwLatency,
    PinWaveRTAudioGetRTAudioPosition,
    PinWaveRTAudioGetClockRegister,
    PinWaveRTAudioGetBufferWithNotification,
    PinWaveRTAudioRegisterNotificationEvent,
    PinWaveRTAudioUnregisterNotificationEvent);


KSPROPERTY_SET PinWaveRTPropertySet[] = {
    {
        &KSPROPSETID_RtAudio,
        sizeof(PinWaveRTAudioSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM *)&PinWaveRTAudioSet,
        0,
        NULL
    }
};
//==================================================================================================================================
NTSTATUS
NTAPI
PinWaveRTAudioGetAudioBuffer(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PinWaveRTAudioGetHwLatency(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    PKSRTAUDIO_HWLATENCY HwLatency;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveRT *Pin;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveRT *)Descriptor->PortPin;

    // cast to output buffer
    HwLatency = (PKSRTAUDIO_HWLATENCY)Data;

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        RtlMoveMemory(HwLatency, &Pin->m_Latency, sizeof(KSRTAUDIO_HWLATENCY));
        Irp->IoStatus.Information = sizeof(KSRTAUDIO_HWLATENCY);
        return STATUS_SUCCESS;
    }
    // not supported
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PinWaveRTAudioGetRTAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PinWaveRTAudioGetClockRegister(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PinWaveRTAudioGetBufferWithNotification(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveRT *Pin;
    NTSTATUS Status;
    PKSRTAUDIO_BUFFER_PROPERTY_WITH_NOTIFICATION Property;
    PKSRTAUDIO_BUFFER Buffer;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveRT *)Descriptor->PortPin;

    // get input buffer
    Property = (PKSRTAUDIO_BUFFER_PROPERTY_WITH_NOTIFICATION)Request;

    Status = Pin->m_StreamNotification->AllocateBufferWithNotification(
        Property->NotificationCount,
        ROUND_UP(Property->RequestedBufferSize, PAGE_SIZE),
        &Pin->m_Mdl,
        &Pin->m_CommonBufferSize,
        &Pin->m_CommonBufferOffset,
        &Pin->m_CacheType);
    if (!NT_SUCCESS(Status))
    {
        // failed to allocate buffer
        DPRINT1("AllocateBufferWithNotification failed with %x\n", Status);
        return Status;
    }
    // get output buffer
    Buffer = (PKSRTAUDIO_BUFFER)Data;

    // get current process
    Pin->m_UserProcess = (PKPROCESS)IoGetCurrentProcess();

    // map buffer
    Pin->m_UserAddress = MmMapLockedPagesSpecifyCache(
        Pin->m_Mdl, UserMode, Pin->m_CacheType, Property->BaseAddress, FALSE, NormalPagePriority);
    if (!Pin->m_UserAddress)
    {
        DPRINT1("MmMapLockedPagesSpecifyCache failed with %x\n", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // return result
    Buffer->BufferAddress = Pin->m_UserAddress;
    Buffer->ActualBufferSize = Pin->m_CommonBufferSize;
    Buffer->CallMemoryBarrier = Pin->m_CacheType == MmWriteCombined;

    Irp->IoStatus.Information = sizeof(KSRTAUDIO_BUFFER);
    DPRINT1("PinWaveRTAudioGetBufferWithNotification success\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PinWaveRTAudioRegisterNotificationEvent(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveRT *Pin;
    NTSTATUS Status;
    PKSRTAUDIO_NOTIFICATION_EVENT_PROPERTY Property;

    DPRINT1("PinWaveRTAudioRegisterNotificationEvent entered\n");

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveRT *)Descriptor->PortPin;

    // get input buffer
    Property = (PKSRTAUDIO_NOTIFICATION_EVENT_PROPERTY)Request;

    // referece notification event
    Status = ObReferenceObjectByHandle(
        Property->NotificationEvent, 0, *ExEventObjectType, UserMode, (PVOID*) &Pin->m_UserEvent, NULL);
    if (!NT_SUCCESS(Status))
    {
        // failed to reference event
        DPRINT1("ObReferenceObjectByHandle failed with %x\n", Status);
        return Status;
    }

    // register event
    Status = Pin->m_StreamNotification->RegisterNotificationEvent(Pin->m_UserEvent);
    if (!NT_SUCCESS(Status))
    {
        // failed to reference event
        DPRINT1("RegisterNotificationEvent failed with %x\n", Status);
        return Status;
    }

    // done
    Irp->IoStatus.Information = 0;
    DPRINT1("PinWaveRTAudioRegisterNotificationEvent success\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PinWaveRTAudioUnregisterNotificationEvent(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data)
{
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveRT *Pin;
    NTSTATUS Status;
    PKSRTAUDIO_NOTIFICATION_EVENT_PROPERTY Property;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveRT *)Descriptor->PortPin;

    // get input buffer
    Property = (PKSRTAUDIO_NOTIFICATION_EVENT_PROPERTY)Request;

    if (Pin->m_UserEvent == NULL)
    {
        // reference notification event
        Status = ObReferenceObjectByHandle(
            Property->NotificationEvent, 0, *ExEventObjectType, UserMode, (PVOID*) &Pin->m_UserEvent, NULL);
        if (!NT_SUCCESS(Status))
        {
            // failed to reference event
            DPRINT1("ObReferenceObjectByHandle failed with %x\n", Status);
            return Status;
        }
    }
    // unregister event
    Status = Pin->m_StreamNotification->UnregisterNotificationEvent(Pin->m_UserEvent);
    if (!NT_SUCCESS(Status))
    {
        // failed to reference event
        DPRINT1("RegisterNotificationEvent failed with %x\n", Status);
        return Status;
    }
    Pin->m_UserEvent = NULL;

    // done
    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}

//==================================================================================================================================

NTSTATUS
NTAPI
CPortPinWaveRT::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    DPRINT("IServiceSink_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, IID_IIrpTarget) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN((IIrpTarget*)this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IPinWaveRT_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        ASSERT(FALSE);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

//==================================================================================================================================

NTSTATUS
NTAPI
CPortPinWaveRT::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWaveRT::HandleKsProperty(
    IN PIRP Irp)
{
    PKSPROPERTY Property;
    NTSTATUS Status;
    //UNICODE_STRING GuidString;
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //DPRINT1("IPortPinWave_HandleKsProperty entered\n");

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (IsEqualGUIDAligned(Property->Set, KSPROPSETID_Connection))
    {
        if (Property->Id == KSPROPERTY_CONNECTION_STATE)
        {
            PKSSTATE State = (PKSSTATE)Irp->UserBuffer;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSSTATE))
            {
                Irp->IoStatus.Information = sizeof(KSSTATE);
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                Status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;

                if (m_Stream)
                {
                    Status = m_Stream->SetState(*State);

                    DPRINT("Setting state %u %x\n", *State, Status);
                    if (NT_SUCCESS(Status))
                        m_State = *State;
                }
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                *State = m_State;
                Irp->IoStatus.Information = sizeof(KSSTATE);
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
        }
        else if (Property->Id == KSPROPERTY_CONNECTION_DATAFORMAT)
        {
            PKSDATAFORMAT DataFormat = (PKSDATAFORMAT)Irp->UserBuffer;
            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                PKSDATAFORMAT NewDataFormat;
                if (!RtlCompareMemory(DataFormat, m_Format, DataFormat->FormatSize))
                {
                    Irp->IoStatus.Information = DataFormat->FormatSize;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_SUCCESS;
                }

                NewDataFormat = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
                if (!NewDataFormat)
                {
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_NO_MEMORY;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_NO_MEMORY;
                }
                RtlMoveMemory(NewDataFormat, DataFormat, DataFormat->FormatSize);

                if (m_Stream)
                {
#if 0
                    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                    ASSERT(NewDataFormat->FormatSize == sizeof(KSDATAFORMAT_WAVEFORMATEX));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.MajorFormat, &KSDATAFORMAT_TYPE_AUDIO));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));

                    ASSERT(m_State == KSSTATE_STOP);
#endif
                    DPRINT("NewDataFormat: Channels %u Bits %u Samples %u\n", ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nChannels,
                                                                                 ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.wBitsPerSample,
                                                                                 ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nSamplesPerSec);

                    Status = m_Stream->SetFormat(NewDataFormat);
                    if (NT_SUCCESS(Status))
                    {
                        if (m_Format)
                            FreeItem(m_Format, TAG_PORTCLASS);

                        m_Format = NewDataFormat;
                        Irp->IoStatus.Information = DataFormat->FormatSize;
                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_SUCCESS;
                    }
                }
                DPRINT("Failed to set format\n");
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                if (!m_Format)
                {
                    DPRINT("No format\n");
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_UNSUCCESSFUL;
                }
                if (m_Format->FormatSize > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
                {
                    Irp->IoStatus.Information = m_Format->FormatSize;
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_BUFFER_TOO_SMALL;
                }

                RtlMoveMemory(DataFormat, m_Format, m_Format->FormatSize);
                Irp->IoStatus.Information = DataFormat->FormatSize;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
        }

    }
    /* handle property with subdevice descriptor */
    Status = PcHandlePropertyWithTable(
        Irp, m_Descriptor->FilterPropertySetCount, m_Descriptor->FilterPropertySet,
        m_Descriptor);
    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return Status;

    //RtlStringFromGUID(Property->Set, &GuidString);
    //DPRINT("Unhanded property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    //RtlFreeUnicodeString(&GuidString);

    //Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    //Irp->IoStatus.Information = 0;
    //IoCompleteRequest(Irp, IO_NO_INCREMENT);
    //return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CPortPinWaveRT::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_KS_PROPERTY:
            return HandleKsProperty(Irp);

        case IOCTL_KS_ENABLE_EVENT:
            /* FIXME UNIMPLEMENTED */
            UNIMPLEMENTED_ONCE;
            break;

        case IOCTL_KS_DISABLE_EVENT:
            /* FIXME UNIMPLEMENTED */
            UNIMPLEMENTED_ONCE;
            break;

        case IOCTL_KS_HANDSHAKE:
            /* FIXME UNIMPLEMENTED */
            UNIMPLEMENTED_ONCE;
            break;

        case IOCTL_KS_METHOD:
            /* FIXME UNIMPLEMENTED */
            UNIMPLEMENTED_ONCE;
            return KsDefaultDeviceIoCompletion(DeviceObject, Irp);

        default:
            return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWaveRT::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveRT::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveRT::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

VOID
NTAPI
CloseStreamRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID Context)
{
    PMINIPORTWAVERTSTREAM Stream;
    NTSTATUS Status;
    ISubdevice *ISubDevice;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveRT * This;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (CPortPinWaveRT*)Ctx->Pin;

    DPRINT("CloseStreamRoutine entered Irql %u\n", KeGetCurrentIrql());

    if (This->m_Stream)
    {
        if (This->m_State != KSSTATE_STOP)
        {
            DPRINT("Set state to stop %u\n", This->m_State);

            if (This->m_State == KSSTATE_RUN)
            {
                DPRINT("Setting to pause\n");
                This->m_Stream->SetState(KSSTATE_PAUSE);
                This->m_State = KSSTATE_PAUSE;
            }

            if (This->m_State == KSSTATE_PAUSE)
            {
                DPRINT("Setting to acquire\n");
                This->m_Stream->SetState(KSSTATE_ACQUIRE);
                This->m_State = KSSTATE_ACQUIRE;
            }
            if (This->m_State == KSSTATE_ACQUIRE)
            {
                DPRINT("Setting to stop\n");
                This->m_Stream->SetState(KSSTATE_STOP);
                This->m_State = KSSTATE_STOP;
            }
        }
    }

    if (This->m_StreamNotification)
    {
        if (This->m_UserAddress)
        {
            KAPC_STATE ApcState;
            PKPROCESS Process = (PKPROCESS)PsGetCurrentProcess();
            if (Process != This->m_UserProcess)
            {
                DPRINT1("Before KeStackAttachProcess\n");
                KeStackAttachProcess(This->m_UserProcess, &ApcState);
            }

            MmUnmapLockedPages(This->m_UserAddress, This->m_Mdl);

            if (Process != This->m_UserProcess)
            {
                DPRINT1("Before KeUnstackDetachProcess\n");
                KeUnstackDetachProcess(&ApcState);
            }

            This->m_UserAddress = NULL;
        }
        if (This->m_CommonBufferSize)
        {
            DPRINT("Before FreeBufferWithNotification\n");
            This->m_StreamNotification->FreeBufferWithNotification(This->m_Mdl, This->m_CommonBufferSize);
            This->m_Mdl = NULL;
            This->m_CommonBufferSize = 0;
        }
        DPRINT("Before UnregisterNotificationEvent\n");
        if (This->m_UserEvent)
        {
            This->m_StreamNotification->UnregisterNotificationEvent(This->m_UserEvent);
            ObDereferenceObject(This->m_UserEvent);
            This->m_UserEvent = NULL;
        }
        DPRINT("Before StreamNotification->Release\n");
        This->m_StreamNotification->Release();
        This->m_StreamNotification = NULL;
    }

    Status = This->m_Port->QueryInterface(IID_ISubdevice, (PVOID*)&ISubDevice);
    if (NT_SUCCESS(Status))
    {
        Status = ISubDevice->GetDescriptor(&Descriptor);
        if (NT_SUCCESS(Status))
        {
            Descriptor->Factory.Instances[This->m_ConnectDetails->PinId].CurrentPinInstanceCount--;
        }
        ISubDevice->Release();
    }

    if (This->m_Format)
    {
        FreeItem(This->m_Format, TAG_PORTCLASS);
        This->m_Format = NULL;
    }

    // complete the irp
    Ctx->Irp->IoStatus.Information = 0;
    Ctx->Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Ctx->Irp, IO_NO_INCREMENT);

    // free the work item
    IoFreeWorkItem(Ctx->WorkItem);

    // free work item ctx
    FreeItem(Ctx, TAG_PORTCLASS);

    if (This->m_Stream)
    {
        Stream = This->m_Stream;
        This->m_Stream = NULL;
        DPRINT("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->Release();
    }

    DPRINT("Freeing Pin %p\n", This);
    This->Release();
}

NTSTATUS
NTAPI
CPortPinWaveRT::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;

    if (m_Stream)
    {
        Ctx = (PCLOSESTREAM_CONTEXT)AllocateItem(NonPagedPool, sizeof(CLOSESTREAM_CONTEXT), TAG_PORTCLASS);
        if (!Ctx)
        {
            DPRINT("Failed to allocate stream context\n");
            goto cleanup;
        }

        Ctx->WorkItem = IoAllocateWorkItem(DeviceObject);
        if (!Ctx->WorkItem)
        {
            DPRINT("Failed to allocate work item\n");
            goto cleanup;
        }

        Ctx->Irp = Irp;
        Ctx->Pin = this;

        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;

        // defer work item
        IoQueueWorkItem(Ctx->WorkItem, CloseStreamRoutine, DelayedWorkQueue, (PVOID)Ctx);
        // Return result
        return STATUS_PENDING;
    }
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

cleanup:

    if (Ctx)
        FreeItem(Ctx, TAG_PORTCLASS);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;

}

NTSTATUS
NTAPI
CPortPinWaveRT::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveRT::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortPinWaveRT::FastDeviceIoControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}

BOOLEAN
NTAPI
CPortPinWaveRT::FastRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}

BOOLEAN
NTAPI
CPortPinWaveRT::FastWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}

NTSTATUS
NTAPI
CPortPinWaveRT::Init(
    IN PPORTWAVERT Port,
    IN PPORTFILTERWAVERT Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    ISubdevice *Subdevice = NULL;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor = NULL;

    Port->AddRef();
    Filter->AddRef();

    m_Port = Port;
    m_Filter = Filter;
    m_KsPinDescriptor = KsPinDescriptor;
    m_ConnectDetails = ConnectDetails;
    m_Miniport = GetWaveRTMiniport(Port);

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("CPortPinWaveRT::Init entered\n");

    m_Format = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!m_Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(m_Format, DataFormat, DataFormat->FormatSize);

    Status = NewPortWaveRTStream(&m_PortStream);
    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }
    if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_IN)
    {
        m_Capture = FALSE;
    }
    else if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        m_Capture = TRUE;
    }
    else
    {
        DPRINT("Unexpected Communication %u DataFlow %u\n", KsPinDescriptor->Communication, KsPinDescriptor->DataFlow);
        KeBugCheck(0);
        while(TRUE);
    }
    Status = m_Miniport->NewStream(&m_Stream, m_PortStream, ConnectDetails->PinId, m_Capture, m_Format);
    DPRINT("CPortPinWaveRT::Init Status %x\n", Status);
    if (!NT_SUCCESS(Status))
        goto cleanup;

    // get subdevice interface
    Status = Port->QueryInterface(IID_ISubdevice, (PVOID *)&Subdevice);

    if (!NT_SUCCESS(Status))
        goto cleanup;

    Status = Subdevice->GetDescriptor(&SubDeviceDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // failed to get descriptor
        Subdevice->Release();
        goto cleanup;
    }

    /* initialize event management */
    InitializeListHead(&m_EventList);
    KeInitializeSpinLock(&m_EventListLock);

    Status = PcCreateSubdeviceDescriptor(
        &m_Descriptor, SubDeviceDescriptor->InterfaceCount, SubDeviceDescriptor->Interfaces,
        0, /* FIXME KSINTERFACE_STANDARD with KSINTERFACE_STANDARD_STREAMING / KSINTERFACE_STANDARD_LOOPED_STREAMING */
        NULL,
        sizeof(PinWaveRTPropertySet) / sizeof(KSPROPERTY_SET),
        PinWaveRTPropertySet,
        0,
        0,
        0,
        NULL,
        0,
        NULL,
        SubDeviceDescriptor->DeviceDescriptor);

    m_Descriptor->UnknownStream = (PUNKNOWN)m_Stream;
    m_Descriptor->UnknownMiniport = SubDeviceDescriptor->UnknownMiniport;
    m_Descriptor->PortPin = (PVOID)this;
    m_Descriptor->EventList = &m_EventList;
    m_Descriptor->EventListLock = &m_EventListLock;

    // release subdevice descriptor
    Subdevice->Release();

    m_Stream->GetHWLatency(&m_Latency);
    DPRINT("Latency FifoSize %u ChipsetDelay %u CodecDelay %u\n", m_Latency.FifoSize, m_Latency.ChipsetDelay, m_Latency.CodecDelay);

    Status = m_Stream->QueryInterface(IID_IMiniportWaveRTStreamNotification, (PVOID*)&m_StreamNotification);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("QueryInterface failed with %x\n", Status);
        goto cleanup;
    }
    m_State = KSSTATE_STOP;
    return STATUS_SUCCESS;

cleanup:
    if (m_Format)
    {
        FreeItem(m_Format, TAG_PORTCLASS);
        m_Format = NULL;
    }

    if (m_StreamNotification)
    {
        if (m_CommonBuffer)
        {
            m_StreamNotification->FreeBufferWithNotification(m_Mdl, m_CommonBufferSize);
            m_Mdl = NULL;
            m_CommonBufferSize = 0;
            m_CommonBuffer = NULL;
            m_CommonBufferOffset = 0;
        }
        m_StreamNotification->Release();
        m_StreamNotification = NULL;
    }

    if (m_Stream)
    {
        m_Stream->Release();
        m_Stream = NULL;
    }
    else
    {
        if (m_PortStream)
        {
            m_PortStream->Release();
            m_PortStream = NULL;
        }

    }
    return Status;
}

NTSTATUS
NewPortPinWaveRT(
    OUT IPortPinWaveRT ** OutPin)
{
    CPortPinWaveRT * This;

    This = new(NonPagedPool, TAG_PORTCLASS) CPortPinWaveRT(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // store result
    *OutPin = (PPORTPINWAVERT)This;

    return STATUS_SUCCESS;
}
