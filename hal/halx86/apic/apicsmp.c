/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        hal/halx86/apic/apicsmp.c
 * PURPOSE:     SMP specific APIC code
 * PROGRAMMERS: Copyright 2021 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "apicp.h"
#define NDEBUG
#include <debug.h>

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
        APIC_DSH_AllExclusingSelf

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
    APIC_INTERRUPT_COMMAND_REGISTER Icr;

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
}


/* SMP SUPPORT FUNCTIONS ******************************************************/

// Should be called by SMP version of HalRequestIpi
VOID
NTAPI
HalpRequestIpi(KAFFINITY TargetProcessors)
{
    UNIMPLEMENTED;
    __debugbreak();
}

// APIC specific SMP code here
