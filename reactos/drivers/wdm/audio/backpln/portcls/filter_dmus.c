/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_dmus.c
 * PURPOSE:         portcls wave pci filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortFilterDMusVtbl *lpVtbl;

    LONG ref;

    IPortDMus* Port;
    IPortPinDMus ** Pins;
    SUBDEVICE_DESCRIPTOR * Descriptor;

}IPortFilterDMusImpl;

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnQueryInterface(
    IPortFilterDMus* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IIrpTarget) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPort))
    {
        *Output = This->Port;
        This->Port->lpVtbl->AddRef(This->Port);
        return STATUS_SUCCESS;
    }


    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterDMus_fnAddRef(
    IPortFilterDMus* iface)
{
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterDMus_fnRelease(
    IPortFilterDMus* iface)
{
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    return This->ref;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnNewIrpTarget(
    IN IPortFilterDMus* iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortPinDMus * Pin;
    PKSPIN_CONNECT ConnectDetails;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl *)iface;

    ASSERT(This->Port);
    ASSERT(This->Descriptor);
    ASSERT(This->Pins);

    DPRINT("IPortFilterDMus_fnNewIrpTarget entered\n");

    /* let's verify the connection request */
    Status = PcValidateConnectRequest(Irp, &This->Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (This->Pins[ConnectDetails->PinId] && This->Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount)
    {
        /* release existing instance */
        ASSERT(0);
    }

    /* now create the pin */
    Status = NewPortPinDMus(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* initialize the pin */
    Status = Pin->lpVtbl->Init(Pin, This->Port, iface, ConnectDetails, &This->Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId], DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        Pin->lpVtbl->Release(Pin);
        return Status;
    }

    /* release existing pin */
    if (This->Pins[ConnectDetails->PinId])
    {
        This->Pins[ConnectDetails->PinId]->lpVtbl->Release(This->Pins[ConnectDetails->PinId]);
    }
    /* store pin */
    This->Pins[ConnectDetails->PinId] = Pin;

    /* store result */
    *OutTarget = (IIrpTarget*)Pin;

    /* increment current instance count */
    This->Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount++;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnDeviceIoControl(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    ISubdevice *SubDevice = NULL;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl *)iface;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY);
    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&SubDevice);
    ASSERT(Status == STATUS_SUCCESS);
    ASSERT(SubDevice != NULL);

    Status = SubDevice->lpVtbl->GetDescriptor(SubDevice, &Descriptor);
    ASSERT(Status == STATUS_SUCCESS);
    ASSERT(Descriptor != NULL);

    SubDevice->lpVtbl->Release(SubDevice);

    return PcPropertyHandler(Irp, Descriptor);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnRead(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnWrite(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnFlush(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnClose(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG Index;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl *)iface;

    for(Index = 0; Index < This->Descriptor->Factory.PinDescriptorCount; Index++)
    {
        if (This->Pins[Index])
        {
            This->Pins[Index]->lpVtbl->Close(This->Pins[Index], DeviceObject, NULL);
        }
    }


    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnQuerySecurity(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterDMus_fnSetSecurity(
    IN IPortFilterDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterDMus_fnFastDeviceIoControl(
    IN IPortFilterDMus* iface,
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
    ULONG Index;
    PKSPROPERTY Property;
    NTSTATUS Status;
    ISubdevice * SubDevice = NULL;
    PSUBDEVICE_DESCRIPTOR Descriptor = NULL;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl *)iface;

    Property = (PKSPROPERTY)InputBuffer;

    if (InputBufferLength < sizeof(KSPROPERTY))
        return FALSE;


    /* get private interface */
    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&SubDevice);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* get descriptor */
    Status = SubDevice->lpVtbl->GetDescriptor(SubDevice, &Descriptor);
    if (!NT_SUCCESS(Status))
    {
        SubDevice->lpVtbl->Release(SubDevice);
        return FALSE;
    }

    for(Index = 0; Index < Descriptor->FilterPropertySet.FreeKsPropertySetOffset; Index++)
    {
        if (IsEqualGUIDAligned(&Property->Set, Descriptor->FilterPropertySet.Properties[Index].Set))
        {
            FastPropertyHandler(FileObject, (PKSPROPERTY)InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, StatusBlock,
                                1,
                                &Descriptor->FilterPropertySet.Properties[Index],
                                Descriptor, SubDevice);
        }
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterDMus_fnFastRead(
    IN IPortFilterDMus* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterDMus_fnFastWrite(
    IN IPortFilterDMus* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
static
NTSTATUS
NTAPI
IPortFilterDMus_fnInit(
    IN IPortFilterDMus* iface,
    IN IPortDMus* Port)
{
    ISubdevice * ISubDevice;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl*)iface;

    This->Port = Port;

    /* get our private interface */
    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* get the subdevice descriptor */
    Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);

    /* release subdevice interface */
    ISubDevice->lpVtbl->Release(ISubDevice);

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* save descriptor */
    This->Descriptor = Descriptor;

    /* allocate pin array */
    This->Pins = AllocateItem(NonPagedPool, Descriptor->Factory.PinDescriptorCount * sizeof(IPortPinDMus*), TAG_PORTCLASS);

    if (!This->Pins)
        return STATUS_UNSUCCESSFUL;

    /* increment reference count */
    Port->lpVtbl->AddRef(Port);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
IPortFilterDMus_fnFreePin(
    IN IPortFilterDMus* iface,
    IN struct IPortPinDMus* Pin)
{
    ULONG Index;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl*)iface;

    for(Index = 0; Index < This->Descriptor->Factory.PinDescriptorCount; Index++)
    {
        if (This->Pins[Index] == Pin)
        {
            This->Pins[Index]->lpVtbl->Release(This->Pins[Index]);
            This->Pins[Index] = NULL;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_UNSUCCESSFUL;
}

static
VOID
NTAPI
IPortFilterDMus_fnNotifyPins(
    IN IPortFilterDMus* iface)
{
    ULONG Index;
    IPortFilterDMusImpl * This = (IPortFilterDMusImpl*)iface;

    DPRINT("Notifying %u pins\n", This->Descriptor->Factory.PinDescriptorCount);

    for(Index = 0; Index < This->Descriptor->Factory.PinDescriptorCount; Index++)
    {
        This->Pins[Index]->lpVtbl->Notify(This->Pins[Index]);
    }
}

static IPortFilterDMusVtbl vt_IPortFilterDMus =
{
    IPortFilterDMus_fnQueryInterface,
    IPortFilterDMus_fnAddRef,
    IPortFilterDMus_fnRelease,
    IPortFilterDMus_fnNewIrpTarget,
    IPortFilterDMus_fnDeviceIoControl,
    IPortFilterDMus_fnRead,
    IPortFilterDMus_fnWrite,
    IPortFilterDMus_fnFlush,
    IPortFilterDMus_fnClose,
    IPortFilterDMus_fnQuerySecurity,
    IPortFilterDMus_fnSetSecurity,
    IPortFilterDMus_fnFastDeviceIoControl,
    IPortFilterDMus_fnFastRead,
    IPortFilterDMus_fnFastWrite,
    IPortFilterDMus_fnInit,
    IPortFilterDMus_fnFreePin,
    IPortFilterDMus_fnNotifyPins
};

NTSTATUS 
NewPortFilterDMus(
    OUT PPORTFILTERDMUS * OutFilter)
{
    IPortFilterDMusImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortFilterDMusImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortFilterDMus */
    This->ref = 1;
    This->lpVtbl = &vt_IPortFilterDMus;

    /* return result */
    *OutFilter = (IPortFilterDMus*)&This->lpVtbl;

    return STATUS_SUCCESS;
}
