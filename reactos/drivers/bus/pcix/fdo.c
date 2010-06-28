/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/fdo.c
 * PURPOSE:         FDO Device Management
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
PciAddDevice(IN PDRIVER_OBJECT DriverObject,
             IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    /* This function is not yet implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

/* EOF */
