#include "priv.h"

/* ===============================================================
    Misc. Helper Functions
*/

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCacheMedium(
    IN  PUNICODE_STRING SymbolicLink,
    IN  PKSPIN_MEDIUM Medium,
    IN  ULONG PinDirection)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/

KSDDKAPI NTSTATUS NTAPI
KsDefaultDispatchPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("KsDefaultDispatchPnp entered with func %x\n", IoStack->MinorFunction);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            return STATUS_INSUFFICIENT_RESOURCES;
        case IRP_MN_REMOVE_DEVICE:
            // FIXME
            // destroy device header, detach device and delete device
        case IRP_MN_START_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            return STATUS_SUCCESS;
        default:
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            //Status = IoCallDriver(NULL /* PnpBaseObject */, Irp);
    }

    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsSetDevicePnpAndBaseObject(
    IN  KSDEVICE_HEADER Header,
    IN  PDEVICE_OBJECT PnpDeviceObject,
    IN  PDEVICE_OBJECT BaseDevice)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsDefaultDispatchPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsSetPowerDispatch(
    IN  KSOBJECT_HEADER Header,
    IN  PFNKSCONTEXT_DISPATCH PowerDispatch OPTIONAL,
    IN  PVOID PowerContext OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsReferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsDereferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsFreeObjectCreateItem(
    IN  KSDEVICE_HEADER Header,
    IN  PUNICODE_STRING CreateItem)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsFreeObjectCreateItemsByContext(
    IN  KSDEVICE_HEADER Header,
    IN  PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsLoadResource(
    IN  PVOID ImageBase,
    IN  POOL_TYPE PoolType,
    IN  ULONG_PTR ResourceName,
    IN  ULONG ResourceType,
    OUT PVOID* Resource,
    OUT PULONG ResourceSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
VOID
KsNullDriverUnload(
    IN  PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersectionEx(
    IN  PIRP Irp,
    IN  PKSP_PIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  ULONG DescriptorSize,
    IN  PFNKSINTERSECTHANDLEREX IntersectHandler OPTIONAL,
    IN  PVOID HandlerContext OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI PDEVICE_OBJECT NTAPI
KsQueryDevicePnpObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsRecalculateStackDepth(
    IN  KSDEVICE_HEADER Header,
    IN  BOOLEAN ReuseStackLocation)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsSetTargetDeviceObject(
    IN  KSOBJECT_HEADER Header,
    IN  PDEVICE_OBJECT TargetDevice OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsSetTargetState(
    IN  KSOBJECT_HEADER Header,
    IN  KSTARGET_STATE TargetState)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsSynchronousIoControlDevice(
    IN  PFILE_OBJECT FileObject,
    IN  KPROCESSOR_MODE RequestorMode,
    IN  ULONG IoControl,
    IN  PVOID InBuffer,
    IN  ULONG InSize,
    OUT PVOID OutBuffer,
    IN  ULONG OUtSize,
    OUT PULONG BytesReturned)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsAcquireDeviceSecurityLock(
    IN KSDEVICE_HEADER DevHeader,
    IN BOOLEAN Exclusive)
{
    NTSTATUS Status;
    PKSIDEVICE_HEADER Header = (PKSIDEVICE_HEADER)DevHeader;

    KeEnterCriticalRegion();

    if (Exclusive)
    {
        Status = ExAcquireResourceExclusiveLite(&Header->SecurityLock, TRUE);
    }
    else
    {
        Status = ExAcquireResourceSharedLite(&Header->SecurityLock, TRUE);
    }
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsReleaseDeviceSecurityLock(
    IN KSDEVICE_HEADER DevHeader)
{
    PKSIDEVICE_HEADER Header = (PKSIDEVICE_HEADER)DevHeader;

    ExReleaseResourceLite(&Header->SecurityLock);
    KeLeaveCriticalRegion();
}

