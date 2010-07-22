/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/ppbridge.c
 * PURPOSE:         PCI-to-PCI Bridge Support
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
PPBridge_SaveCurrentSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
PPBridge_SaveLimits(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
PPBridge_MassageHeaderForLimitsDetermination(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
PPBridge_RestoreCurrent(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
PPBridge_GetAdditionalResourceDescriptors(IN PPCI_CONFIGURATOR_CONTEXT Context,
                                          IN PPCI_COMMON_HEADER PciData,
                                          IN PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
PPBridge_ResetDevice(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
PPBridge_ChangeResourceSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
