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
arbusno_UnpackRequirement(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
                          _Out_ PULONGLONG Minimum,
                          _Out_ PULONGLONG Maximum,
                          _Out_ PULONGLONG Length,
                          _Out_ PULONGLONG Alignment)
{
    /* This arbiter is only ever handed bus number descriptors */
    if (Descriptor->Type != CmResourceTypeBusNumber) return STATUS_INVALID_PARAMETER;

    /* Bus numbers are counted, not addressed, so they are always unit-aligned */
    *Minimum = Descriptor->u.BusNumber.MinBusNumber;
    *Maximum = Descriptor->u.BusNumber.MaxBusNumber;
    *Length = Descriptor->u.BusNumber.Length;
    *Alignment = 1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
arbusno_PackResource(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
                     _In_ ULONGLONG Start,
                     _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
{
    /* Turn the run of bus numbers the engine settled on into a resource */
    if (Descriptor->Type != CmResourceTypeBusNumber) return STATUS_INVALID_PARAMETER;

    Resource->Type = CmResourceTypeBusNumber;
    Resource->Flags = Descriptor->Flags;
    Resource->ShareDisposition = Descriptor->ShareDisposition;
    Resource->u.BusNumber.Start = (ULONG)Start;
    Resource->u.BusNumber.Length = Descriptor->u.BusNumber.Length;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
arbusno_UnpackResource(_In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
                       _Out_ PULONGLONG Start,
                       _Out_ PULONGLONG Length)
{
    /* Read a previously packed run of bus numbers back out */
    if (Resource->Type != CmResourceTypeBusNumber) return STATUS_INVALID_PARAMETER;

    *Start = Resource->u.BusNumber.Start;
    *Length = Resource->u.BusNumber.Length;
    return STATUS_SUCCESS;
}

INT32
NTAPI
arbusno_ScoreRequirement(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONGLONG Minimum, Maximum, Length, Alignment;

    /* A requirement this arbiter cannot read is treated as unsatisfiable */
    if (!NT_SUCCESS(arbusno_UnpackRequirement(Descriptor,
                                              &Minimum,
                                              &Maximum,
                                              &Length,
                                              &Alignment)) ||
        (Maximum < Minimum))
    {
        return -1;
    }

    /*
     * The score is how many places the run could go, so that the engine places
     * the most constrained requirements first.
     */
    if ((Maximum - Minimum) >= MAXLONG) return MAXLONG;
    return (INT32)(Maximum - Minimum + 1);
}

NTSTATUS
NTAPI
arbusno_Initializer(IN PVOID Instance)
{
    PPCI_ARBITER_INSTANCE Arbiter = Instance;
    PPCI_FDO_EXTENSION FdoExtension;
    NTSTATUS Status;

    PAGED_CODE();

    /* Start from a clean engine instance */
    RtlZeroMemory(&Arbiter->CommonInstance, sizeof(Arbiter->CommonInstance));

    FdoExtension = Arbiter->BusFdoExtension;

    /* Teach the engine how to read and write bus number descriptors */
    Arbiter->CommonInstance.UnpackRequirement = arbusno_UnpackRequirement;
    Arbiter->CommonInstance.PackResource = arbusno_PackResource;
    Arbiter->CommonInstance.UnpackResource = arbusno_UnpackResource;
    Arbiter->CommonInstance.ScoreRequirement = arbusno_ScoreRequirement;

    Status = ArbiterLibInitializeInstance(&Arbiter->CommonInstance,
                                          FdoExtension->FunctionalDeviceObject,
                                          CmResourceTypeBusNumber,
                                          Arbiter->InstanceName,
                                          L"Pci",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("arbusno_Initializer: init arbiter return %X\n", Status);
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
    PAGED_CODE();

    UNREFERENCED_PARAMETER(PciInterface);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(Size);

    /* This one only arbitrates bus numbers */
    if ((ULONG_PTR)InterfaceData != CmResourceTypeBusNumber)
    {
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Hand out the instance that was built for this bus */
    return PciArbiterConstructor(FdoExtension,
                                 PciArb_BusNumber,
                                 (PARBITER_INTERFACE)Interface);
}

/* EOF */
