/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/sound/speaker.c
 * PURPOSE:         PC Speaker emulation
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "speaker.h"
#include "hardware/pit.h"

/* Extra PSDK/NDK Headers */
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>

/* DDK Driver Headers */
#include <ntddbeep.h>

/* PRIVATE VARIABLES **********************************************************/

static HANDLE hBeep = NULL;

static LARGE_INTEGER FreqCount, CountStart;
static ULONG PulseTickCount = 0, FreqPulses = 0;

#define SPEAKER_RESPONSE    200     // in milliseconds

#define MIN_AUDIBLE_FREQ    20      // BEEP_FREQUENCY_MINIMUM
#define MAX_AUDIBLE_FREQ    20000   // BEEP_FREQUENCY_MAXIMUM
#define CLICK_FREQ          100


/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
MakeBeep(ULONG Frequency,
         ULONG Duration)
{
    static ULONG LastFrequency = 0, LastDuration = 0;

    IO_STATUS_BLOCK IoStatusBlock;
    BEEP_SET_PARAMETERS BeepSetParameters;

    /* A null frequency means we stop beeping */
    if (Frequency == 0) Duration = 0;

    /*
     * Do nothing if we are replaying exactly the same sound
     * (this avoids hiccups due to redoing the same beeps).
     */
    if (Frequency == LastFrequency && Duration == LastDuration) return;

    /*
     * For small durations we automatically reset the beep so
     * that we can replay short beeps like clicks immediately.
     */
    if (Duration < 10)
    {
        LastFrequency = 0;
        LastDuration  = 0;
    }
    else
    {
        LastFrequency = Frequency;
        LastDuration  = Duration;
    }

    /* Set the data and do the beep */
    BeepSetParameters.Frequency = Frequency;
    BeepSetParameters.Duration  = Duration;

    NtDeviceIoControlFile(hBeep,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatusBlock,
                          IOCTL_BEEP_SET,
                          &BeepSetParameters,
                          sizeof(BeepSetParameters),
                          NULL,
                          0);
}

static
VOID PulseSample(VOID)
{
    static ULONG Pulses = 0, CountStartTick = 0, LastPulsesFreq = 0;
    ULONG LastPulseTickCount, CurrPulsesFreq;
    LARGE_INTEGER Counter;
    LONGLONG Elapsed;

    /*
     * Check how far away was the previous pulse and
     * if it was >= 200ms away then restart counting.
     */
    LastPulseTickCount = PulseTickCount;
    PulseTickCount     = GetTickCount();
    if (PulseTickCount - LastPulseTickCount >= SPEAKER_RESPONSE)
    {
        CountStart.QuadPart = 0;
        Pulses     = 0;
        FreqPulses = 0;
        return;
    }

    /* We have closely spaced pulses. Start counting. */
    if (CountStart.QuadPart == 0)
    {
        NtQueryPerformanceCounter(&CountStart, NULL);
        CountStartTick = PulseTickCount;
        Pulses     = 0;
        FreqPulses = 0;
        return;
    }

    /* A pulse is ongoing */
    ++Pulses;

    /* We require some pulses to have some statistics */
    if (PulseTickCount - CountStartTick <= (SPEAKER_RESPONSE >> 1)) return;

    /* Get count time */
    NtQueryPerformanceCounter(&Counter, NULL);

    /*
     * Get the number of speaker hundreds of microseconds that have passed
     * since we started counting.
     */
    Elapsed = (Counter.QuadPart - CountStart.QuadPart) * 10000 / FreqCount.QuadPart;
    if (Elapsed == 0) ++Elapsed;

    /* Update counting for next pulses */
    CountStart     = Counter;
    CountStartTick = PulseTickCount;

    // HACKHACK!! I need to check why we need to double the number
    // of pulses in order to have the correct frequency...
    Pulses <<= 1;

    /* Get the current pulses frequency */
    CurrPulsesFreq = 10000 * Pulses / Elapsed;

    /* Round the current pulses frequency up and align */
    if ((CurrPulsesFreq & 0x0F) > 7) CurrPulsesFreq += 0x10;
    CurrPulsesFreq &= ~0x0F;

    /* Reinitialize frequency counters if necessary */
    if (LastPulsesFreq == 0) LastPulsesFreq = CurrPulsesFreq;
    if (FreqPulses     == 0) FreqPulses     = LastPulsesFreq;

    /* Fix up the current pulses frequency if needed */
    if (LastPulsesFreq != 0 && CurrPulsesFreq == 0)
        CurrPulsesFreq = LastPulsesFreq;

    /*
     * Magic begins there...
     */
#define UABS(x) (ULONG)((LONG)(x) < 0 ? -(LONG)(x) : (x))
    if (UABS(CurrPulsesFreq - LastPulsesFreq) > 7)
    {
        /*
         * This can be a "large" fluctuation so ignore it for now, but take
         * it into account if it happens to be a real frequency change.
         */
        CurrPulsesFreq = (CurrPulsesFreq + LastPulsesFreq) >> 1;
    }
    else
    {
        // FreqPulses = ((FreqPulses << 2) + LastPulsesFreq + CurrPulsesFreq) / 6;
        FreqPulses = ((FreqPulses << 1) + LastPulsesFreq + CurrPulsesFreq) >> 2;
    }

    /* Round the pulses frequency up and align */
    if ((FreqPulses & 0x0F) > 7) FreqPulses += 0x10;
    FreqPulses &= ~0x0F;

    DPRINT("FreqPulses = %d, LastPulsesFreq = %d, CurrPulsesFreq = %d, Pulses = %d, Elapsed = %d\n",
           FreqPulses, LastPulsesFreq, CurrPulsesFreq, Pulses, Elapsed);

    LastPulsesFreq = CurrPulsesFreq;
    Pulses = 0;
}


/* PUBLIC FUNCTIONS ***********************************************************/

// SpeakerPulse
VOID SpeakerChange(UCHAR Port61hValue)
{
    static BOOLEAN OldSpeakerOff = TRUE;

    BOOLEAN Timer2Gate = !!(Port61hValue & 0x01);
    BOOLEAN SpeakerOn  = !!(Port61hValue & 0x02);

    DPRINT("SpeakerChange -- Timer2Gate == %s ; SpeakerOn == %s\n",
           Timer2Gate ? "true" : "false", SpeakerOn ? "true" : "false");

    if (Timer2Gate)
    {
        if (SpeakerOn)
        {
            /* Start beeping */
            ULONG Frequency = (PIT_BASE_FREQUENCY / PitGetReloadValue(2));
            if (Frequency < MIN_AUDIBLE_FREQ || MAX_AUDIBLE_FREQ < Frequency)
                Frequency = 0;

            MakeBeep(Frequency, INFINITE);
        }
        else
        {
            /* Stop beeping */
            MakeBeep(0, 0);
        }
    }
    else
    {
        if (SpeakerOn)
        {
            if (OldSpeakerOff)
            {
                OldSpeakerOff = FALSE;
                PulseSample();
            }

            if (FreqPulses >= MIN_AUDIBLE_FREQ)
                MakeBeep(FreqPulses, INFINITE);
            else if (CountStart.QuadPart != 0)
                MakeBeep(CLICK_FREQ, 1); /* Click */
            else
                MakeBeep(0, 0);          /* Stop beeping */
        }
        else
        {
            OldSpeakerOff = TRUE;

            /*
             * Check how far away was the previous pulse and if
             * it was >= (200 + eps) ms away then stop beeping.
             */
            if (GetTickCount() - PulseTickCount >= SPEAKER_RESPONSE + (SPEAKER_RESPONSE >> 3))
            {
                CountStart.QuadPart = 0;
                FreqPulses = 0;

                /* Stop beeping */
                MakeBeep(0, 0);
            }
        }
    }
}

VOID SpeakerInitialize(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING BeepDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Retrieve the performance frequency and initialize the timer ticks */
    NtQueryPerformanceCounter(&CountStart, &FreqCount);
    if (FreqCount.QuadPart == 0)
    {
        wprintf(L"FATAL: Performance counter not available\n");
    }

    /* Open the BEEP device */
    RtlInitUnicodeString(&BeepDevice, L"\\Device\\Beep");
    InitializeObjectAttributes(&ObjectAttributes, &BeepDevice, 0, NULL, NULL);
    Status = NtCreateFile(&hBeep,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the Beep driver, Status 0x%08lx\n", Status);
        // hBeep = INVALID_HANDLE_VALUE;
    }
}

VOID SpeakerCleanup(VOID)
{
    NtClose(hBeep);
}

/* EOF */
