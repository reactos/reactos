/*
 * PROJECT:        ReactOS Generic CPU Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/processor/processr/misc.c
 * PURPOSE:        Misc routines
 * PROGRAMMERS:    Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "processr.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    LowerDevice = ((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

/* EOF */
