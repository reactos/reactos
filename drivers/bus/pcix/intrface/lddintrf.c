/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/lddintrf.c
 * PURPOSE:         Legacy Device Detection Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciLegacyDeviceDetectionInterface =
{
    &GUID_LEGACY_DEVICE_DETECTION_STANDARD,
    sizeof(LEGACY_DEVICE_DETECTION_INTERFACE),
    0,
    0,
    PCI_INTERFACE_FDO,
    0,
    PciInterface_LegacyDeviceDetection,
    lddintrf_Constructor,
    lddintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
lddintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI lddintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
lddintrf_Constructor(IN PVOID DeviceExtension,
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
