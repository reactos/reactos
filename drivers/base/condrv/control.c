/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver
 * FILE:            drivers/base/condrv/control.c
 * PURPOSE:         Console Driver - Controller Device
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "condrv.h"

#include <condrv/ntddcon.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI
ConDrvCreateController(IN PDRIVER_OBJECT DriverObject,
                       IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DeviceName, SymlinkName;
    PCONDRV_DRIVER DriverExtension;
    PDEVICE_OBJECT Controller = NULL;

    DPRINT1("Create the Controller device...\n");

    RtlInitUnicodeString(&DeviceName , DD_CONDRV_CTRL_DEVICE_NAME_U);
    RtlInitUnicodeString(&SymlinkName, DD_CONDRV_CTRL_SYMLNK_NAME_U);

    /* Get the driver extension */
    DriverExtension = (PCONDRV_DRIVER)IoGetDriverObjectExtension(DriverObject,
                                                                 DriverObject);

    /* Create the Controller device, if it doesn't exist */
    Status = IoCreateDevice(DriverObject,
                            0,
                            (PUNICODE_STRING)&DeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Controller);
    if (!NT_SUCCESS(Status)) goto Done;

    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(Controller);
        goto Done;
    }

    Controller->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Save the Controller device */
    DriverExtension->Controller = Controller;

Done:
    DPRINT1("Done, Status = 0x%08lx\n", Status);
    return Status;
}

NTSTATUS NTAPI
ConDrvDeleteController(IN PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT Controller;
    UNICODE_STRING SymlinkName;

    DPRINT1("Delete the Controller device...\n");

    /* Retrieve the Controller device */
    Controller = ((PCONDRV_DRIVER)IoGetDriverObjectExtension(DriverObject, DriverObject))->Controller;
    if (!Controller) return STATUS_OBJECT_TYPE_MISMATCH;

    RtlInitUnicodeString(&SymlinkName, DD_CONDRV_CTRL_SYMLNK_NAME_U);
    IoDeleteSymbolicLink(&SymlinkName);

    /* Delete the controller device itself */
    IoDeleteDevice(Controller);

    DPRINT1("Done, Status = 0x%08lx\n", Status);
    return Status;
}

/* EOF */
