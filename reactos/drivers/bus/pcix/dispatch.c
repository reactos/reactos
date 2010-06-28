/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/dispatch.c
 * PURPOSE:         WDM Dispatch Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciDispatchIrp(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    /* This function is not yet implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

/* EOF */
