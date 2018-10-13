/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/locintrf.c
 * PURPOSE:         Location Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciLocationInterface =
{
    &GUID_PNP_LOCATION_INTERFACE,
    sizeof(PNP_LOCATION_INTERFACE),
    PNP_LOCATION_INTERFACE_VERSION,
    PNP_LOCATION_INTERFACE_VERSION,
    PCI_INTERFACE_FDO | PCI_INTERFACE_ROOT | PCI_INTERFACE_PDO,
    0,
    PciInterface_Location,
    locintrf_Constructor,
    locintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
locintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI locintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
locintrf_Constructor(IN PVOID DeviceExtension,
                     IN PVOID Instance,
                     IN PVOID InterfaceData,
                     IN USHORT Version,
                     IN USHORT Size,
                     IN PINTERFACE Interface)
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Interface);

    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
