#include "virtiosnd.h"

#define VIRTIO_SND_MAX_MINIPORTS 2

static PDRIVER_DISPATCH VirtioSndPortClsPnpDispatch;

static NTSTATUS NTAPI
VirtioSndPnpDispatch(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (IrpStack->MinorFunction == IRP_MN_STOP_DEVICE || IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        VirtioSndStopTransport(DeviceObject);
    }

    return VirtioSndPortClsPnpDispatch(DeviceObject, Irp);
}

static NTSTATUS NTAPI
VirtioSndAddDevice(_In_ PDRIVER_OBJECT DriverObject, _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PAGED_CODE();

    return PcAddAdapterDevice(
        DriverObject, PhysicalDeviceObject, (PCPFNSTARTDEVICE)VirtioSndStartDevice, VIRTIO_SND_MAX_MINIPORTS, 0);
}

NTSTATUS
NTAPI
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = PcInitializeAdapterDriver(DriverObject, RegistryPath, (PDRIVER_ADD_DEVICE)VirtioSndAddDevice);
    if (!NT_SUCCESS(Status))
        return Status;

    VirtioSndPortClsPnpDispatch = DriverObject->MajorFunction[IRP_MJ_PNP];
    if (VirtioSndPortClsPnpDispatch == NULL)
        return STATUS_UNSUCCESSFUL;

    DriverObject->MajorFunction[IRP_MJ_PNP] = VirtioSndPnpDispatch;
    return STATUS_SUCCESS;
}
