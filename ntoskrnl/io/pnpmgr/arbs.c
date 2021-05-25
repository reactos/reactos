/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            ntoskrnl/io/pnpmgr/arbs.c
 * PURPOSE:         Root arbiters of the PnP manager
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
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

NTSTATUS
NTAPI
IopBusNumberUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PULONGLONG OutMinimumAddress,
    _Out_ PULONGLONG OutMaximumAddress,
    _Out_ PULONG OutLength,
    _Out_ PULONG OutAlignment)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopBusNumberPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ ULONGLONG Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopBusNumberUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PULONGLONG Start,
    _Out_ PULONG Length)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

LONG
NTAPI
IopBusNumberScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}

#define ARB_MAX_BUS_NUMBER 0xFF

NTSTATUS
NTAPI
IopBusNumberInitialize(VOID)
{
    NTSTATUS Status;

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
                         (ULONGLONG)(ARB_MAX_BUS_NUMBER + 1),
                         (ULONGLONG)(-1),
                         0,
                         0,
                         NULL,
                         NULL);

    return Status;
}

/* Irq arbiter */

NTSTATUS
NTAPI
IopIrqUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PULONGLONG OutMinimumVector,
    _Out_ PULONGLONG OutMaximumVector,
    _Out_ PULONG OutParam1,
    _Out_ PULONG OutParam2)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopIrqPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ ULONGLONG Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopIrqUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PULONGLONG Start,
    _Out_ PULONG OutLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

LONG
NTAPI
IopIrqScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
IopIrqTranslateOrdering(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopIrqInitialize(VOID)
{
    NTSTATUS Status;

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

NTSTATUS
NTAPI
IopDmaUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PULONGLONG OutMinimumChannel,
    _Out_ PULONGLONG OutMaximumChannel,
    _Out_ PULONG OutParam1,
    _Out_ PULONG OutParam2)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopDmaPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ ULONGLONG Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopDmaUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PULONGLONG Start,
    _Out_ PULONG OutLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

LONG
NTAPI
IopDmaScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
IopDmaOverrideConflict()
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopDmaInitialize(VOID)
{
    NTSTATUS Status;

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

NTSTATUS
NTAPI
IopGenericUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PULONGLONG OutMinimumAddress,
    _Out_ PULONGLONG OutMaximumAddress,
    _Out_ PULONG OutLength,
    _Out_ PULONG OutAlignment)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopGenericPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ ULONGLONG Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopGenericUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PULONGLONG Start,
    _Out_ PULONG OutLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

LONG
NTAPI
IopGenericScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
IopGenericTranslateOrdering(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Memory arbiter */

BOOLEAN
NTAPI
IopMemFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE State)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
IopMemInitialize(VOID)
{
    NTSTATUS Status;

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
                         0ull,
                         (ULONGLONG)(PAGE_SIZE - 1),
                         0,
                         0,
                         NULL,
                         NULL);

    return Status;
}

/* Port arbiter */

BOOLEAN
NTAPI
IopPortFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE State)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
IopPortAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE ArbState)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
IopPortBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
IopPortInitialize(VOID)
{
    NTSTATUS Status;

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

/* EOF */
