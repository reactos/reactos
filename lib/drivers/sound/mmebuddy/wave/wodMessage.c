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
#if 0
            PSOUND_DEVICE SoundDevice;
            PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
            LPWAVEOPENDESC OpenDescriptor = (LPWAVEOPENDESC) Parameter1;

            /* FIXME? Do we need the 2nd parameter to go to this routine? */
            Result = MmeQueryWaveDeviceFormatSupport(WAVE_OUT_DEVICE_TYPE,
                                                     DeviceId,
                                                     OpenDescriptor);

            if ( ( Parameter2 & WAVE_FORMAT_QUERY ) ||
                 ( Result == MMSYSERR_NOTSUPPORTED) )
            {
                /* Nothing more to be done */
                break;
            }

            /* The MME API should provide us with a place to store a handle */
            if ( ! PrivateHandle )
            {
                /* Not so much an invalid parameter as something messed up!! */
                SND_ERR(L"MME API supplied a NULL private handle pointer!\n");
                Result = MMSYSERR_ERROR;
                break;
            }

            /* Spawn an instance of the sound device */
            /*Result = MmeOpenWaveDevice(WAVE_OUT_DEVICE_TYPE, DeviceId, OpenDescriptor);*/
            /*GetSoundDevice(WAVE_OUT_DEVICE_TYPE, DeviceId, &SoundDevice);
            Result = CreateSoundDeviceInstance(SoundDevice, &SoundDeviceInstance);*/

            /* TODO... */
#endif
#if 0
            PSOUND_DEVICE SoundDevice;
            PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
            LPWAVEOPENDESC OpenParameters = (LPWAVEOPENDESC) Parameter1;

            /* Obtain the sound device */
            Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, DeviceId, &SoundDevice);

            if ( Result != MMSYSERR_NOERROR )
            {
                Result = TranslateInternalMmResult(Result);
                break;
            }

            /* See if the device supports this format */
            Result = QueryWaveDeviceFormatSupport(SoundDevice,
                                                  OpenParameters->lpFormat,
                                                  sizeof(WAVEFORMATEX));

            if ( Parameter2 & WAVE_FORMAT_QUERY )
            {
                /* Nothing more to be done - keep the result */
                Result = TranslateInternalMmResult(Result);
                break;
            }


            SND_ASSERT( PrivateHandle );
            if ( ! PrivateHandle )
            {
                /* Not so much an invalid parameter as something messed up!! */
                SND_ERR(L"MME API supplied a NULL private handle pointer!\n");
                Result = MMSYSERR_ERROR;
                break;
            }

            /* Spawn an instance of the sound device */
            Result = CreateSoundDeviceInstance(SoundDevice, &SoundDeviceInstance);

            if ( Result != MMSYSERR_NOERROR )
            {
                SND_ERR(L"Failed to create sound device instance\n");
                break;
            }

            /* Set the sample rate, bits per sample, etc. */
            Result = SetWaveDeviceFormat(SoundDeviceInstance,
                                         OpenParameters->lpFormat,
                                         sizeof(WAVEFORMATEX));

            if ( Result != MMSYSERR_NOERROR )
            {
                SND_ERR(L"Failed to set wave device format\n");
                DestroySoundDeviceInstance(SoundDeviceInstance);
                break;
            }

#endif
            break;
        }

        case WODM_CLOSE :
        {
            /*
                What should happen here?
                - Validate the sound device instance
                - Destroy it
            */
            break;
        }
    }

    SND_TRACE(L"wodMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    return Result;
}
