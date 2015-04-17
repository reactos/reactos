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

#include "clock.h"

#include "hardware/cmos.h"
#include "hardware/ps2.h"
#include "hardware/pit.h"
#include "hardware/video/vga.h"
#include "hardware/mouse.h"

/* Extra PSDK/NDK Headers */
#include <ndk/kefuncs.h>

/* DEFINES ********************************************************************/

/*
 * Activate IPS_DISPLAY if you want to display the
 * number of instructions per second.
 */
// #define IPS_DISPLAY

/*
 * Activate WORKING_TIMER when the PIT timing problem is fixed.
 */
// #define WORKING_TIMER

/* Processor speed */
#define STEPS_PER_CYCLE 1024

/* VARIABLES ******************************************************************/

static LIST_ENTRY Timers;
static LARGE_INTEGER StartPerfCount, Frequency;
static DWORD StartTickCount;

#ifdef IPS_DISPLAY
static ULONGLONG Cycles = 0ULL;
#endif

/* PRIVATE FUNCTIONS **********************************************************/

#ifdef IPS_DISPLAY
static VOID FASTCALL IpsDisplayCallback(ULONGLONG ElapsedTime)
{
    DPRINT1("NTVDM: %I64u Instructions Per Second\n", Cycles / ElapsedTime);
    Cycles = 0ULL;
}
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

VOID ClockUpdate(VOID)
{
    extern BOOLEAN CpuRunning;
    UINT i;
    PLIST_ENTRY Entry;
    LARGE_INTEGER Counter;

    while (VdmRunning && CpuRunning)
    {
        /* Get the current number of ticks */
        DWORD CurrentTickCount = GetTickCount();

#ifdef WORKING_TIMER
        if ((PitResolution <= 1000) && (RtcFrequency <= 1000))
        {
            /* Calculate the approximate performance counter value instead */
            Counter.QuadPart = StartPerfCount.QuadPart
                               + ((CurrentTickCount - StartTickCount)
                               * Frequency.QuadPart) / 1000;
        }
        else
#endif
        {
            /* Get the current performance counter value */
            /// DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);
            NtQueryPerformanceCounter(&Counter, NULL);
            /// SetThreadAffinityMask(GetCurrentThread(), oldmask);
        }

        /* Continue CPU emulation */
        for (i = 0; VdmRunning && CpuRunning && (i < STEPS_PER_CYCLE); i++)
        {
            CpuStep();

#ifdef IPS_DISPLAY
            ++Cycles;
#endif
        }

        for (Entry = Timers.Flink; Entry != &Timers; Entry = Entry->Flink)
        {
            ULONGLONG Ticks = (ULONGLONG)-1;
            PHARDWARE_TIMER Timer = CONTAINING_RECORD(Entry, HARDWARE_TIMER, Link);

            ASSERT((Timer->EnableCount > 0) && (Timer->Flags & HARDWARE_TIMER_ENABLED));

            if (Timer->Delay)
            {
                if (Timer->Flags & HARDWARE_TIMER_PRECISE)
                {
                    /* Use the performance counter for precise timers */
                    if (Counter.QuadPart <= Timer->LastTick.QuadPart) continue;
                    Ticks = (Counter.QuadPart - Timer->LastTick.QuadPart) / Timer->Delay;
                }
                else
                {
                    /* Use the regular tick count for normal timers */
                    if (CurrentTickCount <= Timer->LastTick.LowPart) continue;
                    Ticks = (CurrentTickCount - Timer->LastTick.LowPart) / (ULONG)Timer->Delay;
                }

                if (Ticks == 0) continue;
            }

            Timer->Callback(Ticks);

            if (Timer->Flags & HARDWARE_TIMER_ONESHOT)
            {
                /* Disable this timer */
                DisableHardwareTimer(Timer);
            }

            /* Update the time of the last timer tick */
            Timer->LastTick.QuadPart += Ticks * Timer->Delay;
        }
    }
}

PHARDWARE_TIMER CreateHardwareTimer(ULONG Flags, ULONG Frequency, PHARDWARE_TIMER_PROC Callback)
{
    PHARDWARE_TIMER Timer;
    
    Timer = (PHARDWARE_TIMER)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(HARDWARE_TIMER));
    if (Timer == NULL) return NULL;

    Timer->Flags = Flags & ~HARDWARE_TIMER_ENABLED;
    Timer->EnableCount = 0;
    Timer->Callback = Callback;
    SetHardwareTimerDelay(Timer, 1000000000ULL / (ULONGLONG)Frequency);

    if (Flags & HARDWARE_TIMER_ENABLED) EnableHardwareTimer(Timer);
    return Timer;
}

VOID EnableHardwareTimer(PHARDWARE_TIMER Timer)
{
    /* Increment the count */
    Timer->EnableCount++;

    /* Check if the count is above 0 but the timer isn't enabled */
    if ((Timer->EnableCount > 0) && !(Timer->Flags & HARDWARE_TIMER_ENABLED))
    {
        if (Timer->Flags & HARDWARE_TIMER_PRECISE)
        {
            NtQueryPerformanceCounter(&Timer->LastTick, NULL);
        }
        else
        {
            Timer->LastTick.LowPart = GetTickCount();
        }

        Timer->Flags |= HARDWARE_TIMER_ENABLED;
        InsertTailList(&Timers, &Timer->Link);
    }
}

VOID DisableHardwareTimer(PHARDWARE_TIMER Timer)
{
    /* Decrement the count */
    Timer->EnableCount--;

    /* Check if the count is 0 or less but the timer is enabled */
    if ((Timer->EnableCount <= 0) && (Timer->Flags & HARDWARE_TIMER_ENABLED))
    {
        /* Disable the timer */
        Timer->Flags &= ~HARDWARE_TIMER_ENABLED;
        RemoveEntryList(&Timer->Link);
    }
}

VOID SetHardwareTimerDelay(PHARDWARE_TIMER Timer, ULONGLONG NewDelay)
{
    if (Timer->Flags & HARDWARE_TIMER_PRECISE)
    {
        /* Convert the delay from nanoseconds to performance counter ticks */
        Timer->Delay = (NewDelay * Frequency.QuadPart + 500000000ULL) / 1000000000ULL;
    }
    else
    {
        Timer->Delay = NewDelay / 1000000ULL;
    }
}

VOID DestroyHardwareTimer(PHARDWARE_TIMER Timer)
{
    if (Timer->Flags & HARDWARE_TIMER_ENABLED) RemoveEntryList(&Timer->Link);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Timer);
}

BOOLEAN ClockInitialize(VOID)
{
#ifdef IPS_DISPLAY
    PHARDWARE_TIMER IpsTimer;
#endif

    InitializeListHead(&Timers);

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

#ifdef IPS_DISPLAY

    IpsTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED, 1, IpsDisplayCallback);
    if (IpsTimer == NULL)
    {
        wprintf(L"FATAL: Cannot create IPS display timer.\n");
        return FALSE;
    }

#endif

    return TRUE;
}
