/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/devhere.c
 * PURPOSE:         Device Presence Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciDevicePresentInterface =
{
    &GUID_PCI_DEVICE_PRESENT_INTERFACE,
    sizeof(PCI_DEVICE_PRESENT_INTERFACE),
    PCI_DEVICE_PRESENT_INTERFACE_VERSION,
    PCI_DEVICE_PRESENT_INTERFACE_VERSION,
    PCI_INTERFACE_PDO,
    0,
    PciInterface_DevicePresent,
    devpresent_Constructor,
    devpresent_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
devpresent_Initializer(IN PVOID Instance)
{
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG(FALSE, "PCI devpresent_Initializer, unexpected call.");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
devpresent_Constructor(IN PVOID DeviceExtension,
                       IN PVOID Instance,
                       IN PVOID InterfaceData,
                       IN USHORT Version,
                       IN USHORT Size,
                       IN PINTERFACE Interface)
{
    PAGED_CODE();

    /* Not yet implemented */
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
