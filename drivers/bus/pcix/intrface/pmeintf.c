/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/pmeintf.c
 * PURPOSE:         Power Management Event# Signal Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciPmeInterface =
{
    &GUID_PCI_PME_INTERFACE,
    sizeof(PCI_PME_INTERFACE),
    PCI_PME_INTRF_STANDARD_VER,
    PCI_PME_INTRF_STANDARD_VER,
    PCI_INTERFACE_FDO | PCI_INTERFACE_ROOT,
    0,
    PciInterface_PmeHandler,
    PciPmeInterfaceConstructor,
    PciPmeInterfaceInitializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciPmeInterfaceInitializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI PciPmeInterfaceInitializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
PciPmeInterfaceConstructor(IN PVOID DeviceExtension,
                           IN PVOID Instance,
                           IN PVOID InterfaceData,
                           IN USHORT Version,
                           IN USHORT Size,
                           IN PINTERFACE Interface)
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Interface);

    /* Only version 1 is supported */
    if (Version != PCI_PME_INTRF_STANDARD_VER) return STATUS_NOINTERFACE;

    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
