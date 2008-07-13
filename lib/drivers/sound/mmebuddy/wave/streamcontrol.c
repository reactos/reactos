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

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

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
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( BufferHeader );
    VALIDATE_MMSYS_PARAMETER( BufferHeader->lpData );
    VALIDATE_MMSYS_PARAMETER( BufferHeader->dwBufferLength > 0 );
    /* TODO: Check anything more about this buffer? */

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
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( State );

    return CallUsingSoundThread(SoundDeviceInstance,
                                GetWaveDeviceState_Request,
                                State);
}

MMRESULT
DefaultGetWaveDeviceState(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PUCHAR State)
{
//    MMRESULT Result;

    ASSERT(SoundDeviceInstance);
    ASSERT(State);
/*
    Result = WriteSoundDevice(S

MMRESULT
WriteSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID InBuffer,
    DWORD InBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped)
*/
    return MMSYSERR_NOERROR;
}

MMRESULT
PauseWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    return CallUsingSoundThread(SoundDeviceInstance,
                                PauseWaveDevice_Request,
                                NULL);
}

MMRESULT
DefaultPauseWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    TRACE_("Pausing device\n");
    /* TODO */
    return MMSYSERR_NOERROR;
}

MMRESULT
RestartWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    return CallUsingSoundThread(SoundDeviceInstance,
                                RestartWaveDevice_Request,
                                NULL);
}

MMRESULT
DefaultRestartWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    TRACE_("Restarting device\n");
    /* TODO */
    return MMSYSERR_NOERROR;
}
