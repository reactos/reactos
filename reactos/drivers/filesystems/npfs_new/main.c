#include "npfs.h"

PDEVICE_OBJECT NpfsDeviceObject;

NTSTATUS
NTAPI
NpFsdDirectoryControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT1("Next-Generation NPFS-Lite\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = NpFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = NpFsdCreateNamedPipe;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NpFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = NpFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NpFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = NpFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = NpFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = NpFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = NpFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = NpFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = NpFsdQuerySecurityInfo;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = NpFsdSetSecurityInfo;

    DriverObject->DriverUnload = NULL;

    RtlInitUnicodeString(&DeviceName, L"\\Device\\NamedPipe2");
    Status = IoCreateDevice(DriverObject,
                            sizeof(NP_VCB),
                            &DeviceName,
                            FILE_DEVICE_NAMED_PIPE,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create named pipe device! (Status %lx)\n", Status);
        return Status;
    }

    /* Initialize the device object */
    NpfsDeviceObject = DeviceObject;
    DeviceObject->Flags |= DO_LONG_TERM_REQUESTS;

    /* Initialize the Volume Control Block (VCB) */
    NpVcb = DeviceObject->DeviceExtension;
    NpInitializeVcb();
    Status = NpCreateRootDcb();
    ASSERT(Status == STATUS_SUCCESS);
    return STATUS_SUCCESS;
}
