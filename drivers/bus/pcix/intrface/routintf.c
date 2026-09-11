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

/*
 * These are called back from interrupt arbitration, which can run while
 * PciAssignSlotResources holds PciGlobalLock, so they must not take it.
 */

/**
 * @brief Finds the PDO extension behind one of our PDOs or a cached legacy device object.
 * LegacyDevice receives the cache entry when the object came from a non-PnP driver.
 */
static
PPCI_PDO_EXTENSION
NTAPI
routeintrf_FindDevice(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Outptr_result_maybenull_ PPCI_LEGACY_DEVICE *LegacyDevice)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_LEGACY_DEVICE Entry;

    *LegacyDevice = NULL;
    if (!DeviceObject)
        return NULL;

    if (DeviceObject->DriverObject == PciDriverObject)
    {
        /* Our FDOs share the driver object */
        PdoExtension = DeviceObject->DeviceExtension;
        if (PdoExtension->ExtensionType != PciPdoExtensionType)
            return NULL;

        return PdoExtension;
    }

    for (Entry = PciLegacyDeviceHead; Entry; Entry = Entry->Next)
    {
        if (Entry->DeviceObject != DeviceObject || !Entry->PdoExtension)
            continue;

        *LegacyDevice = Entry;
        return Entry->PdoExtension;
    }

    return NULL;
}

static
VOID
NTAPI
routeintrf_Reference(
    _In_ PVOID Context)
{
    /* Nothing is reached through the context, so there is nothing to count */
    UNREFERENCED_PARAMETER(Context);
}

static
VOID
NTAPI
routeintrf_Dereference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/**
 * @brief Describes where a device's interrupt pin is wired.
 * ParentPdo is the PDO of the bridge above the device's bus, which is not ours for a root bus.
 */
static
NTSTATUS
NTAPI
routeintrf_GetInterruptRouting(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ ULONG *Bus,
    _Out_ ULONG *PciSlot,
    _Out_ UCHAR *InterruptLine,
    _Out_ UCHAR *InterruptPin,
    _Out_ UCHAR *ClassCode,
    _Out_ UCHAR *SubClassCode,
    _Out_ PDEVICE_OBJECT *ParentPdo,
    _Out_ ROUTING_TOKEN *RoutingToken,
    _Out_ UCHAR *Flags)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_LEGACY_DEVICE LegacyDevice;
    PAGED_CODE();

    *Flags = 0;

    PdoExtension = routeintrf_FindDevice(Pdo, &LegacyDevice);
    if (!PdoExtension)
        return STATUS_NOT_FOUND;

    if (LegacyDevice)
    {
        /* Answer with what was captured when the legacy driver claimed the slot */
        *Bus = LegacyDevice->BusNumber;
        *PciSlot = LegacyDevice->SlotNumber;
        *InterruptLine = LegacyDevice->InterruptLine;
        *InterruptPin = LegacyDevice->InterruptPin;
        *ClassCode = LegacyDevice->BaseClass;
        *SubClassCode = LegacyDevice->SubClass;
        *ParentPdo = LegacyDevice->PhysicalDeviceObject;
        *RoutingToken = LegacyDevice->RoutingToken;
        return STATUS_SUCCESS;
    }

    *Bus = PdoExtension->ParentFdoExtension->BaseBus;
    *PciSlot = PdoExtension->Slot.u.AsULONG;
    *InterruptLine = PdoExtension->RawInterruptLine;
    *InterruptPin = PdoExtension->InterruptPin;
    *ClassCode = PdoExtension->BaseClass;
    *SubClassCode = PdoExtension->SubClass;
    *ParentPdo = PdoExtension->ParentFdoExtension->PhysicalDeviceObject;
    *RoutingToken = PdoExtension->RoutingToken;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
routeintrf_SetInterruptRoutingToken(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PROUTING_TOKEN RoutingToken)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_LEGACY_DEVICE LegacyDevice;
    PAGED_CODE();

    if (!RoutingToken)
        return STATUS_INVALID_PARAMETER;

    PdoExtension = routeintrf_FindDevice(Pdo, &LegacyDevice);
    if (!PdoExtension)
        return STATUS_NOT_FOUND;

    if (LegacyDevice)
    {
        LegacyDevice->RoutingToken = *RoutingToken;
    }
    else
    {
        PdoExtension->RoutingToken = *RoutingToken;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Programs the IRQ chosen for a device into its interrupt line register.
 * The saved BIOS copy is updated too, so a later enumeration does not restore the old line.
 */
static
VOID
NTAPI
routeintrf_UpdateInterruptLine(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ UCHAR LineRegister)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_LEGACY_DEVICE LegacyDevice;
    PCI_COMMON_HEADER BiosData;
    PAGED_CODE();

    PdoExtension = routeintrf_FindDevice(Pdo, &LegacyDevice);
    if (!PdoExtension)
        return;

    /* PciSetResources writes RawInterruptLine back whenever it programs the device */
    PdoExtension->RawInterruptLine = LineRegister;
    PdoExtension->AdjustedInterruptLine = LineRegister;
    if (LegacyDevice)
        LegacyDevice->InterruptLine = LineRegister;

    PciWriteDeviceConfig(PdoExtension,
                         &LineRegister,
                         FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.InterruptLine),
                         sizeof(LineRegister));

    if (!NT_SUCCESS(PciGetBiosConfig(PdoExtension, &BiosData)))
        return;

    if (BiosData.u.type0.InterruptLine == LineRegister)
        return;

    BiosData.u.type0.InterruptLine = LineRegister;
    PciSaveBiosConfig(PdoExtension, &BiosData);
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
    PINT_ROUTE_INTERFACE_STANDARD RouteInterface = (PINT_ROUTE_INTERFACE_STANDARD)Interface;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Size);

    /* Only version 1 is supported */
    if (Version != PCI_INT_ROUTE_INTRF_STANDARD_VER) return STATUS_NOINTERFACE;

    RouteInterface->Size = sizeof(*RouteInterface);
    RouteInterface->Version = PCI_INT_ROUTE_INTRF_STANDARD_VER;
    RouteInterface->Context = DeviceExtension;
    RouteInterface->InterfaceReference = routeintrf_Reference;
    RouteInterface->InterfaceDereference = routeintrf_Dereference;
    RouteInterface->GetInterruptRouting = routeintrf_GetInterruptRouting;
    RouteInterface->SetInterruptRoutingToken = routeintrf_SetInterruptRoutingToken;
    RouteInterface->UpdateInterruptLine = routeintrf_UpdateInterruptLine;
    return STATUS_SUCCESS;
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
