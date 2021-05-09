#include "partmgr.h"

NTSTATUS
NTAPI
ForwardIrpAndForget(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    // this part of a structure is identical in both FDO and PDO
    PDEVICE_OBJECT LowerDevice = ((PFDO_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

NTSTATUS
IssueSyncIoControlRequest(
    _In_ UINT32 IoControlCode,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl)
{
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();

    /* Allocate a non-paged event */
    Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Event), TAG_PARTMGR);
    if (!Event)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize it */
    KeInitializeEvent(Event, NotificationEvent, FALSE);

    /* Build the IRP */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        InternalDeviceIoControl,
                                        Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        /* Fail, free the event */
        ExFreePoolWithTag(Event, TAG_PARTMGR);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call the driver and check if it's pending */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait on the driver */
        KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Free the event and return the Status */
    ExFreePoolWithTag(Event, TAG_PARTMGR);
    return Status;
}
