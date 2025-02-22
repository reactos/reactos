/*
 * PROJECT:     ReactOS Composite Battery Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Miscellaneous Support Routines
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group <ros.arm@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
BatteryIoctl(
    _In_ ULONG IoControlCode,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _Inout_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl)
{
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    PAGED_CODE();
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING BatteryIoctl\n");

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
        if (!(NT_SUCCESS(Status)) && (CompBattDebug & COMPBATT_DEBUG_ERR))
            DbgPrint("BatteryIoctl: Irp failed - %x\n", Status);

        /* Done */
        if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING BatteryIoctl\n");
    }
    else
    {
        /* Out of memory */
        if (CompBattDebug & COMPBATT_DEBUG_ERR) DbgPrint("BatteryIoctl: couldn't create Irp\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CompBattGetDeviceObjectPointer(
    _In_ PUNICODE_STRING DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PFILE_OBJECT *FileObject,
    _Out_ PDEVICE_OBJECT *DeviceObject)
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
