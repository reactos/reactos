/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};

NTSTATUS
WdmAudControlInitialize(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    LPWSTR SymbolicLinkList;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    /* Get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Get SysAudio device interface */
    Status = GetSysAudioDeviceInterface(&SymbolicLinkList);
    if (NT_SUCCESS(Status))
    {
        /* Wait for initialization finishing */
        KeWaitForSingleObject(&DeviceExtension->InitializationCompletionEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    ExFreePool(SymbolicLinkList);

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudControlOpen(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Status = WdmAudOpenSysAudioDevices(DeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        return SetIrpIoStatus(Irp, Status, 0);
    }

    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        Status = WdmAudControlOpenMixer(DeviceObject, Irp, DeviceInfo, ClientInfo);
    }
    else if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE || DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        Status = WdmAudControlOpenWave(DeviceObject, Irp, DeviceInfo, ClientInfo);
    }
    else if (DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE || DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE)
    {
        Status = WdmAudControlOpenMidi(DeviceObject, Irp, DeviceInfo, ClientInfo);
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
    ULONG Result = 0;

    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        Result = WdmAudGetMixerDeviceCount();
    }
    else if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        Result = WdmAudGetWaveOutDeviceCount();
    }
    else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        Result = WdmAudGetWaveInDeviceCount();
    }
    else if (DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE)
    {
        Result = WdmAudGetMidiInDeviceCount();
    }
    else if (DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE)
    {
        Result = WdmAudGetMidiOutDeviceCount();
    }


    /* store result count */
    DeviceInfo->DeviceIndex = Result;

    DPRINT("WdmAudControlDeviceType Devices %u\n", DeviceInfo->DeviceIndex);
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudCapabilities(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    DPRINT("WdmAudCapabilities entered\n");

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        Status = WdmAudMixerCapabilities(DeviceObject, DeviceInfo, ClientInfo, DeviceExtension);
    }
    else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE || DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        Status = WdmAudWaveCapabilities(DeviceObject, DeviceInfo, ClientInfo, DeviceExtension);
    }
    else if (DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE || DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE)
    {
        Status = WdmAudMidiCapabilities(DeviceObject, DeviceInfo, ClientInfo, DeviceExtension);
    }

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudIoctlClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    ULONG Index;

    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].Handle == DeviceInfo->hDevice && ClientInfo->hPins[Index].Type != MIXER_DEVICE_TYPE)
        {
            DPRINT1("Closing device %p\n", DeviceInfo->hDevice);
            ZwClose(DeviceInfo->hDevice);
            DeviceInfo->hDevice = NULL;
            ClientInfo->hPins[Index].Handle = NULL;
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
        }
        else if (ClientInfo->hPins[Index].Handle == DeviceInfo->hDevice && ClientInfo->hPins[Index].Type == MIXER_DEVICE_TYPE)
        {
            DPRINT1("Closing mixer %p\n", DeviceInfo->hDevice);
            return WdmAudControlCloseMixer(DeviceObject, Irp, DeviceInfo, ClientInfo, Index);
        }
    }

    return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudSetDeviceState(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    KSSTATE State;
    NTSTATUS Status;
    KSPROPERTY Property;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;

    DPRINT("WdmAudControlDeviceState\n");

    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_READ | GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: invalid device handle provided %p Type %x\n", DeviceInfo->hDevice, DeviceInfo->DeviceType);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    State = DeviceInfo->DeviceState->bStart ? KSSTATE_ACQUIRE : KSSTATE_PAUSE;
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("%ls failed with status 0x%lx\n",
                DeviceInfo->DeviceState->bStart ?
                L"KSSTATE_ACQUIRE" : L"KSSTATE_PAUSE",
                Status);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    State = DeviceInfo->DeviceState->bStart ? KSSTATE_PAUSE : KSSTATE_ACQUIRE;
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("%ls failed with status 0x%lx\n",
                DeviceInfo->DeviceState->bStart ?
                L"KSSTATE_PAUSE" : L"KSSTATE_ACQUIRE",
                Status);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    State = DeviceInfo->DeviceState->bStart ? KSSTATE_RUN : KSSTATE_STOP;
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("%ls failed with status 0x%lx\n",
                DeviceInfo->DeviceState->bStart ?
                L"KSSTATE_RUN" : L"KSSTATE_STOP",
                Status);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    ObDereferenceObject(FileObject);

    DPRINT("WdmAudControlDeviceState Status 0x%lx BytesReturned %lu\n", Status, BytesReturned);
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudResetStream(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo)
{
    KSSTATE State;
    NTSTATUS Status;
    KSRESET ResetStream;
    KSPROPERTY Property;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;

    DPRINT("WdmAudResetStream\n");

    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_READ | GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: invalid device handle provided %p Type %x\n", DeviceInfo->hDevice, DeviceInfo->DeviceType);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    State = DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ? KSSTATE_PAUSE : KSSTATE_STOP;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("%ls failed with status 0x%lx\n",
                DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                L"KSSTATE_PAUSE" : L"KSSTATE_STOP",
                Status);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        ResetStream = KSRESET_BEGIN;
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_RESET_STATE, (PVOID)&ResetStream, sizeof(KSRESET), NULL, 0, &BytesReturned);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("KSRESET_BEGIN failed with status 0x%lx\n", Status);
            ObDereferenceObject(FileObject);
            return SetIrpIoStatus(Irp, Status, 0);
        }
        ResetStream = KSRESET_END;
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_RESET_STATE, (PVOID)&ResetStream, sizeof(KSRESET), NULL, 0, &BytesReturned);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("KSRESET_END failed with status 0x%lx\n", Status);
            ObDereferenceObject(FileObject);
            return SetIrpIoStatus(Irp, Status, 0);
        }
    }

    ObDereferenceObject(FileObject);

    DPRINT("WdmAudResetStream Status 0x%lx BytesReturned %lu\n", Status, BytesReturned);
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
IoCompletion (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Ctx)
{
    PKSSTREAM_HEADER Header;
    PMDL Mdl, NextMdl;
    PWDMAUD_COMPLETION_CONTEXT Context = (PWDMAUD_COMPLETION_CONTEXT)Ctx;

    /* Get stream header */
    Header = (PKSSTREAM_HEADER)Irp->UserBuffer;

    /* Sanity check */
    ASSERT(Header);

    /* Time to free all allocated mdls */
    Mdl = Irp->MdlAddress;

    while(Mdl)
    {
        /* Get next mdl */
        NextMdl = Mdl->Next;

        /* Unlock pages */
        MmUnlockPages(Mdl);

        /* Grab next mdl */
        Mdl = NextMdl;
    }

    /* Clear mdl list */
    Irp->MdlAddress = Context->Mdl;

    DPRINT("IoCompletion Irp %p IoStatus %lx Information %lx\n", Irp, Irp->IoStatus.Status, Irp->IoStatus.Information);

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        /* failed */
        Irp->IoStatus.Information = 0;
    }

    /* Free context */
    FreeItem(Context);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WdmAudReadWriteInQueue(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PKSSTREAM_HEADER StreamHeader;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IoStack;
    PWAVEHDR WaveHeader;
    PMDL Mdl;
    PWDMAUD_COMPLETION_CONTEXT Context;

    /* get device info */
    DeviceInfo = (PWDMAUD_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;
    ASSERT(DeviceInfo);

    /* Get queued wave header passed by the caller */
    WaveHeader = (PWAVEHDR)DeviceInfo->DeviceState->WaveQueue;

    /* Allocate stream header */
    StreamHeader = AllocateItem(NonPagedPool, sizeof(KSSTREAM_HEADER));
    if (!StreamHeader)
    {
        /* Not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    StreamHeader->Size = sizeof(KSSTREAM_HEADER);
    StreamHeader->PresentationTime.Numerator = 1;
    StreamHeader->PresentationTime.Denominator = 1;
    StreamHeader->Data = WaveHeader->lpData;
    StreamHeader->FrameExtent = WaveHeader->dwBufferLength;

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        StreamHeader->DataUsed = WaveHeader->dwBufferLength;
    }
    else
    {
        StreamHeader->DataUsed = 0;
    }

    /* allocate completion context */
    Context = AllocateItem(NonPagedPool, sizeof(WDMAUD_COMPLETION_CONTEXT));

    if (!Context)
    {
        /* not enough memory */
        FreeItem(StreamHeader);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* store the input buffer in UserBuffer */
    Irp->UserBuffer = StreamHeader;

    /* sanity check */
    ASSERT(Irp->UserBuffer);

    /* setup context */
    Context->Length = sizeof(KSSTREAM_HEADER);
    Context->Function = (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM);
    Context->Mdl = Irp->MdlAddress;

    /* store mdl address */
    Mdl = Irp->MdlAddress;

    /* remove mdl address */
    Irp->MdlAddress = NULL;

    /* now get sysaudio file object */
    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice,
                                       DeviceInfo->DeviceType ==
                                       WAVE_OUT_DEVICE_TYPE ?
                                       FILE_WRITE_DATA : FILE_READ_DATA,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObReferenceObjectByHandle failed with status 0x%lx for pin handle %p\n", Status, DeviceInfo->hDevice);
        Irp->MdlAddress = Mdl;
        FreeItem(Context);
        FreeItem(StreamHeader);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    /* store file object whose reference is released in the completion callback */
    Context->FileObject = FileObject;

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* prepare stack location */
    IoStack->FileObject = FileObject;
    IoStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.OutputBufferLength = sizeof(KSSTREAM_HEADER);
    IoStack->Parameters.DeviceIoControl.IoControlCode = (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE ? IOCTL_KS_READ_STREAM : IOCTL_KS_WRITE_STREAM);
    IoSetCompletionRoutine(Irp, IoCompletion, (PVOID)Context, TRUE, TRUE, TRUE);

    /* call the driver */
    return IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);
}

NTSTATUS
NTAPI
WdmAudReadWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PKSSTREAM_HEADER StreamHeader;
    PFILE_OBJECT FileObject;
    PWAVEHDR WaveHeader;
    NTSTATUS Status;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PWDMAUD_COMPLETION_CONTEXT Context;

    /* Get device info */
    DeviceInfo = (PWDMAUD_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;

    /* Get wave header passed by the caller */
    WaveHeader = (PWAVEHDR)DeviceInfo->Buffer;

    /* Allocate stream header */
    StreamHeader = AllocateItem(NonPagedPool, sizeof(KSSTREAM_HEADER));
    if (!StreamHeader)
    {
        /* Not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    StreamHeader->Size = sizeof(KSSTREAM_HEADER);
    StreamHeader->PresentationTime.Numerator = 1;
    StreamHeader->PresentationTime.Denominator = 1;
    StreamHeader->Data = WaveHeader->lpData;
    StreamHeader->FrameExtent = WaveHeader->dwBufferLength;

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        StreamHeader->DataUsed = WaveHeader->dwBufferLength;
    }
    else
    {
        StreamHeader->DataUsed = 0;
    }

    /* Allocate completion context */
    Context = AllocateItem(NonPagedPool, sizeof(WDMAUD_COMPLETION_CONTEXT));
    if (!Context)
    {
        /* Not enough memory */
        FreeItem(StreamHeader);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get sysaudio file object */
    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice,
                                       DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                                       FILE_WRITE_DATA : FILE_READ_DATA,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObReferenceObjectByHandle failed with 0x%lx for %p\n", Status, DeviceInfo->hDevice);
        FreeItem(Context);
        FreeItem(StreamHeader);
        return Status;
    }

    /* Setup context */
    Context->Length = sizeof(KSSTREAM_HEADER);
    Context->Function = (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM);
    Context->Mdl = Irp->MdlAddress;

    /* Store file object */
    Context->FileObject = FileObject;

    /* Clear mdl address */
    Irp->MdlAddress = NULL;

    /* Do the streaming */
    Status = KsStreamIo(FileObject,
                        NULL,
                        NULL,
                        IoCompletion,
                        Context,
                        KsInvokeOnSuccess | KsInvokeOnError | KsInvokeOnCancel,
                        Irp->UserIosb,
                        StreamHeader,
                        sizeof(KSSTREAM_HEADER),
                        DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                        KSSTREAM_WRITE : KSSTREAM_READ,
                        KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsStreamIo failed with Status 0x%lx\n", Status);
        FreeItem(Context);
        FreeItem(StreamHeader);
        return Status;
    }

    /* Done */
    return STATUS_SUCCESS;
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

    DPRINT("WdmAudDeviceControl entered\n");
    DPRINT("IOCTL 0x%lx\n", IoStack->Parameters.DeviceIoControl.IoControlCode);

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

    if (!IoStack->FileObject || !IoStack->FileObject->FsContext)
    {
        /* file object parameter */
        DPRINT1("Error: file object is not attached\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }
    ClientInfo = (PWDMAUD_CLIENT)IoStack->FileObject->FsContext;

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INIT_WDMAUD:
            return WdmAudControlInitialize(DeviceObject, Irp);
        case IOCTL_EXIT_WDMAUD:
            /* No op */
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
        case IOCTL_OPEN_WDMAUD:
        case IOCTL_OPEN_MIXER:
            return WdmAudControlOpen(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETNUMDEVS_TYPE:
            return WdmAudControlDeviceType(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETCAPABILITIES:
            return WdmAudCapabilities(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_CLOSE_WDMAUD:
            return WdmAudIoctlClose(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GET_MIXER_EVENT:
            return WdmAudGetMixerEvent(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETLINEINFO:
            return WdmAudGetLineInfo(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETLINECONTROLS:
            return WdmAudGetLineControls(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_SETCONTROLDETAILS:
            return WdmAudSetControlDetails(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETCONTROLDETAILS:
            return WdmAudGetControlDetails(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_RESET_CAPTURE:
        case IOCTL_RESET_PLAYBACK:
            return WdmAudResetStream(DeviceObject, Irp, DeviceInfo);
        case IOCTL_GETINPOS:
        case IOCTL_GETOUTPOS:
            return WdmAudGetPosition(DeviceObject, Irp, DeviceInfo);
        case IOCTL_PAUSE_CAPTURE:
        case IOCTL_START_CAPTURE:
        case IOCTL_PAUSE_PLAYBACK:
        case IOCTL_START_PLAYBACK:
            return WdmAudSetDeviceState(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_READDATA:
        case IOCTL_WRITEDATA:
            return WdmAudReadWrite(DeviceObject, Irp);
        case IOCTL_ADD_DEVNODE:
        case IOCTL_REMOVE_DEVNODE:
        case IOCTL_GETVOLUME:
        case IOCTL_SETVOLUME:

        default:
           DPRINT1("Unhandled 0x%x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
           break;
    }

    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);
}
