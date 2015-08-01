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
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("PdoCreate()\n");

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (Stack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
    {
        DPRINT1("Not a directory\n");
        Status = STATUS_NOT_A_DIRECTORY;
        goto done;
    }

    DPRINT("Open LPT%lu: successful\n", DeviceExtension->LptPort);
    DeviceExtension->OpenCount++;

done:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
PdoClose(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    PPDO_DEVICE_EXTENSION pDeviceExtension;

    DPRINT("PdoClose()\n");

    pDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    pDeviceExtension->OpenCount--;

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
PdoRead(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
    DPRINT("PdoRead()\n");

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
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PUCHAR Buffer;
    ULONG i;
    UCHAR PortStatus;
    ULONG ulCount;

    DPRINT("PdoWrite()\n");

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->AttachedFdo->DeviceExtension;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Buffer = GetUserBuffer(Irp);
    DPRINT("Length: %lu\n", IoStack->Parameters.Write.Length);
    DPRINT("Buffer: %p\n", Buffer);

    if (Buffer != NULL)
    {
        DPRINT("%s\n", Buffer);
    }

    for (i = 0; i < IoStack->Parameters.Write.Length; i++)
    {
        DPRINT("%lu: %c\n", i, Buffer[i]);

        ulCount = 0;

        do
        {
            KeStallExecutionProcessor(10);
            PortStatus = READ_PORT_UCHAR((PUCHAR)(FdoDeviceExtension->BaseAddress + 1));
            ulCount++;
        }
        while (ulCount < 500000 && !(PortStatus & LP_PBUSY));

        if (ulCount == 500000)
        {
            DPRINT("Timed out\n");

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_TIMEOUT;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return STATUS_TIMEOUT;
        }

        /* Write character */
        WRITE_PORT_UCHAR((PUCHAR)FdoDeviceExtension->BaseAddress, Buffer[i]);

        KeStallExecutionProcessor(10);

        WRITE_PORT_UCHAR((PUCHAR)(FdoDeviceExtension->BaseAddress + 2), (LP_PSELECP | LP_PINITP | LP_PSTROBE));

        KeStallExecutionProcessor(10);

        WRITE_PORT_UCHAR((PUCHAR)(FdoDeviceExtension->BaseAddress + 2), (LP_PSELECP | LP_PINITP));
    }

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
