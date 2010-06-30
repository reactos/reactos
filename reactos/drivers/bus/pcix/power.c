/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/power.c
 * PURPOSE:         Bus/Device Power Management
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
PciFdoWaitWake(IN PIRP Irp,
               IN PIO_STACK_LOCATION IoStackLocation,
               IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoSetPowerState(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpQueryPower(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
