/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/widMessage.c
 *
 * PURPOSE:     Provides the widMessage exported function, as required by
 *              the MME API, for wave input device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

/*
    Standard MME driver entry-point for messages relating to wave audio
    input.
*/
DWORD
APIENTRY
widMessage(
    UINT DeviceId,
    UINT Message,
    DWORD_PTR PrivateHandle,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(WAVE_IN_DEVICE_TYPE);

    DPRINT("widMessage - Message type %d\n", Message);

    switch ( Message )
    {
#ifndef USE_MMIXER_LIB
        case WIDM_INIT:
        {
            Result = WdmAudAddRemoveDeviceNode(WAVE_IN_DEVICE_TYPE, TRUE);
            break;
        }

        case DRVM_EXIT:
        {
            Result = WdmAudAddRemoveDeviceNode(WAVE_IN_DEVICE_TYPE, FALSE);
            break;
        }
#endif
        case WIDM_GETNUMDEVS :
        {
            Result = MmeGetNumDevs(WAVE_IN_DEVICE_TYPE);
            break;
        }

        case WIDM_START :
        {
            Result = MmeSetState(PrivateHandle, TRUE);
            break;
        }

        case WIDM_STOP :
        {
            Result = MmeSetState(PrivateHandle, FALSE);
            break;
        }

        case WIDM_GETDEVCAPS :
        {

            Result = MmeGetSoundDeviceCapabilities(WAVE_IN_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }
        case WIDM_OPEN :
        {

            /* Do sanity checks for 'recording' SamplesPerSec value */
            LPWAVEOPENDESC OpenParameters = (LPWAVEOPENDESC)Parameter1;
            if (OpenParameters->lpFormat->nSamplesPerSec > 100000)
                OpenParameters->lpFormat->nSamplesPerSec = 100000;
            if (OpenParameters->lpFormat->nSamplesPerSec < 5000)
                OpenParameters->lpFormat->nSamplesPerSec = 5000;

            Result = MmeOpenDevice(WAVE_IN_DEVICE_TYPE,
                                       DeviceId,
                                       (LPWAVEOPENDESC) Parameter1,
                                       Parameter2,
                                       (DWORD_PTR*) PrivateHandle);
            break;
        }

        case WIDM_CLOSE :
        {
            Result = MmeCloseDevice(PrivateHandle);

            break;
        }

        case WIDM_PREPARE :
        {
            /* TODO: Do we need to pass 2nd parameter? */
            Result = MmePrepareWaveHeader(PrivateHandle, Parameter1);
            break;
        }

        case WIDM_UNPREPARE :
        {
            Result = MmeUnprepareWaveHeader(PrivateHandle, Parameter1);
            break;
        }

        case WIDM_RESET :
        {
            /* Stop playback, reset position to zero */
            Result = MmeResetWavePlayback(PrivateHandle);
            break;
        }

        case WIDM_ADDBUFFER :
        {
            Result = MmeWriteWaveHeader(PrivateHandle, Parameter1);
            break;
        }
    }

    DPRINT("widMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_IN_DEVICE_TYPE);

    return Result;
}
