/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PDEVICE_OBJECT KsecDeviceObject;


/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KsecDD");
    NTSTATUS Status;

    /* Create the KsecDD device */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_KSEC,
                            0x100u,
                            FALSE,
                            &KsecDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create KsecDD device: 0x%lx\n", Status);
        return Status;
    }

    /* Set up dispatch table */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = KsecDdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KsecDdDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = KsecDdDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = KsecDdDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = KsecDdDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = KsecDdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KsecDdDispatch;

    /* Initialize */
    KsecInitializeEncryptionSupport();

    return STATUS_SUCCESS;
}
