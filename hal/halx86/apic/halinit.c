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
#ifdef _M_AMD64
    DPRINT1("HalpInitProcessor: Initializing APIC timer for processor %lu\n", ProcessorNumber);
#endif
    ApicInitializeTimer(ProcessorNumber);
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
#ifdef _M_AMD64
    DPRINT1("HalpInitPhase0: entered\n");
    
    /* Skip DPRINT1 and HalpPrintApicTables on AMD64 - might cause issues */
    DPRINT1("HalpInitPhase0: Skipping DPRINT1 and ACPI table print on AMD64\n");
#else
    DPRINT1("Using HAL: APIC %s %s\n",
            (HalpBuildType & PRCB_BUILD_UNIPROCESSOR) ? "UP" : "SMP",
            (HalpBuildType & PRCB_BUILD_DEBUG) ? "DBG" : "REL");

    HalpPrintApicTables();
#endif

#ifdef _M_AMD64
    DPRINT1("HalpInitPhase0: About to enable interrupt handlers\n");
    
    /* Skip interrupt handler registration on AMD64 for now */
    DPRINT1("HalpInitPhase0: Skipping interrupt handler registration on AMD64\n");
    /* TODO: Properly implement interrupt handlers for AMD64 */
#else
    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);

    /* Enable profile interrupt handler */
    HalpEnableInterruptHandler(IDT_DEVICE,
                               0,
                               APIC_PROFILE_VECTOR,
                               APIC_PROFILE_LEVEL,
                               HalpProfileInterrupt,
                               Latched);
#endif

#ifdef _M_AMD64
    DPRINT1("HalpInitPhase0: completed\n");
#endif
}

VOID
HalpInitPhase1(VOID)
{
    /* Initialize DMA. NT does this in Phase 0 */
    HalpInitDma();
}

/* EOF */
