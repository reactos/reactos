/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

NTSTATUS
NTAPI
Pin_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    NTSTATUS Status;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IoStack;

    DPRINT("Pin_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);

    /* acquire real pin file object */
    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        /* Complete the irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Re-dispatch the request to the real target pin */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IoStack->Parameters.DeviceIoControl.IoControlCode,
                                          IoStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                          IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                          Irp->UserBuffer,
                                          IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                          &BytesReturned);
    /* release file object */
    ObDereferenceObject(FileObject);

    /* Save status and information */
    Irp->IoStatus.Information = BytesReturned;
    Irp->IoStatus.Status = Status;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);

    /* acquire real pin file object */
    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        /* Complete the irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Re-dispatch the request to the real target pin */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_READ_STREAM,
                                          MmGetMdlVirtualAddress(Irp->MdlAddress),
                                          IoStack->Parameters.Read.Length,
                                          NULL,
                                          0,
                                          &BytesReturned);

    /* release file object */
    ObDereferenceObject(FileObject);

    if (Context->hMixerPin)
    {
        // FIXME
        // call kmixer to convert stream
        UNIMPLEMENTED
    }

    /* Save status and information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    ULONG BytesReturned;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);

    if (Context->hMixerPin)
    {
        // FIXME
        // call kmixer to convert stream
        UNIMPLEMENTED
    }

    /* acquire real pin file object */
    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        /* Complete the irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* call the portcls audio pin */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_WRITE_STREAM,
                                          Irp->UserBuffer,
                                          IoStack->Parameters.Write.Length,
                                          NULL,
                                          0,
                                          &BytesReturned);


    /* Release file object */
    ObDereferenceObject(FileObject);

    /* Save status and information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesReturned;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT PinDeviceObject;
    PIRP PinIrp;
    PFILE_OBJECT FileObject;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);


    /* acquire real pin file object */
    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        /* Complete the irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Get Pin's device object */
    PinDeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* release file object */
    ObDereferenceObject(FileObject);

    /* Initialize notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* build target irp */
    PinIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, PinDeviceObject, NULL, 0, NULL, &Event, &IoStatus);
    if (PinIrp)
    {

        /* Get the next stack location */
        IoStack = IoGetNextIrpStackLocation(PinIrp);
        /* The file object must be present in the irp as it contains the KSOBJECT_HEADER */
        IoStack->FileObject = FileObject;

        /* call the driver */
        Status = IoCallDriver(PinDeviceObject, PinIrp);
        /* Has request already completed ? */
        if (Status == STATUS_PENDING)
        {
            /* Wait untill the request has completed */
            KeWaitForSingleObject(&Event, UserRequest, KernelMode, FALSE, NULL);
            /* Update status */
            Status = IoStatus.Status;
        }
    }

    /* store status */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;

    DPRINT("Pin_fnClose called DeviceObject %p Irp %p\n", DeviceObject);

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    if (Context->Handle)
    {
        ZwClose(Context->Handle);
    }
    ZwClose(Context->hMixerPin);

    ExFreePool(Context);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Pin_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT("Pin_fnQuerySecurity called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Pin_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{

    DPRINT("Pin_fnSetSecurity called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
Pin_fnFastDeviceIoControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Pin_fnFastDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);


    return FALSE;
}


BOOLEAN
NTAPI
Pin_fnFastRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Pin_fnFastRead called DeviceObject %p Irp %p\n", DeviceObject);

    return FALSE;

}

BOOLEAN
NTAPI
Pin_fnFastWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PDISPATCH_CONTEXT Context;
    PFILE_OBJECT RealFileObject;
    NTSTATUS Status;

    DPRINT("Pin_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);

    Context = (PDISPATCH_CONTEXT)FileObject->FsContext2;

    if (Context->hMixerPin)
    {
        Status = ObReferenceObjectByHandle(Context->hMixerPin, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&RealFileObject, NULL);
        if (NT_SUCCESS(Status))
        {
            Status = KsStreamIo(RealFileObject, NULL, NULL, NULL, NULL, 0, IoStatus, Buffer, Length, KSSTREAM_WRITE, UserMode);
            ObDereferenceObject(RealFileObject);
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Mixing stream failed with %lx\n", Status);
            DbgBreakPoint();
            return FALSE;
        }
    }

    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&RealFileObject, NULL);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = KsStreamIo(RealFileObject, NULL, NULL, NULL, NULL, 0, IoStatus, Buffer, Length, KSSTREAM_WRITE, UserMode);

    ObDereferenceObject(RealFileObject);

    if (NT_SUCCESS(Status))
        return TRUE;
    else
        return FALSE;
}

static KSDISPATCH_TABLE PinTable =
{
    Pin_fnDeviceIoControl,
    Pin_fnRead,
    Pin_fnWrite,
    Pin_fnFlush,
    Pin_fnClose,
    Pin_fnQuerySecurity,
    Pin_fnSetSecurity,
    Pin_fnFastDeviceIoControl,
    Pin_fnFastRead,
    Pin_fnFastWrite,
};

NTSTATUS
SetMixerInputOutputFormat(
    IN PFILE_OBJECT FileObject,
    IN PKSDATAFORMAT InputFormat,
    IN PKSDATAFORMAT OutputFormat)
{
    KSP_PIN PinRequest;
    ULONG BytesReturned;
    NTSTATUS Status;

    /* re-using pin */
    PinRequest.Property.Set = KSPROPSETID_Connection;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_SET;
    PinRequest.Property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;

    /* set the input format */
    PinRequest.PinId = 0;
    DPRINT("InputFormat %p Size %u WaveFormatSize %u DataFormat %u WaveEx %u\n", InputFormat, InputFormat->FormatSize, sizeof(KSDATAFORMAT_WAVEFORMATEX), sizeof(KSDATAFORMAT), sizeof(WAVEFORMATEX));
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                          (PVOID)&PinRequest,
                                           sizeof(KSP_PIN),
                                          (PVOID)InputFormat,
                                           InputFormat->FormatSize,
                                          &BytesReturned);
    if (!NT_SUCCESS(Status))
        return Status;

    /* set the the output format */
    PinRequest.PinId = 1;
    DPRINT("OutputFormat %p Size %u WaveFormatSize %u DataFormat %u WaveEx %u\n", OutputFormat, OutputFormat->FormatSize, sizeof(KSDATAFORMAT_WAVEFORMATEX), sizeof(KSDATAFORMAT), sizeof(WAVEFORMATEX));
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                          (PVOID)&PinRequest,
                                           sizeof(KSP_PIN),
                                          (PVOID)OutputFormat,
                                           OutputFormat->FormatSize,
                                          &BytesReturned);
    return Status;
}


NTSTATUS
CreateMixerPinAndSetFormat(
    IN HANDLE KMixerHandle,
    IN KSPIN_CONNECT *PinConnect,
    IN PKSDATAFORMAT InputFormat,
    IN PKSDATAFORMAT OutputFormat,
    OUT PHANDLE MixerPinHandle)
{
    NTSTATUS Status;
    HANDLE PinHandle;
    PFILE_OBJECT FileObject;

    Status = KsCreatePin(KMixerHandle, PinConnect, GENERIC_READ | GENERIC_WRITE, &PinHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Mixer Pin with %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    Status = ObReferenceObjectByHandle(PinHandle,
                                       GENERIC_READ | GENERIC_WRITE, 
                                       IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object with %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    Status = SetMixerInputOutputFormat(FileObject, InputFormat, OutputFormat);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(FileObject);
        ZwClose(PinHandle);
    }

    ObDereferenceObject(FileObject);

    *MixerPinHandle = PinHandle;
     return Status;
}


NTSTATUS
NTAPI
InstantiatePins(
    IN PKSAUDIO_DEVICE_ENTRY DeviceEntry,
    IN PKSPIN_CONNECT Connect,
    IN PDISPATCH_CONTEXT DispatchContext,
    IN PSYSAUDIODEVEXT DeviceExtension)
{
    NTSTATUS Status;
    HANDLE RealPinHandle;
    PKSDATAFORMAT_WAVEFORMATEX InputFormat;
    PKSDATAFORMAT_WAVEFORMATEX OutputFormat = NULL;
    PKSPIN_CONNECT MixerPinConnect = NULL;
    KSPIN_CINSTANCES PinInstances;

    DPRINT("InstantiatePins entered\n");

    /* query instance count */
    Status = GetPinInstanceCount(DeviceEntry, &PinInstances, Connect);
    if (!NT_SUCCESS(Status))
    {
        /* failed to query instance count */
        return Status;
    }

    /* can be the pin be instantiated */
    if (PinInstances.PossibleCount == 0)
    {
        /* caller wanted to open an instance-less pin */
        return STATUS_UNSUCCESSFUL;
    }

    /* has the maximum instance count been exceeded */
    if (PinInstances.CurrentCount == PinInstances.PossibleCount)
    {
        /* FIXME pin already exists
         * and kmixer infrastructure is not implemented
         */
        return STATUS_UNSUCCESSFUL;
    }

    /* Fetch input format */
    InputFormat = (PKSDATAFORMAT_WAVEFORMATEX)(Connect + 1);

    /* Let's try to create the audio irp pin */
    Status = KsCreatePin(DeviceEntry->Handle, Connect, GENERIC_READ | GENERIC_WRITE, &RealPinHandle);

    if (!NT_SUCCESS(Status))
    {
        /* the audio irp pin didnt accept the input format
         * let's compute a compatible format
         */
        MixerPinConnect = ExAllocatePool(NonPagedPool, sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX));
        if (!MixerPinConnect)
        {
            /* not enough memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Zero pin connect */
        RtlZeroMemory(MixerPinConnect, sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX));

        /* Copy initial connect details */
        RtlMoveMemory(MixerPinConnect, Connect, sizeof(KSPIN_CONNECT));


        OutputFormat = (PKSDATAFORMAT_WAVEFORMATEX)(MixerPinConnect + 1);

        Status = ComputeCompatibleFormat(DeviceEntry, Connect->PinId, InputFormat, OutputFormat);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ComputeCompatibleFormat failed with %x\n", Status);
            ExFreePool(MixerPinConnect);
            return Status;
        }

        /* Retry with Mixer format */
        Status = KsCreatePin(DeviceEntry->Handle, MixerPinConnect, GENERIC_READ | GENERIC_WRITE, &RealPinHandle);
        if (!NT_SUCCESS(Status))
        {
           /* This should not fail */
            DPRINT1("KsCreatePin failed with %x\n", Status);
            DPRINT1(" InputFormat: SampleRate %u Bits %u Channels %u\n", InputFormat->WaveFormatEx.nSamplesPerSec, InputFormat->WaveFormatEx.wBitsPerSample, InputFormat->WaveFormatEx.nChannels);
            DPRINT1("OutputFormat: SampleRate %u Bits %u Channels %u\n", OutputFormat->WaveFormatEx.nSamplesPerSec, OutputFormat->WaveFormatEx.wBitsPerSample, OutputFormat->WaveFormatEx.nChannels);

            ExFreePool(MixerPinConnect);
            return Status;
        }
    }

    DeviceEntry->Pins[Connect->PinId].References = 0;

    /* initialize dispatch context */
    DispatchContext->Handle = RealPinHandle;
    DispatchContext->PinId = Connect->PinId;
    DispatchContext->AudioEntry = DeviceEntry;


    /* Do we need to transform the audio stream */
    if (OutputFormat != NULL)
    {
        /* Now create the mixer pin */
        Status = CreateMixerPinAndSetFormat(DeviceExtension->KMixerHandle,
                                            MixerPinConnect,
                                            (PKSDATAFORMAT)InputFormat,
                                            (PKSDATAFORMAT)OutputFormat,
                                            &DispatchContext->hMixerPin);

        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create Mixer Pin with %x\n", Status);
            ExFreePool(MixerPinConnect);
        }
    }
    /* done */
    return Status;
}

NTSTATUS
NTAPI
DispatchCreateSysAudioPin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KSOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IoStack;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry;
    PKSPIN_CONNECT Connect = NULL;
    PDISPATCH_CONTEXT DispatchContext;

    DPRINT("DispatchCreateSysAudioPin entered\n");

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity checks */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->RelatedFileObject);
    ASSERT(IoStack->FileObject->RelatedFileObject->FsContext2);

    /* get current attached virtual device */
    DeviceEntry = (PKSAUDIO_DEVICE_ENTRY)IoStack->FileObject->RelatedFileObject->FsContext2;

    /* now validate pin connect request */
    Status = KsValidateConnectRequest(Irp, DeviceEntry->PinDescriptorsCount, DeviceEntry->PinDescriptors, &Connect);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* allocate dispatch context */
    DispatchContext = ExAllocatePool(NonPagedPool, sizeof(DISPATCH_CONTEXT));
    if (!DispatchContext)
    {
        /* failed */
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* zero dispatch context */
    RtlZeroMemory(DispatchContext, sizeof(DISPATCH_CONTEXT));

    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 0, NULL, Irp, &PinTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(DispatchContext);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* now instantiate the pins */
    Status = InstantiatePins(DeviceEntry, Connect, DispatchContext, (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        KsFreeObjectHeader(ObjectHeader);
        ExFreePool(DispatchContext);
    }
    else
    {
        /* store dispatch context */
        IoStack->FileObject->FsContext2 = (PVOID)DispatchContext;
    }


    /* FIXME create items for clocks / allocators */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
