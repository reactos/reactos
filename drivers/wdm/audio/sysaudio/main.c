/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/main.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  8 Jul 07    Started basic implementation
 */

#include <ntddk.h>
/* #include <ks.h> */
#include <debug.h>

NTSTATUS
NTAPI
SysAudio_Create(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("SysAudio_Create called\n");

    /* TODO */

    /* Complete the request */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID
NTAPI
SysAudio_Unload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT DeviceObject;

    DPRINT("SysAudio_Unload called\n");

    /* Get DO and DE */
    DeviceObject = DriverObject->DeviceObject;
/*    DeviceExtension = DeviceObject->DeviceExtension;*/

    /* Delete the object */
    IoDeleteDevice(DeviceObject);
}

#if 0
VOID
NTAPI
SysAudio_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    /* TODO */

    DPRINT("SysAudio_StartIo called\n");

    /* Complete the request and start the next packet */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoStartNextPacket(DeviceObject, TRUE);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}
#endif

NTSTATUS NTAPI
SysAudio_AddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject)
{
    NTSTATUS status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\sysaudio");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\sysaudio");
    PDEVICE_OBJECT DeviceObject;

    DPRINT("SysAudio_AddDevice called\n");

    status = IoCreateDevice(DriverObject,
                            0,  /* Extension size */
                            &DeviceName,
                            FILE_DEVICE_SOUND,  /* is this right? */
                            0,  /* Characteristics */
                            FALSE,
                            &DeviceObject);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT("Failed to create \\Device\\sysaudio !\n");
        return status;
    }

    status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);

    if ( ! NT_SUCCESS(status) )
    {
        IoDeleteDevice(DeviceObject);

        DPRINT("Failed to create \\DosDevices\\sysaudio symlink!\n");
        return status;
    }

    DPRINT("Device created successfully\n");

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    DPRINT("System audio graph builder (sysaudio) started\n");

    DriverObject->DriverExtension->AddDevice = SysAudio_AddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SysAudio_Create;

/* We'd want to handle this but does KS need to know? */
/*    DriverObject->MajorFunction[IRP_MJ_PNP] = KsDefaultDispatchPnp;*/

/* We don't want to handle this though - pass to KS */
/*    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;*/

    DriverObject->DriverUnload = SysAudio_Unload;
/*    DriverObject->DriverStartIo = SysAudio_StartIo; */

    /* Hmm, shouldn't KS.SYS be involved in some way? */

    return STATUS_SUCCESS;
}
