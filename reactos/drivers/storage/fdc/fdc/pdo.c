/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/pdo.c
 * PURPOSE:        Physical Device Object routines
 * PROGRAMMERS:    Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "fdc.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
FdcPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    DPRINT1("FdcPdoPnp()\n");
    return Status;
}

/* EOF */
