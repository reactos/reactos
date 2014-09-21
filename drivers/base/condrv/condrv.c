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
CompleteRequest(IN PIRP      Irp,
                IN NTSTATUS  Status,
                IN ULONG_PTR Information)
{
    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS NTAPI
ConDrvDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
#define HANDLE_CTRL_CODE(Code)  \
    case Code :                 \
    {                           \
        DPRINT1("ConDrv: " #Code ", stack->FileObject = 0x%p\n", stack->FileObject);        \
        if (stack->FileObject)                                                              \
        {                                                                                   \
            DPRINT1("stack->FileObject->FileName = %wZ\n", &stack->FileObject->FileName);   \
        }                                                                                   \
        break;                                                                              \
    }

    PIO_STACK_LOCATION stack    = IoGetCurrentIrpStackLocation(Irp);
    // ULONG              ctrlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG MajorFunction         = stack->MajorFunction;

    /* Just display all the IRP codes for now... */
    switch (MajorFunction)
    {
        HANDLE_CTRL_CODE(IRP_MJ_CREATE);
        HANDLE_CTRL_CODE(IRP_MJ_CREATE_NAMED_PIPE);
        HANDLE_CTRL_CODE(IRP_MJ_CLOSE);
        HANDLE_CTRL_CODE(IRP_MJ_READ);
        HANDLE_CTRL_CODE(IRP_MJ_WRITE);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_SET_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_EA);
        HANDLE_CTRL_CODE(IRP_MJ_SET_EA);
        HANDLE_CTRL_CODE(IRP_MJ_FLUSH_BUFFERS);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_VOLUME_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_SET_VOLUME_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_DIRECTORY_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_FILE_SYSTEM_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_DEVICE_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_INTERNAL_DEVICE_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_SHUTDOWN);
        HANDLE_CTRL_CODE(IRP_MJ_LOCK_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_CLEANUP);
        HANDLE_CTRL_CODE(IRP_MJ_CREATE_MAILSLOT);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_SECURITY);
        HANDLE_CTRL_CODE(IRP_MJ_SET_SECURITY);
        HANDLE_CTRL_CODE(IRP_MJ_POWER);
        HANDLE_CTRL_CODE(IRP_MJ_SYSTEM_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_DEVICE_CHANGE);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_QUOTA);
        HANDLE_CTRL_CODE(IRP_MJ_SET_QUOTA);
        HANDLE_CTRL_CODE(IRP_MJ_PNP);
        // case IRP_MJ_PNP_POWER:
        // case IRP_MJ_MAXIMUM_FUNCTION:

        default:
        {
            DPRINT1("Unknown Major %lu\n", MajorFunction);
            break;
        }
    }

    return CompleteRequest(Irp, STATUS_SUCCESS, 0);
}

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
