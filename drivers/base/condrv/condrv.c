/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver
 * FILE:            drivers/base/condrv/condrv.c
 * PURPOSE:         Console Driver Management Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "condrv.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * Callback functions prototypes
 */
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     ConDrvUnload;
/*
DRIVER_DISPATCH ConDrvIoControl;
DRIVER_DISPATCH ConDrvCreate;
DRIVER_DISPATCH ConDrvClose;
DRIVER_DISPATCH ConDrvRead;
DRIVER_DISPATCH ConDrvWrite;
DRIVER_DISPATCH ConDrvCleanup;
*/
DRIVER_DISPATCH ConDrvDispatch;

NTSTATUS NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    USHORT i;
    PCONDRV_DRIVER DriverExtension = NULL;

    DPRINT1("Loading ReactOS Console Driver v0.0.1...\n");

    DriverObject->DriverUnload = ConDrvUnload;

    /* Initialize the different callback function pointers */
    for (i = 0 ; i <= IRP_MJ_MAXIMUM_FUNCTION ; ++i)
        DriverObject->MajorFunction[i] = ConDrvDispatch;

#if 0
    DriverObject->MajorFunction[IRP_MJ_CREATE]  = ConDrvCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   = ConDrvClose;

    /* temporary deactivated...
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = ConDrvCleanup;
    */
    DriverObject->MajorFunction[IRP_MJ_READ]    = ConDrvRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]   = ConDrvWrite;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ConDrvIoControl;
#endif

    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             DriverObject, // Unique ID for the driver object extension ==> gives it its address !
                                             sizeof(CONDRV_DRIVER),
                                             (PVOID*)&DriverExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoAllocateDriverObjectExtension() failed with status 0x%08lx\n", Status);
        return Status;
    }
    RtlZeroMemory(DriverExtension, sizeof(CONDRV_DRIVER));

    Status = ConDrvCreateController(DriverObject, RegistryPath);

    DPRINT1("Done, Status = 0x%08lx\n", Status);
    return Status;
}

VOID NTAPI
ConDrvUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT1("Unloading ReactOS Console Driver v0.0.1...\n");

    /*
     * Delete the Controller device. This has as effect
     * to delete also all the terminals.
     */
    ConDrvDeleteController(DriverObject);

    /* Sanity check: No devices must exist at this point */
    ASSERT(DriverObject->DeviceObject == NULL);

    DPRINT1("Done\n");
    return;
}

/* EOF */
