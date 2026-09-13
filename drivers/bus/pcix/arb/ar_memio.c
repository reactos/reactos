/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/arb/ar_memiono.c
 * PURPOSE:         Memory and I/O Port Resource Arbitration
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE ArbiterInterfaceMemory =
{
    &GUID_ARBITER_INTERFACE_STANDARD,
    sizeof(ARBITER_INTERFACE),
    0,
    0,
    PCI_INTERFACE_FDO,
    0,
    PciArb_Memory,
    armem_Constructor,
    armem_Initializer
};

PCI_INTERFACE ArbiterInterfaceIo =
{
    &GUID_ARBITER_INTERFACE_STANDARD,
    sizeof(ARBITER_INTERFACE),
    0,
    0,
    PCI_INTERFACE_FDO,
    0,
    PciArb_Io,
    ario_Constructor,
    ario_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
ario_UnpackRequirement(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
                       _Out_ PULONGLONG Minimum,
                       _Out_ PULONGLONG Maximum,
                       _Out_ PULONGLONG Length,
                       _Out_ PULONGLONG Alignment)
{
    /* This arbiter is only ever handed I/O port descriptors */
    if (Descriptor->Type != CmResourceTypePort) return STATUS_INVALID_PARAMETER;

    /* An alignment of zero would place the range nowhere, so make it one byte */
    *Minimum = (ULONGLONG)Descriptor->u.Port.MinimumAddress.QuadPart;
    *Maximum = (ULONGLONG)Descriptor->u.Port.MaximumAddress.QuadPart;
    *Length = Descriptor->u.Port.Length;
    *Alignment = Descriptor->u.Port.Alignment ? Descriptor->u.Port.Alignment : 1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ario_PackResource(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
                  _In_ ULONGLONG Start,
                  _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
{
    /* Turn the placement the engine settled on back into an I/O port resource */
    if (Descriptor->Type != CmResourceTypePort) return STATUS_INVALID_PARAMETER;

    Resource->Type = CmResourceTypePort;
    Resource->Flags = Descriptor->Flags;
    Resource->ShareDisposition = Descriptor->ShareDisposition;
    Resource->u.Port.Start.QuadPart = (LONGLONG)Start;
    Resource->u.Port.Length = Descriptor->u.Port.Length;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ario_UnpackResource(_In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
                    _Out_ PULONGLONG Start,
                    _Out_ PULONGLONG Length)
{
    /* Read a previously packed I/O port resource back out */
    if (Resource->Type != CmResourceTypePort) return STATUS_INVALID_PARAMETER;

    *Start = (ULONGLONG)Resource->u.Port.Start.QuadPart;
    *Length = Resource->u.Port.Length;
    return STATUS_SUCCESS;
}

INT32
NTAPI
ario_ScoreRequirement(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONGLONG Minimum, Maximum, Length, Alignment;

    /* A requirement this arbiter cannot read is treated as unsatisfiable */
    if (!NT_SUCCESS(ario_UnpackRequirement(Descriptor,
                                           &Minimum,
                                           &Maximum,
                                           &Length,
                                           &Alignment)) ||
        (Maximum < Minimum))
    {
        return -1;
    }

    if ((Maximum - Minimum) >= MAXLONG) return MAXLONG;
    return (INT32)(Maximum - Minimum + 1);
}

NTSTATUS
NTAPI
ario_Initializer(IN PVOID Instance)
{
    PPCI_ARBITER_INSTANCE Arbiter = Instance;
    PPCI_FDO_EXTENSION FdoExtension;
    NTSTATUS Status;
    PAGED_CODE();

    /* Start from a clean engine instance */
    RtlZeroMemory(&Arbiter->CommonInstance, sizeof(Arbiter->CommonInstance));
    FdoExtension = Arbiter->BusFdoExtension;

    /* Teach the engine how to read and write I/O port descriptors */
    Arbiter->CommonInstance.UnpackRequirement = ario_UnpackRequirement;
    Arbiter->CommonInstance.PackResource = ario_PackResource;
    Arbiter->CommonInstance.UnpackResource = ario_UnpackResource;
    Arbiter->CommonInstance.ScoreRequirement = ario_ScoreRequirement;

    /* The rest of the engine, and the PCI assignment ordering, is generic */
    Status = ArbiterLibInitializeInstance(&Arbiter->CommonInstance,
                                          FdoExtension->FunctionalDeviceObject,
                                          CmResourceTypePort,
                                          Arbiter->InstanceName,
                                          L"Pci",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ario_Initializer: init arbiter returned %X\n", Status);
    }

    return Status;
}

NTSTATUS
NTAPI
ario_Constructor(IN PVOID DeviceExtension,
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

    /* This one only arbitrates I/O ports */
    if ((ULONG_PTR)InterfaceData != CmResourceTypePort)
    {
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Hand out the instance that was built for this bus */
    return PciArbiterConstructor(FdoExtension,
                                 PciArb_Io,
                                 (PARBITER_INTERFACE)Interface);
}

VOID
NTAPI
ario_ApplyBrokenVideoHack(IN PPCI_FDO_EXTENSION FdoExtension)
{
    PPCI_ARBITER_INSTANCE PciArbiter;
    PARBITER_INSTANCE CommonInstance;
    NTSTATUS Status;

    /* Only valid for root FDOs who are being applied the hack for the first time */
    ASSERT(!FdoExtension->BrokenVideoHackApplied);
    ASSERT(PCI_IS_ROOT_FDO(FdoExtension));

    /* Find the I/O arbiter */
    PciArbiter = (PVOID)PciFindNextSecondaryExtension(FdoExtension->
                                                      SecondaryExtension.Next,
                                                      PciArb_Io);
    ASSERT(PciArbiter);
    if (!PciArbiter) return;

    /* Get the Arb instance */
    CommonInstance = &PciArbiter->CommonInstance;

    /* Free the two lists, enabling full VGA access */
    ArbiterLibFreeOrderingList(&CommonInstance->OrderingList);
    ArbiterLibFreeOrderingList(&CommonInstance->ReservedList);

    /* Build the ordering for broken video PCI access */
    Status = ArbiterLibDefaultAssignmentOrdering(CommonInstance,
                                                 L"Pci",
                                                 L"BrokenVideo",
                                                 NULL);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ario_ApplyBrokenVideoHack: ordering failed %X\n", Status);
    }

    /* Now the hack has been applied */
    FdoExtension->BrokenVideoHackApplied = TRUE;
}

NTSTATUS
NTAPI
armem_UnpackRequirement(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
                        _Out_ PULONGLONG Minimum,
                        _Out_ PULONGLONG Maximum,
                        _Out_ PULONGLONG Length,
                        _Out_ PULONGLONG Alignment)
{
    /* This arbiter is only ever handed device memory descriptors */
    if (Descriptor->Type != CmResourceTypeMemory) return STATUS_INVALID_PARAMETER;

    /* An alignment of zero would place the range nowhere, so make it one byte */
    *Minimum = (ULONGLONG)Descriptor->u.Memory.MinimumAddress.QuadPart;
    *Maximum = (ULONGLONG)Descriptor->u.Memory.MaximumAddress.QuadPart;
    *Length = Descriptor->u.Memory.Length;
    *Alignment = Descriptor->u.Memory.Alignment ? Descriptor->u.Memory.Alignment : 1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
armem_PackResource(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
                   _In_ ULONGLONG Start,
                   _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
{
    /* Turn the placement the engine settled on back into a memory resource */
    if (Descriptor->Type != CmResourceTypeMemory) return STATUS_INVALID_PARAMETER;

    Resource->Type = CmResourceTypeMemory;
    Resource->Flags = Descriptor->Flags;
    Resource->ShareDisposition = Descriptor->ShareDisposition;
    Resource->u.Memory.Start.QuadPart = (LONGLONG)Start;
    Resource->u.Memory.Length = Descriptor->u.Memory.Length;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
armem_UnpackResource(_In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
                     _Out_ PULONGLONG Start,
                     _Out_ PULONGLONG Length)
{
    /* Read a previously packed memory resource back out */
    if (Resource->Type != CmResourceTypeMemory) return STATUS_INVALID_PARAMETER;

    *Start = (ULONGLONG)Resource->u.Memory.Start.QuadPart;
    *Length = Resource->u.Memory.Length;
    return STATUS_SUCCESS;
}

INT32
NTAPI
armem_ScoreRequirement(_In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONGLONG Minimum, Maximum, Length, Alignment;

    /* A requirement this arbiter cannot read is treated as unsatisfiable */
    if (!NT_SUCCESS(armem_UnpackRequirement(Descriptor,
                                            &Minimum,
                                            &Maximum,
                                            &Length,
                                            &Alignment)) ||
        (Maximum < Minimum))
    {
        return -1;
    }

    if ((Maximum - Minimum) >= MAXLONG) return MAXLONG;
    return (INT32)(Maximum - Minimum + 1);
}

NTSTATUS
NTAPI
armem_Initializer(IN PVOID Instance)
{
    PPCI_ARBITER_INSTANCE Arbiter = Instance;
    PPCI_FDO_EXTENSION FdoExtension;
    NTSTATUS Status;
    PAGED_CODE();

    /* Start from a clean engine instance */
    RtlZeroMemory(&Arbiter->CommonInstance, sizeof(Arbiter->CommonInstance));
    FdoExtension = Arbiter->BusFdoExtension;

    /* Teach the engine how to read and write memory descriptors */
    Arbiter->CommonInstance.UnpackRequirement = armem_UnpackRequirement;
    Arbiter->CommonInstance.PackResource = armem_PackResource;
    Arbiter->CommonInstance.UnpackResource = armem_UnpackResource;
    Arbiter->CommonInstance.ScoreRequirement = armem_ScoreRequirement;

    /* The rest of the engine, and the PCI assignment ordering, is generic */
    Status = ArbiterLibInitializeInstance(&Arbiter->CommonInstance,
                                          FdoExtension->FunctionalDeviceObject,
                                          CmResourceTypeMemory,
                                          Arbiter->InstanceName,
                                          L"Pci",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("armem_Initializer: init arbiter returned %X\n", Status);
    }

    return Status;
}

NTSTATUS
NTAPI
armem_Constructor(IN PVOID DeviceExtension,
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

    /* This one only arbitrates device memory */
    if ((ULONG_PTR)InterfaceData != CmResourceTypeMemory)
    {
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Hand out the instance that was built for this bus */
    return PciArbiterConstructor(FdoExtension,
                                 PciArb_Memory,
                                 (PARBITER_INTERFACE)Interface);
}

/* EOF */
