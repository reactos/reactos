/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

const GUID KSPROPSETID_Connection               = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSINTERFACESETID_Standard            = {0x1A8766A0L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard               = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SUBTYPE_PCM             = {0x00000001L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};


NTSTATUS
SetIrpIoStatus(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Length)
{
    Irp->IoStatus.Information = Length;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;

}

NTSTATUS
WdmAudControlOpen(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PSYSAUDIO_INSTANCE_INFO InstanceInfo;
    ULONG BytesReturned;
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess = 0;
    HANDLE PinHandle;
    KSPIN_CONNECT * PinConnect;
    KSDATAFORMAT_WAVEFORMATEX * DataFormat;

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE)
    {
        DPRINT1("FIXME: only waveout devices are supported\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    InstanceInfo = ExAllocatePool(NonPagedPool, sizeof(KSDATAFORMAT_WAVEFORMATEX) + sizeof(KSPIN_CONNECT));
    if (!InstanceInfo)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    InstanceInfo->Property.Set = KSPROPSETID_Sysaudio;
    InstanceInfo->Property.Id = KSPROPERTY_SYSAUDIO_INSTANCE_INFO;
    InstanceInfo->Property.Flags = KSPROPERTY_TYPE_SET;
    InstanceInfo->Flags = 0;
    InstanceInfo->DeviceNumber = DeviceInfo->DeviceIndex;

    Status = KsSynchronousIoControlDevice(ClientInfo->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)InstanceInfo, sizeof(SYSAUDIO_INSTANCE_INFO), NULL, 0, &BytesReturned);

    if (!NT_SUCCESS(Status))
    {
        /* failed to acquire audio device */
        DPRINT1("KsSynchronousIoControlDevice failed with %x\n", Status);
        ExFreePool(InstanceInfo);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        DesiredAccess |= GENERIC_READ;
    }

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE ||
        DeviceInfo->DeviceType == AUX_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        DesiredAccess |= GENERIC_WRITE;
    }

    PinConnect = (KSPIN_CONNECT*)InstanceInfo;


    PinConnect->Interface.Set = KSINTERFACESETID_Standard;
    PinConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    PinConnect->Interface.Flags = 0;
    PinConnect->Medium.Set = KSMEDIUMSETID_Standard;
    PinConnect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    PinConnect->Medium.Flags = 0;
    PinConnect->PinId = 0; //FIXME
    PinConnect->PinToHandle = NULL;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = 1;


    DataFormat = (KSDATAFORMAT_WAVEFORMATEX*) (PinConnect + 1);
    DataFormat->WaveFormatEx.wFormatTag = DeviceInfo->u.WaveFormatEx.wFormatTag;
    DataFormat->WaveFormatEx.nChannels = DeviceInfo->u.WaveFormatEx.nChannels;
    DataFormat->WaveFormatEx.nSamplesPerSec = DeviceInfo->u.WaveFormatEx.nSamplesPerSec;
    DataFormat->WaveFormatEx.nBlockAlign = DeviceInfo->u.WaveFormatEx.nBlockAlign;
    DataFormat->WaveFormatEx.nAvgBytesPerSec = DeviceInfo->u.WaveFormatEx.nAvgBytesPerSec;
    DataFormat->WaveFormatEx.wBitsPerSample = DeviceInfo->u.WaveFormatEx.wBitsPerSample;
    DataFormat->WaveFormatEx.cbSize = 0;
    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = 0;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

    if (DeviceInfo->u.WaveFormatEx.wFormatTag != WAVE_FORMAT_PCM)
        DPRINT1("FIXME\n");

    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = 4;


    Status = KsCreatePin(ClientInfo->hSysAudio, PinConnect, DesiredAccess, &PinHandle);
    DPRINT1("KsCreatePin Status %x\n", Status);


    /* free buffer */
    ExFreePool(InstanceInfo);

    if (NT_SUCCESS(Status))
    {
        DeviceInfo->hDevice = PinHandle;
    }
    else
    {
        DeviceInfo->hDevice = NULL;
    }

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudControlDeviceType(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    KSPROPERTY Property;
    ULONG Result, BytesReturned;
    NTSTATUS Status;

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE)
    {
        DPRINT1("FIXME: only waveout devices are supported\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Property.Set = KSPROPSETID_Sysaudio;
    Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Property.Flags = KSPROPERTY_TYPE_GET;

    Status = KsSynchronousIoControlDevice(ClientInfo->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&Result, sizeof(ULONG), &BytesReturned);

    if (NT_SUCCESS(Status))
        DeviceInfo->DeviceCount = Result;
    else
        DeviceInfo->DeviceCount = 0;

    DPRINT1("WdmAudControlDeviceType Status %x Devices %u\n", Status, DeviceInfo->DeviceCount);
    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudControlDeviceState(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    KSPROPERTY Property;
    KSSTATE State;
    NTSTATUS Status;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE)
    {
        DPRINT1("FIXME: only waveout devices are supported\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: invalid device handle provided %p\n", DeviceInfo->hDevice);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    State = DeviceInfo->State;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);

    ObDereferenceObject(FileObject);

    DPRINT1("WdmAudControlDeviceState Status %x\n", Status);
    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudWriteCompleted(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Ctx)
{
    PWRITE_CONTEXT Context = (PWRITE_CONTEXT)Ctx;

    Context->Irp->IoStatus.Information = Context->Length;
    Context->Irp->IoStatus.Status = Irp->IoStatus.Status;
    IoCompleteRequest(Context->Irp, IO_SOUND_INCREMENT);

    ExFreePool(Context);
    return STATUS_SUCCESS;
}

NTSTATUS
WdmAudControlWriteData(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PKSSTREAM_HEADER Packet;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PWRITE_CONTEXT Context;
    ULONG BytesReturned;
    PUCHAR Buffer;

    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid buffer handle %x\n", DeviceInfo->hDevice);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE)
    {
        DPRINT1("FIXME: only waveout devices are supported\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    _SEH2_TRY
    {
        ProbeForRead(DeviceInfo->Buffer, DeviceInfo->BufferSize, TYPE_ALIGNMENT(char));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Exception, get the error code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid buffer supplied\n");
        return SetIrpIoStatus(Irp, Status, 0);
    }

    Buffer = ExAllocatePool(NonPagedPool, DeviceInfo->BufferSize);
    if (!Buffer)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    RtlMoveMemory(Buffer, DeviceInfo->Buffer, DeviceInfo->BufferSize);


    Context = ExAllocatePool(NonPagedPool, sizeof(WRITE_CONTEXT));
    if (!Context)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    /* setup completion context */
    Context->Irp = Irp;
    Context->Length = DeviceInfo->BufferSize;

    /* setup stream context */
    Packet = (PKSSTREAM_HEADER)ExAllocatePool(NonPagedPool, sizeof(KSSTREAM_HEADER));
    Packet->Data = Buffer;
    Packet->FrameExtent = DeviceInfo->BufferSize;
    Packet->DataUsed = DeviceInfo->BufferSize;
    Packet->Size = sizeof(KSSTREAM_HEADER);
    Packet->PresentationTime.Numerator = 1;
    Packet->PresentationTime.Denominator = 1;
    ASSERT(FileObject->FsContext != NULL);


    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_WRITE_STREAM, (PVOID)Packet, sizeof(KSSTREAM_HEADER), NULL, 0, &BytesReturned);

    DPRINT1("KsSynchronousIoControlDevice result %x\n", Status);

    IoMarkIrpPending(Irp);
    Irp->IoStatus.Information = DeviceInfo->BufferSize;
    Irp->IoStatus.Status = Status;

    ExFreePool(Buffer);

    return Status;
}



NTSTATUS
NTAPI
WdmAudDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PWDMAUD_CLIENT ClientInfo;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("WdmAudDeviceControl entered\n");
    DbgBreakPoint();

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(WDMAUD_DEVICE_INFO))
    {
        /* invalid parameter */
        DPRINT1("Input buffer too small size %u expected %u\n", IoStack->Parameters.DeviceIoControl.InputBufferLength, sizeof(WDMAUD_DEVICE_INFO));
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    DeviceInfo = (PWDMAUD_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;

    if (DeviceInfo->DeviceType < MIN_SOUND_DEVICE_TYPE || DeviceInfo->DeviceType > MAX_SOUND_DEVICE_TYPE)
    {
        /* invalid parameter */
        DPRINT1("Error: device type not set\n");
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    if (!IoStack->FileObject)
    {
        /* file object parameter */
        DPRINT1("Error: file object is not attached\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }
    ClientInfo = (PWDMAUD_CLIENT)IoStack->FileObject->FsContext;

    DPRINT1("WdmAudDeviceControl entered\n");

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_OPEN_WDMAUD:
            return WdmAudControlOpen(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETNUMDEVS_TYPE:
            return WdmAudControlDeviceType(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_SETDEVICE_STATE:
            return WdmAudControlDeviceState(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_WRITEDATA:
            return WdmAudControlWriteData(DeviceObject, Irp, DeviceInfo, ClientInfo);

        case IOCTL_CLOSE_WDMAUD:
        case IOCTL_GETDEVID:
        case IOCTL_GETVOLUME:
        case IOCTL_SETVOLUME:
        case IOCTL_GETCAPABILITIES:
           DPRINT1("Unhandeled %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
           break;
    }

    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);
}
