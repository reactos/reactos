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

static BYTE Port61hState = 0x00;
HANDLE hBeep = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static BYTE SpeakerReadStatus(VOID)
{
    return Port61hState;
}

static VOID SpeakerWriteCommand(BYTE Value)
{
    BOOLEAN IsConnectedToPITChannel2;
    UCHAR   SpeakerData;

    Port61hState = Value;
    IsConnectedToPITChannel2 = ((Port61hState & 0x01) != 0);
    SpeakerData = (Port61hState & 0x02);

    if (PitChannel2 && IsConnectedToPITChannel2)
    {
        /* Set bit 5 of Port 61h */
        Port61hState |= 1 << 5;
    }
    else
    {
        /* Clear bit 5 of Port 61h */
        Port61hState &= ~(1 << 5);
    }

    if (PitChannel2 && IsConnectedToPITChannel2 && (SpeakerData != 0))
    {
        /* Start beeping - Adapted from kernel32:Beep() */
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;
        BEEP_SET_PARAMETERS BeepSetParameters;

        DWORD PitChannel2ReloadValue = PitChannel2->ReloadValue;
        if (PitChannel2ReloadValue == 0) PitChannel2ReloadValue = 65536;

        /* Set beep data */
        BeepSetParameters.Frequency = (PIT_BASE_FREQUENCY / PitChannel2ReloadValue) *
                                      (PitChannel2->Mode == PIT_MODE_SQUARE_WAVE ? 2 : 1);
        BeepSetParameters.Duration  = INFINITE;

        /* Send the beep */
        Status = NtDeviceIoControlFile(hBeep,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_BEEP_SET,
                                       &BeepSetParameters,
                                       sizeof(BeepSetParameters),
                                       NULL,
                                       0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Beep (%lu, %lu) failed, Status 0x%08lx\n",
                    BeepSetParameters.Frequency,
                    BeepSetParameters.Duration,
                    Status);
        }
    }
    else
    {
        /* Stop beeping */
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;
        BEEP_SET_PARAMETERS BeepSetParameters;

        /* Set beep data */
        BeepSetParameters.Frequency = 0x00;
        BeepSetParameters.Duration  = 0x00;

        /* Send the beep */
        Status = NtDeviceIoControlFile(hBeep,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_BEEP_SET,
                                       &BeepSetParameters,
                                       sizeof(BeepSetParameters),
                                       NULL,
                                       0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Beep (%lu, %lu) failed, Status 0x%08lx\n",
                    BeepSetParameters.Frequency,
                    BeepSetParameters.Duration,
                    Status);
        }
    }
}

static BYTE WINAPI SpeakerReadPort(ULONG Port)
{
    return SpeakerReadStatus();
}

static VOID WINAPI SpeakerWritePort(ULONG Port, BYTE Data)
{
    SpeakerWriteCommand(Data);
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID SpeakerInitialize(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING BeepDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Adapted from kernel32:Beep() */

    //
    // On TS systems, we need to Load Winsta.dll and call WinstationBeepOpen
    // after doing a GetProcAddress for it
    //

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

    /* Register the I/O Ports */
    RegisterIoPort(SPEAKER_CONTROL_PORT, SpeakerReadPort, SpeakerWritePort);
}

VOID SpeakerCleanup(VOID)
{
    NtClose(hBeep);
}

/* EOF */
