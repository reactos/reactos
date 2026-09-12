/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Legacy HAL IRQ Arbiter
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <hal.h>
#include <arbiter.h>

#define NDEBUG
#include <debug.h>

#define HALP_PIC_CASCADE_IRQ 2

ARBITER_INSTANCE LegacyPCArbiter;

NTSTATUS
NTAPI
HalpLegacyPCArbUnpackRequirement(_In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
                                 _Out_ PUINT64 OutMinimumAddress,
                                 _Out_ PUINT64 OutMaximumAddress,
                                 _Out_ PUINT64 OutLength,
                                 _Out_ PUINT64 OutAlignment)
{
    PAGED_CODE();

    *OutMinimumAddress = (UINT64)IoDescriptor->u.Interrupt.MinimumVector;
    *OutMaximumAddress = (UINT64)IoDescriptor->u.Interrupt.MaximumVector;
    *OutLength = 1;
    *OutAlignment = 1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HalpLegacyPCArbPackResource(_In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
                            _In_ UINT64 Start,
                            _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    CmDescriptor->Type = CmResourceTypeInterrupt;
    CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;
    CmDescriptor->Flags = IoDescriptor->Flags;
    CmDescriptor->u.Interrupt.Level = (ULONG)Start;
    CmDescriptor->u.Interrupt.Vector = (ULONG)Start;
    CmDescriptor->u.Interrupt.Affinity = (ULONG)-1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HalpLegacyPCArbUnpackResource(_In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
                              _Out_ PUINT64 Start,
                              _Out_ PUINT64 OutLength)
{
    PAGED_CODE();

    *Start = CmDescriptor->u.Interrupt.Vector;
    *OutLength = 1;
    return STATUS_SUCCESS;
}

INT32
NTAPI
HalpLegacyPCArbScoreRequirement(_In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    INT32 ScoreReg;

    PAGED_CODE();

    ScoreReg = (INT32)(IoDescriptor->u.Interrupt.MaximumVector - IoDescriptor->u.Interrupt.MinimumVector + 1);

    return (ScoreReg < 0) ? MAXLONG : ScoreReg;
}

VOID
NTAPI
HalpLegacyPCArbReference(PVOID Context)
{
    NOTHING;
}

VOID
NTAPI
HalpLegacyPCArbDereference(PVOID Context)
{
    NOTHING;
}

/**
 * @brief
 * Initialize the legacy PC arbiter that uses PIR for PIC IRQ assignmetn.
 *
 * @param[in] BusFdo
 * Hal Bus FDO deviceobject
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyPCCreateArbiter(_In_ PDEVICE_OBJECT BusFdo)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (LegacyPCArbiter.MutexEvent)
        return STATUS_SUCCESS;

    LegacyPCArbiter.UnpackRequirement = HalpLegacyPCArbUnpackRequirement;
    LegacyPCArbiter.PackResource = HalpLegacyPCArbPackResource;
    LegacyPCArbiter.UnpackResource = HalpLegacyPCArbUnpackResource;
    LegacyPCArbiter.ScoreRequirement = HalpLegacyPCArbScoreRequirement;

    /*
     * TODO: Later we'll override more when we implement PIR.
     * The most important piece for now here is the clamp.
     * This will let us boot for now, preserving orignal reactos behavior.
     * (Plug and Pray)
     */

    Status = ArbiterLibInitializeInstance(&LegacyPCArbiter,
                                          BusFdo,
                                          CmResourceTypeInterrupt,
                                          L"HalIRQ",
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        LegacyPCArbiter.MutexEvent = NULL;
        return Status;
    }

    /* Lock to the 0-15 the PIR table only understands. */
    RtlAddRange(LegacyPCArbiter.Allocation, 16, MAXULONG, 0,
                RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);
    RtlAddRange(LegacyPCArbiter.Allocation, HALP_PIC_CASCADE_IRQ, HALP_PIC_CASCADE_IRQ,
                0, RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);

    return Status;
}

/**
 * @brief
 * Query the legacy PC Arbiter
 *
 * @param[out] Interface
 * Caller-supplied buffer that receives the ARBITER_INTERFACE.
 *
 * @param[in] Size
 * Size of the Interface buffer, in bytes.
 *
 * @param[out] Length
 * Receives sizeof(ARBITER_INTERFACE), whether or not the buffer was large
 * enough.
 *
 * @return
 * STATUS_SUCCESS on success, or STATUS_BUFFER_TOO_SMALL if Size is smaller
 * than sizeof(ARBITER_INTERFACE).
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyPCQueryArbInterface(_Out_writes_bytes_(Size) PVOID Interface,
                              _In_                     ULONG Size,
                              _Out_                    PULONG Length)
{
    PARBITER_INTERFACE ArbiterInterface = Interface;

    PAGED_CODE();

    *Length = sizeof(ARBITER_INTERFACE);
    if (Size < sizeof(ARBITER_INTERFACE))
        return STATUS_BUFFER_TOO_SMALL;

    ArbiterInterface->Size = sizeof(ARBITER_INTERFACE);
    ArbiterInterface->Version = 1;
    ArbiterInterface->Context = &LegacyPCArbiter;
    ArbiterInterface->ArbiterHandler = ArbiterLibHandler;
    ArbiterInterface->Flags = 0;

    ArbiterInterface->InterfaceDereference = HalpLegacyPCArbDereference;
    ArbiterInterface->InterfaceReference = HalpLegacyPCArbReference;

    return STATUS_SUCCESS;
}
