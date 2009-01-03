/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/wodMessage.c
 *
 * PURPOSE:     Provides the wodMessage exported function, as required by
 *              the MME API, for wave output device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>

#include <mmebuddy.h>

/*
    Standard MME driver entry-point for messages relating to wave audio
    output.
*/
APIENTRY DWORD
wodMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    SND_TRACE(L"wodMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case WODM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(WAVE_OUT_DEVICE_TYPE);
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
            Result = MmeOpenWaveDevice(WAVE_OUT_DEVICE_TYPE,
                                       DeviceId,
                                       (LPWAVEOPENDESC) Parameter1,
                                       Parameter2,
                                       (DWORD*) PrivateHandle);
            break;
        }

        case WODM_CLOSE :
        {
            Result = MmeCloseDevice(PrivateHandle);

            break;
        }

        case WODM_PREPARE :
        {
            /*Result = MmeWavePrepare(*/
        }

        case WODM_UNPREPARE :
        {
        }
    }

    SND_TRACE(L"wodMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    return Result;
}
