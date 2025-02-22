/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Root arbiters of the PnP manager
 * COPYRIGHT:   Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootBusNumberArbiter;
extern ARBITER_INSTANCE IopRootIrqArbiter;
extern ARBITER_INSTANCE IopRootDmaArbiter;
extern ARBITER_INSTANCE IopRootMemArbiter;
extern ARBITER_INSTANCE IopRootPortArbiter;

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

/* BusNumber arbiter */

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopBusNumberUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT32 OutLength,
    _Out_ PUINT32 OutAlignment)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopBusNumberPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopBusNumberUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 Length)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
INT32
NTAPI
IopBusNumberScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return 0;
}

#define ARB_MAX_BUS_NUMBER 0xFF

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopBusNumberInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("IopRootBusNumberArbiter %p\n", &IopRootBusNumberArbiter);

    IopRootBusNumberArbiter.UnpackRequirement = IopBusNumberUnpackRequirement;
    IopRootBusNumberArbiter.PackResource = IopBusNumberPackResource;
    IopRootBusNumberArbiter.UnpackResource = IopBusNumberUnpackResource;
    IopRootBusNumberArbiter.ScoreRequirement = IopBusNumberScoreRequirement;

    Status = ArbInitializeArbiterInstance(&IopRootBusNumberArbiter,
                                          NULL,
                                          CmResourceTypeBusNumber,
                                          L"RootBusNumber",
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBusNumberInitialize: Status %p\n", Status);
        ASSERT(FALSE);
        return Status;
    }

    Status = RtlAddRange(IopRootBusNumberArbiter.Allocation,
                         (UINT64)(ARB_MAX_BUS_NUMBER + 1),
                         (UINT64)(-1),
                         0,
                         0,
                         NULL,
                         NULL);

    return Status;
}

/* Irq arbiter */

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopIrqUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumVector,
    _Out_ PUINT64 OutMaximumVector,
    _Out_ PUINT32 OutParam1,
    _Out_ PUINT32 OutParam2)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopIrqPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopIrqUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 OutLength)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
INT32
NTAPI
IopIrqScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return 0;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopIrqTranslateOrdering(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopIrqInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("IopRootIrqArbiter %p\n", &IopRootIrqArbiter);

    IopRootIrqArbiter.UnpackRequirement = IopIrqUnpackRequirement;
    IopRootIrqArbiter.PackResource = IopIrqPackResource;
    IopRootIrqArbiter.UnpackResource = IopIrqUnpackResource;
    IopRootIrqArbiter.ScoreRequirement = IopIrqScoreRequirement;

    Status = ArbInitializeArbiterInstance(&IopRootIrqArbiter,
                                          NULL,
                                          CmResourceTypeInterrupt,
                                          L"RootIRQ",
                                          L"Root",
                                          IopIrqTranslateOrdering);
    return Status;
}

/* Dma arbiter */

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopDmaUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumChannel,
    _Out_ PUINT64 OutMaximumChannel,
    _Out_ PUINT32 OutParam1,
    _Out_ PUINT32 OutParam2)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopDmaPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopDmaUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 OutLength)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
INT32
NTAPI
IopDmaScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return 0;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopDmaOverrideConflict(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopDmaInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("IopRootDmaArbiter %p\n", &IopRootDmaArbiter);

    IopRootDmaArbiter.UnpackRequirement = IopDmaUnpackRequirement;
    IopRootDmaArbiter.PackResource = IopDmaPackResource;
    IopRootDmaArbiter.UnpackResource = IopDmaUnpackResource;
    IopRootDmaArbiter.ScoreRequirement = IopDmaScoreRequirement;

    IopRootDmaArbiter.OverrideConflict = IopDmaOverrideConflict;

    Status = ArbInitializeArbiterInstance(&IopRootDmaArbiter,
                                          NULL,
                                          CmResourceTypeDma,
                                          L"RootDMA",
                                          L"Root",
                                          NULL);
    return Status;
}

/* Common for Memory and Port arbiters */

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopGenericUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT32 OutLength,
    _Out_ PUINT32 OutAlignment)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopGenericPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopGenericUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT32 OutLength)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
INT32
NTAPI
IopGenericScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return 0;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopGenericTranslateOrdering(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Memory arbiter */

CODE_SEG("PAGE")
BOOLEAN
NTAPI
IopMemFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE State)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return FALSE;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopMemInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("IopRootMemArbiter %p\n", &IopRootMemArbiter);

    IopRootMemArbiter.UnpackRequirement = IopGenericUnpackRequirement;
    IopRootMemArbiter.PackResource = IopGenericPackResource;
    IopRootMemArbiter.UnpackResource = IopGenericUnpackResource;
    IopRootMemArbiter.ScoreRequirement = IopGenericScoreRequirement;

    IopRootMemArbiter.FindSuitableRange = IopMemFindSuitableRange;

    Status = ArbInitializeArbiterInstance(&IopRootMemArbiter,
                                          NULL,
                                          CmResourceTypeMemory,
                                          L"RootMemory",
                                          L"Root",
                                          IopGenericTranslateOrdering);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopMemInitialize: Status %p\n", Status);
        ASSERT(FALSE);
        return Status;
    }

    Status = RtlAddRange(IopRootMemArbiter.Allocation,
                         0,
                         (UINT64)(PAGE_SIZE - 1),
                         0,
                         0,
                         NULL,
                         NULL);

    return Status;
}

/* Port arbiter */

CODE_SEG("PAGE")
BOOLEAN
NTAPI
IopPortFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE State)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return FALSE;
}

CODE_SEG("PAGE")
VOID
NTAPI
IopPortAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
VOID
NTAPI
IopPortBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IopPortInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("IopRootPortArbiter %p\n", &IopRootPortArbiter);

    IopRootPortArbiter.UnpackRequirement = IopGenericUnpackRequirement;
    IopRootPortArbiter.PackResource = IopGenericPackResource;
    IopRootPortArbiter.UnpackResource = IopGenericUnpackResource;
    IopRootPortArbiter.ScoreRequirement = IopGenericScoreRequirement;

    IopRootPortArbiter.FindSuitableRange = IopPortFindSuitableRange;
    IopRootPortArbiter.AddAllocation = IopPortAddAllocation;
    IopRootPortArbiter.BacktrackAllocation = IopPortBacktrackAllocation;

    Status = ArbInitializeArbiterInstance(&IopRootPortArbiter,
                                          NULL,
                                          CmResourceTypePort,
                                          L"RootPort",
                                          L"Root",
                                          IopGenericTranslateOrdering);
    return Status;
}
