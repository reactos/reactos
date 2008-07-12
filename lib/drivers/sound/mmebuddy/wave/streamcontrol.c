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

#include <mmebuddy.h>
#include "wave.h"

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
                                QueueBuffer_Request,
                                BufferHeader);
}
