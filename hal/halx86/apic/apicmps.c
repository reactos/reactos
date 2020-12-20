/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicmps.c
 * PURPOSE:         MPS part APIC HALs code
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"

/* GLOBALS ********************************************************************/

BOOLEAN HalpPciLockSettings;

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpGetParameters(IN PCHAR CommandLine)
{
    /* Check if PCI is locked */
    if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

    /* Check for initial breakpoint */
    if (strstr(CommandLine, "BREAK")) DbgBreakPoint();

    // FIXME parameters: "INTAFFINITY", "MAXAPICCLUSTER", "MAXPROCSPERCLUSTER", "USEPHYSICALAPIC" "CLKLVL", "TIMERES", "USE8254"
}

BOOLEAN
NTAPI 
DetectMP(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
    return FALSE;
}

VOID
HalpInitPhase0a(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
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

    /* Do some HAL-specific initialization */
    HalpInitPhase0(LoaderBlock);

    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);
}

/* EOF */
