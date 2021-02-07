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

CODE_SEG("INIT")
VOID
NTAPI
HalpMarkProcessorStarted(_In_ UCHAR Id,
                         _In_ ULONG PrcNumber)
{
    UNIMPLEMENTED;
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
    if (HalDispatchTableVersion >= HAL_DISPATCH_VERSION)
    {
        /* Fill out HalDispatchTable */
        //HalGetInterruptTranslator = HaliGetInterruptTranslator;        // FIXME: TODO
    }

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

    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);
}

VOID
NTAPI
HaliAcpiSetUsePmClock(VOID)
{

}

#ifndef _MINIHAL_
VOID
FASTCALL
HalpClockInterruptHandler(_In_ PKTRAP_FRAME TrapFrame)
{
    HaliClockInterrupt(TrapFrame, TRUE);
}
#endif

/* PUBLIC TIMER FUNCTIONS *****************************************************/

VOID
NTAPI
KeStallExecutionProcessor(_In_ ULONG MicroSeconds)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

VOID
NTAPI
HalCalibratePerformanceCounter(_In_ volatile PLONG Count,
                               _In_ ULONGLONG NewCount)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(_Out_opt_ LARGE_INTEGER * OutPerformanceFrequency)
{
    LARGE_INTEGER Result;
    UNIMPLEMENTED;
    ASSERT(FALSE);Result.QuadPart = 0;
    return Result;
}

ULONG
NTAPI
HalSetTimeIncrement(_In_ ULONG Increment)
{
    ULONG Result;
    UNIMPLEMENTED;
    ASSERT(FALSE);Result = 0;
    return Result;
}

/* EOF */
