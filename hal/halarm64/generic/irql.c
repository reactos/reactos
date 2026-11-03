/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IRQL and software interrupt management
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/*
 * The IRQL lives in the PCR and the GIC priority mask follows it, so only
 * interrupts above the current IRQL reach the processor. The software
 * interrupts are SGIs numbered like their IRQL, lowering the IRQL lets the
 * pending ones in on their own.
 */

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalpSetPriorityMask(
    _In_ KIRQL Irql)
{
    KeArm64WriteRegister(ARM64_ICC_PMR_EL1, HalpIrqlToPriority(Irql));
    __dsb(_ARM64_BARRIER_SY);
    __isb(_ARM64_BARRIER_SY);
}

/* Updates the PCR and the mask together so an interrupt never sees them disagree */
static
VOID
HalpChangeIrql(
    _In_ KIRQL NewIrql)
{
    BOOLEAN Enabled;

    Enabled = KeDisableInterrupts();
    KeGetPcr()->CurrentIrql = NewIrql;
    HalpSetPriorityMask(NewIrql);
    KeRestoreInterrupts(Enabled);
}

#undef KeGetCurrentIrql
KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
    return KeGetPcr()->CurrentIrql;
}

KIRQL
FASTCALL
KfRaiseIrql(
    _In_ KIRQL NewIrql)
{
    KIRQL OldIrql;

    OldIrql = KeGetPcr()->CurrentIrql;
    if (NewIrql < OldIrql)
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL, OldIrql, NewIrql, 0, 9);

    HalpChangeIrql(NewIrql);
    return OldIrql;
}

VOID
FASTCALL
KfLowerIrql(
    _In_ KIRQL NewIrql)
{
    if (NewIrql > KeGetPcr()->CurrentIrql)
        KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL, NewIrql, KeGetPcr()->CurrentIrql, 0, 9);

    HalpChangeIrql(NewIrql);
}

VOID
FASTCALL
HalRequestSoftwareInterrupt(
    _In_ KIRQL Irql)
{
    HalpGicSendSgi(Irql, AFFINITY_MASK(KeGetCurrentProcessorNumber()));
}

VOID
FASTCALL
HalClearSoftwareInterrupt(
    _In_ KIRQL Irql)
{
    /* A requested SGI is always delivered, the handlers cope with an empty queue */
    UNREFERENCED_PARAMETER(Irql);
}

VOID
NTAPI
HalRequestIpi(
    _In_ KAFFINITY TargetProcessors)
{
    HalpGicSendSgi(IPI_LEVEL, TargetProcessors);
}

/**
 * @brief Acknowledges the highest priority pending interrupt.
 *
 * @return The INTID, or GIC_SPURIOUS_INTID when there is none.
 */
ULONG
FASTCALL
HalGetInterruptSource(VOID)
{
    return (ULONG)KeArm64ReadRegister(ARM64_ICC_IAR1_EL1) & 0xFFFFFF;
}

/**
 * @brief Raises to the IRQL of the interrupt and ends it at the GIC.
 *
 * The early end of interrupt only drops the running priority, the priority
 * mask keeps the line quiet until the IRQL goes back down.
 */
BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    _In_ KIRQL Irql,
    _In_ ULONG Vector,
    _Out_ PKIRQL OldIrql)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetPcr()->CurrentIrql;
    *OldIrql = CurrentIrql;

    if (Irql > CurrentIrql)
    {
        KeGetPcr()->CurrentIrql = Irql;
        HalpSetPriorityMask(Irql);
    }

    KeArm64WriteRegister(ARM64_ICC_EOIR1_EL1, Vector);
    __isb(_ARM64_BARRIER_SY);

    if (Irql > CurrentIrql)
        return TRUE;

    /* It slipped in before the mask went up, send software interrupts again */
    if (Vector < GIC_SGI_COUNT)
        HalpGicSendSgi(Vector, AFFINITY_MASK(KeGetCurrentProcessorNumber()));

    return FALSE;
}

VOID
NTAPI
HalEndSystemInterrupt(
    _In_ KIRQL OldIrql,
    _In_ PKTRAP_FRAME TrapFrame)
{
    UNREFERENCED_PARAMETER(TrapFrame);

    KeGetPcr()->CurrentIrql = OldIrql;
    HalpSetPriorityMask(OldIrql);
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    _In_ ULONG Vector,
    _In_ KIRQL Irql,
    _In_ KINTERRUPT_MODE InterruptMode)
{
    if ((Vector < GIC_SGI_COUNT) || (Vector > GIC_MAX_INTID))
        return FALSE;

    HalpGicSetPriority(Vector, HalpIrqlToPriority(Irql));
    return HalpGicEnableInterrupt(Vector, InterruptMode);
}

VOID
NTAPI
HalDisableSystemInterrupt(
    _In_ ULONG Vector,
    _In_ KIRQL Irql)
{
    UNREFERENCED_PARAMETER(Irql);

    if ((Vector < GIC_SGI_COUNT) || (Vector > GIC_MAX_INTID))
        return;

    HalpGicDisableInterrupt(Vector);
}

/**
 * @brief Translates a bus interrupt into a system vector and IRQL.
 *
 * Firmware describes interrupts as GSIVs, which are the GIC INTIDs, so the
 * vector is the level itself. Device interrupts run between DISPATCH_LEVEL
 * and CLOCK_LEVEL.
 */
ULONG
NTAPI
HalGetInterruptVector(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity)
{
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(BusInterruptLevel);

    if ((BusInterruptVector < GIC_PPI_FIRST) || (BusInterruptVector > GIC_MAX_INTID))
    {
        *Irql = 0;
        *Affinity = 0;
        return 0;
    }

    *Irql = (KIRQL)(DISPATCH_LEVEL + 1 + (BusInterruptVector % (CLOCK_LEVEL - DISPATCH_LEVEL - 1)));
    *Affinity = 1;
    return BusInterruptVector;
}

BOOLEAN
FASTCALL
HalSystemVectorDispatchEntry(
    _In_ ULONG Vector,
    _Out_ PKINTERRUPT_ROUTINE **FlatDispatch,
    _Out_ PKINTERRUPT_ROUTINE *NoConnection)
{
    UNREFERENCED_PARAMETER(Vector);
    UNREFERENCED_PARAMETER(FlatDispatch);
    UNREFERENCED_PARAMETER(NoConnection);
    return FALSE;
}

/* Line interrupts reach the GIC as their GSIV, which is also the vector */
static
PINTERRUPT_VECTOR_DATA
HalpGetLineVector(
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    PINTERRUPT_VECTOR_DATA VectorData;

    if ((ConnectionData == NULL) || (ConnectionData->Count != 1))
        return NULL;

    VectorData = &ConnectionData->Vectors[0];
    if ((VectorData->Type != InterruptTypeControllerInput) ||
        (VectorData->Vector != VectorData->ControllerInput.Gsiv))
    {
        return NULL;
    }

    return VectorData;
}

NTSTATUS
NTAPI
HalEnableInterrupt(
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    PINTERRUPT_VECTOR_DATA VectorData = HalpGetLineVector(ConnectionData);

    if (!VectorData)
        return STATUS_INVALID_PARAMETER;

    if (!HalEnableSystemInterrupt(VectorData->Vector, VectorData->Irql, VectorData->Mode))
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HalDisableInterrupt(
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    PINTERRUPT_VECTOR_DATA VectorData = HalpGetLineVector(ConnectionData);

    if (!VectorData)
        return STATUS_INVALID_PARAMETER;

    HalDisableSystemInterrupt(VectorData->Vector, VectorData->Irql);
    return STATUS_SUCCESS;
}

/* Message signaled interrupts need the GIC ITS */
NTSTATUS
NTAPI
HalGetMessageRoutingInfo(
    _In_ PHAL_MESSAGE_SIGNAL_TARGET_REQUEST Request,
    _Out_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(ConnectionData);
    return STATUS_NOT_SUPPORTED;
}
