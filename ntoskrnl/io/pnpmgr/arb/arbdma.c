/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root DMA Arbiter
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootDmaArbiter;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopArbDmaUnpackRequirements(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT32 OutLength,
    _Out_ PUINT32 OutAlignment)
{
    PAGED_CODE();
    DPRINT("IopArbDmaUnpackRequirements: IoDescriptor: %p, OutMinimumAddress: %p, OutMaximumAddress: %p, OutLength: %p, OutAlignment: %p\n",
           IoDescriptor,
           OutMinimumAddress,
           OutMaximumAddress,
           OutLength,
           OutAlignment);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopArbDmaPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();
    DPRINT("IopArbDmaPackResource: IoDescriptor: %p, Start: %p, CmDescriptor: %p\n",
           IoDescriptor,
           Start,
           CmDescriptor);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopArbDmaUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 OutLength)
{
    PAGED_CODE();
    DPRINT("IopArbDmaUnpackResource: CmDescriptor: %p, Start: %p, OutLength: %p\n",
           CmDescriptor,
           Start,
           OutLength);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

INT32
NTAPI
IopArbDmaScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();
    DPRINT("IopArbDmaScoreRequirement: IoDescriptor: %p\n",
           IoDescriptor);

    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
IopArbDmaInitialize(VOID)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();
    IopRootDmaArbiter.Name = L"RootDma";
    IopRootDmaArbiter.UnpackRequirement = IopArbDmaUnpackRequirements;
    IopRootDmaArbiter.PackResource = IopArbDmaPackResource;
    IopRootDmaArbiter.UnpackResource = IopArbDmaUnpackResource;
    IopRootDmaArbiter.ScoreRequirement = IopArbDmaScoreRequirement;

    Status = ArbInitializeArbiterInstance(&IopRootDmaArbiter,
                                          NULL,
                                          CmResourceTypeBusNumber,
                                          IopRootDmaArbiter.Name,
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbDmaInitialize: Failed with %X", Status);
    }

    return Status;
}
