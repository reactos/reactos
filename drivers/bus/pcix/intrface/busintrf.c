/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/busintrf.c
 * PURPOSE:         Bus Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE BusHandlerInterface =
{
    &GUID_BUS_INTERFACE_STANDARD,
    sizeof(BUS_INTERFACE_STANDARD),
    1,
    1,
    PCI_INTERFACE_PDO,
    0,
    PciInterface_BusHandler,
    busintrf_Constructor,
    busintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
busintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI busintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
busintrf_Constructor(IN PVOID DeviceExtension,
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
