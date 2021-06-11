/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/misc.c
 * PURPOSE:         Miscellaneous functions
 */

#include "parport.h"


/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
ForwardIrpAndForget(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    else
        LowerDevice = ((PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedFdo;
    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}


PVOID
GetUserBuffer(IN PIRP Irp)
{
    ASSERT(Irp);

    if (Irp->MdlAddress)
        return Irp->MdlAddress;
    else
        return Irp->AssociatedIrp.SystemBuffer;
}

/* EOF */
