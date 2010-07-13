/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/state.c
 * PURPOSE:         Bus/Device State Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PciInitializeState(IN PPCI_FDO_EXTENSION DeviceExtension)
{
    /* Set the initial state */
    DeviceExtension->DeviceState = PciNotStarted;
    DeviceExtension->TentativeNextState = PciNotStarted;
}

/* EOF */
