#include "priv.h"

typedef struct
{
    IKsDeviceVtbl *lpVtbl;

    LONG ref;
    KSDEVICE KsDevice;

}IKsDeviceImpl;


NTSTATUS
NTAPI
IKsDevice_fnQueryInterface(
    IN IKsDevice * iface,
    REFIID InterfaceId,
    PVOID* Interface)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnQueryInterface %p\n", This);
    return STATUS_NOT_IMPLEMENTED;
}

ULONG
NTAPI
IKsDevice_fnAddRef(
    IN IKsDevice * iface)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsDevice_fnRelease(
    IN IKsDevice * iface)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        ExFreePoolWithTag(This, TAG_KSDEVICE);
        return 0;
    }

    return This->ref;
}



KSDEVICE *
NTAPI
IKsDevice_fnGetStruct(
    IN IKsDevice * iface)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    return &This->KsDevice;
}

NTSTATUS
NTAPI
IKsDevice_fnInitializeObjectBag(
    IN IKsDevice * iface,
    IN struct KSIOBJECTBAG *Bag,
    IN KMUTANT * Mutant)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnInitializeObjectBag %p\n", This);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsDevice_fnAcquireDevice(
    IN IKsDevice * iface)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnAcquireDevice %p\n", This);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsDevice_fnReleaseDevice(
    IN IKsDevice * iface)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnReleaseDevice %p\n", This);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsDevice_fnGetAdapterObject(
    IN IKsDevice * iface,
    IN PADAPTER_OBJECT Object,
    IN PULONG Unknown1,
    IN PULONG Unknown2)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnGetAdapterObject %p\n", This);
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
IKsDevice_fnAddPowerEntry(
    IN IKsDevice * iface,
    IN struct KSPOWER_ENTRY * Entry,
    IN IKsPowerNotify* Notify)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnAddPowerEntry %p\n", This);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsDevice_fnRemovePowerEntry(
    IN IKsDevice * iface,
    IN struct KSPOWER_ENTRY * Entry)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnRemovePowerEntry %p\n", This);
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
IKsDevice_fnPinStateChange(
    IN IKsDevice * iface,
    IN KSPIN Pin,
    IN PIRP Irp,
    IN KSSTATE OldState,
    IN KSSTATE NewState)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnPinStateChange %p\n", This);
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
IKsDevice_fnArbitrateAdapterChannel(
    IN IKsDevice * iface,
    IN ULONG ControlCode,
    IN IO_ALLOCATION_ACTION Action,
    IN PVOID Context)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnArbitrateAdapterChannel %p\n", This);
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
IKsDevice_fnCheckIoCapability(
    IN IKsDevice * iface,
    IN ULONG Unknown)
{
    IKsDeviceImpl * This = (IKsDeviceImpl*)iface;

    DPRINT1("IKsDevice_fnCheckIoCapability %p\n", This);
    return STATUS_NOT_IMPLEMENTED;
}

static IKsDeviceVtbl vt_IKsDevice = 
{
    IKsDevice_fnQueryInterface,
    IKsDevice_fnAddRef,
    IKsDevice_fnRelease,
    IKsDevice_fnGetStruct,
    IKsDevice_fnInitializeObjectBag,
    IKsDevice_fnAcquireDevice,
    IKsDevice_fnReleaseDevice,
    IKsDevice_fnGetAdapterObject,
    IKsDevice_fnAddPowerEntry,
    IKsDevice_fnRemovePowerEntry,
    IKsDevice_fnPinStateChange,
    IKsDevice_fnArbitrateAdapterChannel,
    IKsDevice_fnCheckIoCapability
};



NTSTATUS
NTAPI
NewIKsDevice(IKsDevice** OutDevice)
{
    IKsDeviceImpl * This;

    This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IKsDeviceImpl), TAG_KSDEVICE);
    if (!This)
       return STATUS_INSUFFICIENT_RESOURCES;

    This->ref = 1;
    This->lpVtbl = &vt_IKsDevice;

    *OutDevice = (IKsDevice*)This;
    return STATUS_SUCCESS;
}

