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

VOID
NTAPI
PciPmeInterface_Reference(IN PVOID Context)
{
    PPCI_FDO_EXTENSION FdoExtension = (PPCI_FDO_EXTENSION)Context;

    InterlockedIncrement(&FdoExtension->PciPmeInterfaceCount);
}

VOID
NTAPI
PciPmeInterface_Dereference(IN PVOID Context)
{
    PPCI_FDO_EXTENSION FdoExtension = (PPCI_FDO_EXTENSION)Context;

    InterlockedDecrement(&FdoExtension->PciPmeInterfaceCount);
}

VOID
NTAPI
PciPmeInterface_UpdateEnable(IN PDEVICE_OBJECT DeviceObject,
                             IN BOOLEAN Enable)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Enable);

    DPRINT1("PciPmeInterface_UpdateEnable is not yet implemented\n");
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
    PPCI_PME_INTERFACE PmeInterface;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Version);

    PAGED_CODE();

    if (Size < sizeof(PCI_PME_INTERFACE))
    {
        return STATUS_UNSUCCESSFUL;
    }

    PmeInterface = (PPCI_PME_INTERFACE)Interface;
    PmeInterface->Size = sizeof(PCI_PME_INTERFACE);
    PmeInterface->Version = PCI_PME_INTRF_STANDARD_VER;
    PmeInterface->Context = DeviceExtension;
    PmeInterface->InterfaceReference = PciPmeInterface_Reference;
    PmeInterface->InterfaceDereference = PciPmeInterface_Dereference;
    PmeInterface->UpdateEnable = PciPmeInterface_UpdateEnable;

    return STATUS_SUCCESS;
}

/* EOF */
