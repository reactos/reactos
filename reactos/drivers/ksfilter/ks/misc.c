#include <ntddk.h>
#include <debug.h>
#include <ks.h>

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
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsInitializeDriver(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING  RegistryPath,
    IN const KSDEVICE_DESCRIPTOR  *Descriptor OPTIONAL
)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

