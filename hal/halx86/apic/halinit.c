/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the APIC HAL
 * COPYRIGHT:   Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#include "apicp.h"
#include <smp.h>
#define NDEBUG
#include <debug.h>

VOID
NTAPI
ApicInitializeLocalApic(ULONG Cpu);

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (ProcessorNumber == 0)
    {
        HalpParseApicTables(LoaderBlock);
    }

    HalpSetupProcessorsTable(ProcessorNumber);

    /* Initialize the local APIC for this cpu */
    ApicInitializeLocalApic(ProcessorNumber);

    /* Initialize profiling data (but don't start it) */
    HalInitializeProfiling();

    /* Initialize the timer */
    //ApicInitializeTimer(ProcessorNumber);
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    DPRINT1("Using HAL: APIC %s %s\n",
            (HalpBuildType & PRCB_BUILD_UNIPROCESSOR) ? "UP" : "SMP",
            (HalpBuildType & PRCB_BUILD_DEBUG) ? "DBG" : "REL");

    HalpPrintApicTables();

    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);

    /* Measure the profile timer against the calibrated TSC */
    HalpInitializeProfileTimer();

    /* The profile interrupt is the local APIC timer, not an I/O APIC input */
    HalpRegisterVector(IDT_DEVICE | IDT_LATCHED,
                       0,
                       APIC_PROFILE_VECTOR,
                       APIC_PROFILE_LEVEL);
    KeRegisterInterruptHandler(APIC_PROFILE_VECTOR, HalpProfileInterrupt);

    /* Profile list updates must mask the profile interrupt */
    KeSetProfileIrql(APIC_PROFILE_LEVEL);
}

VOID
HalpInitPhase1(VOID)
{
    /* Initialize DMA. NT does this in Phase 0 */
    HalpInitDma();
}

/* EOF */
