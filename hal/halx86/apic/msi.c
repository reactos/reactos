/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Message-signaled interrupt support for the APIC HAL
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "apicp.h"
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* Defined in generic/misc.c */
extern HALP_APIC_INFO_TABLE HalpApicInfoTable;

/* Resolved once at phase 0, see HalpInitializeMessageInterrupts */
static BOOLEAN HalpMessageInterruptsEnabled;

/* A block of message vectors has to fit in a single priority row */
#define HALP_MAX_MESSAGE_VECTORS 16

/* Size of each allocated block, recorded at its base vector */
static UCHAR HalpMessageBlockSize[256];

/* PRIVATE FUNCTIONS **********************************************************/

/* Intel family 15 or family 6 model 13 and later, AMD family 15 and later */
static
BOOLEAN
NTAPI
HalpCpuSupportsMessageInterrupts(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    BOOLEAN IsIntel, IsAmd;

    IsIntel = (strncmp((PCSTR)Prcb->VendorString, "GenuineIntel", 12) == 0);
    IsAmd = (strncmp((PCSTR)Prcb->VendorString, "AuthenticAMD", 12) == 0);
    if (!IsIntel && !IsAmd)
    {
        return FALSE;
    }

    if (Prcb->CpuType >= 15)
    {
        return TRUE;
    }

    /* Pentium M and the family 6 models that came after it */
    return IsIntel && (Prcb->CpuType == 6) && ((Prcb->CpuStep >> 8) >= 0x0D);
}

/* Flat logical mode holds one bit per processor, so it only covers 8 of them */
static
HAL_APIC_DESTINATION_MODE
NTAPI
HalpGetApicDestinationMode(VOID)
{
    ULONG Count = max(HalpApicInfoTable.ProcessorCount, (ULONG)KeNumberProcessors);

    return (Count > 8) ? ApicDestinationModePhysical : ApicDestinationModeLogicalFlat;
}

static
NTSTATUS
NTAPI
HalpGetLocalApicIdForProcessor(
    _In_ ULONG ProcessorNumber,
    _Out_ PULONG ApicId)
{
    if (ProcessorNumber == KeGetCurrentProcessorNumber())
    {
        *ApicId = ApicRead(APIC_ID) >> 24;
        return STATUS_SUCCESS;
    }

    if ((ProcessorNumber >= HalpApicInfoTable.ProcessorCount) ||
        (ProcessorNumber >= MAXIMUM_PROCESSORS))
    {
        return STATUS_NOT_FOUND;
    }

    *ApicId = HalpProcessorIdentity[ProcessorNumber].LapicId;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HalpGetProcessorForLocalApicId(
    _In_ ULONG ApicId,
    _Out_ PULONG ProcessorNumber)
{
    ULONG Index;

    if (ApicId == (ApicRead(APIC_ID) >> 24))
    {
        *ProcessorNumber = KeGetCurrentProcessorNumber();
        return STATUS_SUCCESS;
    }

    for (Index = 0; Index < HalpApicInfoTable.ProcessorCount; Index++)
    {
        if (HalpProcessorIdentity[Index].LapicId == ApicId)
        {
            *ProcessorNumber = Index;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

/**
 * @brief
 * Turns a processor set into an APIC destination. A lone processor is named by
 * its physical APIC ID, a set of them by the flat logical IDs of its members.
 *
 * @param[in] TargetProcessors
 * Processors the interrupt should reach.
 *
 * @param[out] Logical
 * Receives TRUE when the destination holds logical IDs.
 *
 * @param[out] Destination
 * Receives the destination field of a redirection entry or message address.
 */
NTSTATUS
NTAPI
HalpBuildInterruptDestination(
    _In_ KAFFINITY TargetProcessors,
    _Out_ PBOOLEAN Logical,
    _Out_ PUCHAR Destination)
{
    ULONG Processor, ApicId;
    NTSTATUS Status;

    if (TargetProcessors == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((TargetProcessors & (TargetProcessors - 1)) == 0)
    {
        BitScanForwardAffinity(&Processor, TargetProcessors);
        Status = HalpGetLocalApicIdForProcessor(Processor, &ApicId);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        *Logical = FALSE;
        *Destination = (UCHAR)ApicId;
        return STATUS_SUCCESS;
    }

    /* Bit N of a flat logical ID belongs to processor N */
    if ((HalpGetApicDestinationMode() != ApicDestinationModeLogicalFlat) ||
        (TargetProcessors > 0xFF))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *Logical = TRUE;
    *Destination = (UCHAR)TargetProcessors;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Reserves a run of vectors for message-signaled interrupts. The run starts on
 * a multiple of its own size, so a device can add the message number to the
 * base vector and land on one of them.
 *
 * @param[in] Count
 * Number of vectors wanted.
 *
 * @param[out] BaseVector
 * Receives the first vector of the run.
 */
static
NTSTATUS
NTAPI
HalpAllocateMessageVectors(
    _In_ ULONG Count,
    _Out_ PUCHAR BaseVector)
{
    KIRQL OldIrql, Irql;
    ULONG Alignment, Row, LastRow, Base, Index;

    if ((Count == 0) || (Count > HALP_MAX_MESSAGE_VECTORS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* A device enabled for Count messages decodes a power of two of them, so
       the whole rounded block is reserved */
    for (Alignment = 1; Alignment < Count; Alignment <<= 1)
    {
        NOTHING;
    }

    OldIrql = HalpAcquireVectorLock();

    LastRow = 0;
    for (Irql = CLOCK_LEVEL - 1; Irql >= CMCI_LEVEL; Irql--)
    {
        /* Profile level is not a device level */
        if (IrqlToTpr(Irql) >= IrqlToTpr(PROFILE_LEVEL))
            continue;

        /* Several IRQLs can share a row, so walk each row only once */
        Row = IrqlToTpr(Irql) & 0xF0;
        if (Row == LastRow)
        {
            continue;
        }
        LastRow = Row;

        for (Base = Row; Base + Alignment <= Row + 16; Base += Alignment)
        {
            /* Vectors granted to the ACPI driver are its to hand out */
            for (Index = 0; Index < Alignment; Index++)
            {
                if ((HalpVectorToIndex[Base + Index] != APIC_FREE_VECTOR) ||
                    HalpIsVectorGranted(Base + Index))
                {
                    break;
                }
            }

            if (Index != Alignment)
            {
                continue;
            }

            for (Index = 0; Index < Alignment; Index++)
            {
                HalpVectorToIndex[Base + Index] = APIC_MSI_VECTOR;
            }
            HalpMessageBlockSize[Base] = (UCHAR)Alignment;

            HalpReleaseVectorLock(OldIrql);
            *BaseVector = (UCHAR)Base;
            return STATUS_SUCCESS;
        }
    }

    HalpReleaseVectorLock(OldIrql);
    DPRINT1("No room for %lu message vector(s)\n", Count);
    return STATUS_INSUFFICIENT_RESOURCES;
}

/* Frees a message mark that no HAL block owns. The caller holds the vector lock. */
VOID
NTAPI
HalpReleaseMessageMark(
    _In_ ULONG Vector)
{
    ULONG Base;

    if ((Vector > 0xFF) || (HalpVectorToIndex[Vector] != APIC_MSI_VECTOR))
        return;

    for (Base = Vector & 0xF0; Base <= Vector; Base++)
    {
        if ((HalpMessageBlockSize[Base] != 0) && (Vector < Base + HalpMessageBlockSize[Base]))
            return;
    }

    HalpVectorToIndex[Vector] = APIC_FREE_VECTOR;
}

/* Frees the whole block holding Vector, a block never crosses a priority row */
static
VOID
NTAPI
HalpFreeMessageVectors(
    _In_ UCHAR Vector)
{
    KIRQL OldIrql;
    ULONG Base, Size, Index;

    OldIrql = HalpAcquireVectorLock();

    for (Base = Vector & 0xF0; Base <= Vector; Base++)
    {
        Size = HalpMessageBlockSize[Base];
        if ((Size != 0) && (Vector < Base + Size))
        {
            for (Index = 0; Index < Size; Index++)
            {
                HalpVectorToIndex[Base + Index] = APIC_FREE_VECTOR;
            }
            HalpMessageBlockSize[Base] = 0;
            break;
        }
    }

    HalpReleaseVectorLock(OldIrql);
}

/**
 * @brief
 * Private dispatch routine handing a device its message vectors. A vector is
 * good on every processor, so the processor set is only tested for members.
 */
static
NTSTATUS
NTAPI
HalpAllocateMessageTarget(
    _In_ PDEVICE_OBJECT Owner,
    _In_ KAFFINITY ProcessorSet,
    _In_ ULONG NumberOfIdtEntries,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN ShareVector,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql,
    _Out_ PULONG IdtEntry)
{
    UCHAR Base;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Owner);
    UNREFERENCED_PARAMETER(Mode);
    UNREFERENCED_PARAMETER(ShareVector);

    if ((ProcessorSet == 0) || (NumberOfIdtEntries == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!HalpMessageInterruptsEnabled)
    {
        return STATUS_NOT_SUPPORTED;
    }

    Status = HalpAllocateMessageVectors(NumberOfIdtEntries, &Base);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *Vector = Base;
    *IdtEntry = Base;
    *Irql = HalpVectorToIrql(Base);
    return STATUS_SUCCESS;
}

/* Private dispatch routine giving back the block that holds a message vector */
static
VOID
NTAPI
HalpFreeMessageTarget(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Vector,
    _In_ KAFFINITY ProcessorSet)
{
    UNREFERENCED_PARAMETER(Owner);
    UNREFERENCED_PARAMETER(ProcessorSet);

    if (Vector <= 0xFF)
    {
        HalpFreeMessageVectors((UCHAR)Vector);
    }
}

VOID
NTAPI
HalpInitializeMessageInterrupts(VOID)
{
    HalpMessageInterruptsEnabled = HalpCpuSupportsMessageInterrupts();

    /* Leave the routines the ACPI driver installed alone */
    if (HalAllocateMessageTargetOverride == NULL)
    {
        HalAllocateMessageTargetOverride = HalpAllocateMessageTarget;
        HalFreeMessageTargetOverride = HalpFreeMessageTarget;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
HalGetProcessorIdByNtNumber(
    _In_ ULONG ProcessorNumber,
    _Out_ PULONG ProcessorId)
{
    if (ProcessorNumber >= (ULONG)KeNumberProcessors)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return HalpGetLocalApicIdForProcessor(ProcessorNumber, ProcessorId);
}

/**
 * @brief
 * Reports how interrupts can be aimed, either for the machine as a whole or
 * for the processor owning a local APIC ID.
 *
 * @param[in] Type
 * TargetGlobal or TargetApic.
 *
 * @param[in] Id
 * Local APIC ID of the processor, read for TargetApic only.
 *
 * @param[out] Information
 * Receives the targeting information.
 */
NTSTATUS
NTAPI
HalGetInterruptTargetInformation(
    _In_ INTERRUPT_TARGET_TYPE Type,
    _In_ ULONG Id,
    _Out_ PHAL_INTERRUPT_TARGET_DESCRIPTOR Information)
{
    HAL_APIC_DESTINATION_MODE Mode;
    ULONG Processor;
    NTSTATUS Status;

#if (NTDDI_VERSION >= NTDDI_WIN7)
    if ((Type != TargetGlobal) && (Type != TargetApic))
#else
    if (Type != TargetApic)
#endif
    {
        return STATUS_INVALID_PARAMETER;
    }

    Mode = HalpGetApicDestinationMode();

    RtlZeroMemory(Information, sizeof(*Information));
    Information->TargetType = Type;
    Information->ApicRouting.DestinationFormat = Mode;

    /* Processors never leave, so a destination stays good once it is built */
    Information->Capabilities = HAL_TARGET_FIXED_DESTINATIONS;
    if (HalpMessageInterruptsEnabled)
    {
        Information->Capabilities |= HAL_TARGET_MSI_CAPABLE;
    }

    if (Type == TargetApic)
    {
        Status = HalpGetProcessorForLocalApicId(Id, &Processor);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

#if (NTDDI_VERSION >= NTDDI_WIN7)
        Information->Processor.Number = (UCHAR)Processor;
#else
        Information->ProcessorIndex = Processor;
#endif

        if ((Mode == ApicDestinationModeLogicalFlat) && (Processor < 8))
        {
            Information->Capabilities |= HAL_TARGET_LOGICAL_DESTINATION_VALID;
            Information->ApicRouting.LogicalDestination = ApicLogicalId(Processor);
        }
        else
        {
            Information->ApicRouting.DestinationFormat = ApicDestinationModePhysical;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Builds the address and data a device writes to raise a vector on a processor
 * set. TargetApicRequest only checks the request over and hands it
 * back for the caller to finish later.
 *
 * @param[in] Request
 * The vector, the processor set and the destination mode wanted.
 *
 * @param[out] ConnectionData
 * Receives a connection data block holding the single vector.
 */
NTSTATUS
NTAPI
HalGetMessageRoutingInfo(
    _In_ PHAL_MESSAGE_SIGNAL_TARGET_REQUEST Request,
    _Out_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    PINTERRUPT_VECTOR_DATA VectorData;
    HAL_APIC_DESTINATION_MODE Mode;
    KAFFINITY Targets;
    BOOLEAN Logical, MultiTarget;
    UCHAR Destination;
    ULONG Address, Data;
    NTSTATUS Status;

    if ((Request->InterruptTargetType != TargetApic) &&
        (Request->InterruptTargetType != TargetApicRequest))
    {
        return STATUS_INVALID_PARAMETER;
    }

#if (NTDDI_VERSION >= NTDDI_WIN7)
    if (Request->ApicTarget.TargetProcessors.Group != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Targets = Request->ApicTarget.TargetProcessors.Mask;
#else
    Targets = Request->ApicTarget.TargetProcessors;
#endif

    if ((Request->ApicTarget.InterruptVector > 0xFF) || (Targets == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    MultiTarget = ((Targets & (Targets - 1)) != 0);
    Mode = Request->ApicTarget.ApicDestinationMode;

    if (Mode == ApicDestinationModeLogicalFlat)
    {
        /* Flat mode names even a lone processor by its logical ID */
        if ((HalpGetApicDestinationMode() != ApicDestinationModeLogicalFlat) ||
            (Targets > 0xFF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Logical = TRUE;
        Destination = (UCHAR)Targets;
    }
    else if (Mode == ApicDestinationModePhysical)
    {
        if (MultiTarget)
        {
            return STATUS_INVALID_PARAMETER;
        }

        Status = HalpBuildInterruptDestination(Targets, &Logical, &Destination);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(ConnectionData, FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors[1]));
    ConnectionData->Count = 1;

    VectorData = &ConnectionData->Vectors[0];
    VectorData->Vector = Request->ApicTarget.InterruptVector;
    VectorData->Irql = HalpVectorToIrql((UCHAR)Request->ApicTarget.InterruptVector);
    VectorData->Polarity = InterruptActiveHigh;
    VectorData->Mode = Latched;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    VectorData->TargetProcessors.Mask = Targets;
#else
    VectorData->TargetProcessors = Targets;
#endif

    if (Request->InterruptTargetType == TargetApicRequest)
    {
        VectorData->Type = InterruptTypeMessageRequest;
        VectorData->MessageRequest.DestinationMode = Mode;
        return STATUS_SUCCESS;
    }

    Address = APIC_MSI_ADDRESS_BASE | ((ULONG)Destination << 12);
    Data = Request->ApicTarget.InterruptVector | APIC_MSI_DATA_ASSERT;
    if (Logical)
    {
        Address |= APIC_MSI_ADDRESS_LOGICAL;
        Data |= APIC_MSI_DATA_LOGICAL;

        /* Let the hardware pick the least busy processor of the set */
        if (MultiTarget)
        {
            Address |= APIC_MSI_ADDRESS_REDIRHINT;
            Data |= APIC_MSI_DATA_LOWEST_PRIORITY;
        }
    }

    VectorData->Type = InterruptTypeXapicMessage;
    VectorData->XapicMessage.Address.QuadPart = Address;
    VectorData->XapicMessage.DataPayload = Data;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    VectorData->IntRemapInfo.FlagTranslated = 1;
    VectorData->IntRemapInfo.u.Msi.MessageAddressLow = Address;
    VectorData->IntRemapInfo.u.Msi.MessageData = (USHORT)Data;
#endif

    return STATUS_SUCCESS;
}

/* EOF */
