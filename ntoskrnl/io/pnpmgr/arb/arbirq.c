/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root IRQ Arbiter
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootIrqArbiter;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopArbIrqUnpackRequirements(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT32 OutLength,
    _Out_ PUINT32 OutAlignment)
{
    PAGED_CODE();

    *OutMinimumAddress = IoDescriptor->u.Interrupt.MinimumVector;
    *OutMaximumAddress = IoDescriptor->u.Interrupt.MaximumVector;
    *OutLength = 1;
    *OutAlignment = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopArbIrqPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    CmDescriptor->Type = CmResourceTypeInterrupt;
    CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;
    CmDescriptor->Flags = IoDescriptor->Flags;

    CmDescriptor->u.Interrupt.Level = (ULONG)Start;
    CmDescriptor->u.Interrupt.Vector = (ULONG)Start;
    CmDescriptor->u.Interrupt.Affinity = (KAFFINITY)-1;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopArbIrqUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 OutLength)
{
    PAGED_CODE();

    *Start = CmDescriptor->u.Interrupt.Vector;
    *OutLength = 1;

    return STATUS_SUCCESS;
}

INT32
NTAPI
IopArbIrqScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    return (INT32)(IoDescriptor->u.Interrupt.MaximumVector -
                   IoDescriptor->u.Interrupt.MinimumVector + 1);
}

NTSTATUS
NTAPI
IopArbIrqTranslateOrdering(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    KIRQL Irql;
    KAFFINITY Affinity = 0;

    PAGED_CODE();

    *OutIoDescriptor = *IoDescriptor;

    if (IoDescriptor->Type != CmResourceTypeInterrupt)
        return STATUS_SUCCESS;

    OutIoDescriptor->u.Interrupt.MinimumVector =
        HalGetInterruptVector(Isa,
                              0,
                              IoDescriptor->u.Interrupt.MinimumVector,
                              IoDescriptor->u.Interrupt.MinimumVector,
                              &Irql,
                              &Affinity);

    if (Affinity != 0)
    {
        Affinity = 0;
        OutIoDescriptor->u.Interrupt.MaximumVector =
            HalGetInterruptVector(Isa,
                                  0,
                                  IoDescriptor->u.Interrupt.MaximumVector,
                                  IoDescriptor->u.Interrupt.MaximumVector,
                                  &Irql,
                                  &Affinity);
    }

    if (Affinity == 0)
        *OutIoDescriptor = *IoDescriptor;

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize the root IRQ arbiter.
 *
 * This is the fallback that sits at the top of the tree and takes
 * over when no closer arbiter claims the request, e.g
 * root-enumerated / HAL legacy devices (ISA-style, directly under the root),
 * and platforms or boot paths where no bus driver above the device provides an
 * interrupt arbiter (e.g. no ACPI _PRT routing in effect).
 *
 * In those cases the arbitrated "vectors" are the legacy ISA IRQ lines, which is
 * why IopArbIrqTranslateOrdering maps the ordering table through
 * HalGetInterruptVector for the ISA bus. Otherwise, such devices
 * would have nowhere to arbitrate their interrupts. 
 * (So this is purely for legacy PIC + No ACPI)
 *
 * @return NTSTATUS
 * @retval STATUS_SUCCESS
 * @retval STATUS_UNSUCCESSFUL
 * @retval STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS
NTAPI
IopArbIrqInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    IopRootIrqArbiter.Name = L"RootIRQ";
    IopRootIrqArbiter.UnpackRequirement = IopArbIrqUnpackRequirements;
    IopRootIrqArbiter.PackResource = IopArbIrqPackResource;
    IopRootIrqArbiter.UnpackResource = IopArbIrqUnpackResource;
    IopRootIrqArbiter.ScoreRequirement = IopArbIrqScoreRequirement;

    Status = ArbInitializeArbiterInstance(&IopRootIrqArbiter,
                                          NULL,
                                          CmResourceTypeInterrupt,
                                          IopRootIrqArbiter.Name,
                                          L"Root",
                                          IopArbIrqTranslateOrdering);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbIrqInitialize: Failed with %X\n", Status);
    }

    return Status;
}
