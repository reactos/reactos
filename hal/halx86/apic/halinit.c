/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the x86 HAL
 * COPYRIGHT:   Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>
#include "apic.h"

VOID
NTAPI
ApicInitializeLocalApic(ULONG Cpu);

/* GLOBALS ******************************************************************/

const USHORT HalpBuildType = HAL_BUILD_TYPE;

#ifdef _M_IX86
PKPCR HalpProcessorPCR[32];
#endif

/* FUNCTIONS ****************************************************************/

#ifdef _M_IX86
VOID
NTAPI
HalInitApicInterruptHandlers()
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
}

VOID
NTAPI
HalpInitializeLocalUnit()
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
}
#endif

#ifdef _M_IX86
VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKPCR Pcr = KeGetPcr();

    /* Set default IDR */
    Pcr->IDR = 0xFFFFFFFF;

    *(PUCHAR)(Pcr->HalReserved) = ProcessorNumber; // FIXME
    HalpProcessorPCR[ProcessorNumber] = Pcr;

    // FIXME support '/INTAFFINITY' key in .ini

    /* By default, the HAL allows interrupt requests to be received by all processors */
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity, ProcessorNumber);

    if (ProcessorNumber == 0)
    {
        if (!DetectMP(KeLoaderBlock))
        {
            __halt();
        }

        /* Register routines for KDCOM */
        HalpRegisterKdSupportFunctions();

        // FIXME HalpGlobal8259Mask

        WRITE_PORT_UCHAR((PUCHAR)PIC1_DATA_PORT, 0xFF);
        WRITE_PORT_UCHAR((PUCHAR)PIC2_DATA_PORT, 0xFF);
    }

    HalInitApicInterruptHandlers();
    HalpInitializeLocalUnit();

    /* Update the interrupt affinity */
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity,
                             ProcessorNumber);

    /* Register routines for KDCOM */
    HalpRegisterKdSupportFunctions();
}
#else
VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Initialize the local APIC for this cpu */
    ApicInitializeLocalApic(ProcessorNumber);

    /* Initialize profiling data (but don't start it) */
    HalInitializeProfiling();

    /* Initialize the timer */
    //ApicInitializeTimer(ProcessorNumber);

    /* Update the interrupt affinity */
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity,
                             ProcessorNumber);

    /* Register routines for KDCOM */
    HalpRegisterKdSupportFunctions();
}
#endif


VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{

    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);
}

VOID
HalpInitPhase1(VOID)
{
    /* Initialize DMA. NT does this in Phase 0 */
    HalpInitDma();
}

/* EOF */
