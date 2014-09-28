/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            speaker.c
 * PURPOSE:         PC Speaker emulation
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "speaker.h"
#include "io.h"
#include "timer.h"

/* Extra PSDK/NDK Headers */
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

/* DDK Driver Headers */
#include <ntddbeep.h>

/* PRIVATE VARIABLES **********************************************************/

static HANDLE hBeep = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static DWORD OldReloadValue = 0;
static PIT_MODE OldMode = 0;

/* PUBLIC FUNCTIONS ***********************************************************/

VOID PlaySound(DWORD Frequency,
               DWORD Duration)
{
    /* Adapted from kernel32:Beep() */

    IO_STATUS_BLOCK IoStatusBlock;
    BEEP_SET_PARAMETERS BeepSetParameters;

    /* Set beep data */
    BeepSetParameters.Frequency = Frequency;
    BeepSetParameters.Duration  = Duration;

    /* Send the beep */
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

VOID SpeakerChange(VOID)
{
    BYTE    Port61hState = IOReadB(CONTROL_SYSTEM_PORT61H);
    BOOLEAN IsConnectedToPITChannel2 = !!(Port61hState & 0x01);
    BOOLEAN SpeakerDataOn = !!(Port61hState & 0x02);

    if (PitChannel2 && IsConnectedToPITChannel2 && SpeakerDataOn)
    {
        /* Start beeping */

        DWORD Frequency, Duration;

        DWORD PitChannel2ReloadValue = PitChannel2->ReloadValue;
        if (PitChannel2ReloadValue == 0) PitChannel2ReloadValue = 65536;

        DPRINT("(1) PitChannel2(Mode = %d ; ReloadValue = %d)\n", PitChannel2->Mode, PitChannel2ReloadValue);

        if (OldMode == PitChannel2->Mode && OldReloadValue == PitChannel2ReloadValue)
            return;

        OldMode = PitChannel2->Mode;
        OldReloadValue = PitChannel2ReloadValue;

        DPRINT("(2) PitChannel2(Mode = %d ; ReloadValue = %d)\n", PitChannel2->Mode, PitChannel2ReloadValue);

        Frequency = (PIT_BASE_FREQUENCY / PitChannel2ReloadValue);
        Duration  = INFINITE;

        PlaySound(Frequency, Duration);
    }
    else
    {
        /* Stop beeping */

        OldMode = 0;
        OldReloadValue = 0;

        PlaySound(0x00, 0x00);
    }
}

VOID SpeakerInitialize(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING BeepDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Adapted from kernel32:Beep() */

    /* Open the device */
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
        DPRINT1("Failed to open Beep driver, Status 0x%08lx\n", Status);
    }
}

VOID SpeakerCleanup(VOID)
{
    NtClose(hBeep);
}

/* EOF */
