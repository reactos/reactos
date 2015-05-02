/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * PURPOSE:         Parport driver loading/unloading
 */

#include "parport.h"

static DRIVER_UNLOAD DriverUnload;
DRIVER_INITIALIZE DriverEntry;

static
VOID
NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("Parport DriverUnload\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegPath)
{
    DPRINT("Parport DriverEntry\n");

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}
