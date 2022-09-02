/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/wodMessage.c
 *
 * PURPOSE:     Provides the wodMessage exported function, as required by
 *              the MME API, for wave output device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

/*
    Standard MME driver entry-point for messages relating to wave audio
    output.
*/
DWORD
APIENTRY
wodMessage(
    UINT DeviceId,
    UINT Message,
    DWORD_PTR PrivateHandle,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    DPRINT("wodMessage - Message type %d\n", Message);

    switch ( Message )
    {
#ifndef USE_MMIXER_LIB
        case WODM_INIT:
        {
            Result = WdmAudAddRemoveDeviceNode(WAVE_OUT_DEVICE_TYPE, TRUE);
            break;
        }

        case DRVM_EXIT:
        {
            Result = WdmAudAddRemoveDeviceNode(WAVE_OUT_DEVICE_TYPE, FALSE);
            break;
        }
#endif

        case WODM_GETNUMDEVS :
        {
            Result = MmeGetNumDevs(WAVE_OUT_DEVICE_TYPE);
            break;
        }

        case WODM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(WAVE_OUT_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }

        case WODM_OPEN :
        {
            Result = MmeOpenDevice(WAVE_OUT_DEVICE_TYPE,
                                   DeviceId,
                                   (LPWAVEOPENDESC) Parameter1,
                                   Parameter2,
                                   (DWORD_PTR*)PrivateHandle);
            break;
        }

        case WODM_CLOSE :
        {
            Result = MmeCloseDevice(PrivateHandle);

            break;
        }

        case WODM_PREPARE :
        {
            /* TODO: Do we need to pass 2nd parameter? */
            Result = MmePrepareWaveHeader(PrivateHandle, Parameter1);
            break;
        }

        case WODM_UNPREPARE :
        {
            Result = MmeUnprepareWaveHeader(PrivateHandle, Parameter1);
            break;
        }

        case WODM_WRITE :
        {
            Result = MmeWriteWaveHeader(PrivateHandle, Parameter1);
            break;
        }

        case WODM_RESET :
        {
            /* Stop playback, reset position to zero */
            Result = MmeResetWavePlayback(PrivateHandle);
            break;
        }

        case WODM_RESTART :
        {
            /* Continue playback when paused */
            Result = MmeSetState(PrivateHandle, TRUE);
            break;
        }
        case WODM_PAUSE :
        {
            /* Pause playback */
            Result = MmeSetState(PrivateHandle, FALSE);
            break;
        }

        case WODM_GETPOS :
        {
            Result = MmeGetPosition(PrivateHandle, (MMTIME*)Parameter1, Parameter2);
            break;
        }
    }

    DPRINT("wodMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    return Result;
}
