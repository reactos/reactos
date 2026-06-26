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

VOID
NTAPI
PciBusInterface_Reference(IN PVOID Context)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;

    InterlockedIncrement(&PdoExtension->BusInterfaceReferenceCount);
}

VOID
NTAPI
PciBusInterface_Dereference(IN PVOID Context)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;

    InterlockedDecrement(&PdoExtension->BusInterfaceReferenceCount);
}

BOOLEAN
NTAPI
PciBusInterface_TranslateBusAddress(IN PVOID Context,
                                    IN PHYSICAL_ADDRESS BusAddress,
                                    IN ULONG Length,
                                    IN OUT PULONG AddressSpace,
                                    OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;
    PPCI_FDO_EXTENSION FdoExtension = PdoExtension->ParentFdoExtension;

    UNREFERENCED_PARAMETER(Length);

    return HalTranslateBusAddress(PCIBus,
                                  FdoExtension->BaseBus,
                                  BusAddress,
                                  AddressSpace,
                                  TranslatedAddress);
}

PDMA_ADAPTER
NTAPI
PciBusInterface_GetDmaAdapter(IN PVOID Context,
                              IN PDEVICE_DESCRIPTION DeviceDescriptor,
                              OUT PULONG NumberOfMapRegisters)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;
    PPCI_FDO_EXTENSION FdoExtension = PdoExtension->ParentFdoExtension;

    if (DeviceDescriptor->InterfaceType == PCIBus)
    {
        DeviceDescriptor->BusNumber = FdoExtension->BaseBus;
    }

    return IoGetDmaAdapter(FdoExtension->PhysicalDeviceObject,
                           DeviceDescriptor,
                           NumberOfMapRegisters);
}

ULONG
NTAPI
PciBusInterface_GetBusData(IN PVOID Context,
                           IN ULONG WhichSpace,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;

    UNREFERENCED_PARAMETER(WhichSpace);

    return HalGetBusDataByOffset(PCIConfiguration,
                                 PdoExtension->ParentFdoExtension->BaseBus,
                                 PdoExtension->Slot.u.AsULONG,
                                 Buffer,
                                 Offset,
                                 Length);
}

ULONG
NTAPI
PciBusInterface_SetBusData(IN PVOID Context,
                           IN ULONG WhichSpace,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;

    UNREFERENCED_PARAMETER(WhichSpace);

    return HalSetBusDataByOffset(PCIConfiguration,
                                 PdoExtension->ParentFdoExtension->BaseBus,
                                 PdoExtension->Slot.u.AsULONG,
                                 Buffer,
                                 Offset,
                                 Length);
}

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
    PBUS_INTERFACE_STANDARD BusInterface;
    PPCI_PDO_EXTENSION PdoExtension;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Version);
    PAGED_CODE();

    if (Size < sizeof(BUS_INTERFACE_STANDARD))
    {
        return STATUS_UNSUCCESSFUL;
    }

    BusInterface = (PBUS_INTERFACE_STANDARD)Interface;
    PdoExtension = (PPCI_PDO_EXTENSION)DeviceExtension;

    BusInterface->Size = sizeof(BUS_INTERFACE_STANDARD);
    BusInterface->Version = 1;
    BusInterface->Context = PdoExtension;
    BusInterface->InterfaceReference = PciBusInterface_Reference;
    BusInterface->InterfaceDereference = PciBusInterface_Dereference;

    BusInterface->TranslateBusAddress = PciBusInterface_TranslateBusAddress;
    BusInterface->GetDmaAdapter = PciBusInterface_GetDmaAdapter;
    BusInterface->SetBusData = PciBusInterface_SetBusData;
    BusInterface->GetBusData = PciBusInterface_GetBusData;

    return STATUS_SUCCESS;
}

/* EOF */
