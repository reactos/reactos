/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */

#include "wdmaud.h"

#define NDEBUG
#include <debug.h>

const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};

NTSTATUS
WdmAudControlOpen(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        return WdmAudControlOpenMixer(DeviceObject, Irp, DeviceInfo, ClientInfo);
    }

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE || DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        return WdmAudControlOpenWave(DeviceObject, Irp, DeviceInfo, ClientInfo);
    }

    if (DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE || DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE)
    {
        return WdmAudControlOpenMidi(DeviceObject, Irp, DeviceInfo, ClientInfo);
    }


    return SetIrpIoStatus(Irp, STATUS_NOT_SUPPORTED, sizeof(WDMAUD_DEVICE_INFO));
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
    DeviceInfo->DeviceCount = Result;

    DPRINT("WdmAudControlDeviceType Devices %u\n", DeviceInfo->DeviceCount);
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
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

    State = DeviceInfo->u.State;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);

    ObDereferenceObject(FileObject);

    DPRINT("WdmAudControlDeviceState Status %x\n", Status);
    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
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
            ClientInfo->hPins[Index].Handle = NULL;
            SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
            return STATUS_SUCCESS;
        }
        else if (ClientInfo->hPins[Index].Handle == DeviceInfo->hDevice && ClientInfo->hPins[Index].Type == MIXER_DEVICE_TYPE)
        {
            DPRINT1("Closing mixer %p\n", DeviceInfo->hDevice);
            return WdmAudControlCloseMixer(DeviceObject, Irp, DeviceInfo, ClientInfo, Index);
        }
    }

    SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, sizeof(WDMAUD_DEVICE_INFO));
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
NTAPI
WdmAudFrameSize(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PFILE_OBJECT FileObject;
    KSPROPERTY Property;
    ULONG BytesReturned;
    KSALLOCATOR_FRAMING Framing;
    NTSTATUS Status;

    /* Get sysaudio pin file object */
    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid buffer handle %p\n", DeviceInfo->hDevice);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    /* Setup get framing request */
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Connection;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&Framing, sizeof(KSALLOCATOR_FRAMING), &BytesReturned);
    /* Did we succeed */
    if (NT_SUCCESS(Status))
    {
        /* Store framesize */
        DeviceInfo->u.FrameSize = Framing.FrameSize;
    }

    /* Release file object */
    ObDereferenceObject(FileObject);

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));

}

NTSTATUS
NTAPI
WdmAudGetDeviceInterface(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo)
{
    NTSTATUS Status;
    LPWSTR Device;
    ULONG Size, Length;

    /* get device interface string input length */
    Size = DeviceInfo->u.Interface.DeviceInterfaceStringSize;

   /* get mixer info */
   Status = WdmAudGetPnpNameByIndexAndType(DeviceInfo->DeviceIndex, DeviceInfo->DeviceType, &Device);

   /* check for success */
   if (!NT_SUCCESS(Status))
   {
        /* invalid device id */
        return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
   }

   /* calculate length */
   Length = (wcslen(Device)+1) * sizeof(WCHAR);

    if (!Size)
    {
        /* store device interface size */
        DeviceInfo->u.Interface.DeviceInterfaceStringSize = Length;
    }
    else if (Size < Length)
    {
        /* buffer too small */
        DeviceInfo->u.Interface.DeviceInterfaceStringSize = Length;
        return SetIrpIoStatus(Irp, STATUS_BUFFER_OVERFLOW, sizeof(WDMAUD_DEVICE_INFO));
    }
    else
    {
        //FIXME SEH
        RtlMoveMemory(DeviceInfo->u.Interface.DeviceInterfaceString, Device, Length);
    }

    FreeItem(Device);
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudResetStream(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo)
{
    KSRESET ResetStream;
    NTSTATUS Status;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;

    DPRINT("WdmAudResetStream\n");

    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_READ | GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: invalid device handle provided %p Type %x\n", DeviceInfo->hDevice, DeviceInfo->DeviceType);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    ResetStream = DeviceInfo->u.ResetStream;
    ASSERT(ResetStream == KSRESET_BEGIN || ResetStream == KSRESET_END);

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_RESET_STATE, (PVOID)&ResetStream, sizeof(KSRESET), NULL, 0, &BytesReturned);

    ObDereferenceObject(FileObject);

    DPRINT("WdmAudResetStream Status %x\n", Status);
    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
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

    DPRINT("WdmAudDeviceControl entered\n");

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_OPEN_WDMAUD:
            return WdmAudControlOpen(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETNUMDEVS_TYPE:
            return WdmAudControlDeviceType(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_SETDEVICE_STATE:
            return WdmAudControlDeviceState(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETCAPABILITIES:
            return WdmAudCapabilities(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_CLOSE_WDMAUD:
            return WdmAudIoctlClose(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETFRAMESIZE:
            return WdmAudFrameSize(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETLINEINFO:
            return WdmAudGetLineInfo(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETLINECONTROLS:
            return WdmAudGetLineControls(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_SETCONTROLDETAILS:
            return WdmAudSetControlDetails(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETCONTROLDETAILS:
            return WdmAudGetControlDetails(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_QUERYDEVICEINTERFACESTRING:
            return WdmAudGetDeviceInterface(DeviceObject, Irp, DeviceInfo);
        case IOCTL_GET_MIXER_EVENT:
            return WdmAudGetMixerEvent(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_RESET_STREAM:
            return WdmAudResetStream(DeviceObject, Irp, DeviceInfo);
        case IOCTL_GETPOS:
            return WdmAudGetPosition(DeviceObject, Irp, DeviceInfo);
        case IOCTL_GETDEVID:
        case IOCTL_GETVOLUME:
        case IOCTL_SETVOLUME:

           DPRINT1("Unhandled %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
           break;
    }

    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);
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

    /* get stream header */
    Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;

    /* sanity check */
    ASSERT(Header);

    /* time to free all allocated mdls */
    Mdl = Irp->MdlAddress;

    while(Mdl)
    {
        /* get next mdl */
        NextMdl = Mdl->Next;

        /* unlock pages */
        MmUnlockPages(Mdl);

        /* grab next mdl */
        Mdl = NextMdl;
    }
    //IoFreeMdl(Mdl);
    /* clear mdl list */
    Irp->MdlAddress = Context->Mdl;



    DPRINT("IoCompletion Irp %p IoStatus %lx Information %lx\n", Irp, Irp->IoStatus.Status, Irp->IoStatus.Information);

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        /* failed */
        Irp->IoStatus.Information = 0;
    }

    /* dereference file object */
    ObDereferenceObject(Context->FileObject);

    /* free context */
    FreeItem(Context);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WdmAudReadWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IoStack;
    ULONG Length;
    PMDL Mdl;
    BOOLEAN Read = TRUE;
    PWDMAUD_COMPLETION_CONTEXT Context;

    /* allocate completion context */
    Context = AllocateItem(NonPagedPool, sizeof(WDMAUD_COMPLETION_CONTEXT));

    if (!Context)
    {
        /* not enough memory */
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        /* done */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* store the input buffer in UserBuffer - as KsProbeStreamIrp operates on IRP_MJ_DEVICE_CONTROL */
    Irp->UserBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

    /* sanity check */
    ASSERT(Irp->UserBuffer);

    /* get the length of the request length */
    Length = IoStack->Parameters.Write.Length;

    /* store outputbuffer length */
    IoStack->Parameters.DeviceIoControl.OutputBufferLength = Length;

    /* setup context */
    Context->Length = Length;
    Context->Function = (IoStack->MajorFunction == IRP_MJ_WRITE ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM);
    Context->Mdl = Irp->MdlAddress;

    /* store mdl address */
    Mdl = Irp->MdlAddress;

    /* remove mdladdress as KsProbeStreamIrp will interpret it as an already probed audio buffer */
    Irp->MdlAddress = NULL;

    if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        /* probe the write stream irp */
        Read = FALSE;
        Status = KsProbeStreamIrp(Irp, KSPROBE_STREAMWRITE | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK, Length);
    }
    else
    {
        /* probe the read stream irp */
        Status = KsProbeStreamIrp(Irp, KSPROBE_STREAMREAD | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK, Length);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsProbeStreamIrp failed with Status %x Cancel %u\n", Status, Irp->Cancel);
        Irp->MdlAddress = Mdl;
        FreeItem(Context);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    /* get device info */
    DeviceInfo = (PWDMAUD_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;
    ASSERT(DeviceInfo);

    /* now get sysaudio file object */
    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid pin handle %p\n", DeviceInfo->hDevice);
        Irp->MdlAddress = Mdl;
        FreeItem(Context);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    /* store file object whose reference is released in the completion callback */
    Context->FileObject = FileObject;

    /* skip current irp stack location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* prepare stack location */
    IoStack->FileObject = FileObject;
    IoStack->Parameters.Write.Length = Length;
    IoStack->MajorFunction = IRP_MJ_WRITE;
    IoStack->Parameters.DeviceIoControl.IoControlCode = (Read ? IOCTL_KS_READ_STREAM : IOCTL_KS_WRITE_STREAM);
    IoSetCompletionRoutine(Irp, IoCompletion, (PVOID)Context, TRUE, TRUE, TRUE);

    /* mark irp as pending */
//    IoMarkIrpPending(Irp);
    /* call the driver */
    Status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);
    return Status;
}
