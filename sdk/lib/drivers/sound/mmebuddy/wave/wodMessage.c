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

#include "precomp.h"

#if 0
MMRESULT HelloWorld(PSOUND_DEVICE_INSTANCE Instance, PVOID String)
{
    PWSTR WString = (PWSTR) String;
    SND_TRACE(WString);
    return MMSYSERR_NOTSUPPORTED;
}
#endif

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
            /* pause playback */
            Result = MmeSetState(PrivateHandle, FALSE);
            break;
        }

        case WODM_GETPOS :
        {
            Result = MmeGetPosition(WAVE_OUT_DEVICE_TYPE, DeviceId, PrivateHandle, (MMTIME*)Parameter1, Parameter2);
            break;
        }

        case WODM_GETVOLUME:
        {
            Result = MmeGetVolume(WAVE_OUT_DEVICE_TYPE,
                                  DeviceId,
                                  PrivateHandle,
                                  Parameter1);
            break;
        }

        case WODM_SETVOLUME:
        {
            Result = MmeSetVolume(WAVE_OUT_DEVICE_TYPE,
                                  DeviceId,
                                  PrivateHandle,
                                  Parameter1);
            break;
        }

        case DRV_QUERYDEVICEINTERFACESIZE :
        {
            Result = MmeGetDeviceInterfaceString(WAVE_OUT_DEVICE_TYPE, DeviceId, NULL, 0, (DWORD*)Parameter1); //FIXME DWORD_PTR
            break;
        }

        case DRV_QUERYDEVICEINTERFACE :
        {
            Result = MmeGetDeviceInterfaceString(WAVE_OUT_DEVICE_TYPE, DeviceId, (LPWSTR)Parameter1, Parameter2, NULL); //FIXME DWORD_PTR
            break;
        }
    }

    SND_TRACE(L"wodMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    return Result;
}
