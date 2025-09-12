/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
Pin_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    NTSTATUS Status;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject = NULL;
    PIO_STACK_LOCATION IoStack;

    DPRINT("Pin_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject, Irp);

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    /* Sanity check */
    ASSERT(Context);

    /* acquire real pin file object */
    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
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
Pin_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    /* Sanity check */
    ASSERT(Context);

    if (Context->hMixerPin)
    {
        // FIXME
        // call kmixer to convert stream
        UNIMPLEMENTED;
    }

    /* acquire real pin file object */
    Status = ObReferenceObjectByHandle(Context->Handle, GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("failed\n");
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        /* Complete the irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* skip current irp location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);
    /* store file object of next device object */
    IoStack->FileObject = FileObject;
    IoStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    //ASSERT(Irp->AssociatedIrp.SystemBuffer);

    /* now call the driver */
    Status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);

    /* dereference file object */
    ObDereferenceObject(FileObject);

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

    //DPRINT("Pin_fnClose called DeviceObject %p Irp %p\n", DeviceObject, Irp);

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    if (Context->Handle)
    {
        ZwClose(Context->Handle);
    }

    if (Context->hMixerPin)
    {
        ZwClose(Context->hMixerPin);
    }

    FreeItem(Context);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static KSDISPATCH_TABLE PinTable =
{
    Pin_fnDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    Pin_fnWrite,
    KsDispatchInvalidDeviceRequest,
    Pin_fnClose,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastWriteFailure,
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
    PFILE_OBJECT FileObject = NULL;

    Status = KsCreatePin(KMixerHandle, PinConnect, GENERIC_READ | GENERIC_WRITE, &PinHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Mixer Pin with %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    Status = ObReferenceObjectByHandle(PinHandle,
                                       GENERIC_READ | GENERIC_WRITE,
                                       *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

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
        return Status;
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
        /* FIXME disable kmixer
         */
        return STATUS_UNSUCCESSFUL;
    }
#if 0
    if (!NT_SUCCESS(Status))
    {
        /* the audio irp pin didnt accept the input format
         * let's compute a compatible format
         */
        MixerPinConnect = AllocateItem(NonPagedPool, sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX));
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
            FreeItem(MixerPinConnect);
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

            FreeItem(MixerPinConnect);
            return Status;
        }
    }
#endif

    //DeviceEntry->Pins[Connect->PinId].References = 0;

    /* initialize dispatch context */
    DispatchContext->Handle = RealPinHandle;
    DispatchContext->PinId = Connect->PinId;
    DispatchContext->AudioEntry = DeviceEntry;


    DPRINT("RealPinHandle %p\n", RealPinHandle);

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
            FreeItem(MixerPinConnect);
        }
    }
    /* done */
    return Status;
}

NTSTATUS
GetConnectRequest(
    IN PIRP Irp,
    OUT PKSPIN_CONNECT * Result)
{
    PIO_STACK_LOCATION IoStack;
    ULONG ObjectLength, ParametersLength;
    PVOID Buffer;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get object class length */
    ObjectLength = (wcslen(KSSTRING_Pin) + 1) * sizeof(WCHAR);

    /* check for minium length requirement */
    if (ObjectLength  + sizeof(KSPIN_CONNECT) > IoStack->FileObject->FileName.MaximumLength)
        return STATUS_UNSUCCESSFUL;

    /* extract parameters length */
    ParametersLength = IoStack->FileObject->FileName.MaximumLength - ObjectLength;

    /* allocate buffer */
    Buffer = AllocateItem(NonPagedPool, ParametersLength);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* copy parameters */
    RtlMoveMemory(Buffer, &IoStack->FileObject->FileName.Buffer[ObjectLength / sizeof(WCHAR)], ParametersLength);

    /* store result */
    *Result = (PKSPIN_CONNECT)Buffer;

    return STATUS_SUCCESS;
}



NTSTATUS
NTAPI
DispatchCreateSysAudioPin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStack;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry;
    PKSPIN_CONNECT Connect;
    PDISPATCH_CONTEXT DispatchContext;

    DPRINT("DispatchCreateSysAudioPin entered\n");

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity checks */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->RelatedFileObject);
    ASSERT(IoStack->FileObject->RelatedFileObject->FsContext);

    /* get current attached virtual device */
    DeviceEntry = (PKSAUDIO_DEVICE_ENTRY)IoStack->FileObject->RelatedFileObject->FsContext;

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* get connect details */
    Status = GetConnectRequest(Irp, &Connect);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed to obtain connect details */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }


    /* allocate dispatch context */
    DispatchContext = AllocateItem(NonPagedPool, sizeof(DISPATCH_CONTEXT));
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
    Status = KsAllocateObjectHeader(&DispatchContext->ObjectHeader, 0, NULL, Irp, &PinTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        FreeItem(DispatchContext);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* now instantiate the pins */
    Status = InstantiatePins(DeviceEntry, Connect, DispatchContext, (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        KsFreeObjectHeader(DispatchContext->ObjectHeader);
        FreeItem(DispatchContext);
    }
    else
    {
        /* store dispatch context */
        IoStack->FileObject->FsContext = (PVOID)DispatchContext;
    }


    /* FIXME create items for clocks / allocators */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
