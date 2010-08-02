/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/device.c
 * PURPOSE:         Device Management
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
Device_SaveCurrentSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
Device_SaveLimits(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
Device_MassageHeaderForLimitsDetermination(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
Device_RestoreCurrent(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
Device_GetAdditionalResourceDescriptors(IN PPCI_CONFIGURATOR_CONTEXT Context,
                                        IN PPCI_COMMON_HEADER PciData,
                                        IN PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
Device_ResetDevice(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
Device_ChangeResourceSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
