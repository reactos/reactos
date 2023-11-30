/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SMP specific APIC code
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "apicp.h"
#include <smp.h>

#define NDEBUG
#include <debug.h>


extern PPROCESSOR_IDENTITY HalpProcessorIdentity;

/* INTERNAL FUNCTIONS *********************************************************/

/*!
    \param Vector - Specifies the interrupt vector to be delivered.

    \param MessageType - Specifies the message type sent to the CPU core
        interrupt handler. This can be one of the following values:
        APIC_MT_Fixed - Delivers an interrupt to the target local APIC
            specified in Destination field.
        APIC_MT_LowestPriority - Delivers an interrupt to the local APIC
            executing at the lowest priority of all local APICs.
        APIC_MT_SMI - Delivers an SMI interrupt to target local APIC(s).
        APIC_MT_RemoteRead - Delivers a read request to read an APIC register
            in the target local APIC specified in Destination field.
        APIC_MT_NMI - Delivers a non-maskable interrupt to the target local
            APIC specified in the Destination field. Vector is ignored.
        APIC_MT_INIT - Delivers an INIT request to the target local APIC(s)
            specified in the Destination field. TriggerMode must be
            APIC_TGM_Edge, Vector must be 0.
        APIC_MT_Startup - Delivers a start-up request (SIPI) to the target
            local APIC(s) specified in Destination field. Vector specifies
            the startup address.
        APIC_MT_ExtInt - Delivers an external interrupt to the target local
            APIC specified in Destination field.

    \param TriggerMode - The trigger mode of the interrupt. Can be:
        APIC_TGM_Edge - The interrupt is edge triggered.
        APIC_TGM_Level - The interrupt is level triggered.

    \param DestinationShortHand - Specifies where to send the interrupt.
        APIC_DSH_Destination
        APIC_DSH_Self
        APIC_DSH_AllIncludingSelf
        APIC_DSH_AllExcludingSelf

    \see "AMD64 Architecture Programmer's Manual Volume 2 System Programming"
        Chapter 16 "Advanced Programmable Interrupt Controller (APIC)"
        16.5 "Interprocessor Interrupts (IPI)"

 */
FORCEINLINE
VOID
ApicRequestGlobalInterrupt(
    _In_ UCHAR DestinationProcessor,
    _In_ UCHAR Vector,
    _In_ APIC_MT MessageType,
    _In_ APIC_TGM TriggerMode,
    _In_ APIC_DSH DestinationShortHand)
{
    ULONG Flags;
    APIC_INTERRUPT_COMMAND_REGISTER Icr;

    /* Disable interrupts so that we can change IRR without being interrupted */
    Flags = __readeflags();
    _disable();

    /* Wait for the APIC to be idle */
    do
    {
        Icr.Long0 = ApicRead(APIC_ICR0);
    } while (Icr.DeliveryStatus);

    /* Setup the command register */
    Icr.LongLong = 0;
    Icr.Vector = Vector;
    Icr.MessageType = MessageType;
    Icr.DestinationMode = APIC_DM_Physical;
    Icr.DeliveryStatus = 0;
    Icr.Level = 0;
    Icr.TriggerMode = TriggerMode;
    Icr.RemoteReadStatus = 0;
    Icr.DestinationShortHand = DestinationShortHand;
    Icr.Destination = DestinationProcessor;

    /* Write the low dword last to send the interrupt */
    ApicWrite(APIC_ICR1, Icr.Long1);
    ApicWrite(APIC_ICR0, Icr.Long0);

    /* Finally, restore the original interrupt state */
    if (Flags & EFLAGS_INTERRUPT_MASK)
    {
        _enable();
    }
}


/* SMP SUPPORT FUNCTIONS ******************************************************/

VOID
ApicStartApplicationProcessor(
    _In_ ULONG NTProcessorNumber,
    _In_ PHYSICAL_ADDRESS StartupLoc)
{
    ASSERT(StartupLoc.HighPart == 0);
    ASSERT((StartupLoc.QuadPart & 0xFFF) == 0);
    ASSERT((StartupLoc.QuadPart & 0xFFF00FFF) == 0);

    /* Init IPI */
    ApicRequestGlobalInterrupt(HalpProcessorIdentity[NTProcessorNumber].LapicId, 0,
        APIC_MT_INIT, APIC_TGM_Edge, APIC_DSH_Destination);

    /* De-Assert Init IPI */
    ApicRequestGlobalInterrupt(HalpProcessorIdentity[NTProcessorNumber].LapicId, 0,
        APIC_MT_INIT, APIC_TGM_Level, APIC_DSH_Destination);

    /* Stall execution for a bit to give APIC time: MPS Spec - B.4 */
    KeStallExecutionProcessor(200);

    /* Startup IPI */
    ApicRequestGlobalInterrupt(HalpProcessorIdentity[NTProcessorNumber].LapicId, (StartupLoc.LowPart) >> 12,
        APIC_MT_Startup, APIC_TGM_Edge, APIC_DSH_Destination);
}

/* HAL IPI FUNCTIONS **********************************************************/

/*!
 *  \brief Broadcasts an IPI with a specified vector to all processors.
 *
 *  \param Vector - Specifies the interrupt vector to be delivered.
 *  \param IncludeSelf - Specifies whether to include the current processor.
 */
VOID
NTAPI
HalpBroadcastIpiSpecifyVector(
    _In_ UCHAR Vector,
    _In_ BOOLEAN IncludeSelf)
{
    APIC_DSH DestinationShortHand = IncludeSelf ?
        APIC_DSH_AllIncludingSelf : APIC_DSH_AllExcludingSelf;

    /* Request the interrupt targeted at all processors */
    ApicRequestGlobalInterrupt(0, // Ignored
                               Vector,
                               APIC_MT_Fixed,
                               APIC_TGM_Edge,
                               DestinationShortHand);
}

/*!
 *  \brief Requests an IPI with a specified vector on the specified processors.
 *
 *  \param TargetSet - Specifies the set of processors to send the IPI to.
 *  \param Vector - Specifies the interrupt vector to be delivered.
 *
 *  \remarks This function is exported on Windows 10.
 */
VOID
NTAPI
HalRequestIpiSpecifyVector(
    _In_ KAFFINITY TargetSet,
    _In_ UCHAR Vector)
{
    KAFFINITY ActiveProcessors = HalpActiveProcessors;
    KAFFINITY RemainingSet, SetMember;
    ULONG ProcessorIndex;
    ULONG LApicId;

    /* Sanitize the target set */
    TargetSet &= ActiveProcessors;

    /* Check if all processors are requested */
    if (TargetSet == ActiveProcessors)
    {
        /* Send an IPI to all processors, including this processor */
        HalpBroadcastIpiSpecifyVector(Vector, TRUE);
        return;
    }

    /* Check if all processors except the current one are requested */
    if (TargetSet == (ActiveProcessors & ~KeGetCurrentPrcb()->SetMember))
    {
        /* Send an IPI to all processors, excluding this processor */
        HalpBroadcastIpiSpecifyVector(Vector, FALSE);
        return;
    }

    /* Loop while we have more processors */
    RemainingSet = TargetSet;
    while (RemainingSet != 0)
    {
        NT_VERIFY(BitScanForwardAffinity(&ProcessorIndex, RemainingSet) != 0);
        ASSERT(ProcessorIndex < KeNumberProcessors);
        SetMember = AFFINITY_MASK(ProcessorIndex);
        RemainingSet &= ~SetMember;

        /* Send the interrupt to the target processor */
        LApicId = HalpProcessorIdentity[ProcessorIndex].LapicId;
        ApicRequestGlobalInterrupt(LApicId,
                                   Vector,
                                   APIC_MT_Fixed,
                                   APIC_TGM_Edge,
                                   APIC_DSH_Destination);
    }
}

/*!
 *  \brief Requests an IPI interrupt on the specified processors.
 *
 *  \param TargetSet - Specifies the set of processors to send the IPI to.
 */
VOID
NTAPI
HalpRequestIpi(
    _In_ KAFFINITY TargetSet)
{
    /* Request the IPI vector */
    HalRequestIpiSpecifyVector(TargetSet, APIC_IPI_VECTOR);
}

#ifdef _M_AMD64

/*!
 *  \brief Requests a software interrupt on the specified processors.
 *
 *  \param TargetSet - Specifies the set of processors to send the IPI to.
 *  \param Irql - Specifies the IRQL of the software interrupt.
 */
VOID
NTAPI
HalpSendSoftwareInterrupt(
    _In_ KAFFINITY TargetSet,
    _In_ KIRQL Irql)
{
    UCHAR Vector;

    /* Get the vector for the requested IRQL */
    if (Irql == APC_LEVEL)
    {
        Vector = APC_VECTOR;
    }
    else if (Irql == DISPATCH_LEVEL)
    {
        Vector = DISPATCH_VECTOR;
    }
    else
    {
        ASSERT(FALSE);
        return;
    }

    /* Request the IPI with the specified vector */
    HalRequestIpiSpecifyVector(TargetSet, Vector);
}

/*!
 *  \brief Requests an NMI interrupt on the specified processors.
 *
 *  \param TargetSet - Specifies the set of processors to send the IPI to.
 */
VOID
NTAPI
HalpSendNMI(
    _In_ KAFFINITY TargetSet)
{
    KAFFINITY RemainingSet, SetMember;
    ULONG ProcessorIndex;
    ULONG LApicId;

    /* Make sure we do not send an NMI to ourselves */
    ASSERT((TargetSet & KeGetCurrentPrcb()->SetMember) == 0);

    /* Loop while we have more processors */
    RemainingSet = TargetSet;
    while (RemainingSet != 0)
    {
        NT_VERIFY(BitScanForwardAffinity(&ProcessorIndex, RemainingSet) != 0);
        ASSERT(ProcessorIndex < KeNumberProcessors);
        SetMember = AFFINITY_MASK(ProcessorIndex);
        RemainingSet &= ~SetMember;

        /* Send and NMI to the target processor */
        LApicId = HalpProcessorIdentity[ProcessorIndex].LapicId;
        ApicRequestGlobalInterrupt(LApicId,
                                   0,
                                   APIC_MT_NMI,
                                   APIC_TGM_Edge,
                                   APIC_DSH_Destination);
    }
}

#endif // _M_AMD64
