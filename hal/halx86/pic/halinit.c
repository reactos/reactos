/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the x86 HAL
 * COPYRIGHT:   Copyright 1998 David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

const USHORT HalpBuildType = HAL_BUILD_TYPE;
BOOLEAN HalpPciLockSettings;

/* FUNCTIONS ****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpGetParameters(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine;

    /* Make sure we have a loader block and command line */
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        /* Read the command line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Check if PCI is locked */
        if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

        /* Check for initial breakpoint */
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set default IDR */
    KeGetPcr()->IDR = 0xFFFFFFFF & ~(1 << PIC_CASCADE_IRQ);

    /* Update the interrupt affinity */
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity,
                             ProcessorNumber);

    /* Register routines for KDCOM */
    HalpRegisterKdSupportFunctions();
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{

}

VOID
HalpInitPhase1(VOID)
{
        /* Enable timer interrupt handler */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE + PIC_TIMER_IRQ,
                                   CLOCK2_LEVEL,
                                   HalpClockInterrupt,
                                   Latched);

        /* Enable RTC interrupt handler */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE + PIC_RTC_IRQ,
                                   PROFILE_LEVEL,
                                   HalpProfileInterrupt,
                                   Latched);

        /* Initialize DMA. NT does this in Phase 0 */
        HalpInitDma();
}

/* EOF */
