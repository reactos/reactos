/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/streamcontrol.c
 *
 * PURPOSE:     Controls the wave streaming thread.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>

#include <ntddk.h>
#include <ntddsnd.h>

#include <mmebuddy.h>
#include "wave.h"

MMRESULT
InitWaveStreamData(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PWAVE_STREAM_INFO StreamInfo;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    StreamInfo->State = WAVE_DD_IDLE;

    StreamInfo->BufferQueueHead = NULL;
    StreamInfo->BufferQueueTail = NULL;
    StreamInfo->CurrentBuffer = NULL;

    StreamInfo->BuffersOutstanding = 0;

    return MMSYSERR_NOERROR;
}

MMRESULT
QueueWaveDeviceBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR BufferHeader)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! BufferHeader )
        return MMSYSERR_INVALPARAM;

    if ( ! BufferHeader->lpData )
        return MMSYSERR_INVALPARAM;

    if ( ! BufferHeader->dwBufferLength )
        return MMSYSERR_INVALPARAM;

    if ( ! (BufferHeader->dwFlags & WHDR_PREPARED ) )
        return WAVERR_UNPREPARED;

    /* TODO: WHDR_INQUEUE */

    BufferHeader->dwFlags &= ~WHDR_DONE;
    BufferHeader->lpNext = NULL;

    return CallUsingSoundThread(SoundDeviceInstance,
                                QueueWaveBuffer_Request,
                                BufferHeader);
}

MMRESULT
GetWaveDeviceState(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PUCHAR State)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! State )
        return MMSYSERR_INVALPARAM;

    return CallUsingSoundThread(SoundDeviceInstance,
                                GetWaveDeviceState_Request,
                                State);
}

MMRESULT
PauseWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    /* TODO */

    return MMSYSERR_NOERROR;
}
