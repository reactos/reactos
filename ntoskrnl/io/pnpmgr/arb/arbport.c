/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root Port Arbiter
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootPortArbiter;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopPortMemUnpackRequirements(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT32 OutLength,
    _Out_ PUINT32 OutAlignment)
{
    PAGED_CODE();
    DPRINT("IopPortMemUnpackRequirements: IoDescriptor: %p, OutMinimumAddress: %p, OutMaximumAddress: %p, OutLength: %p, OutAlignment: %p\n",
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
IopPortMemPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();
    DPRINT("IopPortMemPackResource: IoDescriptor: %p, Start: %p, CmDescriptor: %p\n",
           IoDescriptor,
           Start,
           CmDescriptor);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopPortMemUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 OutLength)
{
    PAGED_CODE();
    DPRINT("IopPortMemUnpackResource: CmDescriptor: %p, Start: %p, OutLength: %p\n",
           CmDescriptor,
           Start,
           OutLength);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

INT32
NTAPI
IopPortMemScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();
    DPRINT("IopPortMemScoreRequirement: IoDescriptor: %p\n",
           IoDescriptor);

    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
IopArbPortInitialize(VOID)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();
    IopRootPortArbiter.Name = L"RootPort";
    IopRootPortArbiter.UnpackRequirement = IopPortMemUnpackRequirements;
    IopRootPortArbiter.PackResource = IopPortMemPackResource;
    IopRootPortArbiter.UnpackResource = IopPortMemUnpackResource;
    IopRootPortArbiter.ScoreRequirement = IopPortMemScoreRequirement;

    Status = ArbInitializeArbiterInstance(&IopRootPortArbiter,
                                          NULL,
                                          CmResourceTypeBusNumber,
                                          IopRootPortArbiter.Name,
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbDmaInitialize: Failed with %X", Status);
    }

    return Status;
}
