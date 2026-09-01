/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/agpintrf.c
 * PURPOSE:         AGP Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#include <ntagp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE AgpTargetInterface =
{
    &GUID_AGP_TARGET_BUS_INTERFACE_STANDARD,
    sizeof(AGP_TARGET_BUS_INTERFACE_STANDARD),
    AGP_BUS_INTERFACE_V1,
    AGP_BUS_INTERFACE_V1,
    PCI_INTERFACE_PDO,
    0,
    PciInterface_AgpTarget,
    agpintrf_Constructor,
    agpintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

static
VOID
NTAPI
agpintrf_Reference(
    _In_ PVOID Context)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;

    InterlockedIncrement(&PdoExtension->AgpInterfaceReferenceCount);
    ObReferenceObject(PdoExtension->PhysicalDeviceObject);
}

static
VOID
NTAPI
agpintrf_Dereference(
    _In_ PVOID Context)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)Context;

    ObDereferenceObject(PdoExtension->PhysicalDeviceObject);
    InterlockedDecrement(&PdoExtension->AgpInterfaceReferenceCount);
}

/**
 * @brief Finds the one host bridge on a root bus that has an AGP capability.
 */
static
NTSTATUS
NTAPI
agpintrf_FindHostBridge(
    _In_ PPCI_FDO_EXTENSION FdoExtension,
    _Out_ PPCI_PDO_EXTENSION *HostBridge)
{
    PPCI_PDO_EXTENSION Child;
    PPCI_PDO_EXTENSION Found = NULL;
    NTSTATUS Status = STATUS_NO_SUCH_DEVICE;
    PAGED_CODE();

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&FdoExtension->ChildListLock, Executive, KernelMode, FALSE, NULL);

    for (Child = FdoExtension->ChildPdoList; Child; Child = Child->Next)
    {
        if ((Child->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
            (Child->SubClass != PCI_SUBCLASS_BR_HOST) ||
            !Child->TargetAgpCapabilityId)
        {
            continue;
        }

        /* More than one candidate leaves no way to pick the right one */
        if (Found)
        {
            Found = NULL;
            Status = STATUS_NOT_SUPPORTED;
            break;
        }

        Found = Child;
        Status = STATUS_SUCCESS;
    }

    KeSetEvent(&FdoExtension->ChildListLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

    *HostBridge = Found;
    return Status;
}

NTSTATUS
NTAPI
agpintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI agpintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
agpintrf_Constructor(IN PVOID DeviceExtension,
                     IN PVOID Instance,
                     IN PVOID InterfaceData,
                     IN USHORT Version,
                     IN USHORT Size,
                     IN PINTERFACE Interface)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)DeviceExtension;
    PAGP_TARGET_BUS_INTERFACE_STANDARD AgpInterface;
    PPCI_PDO_EXTENSION Target;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Size);
    PAGED_CODE();

    /* Only AGP bridges are supported (which are PCI-to-PCI Bridge Devices) */
    if ((PdoExtension->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
        (PdoExtension->SubClass != PCI_SUBCLASS_BR_PCI_TO_PCI))
    {
        /* Fail any other PDO */
        return STATUS_NOT_SUPPORTED;
    }

    /* Without its own target capability the bridge relies on a host bridge beside it */
    Target = PdoExtension;
    if (!PdoExtension->TargetAgpCapabilityId)
    {
        if (!PCI_IS_ROOT_FDO(PdoExtension->ParentFdoExtension))
            return STATUS_NOT_SUPPORTED;

        Status = agpintrf_FindHostBridge(PdoExtension->ParentFdoExtension, &Target);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    AgpInterface = (PAGP_TARGET_BUS_INTERFACE_STANDARD)Interface;
    AgpInterface->Size = sizeof(*AgpInterface);
    AgpInterface->Version = Version;
    AgpInterface->Context = Target;
    AgpInterface->InterfaceReference = agpintrf_Reference;
    AgpInterface->InterfaceDereference = agpintrf_Dereference;
    AgpInterface->SetBusData = PciBusInterface_SetBusData;
    AgpInterface->GetBusData = PciBusInterface_GetBusData;
    AgpInterface->CapabilityID = Target->TargetAgpCapabilityId;

    return STATUS_SUCCESS;
}

/* EOF */
