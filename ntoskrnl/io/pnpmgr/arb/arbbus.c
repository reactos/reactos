/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root Bus Arbiter
 * COPYRIGHT:   Copyright 2025-2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootBusNumberArbiter;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopArbBusNumberUnpackRequirements(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT64 OutLength,
    _Out_ PUINT64 OutAlignment)
{
    PAGED_CODE();

    *OutMinimumAddress = IoDescriptor->u.BusNumber.MinBusNumber;
    *OutMaximumAddress = IoDescriptor->u.BusNumber.MaxBusNumber;
    *OutLength = IoDescriptor->u.BusNumber.Length;
    *OutAlignment = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopArbBusNumberPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    CmDescriptor->Type = CmResourceTypeBusNumber;
    CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;
    CmDescriptor->Flags = IoDescriptor->Flags;
    CmDescriptor->u.BusNumber.Start = (ULONG)Start;
    CmDescriptor->u.BusNumber.Length = IoDescriptor->u.BusNumber.Length;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopArbBusNumberUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT64 OutLength)
{
    PAGED_CODE();

    *Start  = CmDescriptor->u.BusNumber.Start;
    *OutLength = CmDescriptor->u.BusNumber.Length;

    return STATUS_SUCCESS;
}

INT32
NTAPI
IopArbBusNumberScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();

    ASSERT(IoDescriptor->u.BusNumber.Length != 0);
    if (IoDescriptor->u.BusNumber.Length == 0)
        return -1;

    return (IoDescriptor->u.BusNumber.MaxBusNumber - IoDescriptor->u.BusNumber.MinBusNumber)
           / IoDescriptor->u.BusNumber.Length;
}

/**
 * @brief Initialize the RootBusArbiter
 *
 * The root bus-number arbiter owns the flat 0..255 bus-number space and hands 
 * each bridge a sub-range.
 *
 * @return NTSTATUS
 * @retval STATUS_SUCCESS
 * @retval STATUS_UNSUCCESSFUL
 * @retval STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS
NTAPI
IopArbBusNumberInitialize(VOID)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();
    IopRootBusNumberArbiter.Name = L"RootBusNumber";
    IopRootBusNumberArbiter.UnpackRequirement = IopArbBusNumberUnpackRequirements;
    IopRootBusNumberArbiter.PackResource = IopArbBusNumberPackResource;
    IopRootBusNumberArbiter.UnpackResource = IopArbBusNumberUnpackResource;
    IopRootBusNumberArbiter.ScoreRequirement = IopArbBusNumberScoreRequirement;

    Status = ArbiterLibInitializeInstance(&IopRootBusNumberArbiter,
                                          NULL,
                                          CmResourceTypeBusNumber,
                                          IopRootBusNumberArbiter.Name,
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbBusNumberInitialize: Failed with %X", Status);
    }

    return Status;
}
