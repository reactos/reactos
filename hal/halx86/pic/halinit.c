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

#if defined(SARCH_PC98)

ADDRESS_USAGE HalpDefaultIoSpace =
{
    NULL, CmResourceTypePort, IDT_INTERNAL,
    {
        /* PIC 1 */
        {0x00,  1},
        {0x02,  1},
        /* PIC 2 */
        {0x08,  1},
        {0x0A,  1},
        /* DMA */
        {0x01,  1},
        {0x03,  1},
        {0x05,  1},
        {0x07,  1},
        {0x09,  1},
        {0x0B,  1},
        {0x0D,  1},
        {0x0F,  1},
        {0x11,  1},
        {0x13,  1},
        {0x15,  1},
        {0x17,  1},
        {0x19,  1},
        {0x1B,  1},
        {0x1D,  1},
        {0x1F,  1},
        {0x21,  1},
        {0x23,  1},
        {0x25,  1},
        {0x27,  1},
        {0x29,  1},
        {0x2B,  1},
        {0x2D,  1},
        {0xE05, 1},
        {0xE07, 1},
        {0xE09, 1},
        {0xE0B, 1},
        /* RTC */
        {0x20,  1},
        {0x22,  1},
        {0x128, 1},
        /* System Control */
        {0x33,  1},
        {0x37,  1},
        /* PIT */
        {0x71,  1},
        {0x73,  1},
        {0x75,  1},
        {0x77,  1},
        {0x3FD9,1},
        {0x3FDB,1},
        {0x3FDD,1},
        {0x3FDF,1},
        /* x87 Coprocessor */
        {0xF8,  8},
        {0xCF8, 0x8},  /* PCI 0 */
        {0,0},
    }
};

#else

#ifdef _M_IX86
ADDRESS_USAGE HalpDefaultIoSpace =
{
    NULL, CmResourceTypePort, IDT_DEVICE,
    {
        {0x00,  0x20}, /* DMA 1 */
        {0xC0,  0x20}, /* DMA 2 */
        {0x80,  0x10}, /* DMA EPAR */
        {0x20,  0x2},  /* PIC 1 */
        {0xA0,  0x2},  /* PIC 2 */
        {0x40,  0x4},  /* PIT 1 */
        {0x48,  0x4},  /* PIT 2 */
        {0x92,  0x1},  /* System Control Port A */
        {0x70,  0x2},  /* CMOS  */
        {0xF0,  0x10}, /* x87 Coprocessor */
        {0xCF8, 0x8},  /* PCI 0 */
        {0,0},
    }
};
#endif

#endif

const USHORT HalpBuildType = HAL_BUILD_TYPE;
BOOLEAN HalpPciLockSettings;

/* FUNCTIONS ****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpGetParameters(IN PCHAR CommandLine)
{
    /* Check if PCI is locked */
    if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

    /* Check for initial breakpoint */
    if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
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
    /* Fill out the dispatch tables */
    HalSetSystemInformation = HaliSetSystemInformation;

    /* Initialize ACPI */
    HalpSetupAcpiPhase0(LoaderBlock);

    /* Initialize the PICs */
    HalpInitializePICs(TRUE);

    /* Initialize CMOS lock */
    KeInitializeSpinLock(&HalpSystemHardwareLock);

    /* Initialize CMOS */
    HalpInitializeCmos();

    /* Setup busy waiting */
    HalpCalibrateStallExecution();

    /* Initialize the clock */
    HalpInitializeClock();

    /*
     * We could be rebooting with a pending profile interrupt,
     * so clear it here before interrupts are enabled
     */
    HalStopProfileInterrupt(ProfileTime);
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
