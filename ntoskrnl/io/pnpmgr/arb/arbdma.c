/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root DMA Arbiter
 * COPYRIGHT:   Copyright 2025-2026 Justin Miller <justin.miller@reactos.org>
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
    _Out_ PUINT64 OutLength,
    _Out_ PUINT64 OutAlignment)
{
    PAGED_CODE();

    *OutMinimumAddress = IoDescriptor->u.Dma.MinimumChannel;
    *OutMaximumAddress = IoDescriptor->u.Dma.MaximumChannel;
    *OutLength    = 1;
    *OutAlignment = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopArbDmaPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    CmDescriptor->Type = CmResourceTypeDma;
    CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;
    CmDescriptor->Flags = IoDescriptor->Flags;

    /*
     * TODO: this is only version gated temporarily,
     * we need this for USB3 eventually, I marked it for now.
     */
#if (NTDDI_VERSION >= NTDDI_WIN8)
    if (IoDescriptor->Flags & CM_RESOURCE_DMA_V3)
    {
        CmDescriptor->u.DmaV3.Channel       = IoDescriptor->u.DmaV3.Channel;
        CmDescriptor->u.DmaV3.RequestLine   = IoDescriptor->u.DmaV3.RequestLine;
        CmDescriptor->u.DmaV3.TransferWidth = (UCHAR)IoDescriptor->u.DmaV3.TransferWidth;
        return STATUS_SUCCESS;
    }
#endif

    CmDescriptor->u.Dma.Channel = (ULONG)Start;
    CmDescriptor->u.Dma.Port    = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopArbDmaUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT64 OutLength)
{
    PAGED_CODE();

    *Start = CmDescriptor->u.Dma.Channel;
    *OutLength = 1;

    return STATUS_SUCCESS;
}

INT32
NTAPI
IopArbDmaScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PAGED_CODE();
    return (INT32)(IoDescriptor->u.Dma.MaximumChannel - IoDescriptor->u.Dma.MinimumChannel);
}

/**
 * @brief
 * The Root DMA arbiter's OverrideConflict: refuses every conflict.
 *
 * @param[in] Arbiter
 * The Root DMA arbiter instance.
 *
 * @param[in,out] ArbState
 * The allocation state of the requirement that could not be placed.
 *
 * @return
 * Returns FALSE, always.
 **/
static
BOOLEAN
NTAPI
IopArbDmaOverrideConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Arbiter);
    UNREFERENCED_PARAMETER(ArbState);

    return FALSE;
}

/**
 * @brief Initialize the RootDmaArbiter.
 *
 * @return NTSTATUS
 * @retval STATUS_SUCCESS
 * @retval STATUS_UNSUCCESSFUL
 * @retval STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS
NTAPI
IopArbDmaInitialize(VOID)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();
    IopRootDmaArbiter.Name = L"RootDMA";
    IopRootDmaArbiter.UnpackRequirement = IopArbDmaUnpackRequirements;
    IopRootDmaArbiter.PackResource = IopArbDmaPackResource;
    IopRootDmaArbiter.UnpackResource = IopArbDmaUnpackResource;
    IopRootDmaArbiter.ScoreRequirement = IopArbDmaScoreRequirement;
    IopRootDmaArbiter.OverrideConflict = IopArbDmaOverrideConflict;

    Status = ArbiterLibInitializeInstance(&IopRootDmaArbiter,
                                          NULL,
                                          CmResourceTypeDma,
                                          IopRootDmaArbiter.Name,
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbDmaInitialize: Failed with %X\n", Status);
    }

    return Status;
}
