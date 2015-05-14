/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/pdo.c
 * PURPOSE:         PDO functions
 */

#include "parport.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
PdoCreate(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    DPRINT("PdoCreate()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PdoClose(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    DPRINT("PdoClose()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PdoCleanup(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    DPRINT("PdoCleanup()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PdoWrite(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    DPRINT("PdoWrite()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PdoPnp(IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp)
{
    DPRINT("PdoPnp()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PdoPower(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    DPRINT("PdoPower()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/* EOF */
