/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/busno.c
 * PURPOSE:         Bus Number Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
PciAreBusNumbersConfigured(IN PPCI_PDO_EXTENSION PdoExtension)
{
    UCHAR PrimaryBus, BaseBus, SecondaryBus, SubordinateBus;

    PAGED_CODE();

    /* Get all relevant bus number details */
    PrimaryBus = PdoExtension->Dependent.type1.PrimaryBus;
    BaseBus = PdoExtension->ParentFdoExtension->BaseBus;
    SecondaryBus = PdoExtension->Dependent.type1.SecondaryBus;
    SubordinateBus = PdoExtension->Dependent.type1.SubordinateBus;

    /* The primary bus should be the base bus of the parent */
    if ((PrimaryBus != BaseBus) || (SecondaryBus <= PrimaryBus)) return FALSE;

    /* The subordinate should be a higher bus number than the secondary */
    return SubordinateBus >= SecondaryBus;
}

/* EOF */
