/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            clock.c
 * PURPOSE:         Clock for VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "cpu/cpu.h"

// #include "clock.h"

#include "hardware/cmos.h"
#include "hardware/ps2.h"
#include "hardware/pit.h"
#include "hardware/video/vga.h"

/* Extra PSDK/NDK Headers */
#include <ndk/kefuncs.h>

/* DEFINES ********************************************************************/

/*
 * Activate IPS_DISPLAY if you want to display the
 * number of instructions per second, as well as
 * the computed number of ticks for the PIT.
 */
// #define IPS_DISPLAY

/*
 * Activate WORKING_TIMER when the PIT timing problem is fixed.
 */
// #define WORKING_TIMER


/* Processor speed */
#define STEPS_PER_CYCLE 256
#define IRQ1_CYCLES     16
#define IRQ12_CYCLES    16

/* VARIABLES ******************************************************************/

static LARGE_INTEGER StartPerfCount, Frequency;

static LARGE_INTEGER LastTimerTick, LastRtcTick, Counter;
static LONGLONG TimerTicks;
static DWORD StartTickCount, CurrentTickCount;
static DWORD LastClockUpdate;
static DWORD LastVerticalRefresh;

static DWORD LastIrq1Tick = 0, LastIrq12Tick = 0;

#ifdef IPS_DISPLAY
    static DWORD LastCyclePrintout;
    static ULONGLONG Cycles = 0;
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

VOID ClockUpdate(VOID)
{
    extern BOOLEAN CpuRunning;
    UINT i;
    // LARGE_INTEGER Counter;

#ifdef WORKING_TIMER
    DWORD PitResolution;
#endif
    DWORD RtcFrequency;

    while (VdmRunning && CpuRunning)
    {

#ifdef WORKING_TIMER
    PitResolution = PitGetResolution();
#endif
    RtcFrequency = RtcGetTicksPerSecond();

    /* Get the current number of ticks */
    CurrentTickCount = GetTickCount();

#ifdef WORKING_TIMER
    if ((PitResolution <= 1000) && (RtcFrequency <= 1000))
    {
        /* Calculate the approximate performance counter value instead */
        Counter.QuadPart = StartPerfCount.QuadPart
                           + (CurrentTickCount - StartTickCount)
                           * (Frequency.QuadPart / 1000);
    }
    else
#endif
    {
        /* Get the current performance counter value */
        /// DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);
        NtQueryPerformanceCounter(&Counter, NULL);
        /// SetThreadAffinityMask(GetCurrentThread(), oldmask);
    }

    /* Get the number of PIT ticks that have passed */
    TimerTicks = ((Counter.QuadPart - LastTimerTick.QuadPart)
                 * PIT_BASE_FREQUENCY) / Frequency.QuadPart;

    /* Update the PIT */
    if (TimerTicks > 0)
    {
        PitClock(TimerTicks);
        LastTimerTick = Counter;
    }

    /* Check for RTC update */
    if ((CurrentTickCount - LastClockUpdate) >= 1000)
    {
        RtcTimeUpdate();
        LastClockUpdate = CurrentTickCount;
    }

    /* Check for RTC periodic tick */
    if ((Counter.QuadPart - LastRtcTick.QuadPart)
        >= (Frequency.QuadPart / (LONGLONG)RtcFrequency))
    {
        RtcPeriodicTick();
        LastRtcTick = Counter;
    }

    /* Check for vertical retrace */
    if ((CurrentTickCount - LastVerticalRefresh) >= 15)
    {
        VgaRefreshDisplay();
        LastVerticalRefresh = CurrentTickCount;
    }

    if ((CurrentTickCount - LastIrq1Tick) >= IRQ1_CYCLES)
    {
        GenerateIrq1();
        LastIrq1Tick = CurrentTickCount;
    }

    if ((CurrentTickCount - LastIrq12Tick) >= IRQ12_CYCLES)
    {
        GenerateIrq12();
        LastIrq12Tick = CurrentTickCount;
    }

    /* Horizontal retrace occurs as fast as possible */
    VgaHorizontalRetrace();

    /* Continue CPU emulation */
    for (i = 0; VdmRunning && CpuRunning && (i < STEPS_PER_CYCLE); i++)
    {
        CpuStep();
#ifdef IPS_DISPLAY
        ++Cycles;
#endif
    }

#ifdef IPS_DISPLAY
    if ((CurrentTickCount - LastCyclePrintout) >= 1000)
    {
        DPRINT1("NTVDM: %I64u Instructions Per Second; TimerTicks = %I64d\n", Cycles * 1000 / (CurrentTickCount - LastCyclePrintout), TimerTicks);
        LastCyclePrintout = CurrentTickCount;
        Cycles = 0;
    }
#endif

    }
}

BOOLEAN ClockInitialize(VOID)
{
    /* Initialize the performance counter (needed for hardware timers) */
    /* Find the starting performance */
    NtQueryPerformanceCounter(&StartPerfCount, &Frequency);
    if (Frequency.QuadPart == 0)
    {
        wprintf(L"FATAL: Performance counter not available\n");
        return FALSE;
    }

    /* Find the starting tick count */
    StartTickCount = GetTickCount();

    /* Set the different last counts to the starting count */
    LastClockUpdate = LastVerticalRefresh =
#ifdef IPS_DISPLAY
    LastCyclePrintout =
#endif
    StartTickCount;

    /* Set the last timer ticks to the current time */
    LastTimerTick = LastRtcTick = StartPerfCount;

    return TRUE;
}
