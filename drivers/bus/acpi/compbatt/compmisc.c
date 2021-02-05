/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
BatteryIoctl(IN ULONG IoControlCode,
             IN PDEVICE_OBJECT DeviceObject,
             IN PVOID InputBuffer,
             IN ULONG InputBufferLength,
             IN PVOID OutputBuffer,
             IN ULONG OutputBufferLength,
             IN BOOLEAN InternalDeviceIoControl)
{
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    PAGED_CODE();
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: ENTERING BatteryIoctl\n");

    /* Initialize the event and IRP */
    KeInitializeEvent(&Event, SynchronizationEvent, 0);
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        InternalDeviceIoControl,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp)
    {
        /* Call the class driver miniport */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait for result */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        /* Print failure */
        if (!(NT_SUCCESS(Status)) && (CompBattDebug & 8))
            DbgPrint("BatteryIoctl: Irp failed - %x\n", Status);

        /* Done */
        if (CompBattDebug & 0x100) DbgPrint("CompBatt: EXITING BatteryIoctl\n");
    }
    else
    {
        /* Out of memory */
        if (CompBattDebug & 8) DbgPrint("BatteryIoctl: couldn't create Irp\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CompBattGetDeviceObjectPointer(IN PUNICODE_STRING DeviceName,
                               IN ACCESS_MASK DesiredAccess,
                               OUT PFILE_OBJECT *FileObject,
                               OUT PDEVICE_OBJECT *DeviceObject)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_OBJECT LocalFileObject;
    HANDLE DeviceHandle;
    PAGED_CODE();

    /* Open a file object handle to the device */
    InitializeObjectAttributes(&ObjectAttributes,
                               DeviceName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateFile(&DeviceHandle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        /* Reference the file object */
        Status = ObReferenceObjectByHandle(DeviceHandle,
                                           0,
                                           *IoFileObjectType,
                                           KernelMode,
                                           (PVOID)&LocalFileObject,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            /* Return the FO and the associated DO */
            *FileObject = LocalFileObject;
            *DeviceObject = IoGetRelatedDeviceObject(LocalFileObject);
        }

        /* Close the handle */
        ZwClose(DeviceHandle);
    }

    /* Return status */
    return Status;
}

/* EOF */
