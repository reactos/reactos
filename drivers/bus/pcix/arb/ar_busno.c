/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/arb/ar_busno.c
 * PURPOSE:         Bus Number Arbitration
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE ArbiterInterfaceBusNumber =
{
    &GUID_ARBITER_INTERFACE_STANDARD,
    sizeof(ARBITER_INTERFACE),
    0,
    0,
    PCI_INTERFACE_FDO,
    0,
    PciArb_BusNumber,
    arbusno_Constructor,
    arbusno_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
arbusno_Initializer(IN PVOID Instance)
{
    PPCI_ARBITER_INSTANCE Arbiter = Instance;
    PPCI_FDO_EXTENSION FdoExtension;
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory(&Arbiter->CommonInstance, sizeof(Arbiter->CommonInstance));

    FdoExtension = Arbiter->BusFdoExtension;

    /* Not yet implemented */
    UNIMPLEMENTED;

#if 0
    Arbiter->CommonInstance.UnpackRequirement = arbusno_UnpackRequirement;
    Arbiter->CommonInstance.PackResource = arbusno_PackResource;
    Arbiter->CommonInstance.UnpackResource = arbusno_UnpackResource;
    Arbiter->CommonInstance.ScoreRequirement = arbusno_ScoreRequirement;
#endif

    Status = ArbInitializeArbiterInstance(&Arbiter->CommonInstance,
                                          FdoExtension->FunctionalDeviceObject,
                                          CmResourceTypeBusNumber,
                                          Arbiter->InstanceName,
                                          L"Pci",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("arbusno_Initializer: init arbiter return %X", Status);
    }

    return Status;
}

NTSTATUS
NTAPI
arbusno_Constructor(IN PVOID DeviceExtension,
                    IN PVOID PciInterface,
                    IN PVOID InterfaceData,
                    IN USHORT Version,
                    IN USHORT Size,
                    IN PINTERFACE Interface)
{
    PPCI_FDO_EXTENSION FdoExtension = (PPCI_FDO_EXTENSION)DeviceExtension;
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(PciInterface);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Interface);

    /* Make sure it's the expected interface */
    if ((ULONG_PTR)InterfaceData != CmResourceTypeBusNumber)
    {
        /* Arbiter support must have been initialized first */
        if (FdoExtension->ArbitersInitialized)
        {
            /* Not yet implemented */
            UNIMPLEMENTED;
            while (TRUE);
        }
        else
        {
            /* No arbiters for this FDO */
            Status = STATUS_NOT_SUPPORTED;
        }
    }
    else
    {
        /* Not the right interface */
        Status = STATUS_INVALID_PARAMETER_5;
    }

    /* Return the status */
    return Status;
}

/* EOF */
