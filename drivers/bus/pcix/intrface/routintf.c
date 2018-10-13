/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/routinf.c
 * PURPOSE:         Routing Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PPCI_LEGACY_DEVICE PciLegacyDeviceHead;

PCI_INTERFACE PciRoutingInterface =
{
    &GUID_INT_ROUTE_INTERFACE_STANDARD,
    sizeof(INT_ROUTE_INTERFACE_STANDARD),
    PCI_INT_ROUTE_INTRF_STANDARD_VER,
    PCI_INT_ROUTE_INTRF_STANDARD_VER,
    PCI_INTERFACE_FDO,
    0,
    PciInterface_IntRouteHandler,
    routeintrf_Constructor,
    routeintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
routeintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI routeintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
routeintrf_Constructor(IN PVOID DeviceExtension,
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
    if (Version != PCI_INT_ROUTE_INTRF_STANDARD_VER) return STATUS_NOINTERFACE;

    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PciCacheLegacyDeviceRouting(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG BusNumber,
                            IN ULONG SlotNumber,
                            IN UCHAR InterruptLine,
                            IN UCHAR InterruptPin,
                            IN UCHAR BaseClass,
                            IN UCHAR SubClass,
                            IN PDEVICE_OBJECT PhysicalDeviceObject,
                            IN PPCI_PDO_EXTENSION PdoExtension,
                            OUT PDEVICE_OBJECT *pFoundDeviceObject)
{
    PPCI_LEGACY_DEVICE *Link;
    PPCI_LEGACY_DEVICE LegacyDevice;
    PDEVICE_OBJECT FoundDeviceObject;
    PAGED_CODE();

    /* Scan current registered devices */
    LegacyDevice = PciLegacyDeviceHead;
    Link = &PciLegacyDeviceHead;
    while (LegacyDevice)
    {
        /* Find a match */
        if ((BusNumber == LegacyDevice->BusNumber) &&
            (SlotNumber == LegacyDevice->SlotNumber))
        {
            /* We already know about this routing */
            break;
        }

        /* We know about device already, but for a different location */
        if (LegacyDevice->DeviceObject == DeviceObject)
        {
            /* Free the existing structure, move to the next one */
            *Link = LegacyDevice->Next;
            ExFreePoolWithTag(LegacyDevice, 0);
            LegacyDevice = *Link;
        }
        else
        {
            /* Keep going */
            Link = &LegacyDevice->Next;
            LegacyDevice = LegacyDevice->Next;
        }
    }

    /* Did we find a match? */
    if (!LegacyDevice)
    {
        /* Allocate a new cache structure */
        LegacyDevice = ExAllocatePoolWithTag(PagedPool,
                                             sizeof(PCI_LEGACY_DEVICE),
                                             'PciR');
        if (!LegacyDevice) return STATUS_INSUFFICIENT_RESOURCES;

        /* Save all the data in it */
        RtlZeroMemory(LegacyDevice, sizeof(PCI_LEGACY_DEVICE));
        LegacyDevice->BusNumber = BusNumber;
        LegacyDevice->SlotNumber = SlotNumber;
        LegacyDevice->InterruptLine = InterruptLine;
        LegacyDevice->InterruptPin = InterruptPin;
        LegacyDevice->BaseClass = BaseClass;
        LegacyDevice->SubClass = SubClass;
        LegacyDevice->PhysicalDeviceObject = PhysicalDeviceObject;
        LegacyDevice->DeviceObject = DeviceObject;
        LegacyDevice->PdoExtension = PdoExtension;

        /* Link it in the list */
        LegacyDevice->Next = PciLegacyDeviceHead;
        PciLegacyDeviceHead = LegacyDevice;
    }

    /* Check if we found, or created, a matching caching structure */
    FoundDeviceObject = LegacyDevice->DeviceObject;
    if (FoundDeviceObject == DeviceObject)
    {
        /* Return the device object and success */
        if (pFoundDeviceObject) *pFoundDeviceObject = DeviceObject;
        return STATUS_SUCCESS;
    }

    /* Otherwise, this is a new device object for this location */
    LegacyDevice->DeviceObject = DeviceObject;
    if (pFoundDeviceObject) *pFoundDeviceObject = FoundDeviceObject;
    return STATUS_SUCCESS;
}

/* EOF */
