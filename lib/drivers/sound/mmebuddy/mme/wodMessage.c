/*
    ReactOS Sound System
    MME Interface

    Purpose:
        Wave output device message handler

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <debug.h>

#include <ntddsnd.h>
#include <mmebuddy.h>

APIENTRY DWORD
wodMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    MMRESULT Result = MMSYSERR_NOERROR;
    PSOUND_DEVICE Device = NULL;
    PSOUND_DEVICE_INSTANCE Instance =
        (PSOUND_DEVICE_INSTANCE)private_handle;

    SOUND_DEBUG(L"wodMessageStub called\n");

    switch ( message )
    {
        case WODM_GETNUMDEVS :
            return GetSoundDeviceCount(WAVE_OUT_DEVICE_TYPE);

        case WODM_GETDEVCAPS :
        {
            UNIVERSAL_CAPS Capabilities;

            if ( parameter2 < sizeof(WAVEOUTCAPS) )
                return MMSYSERR_INVALPARAM;

            Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, device_id, &Device);
            if ( Result != MMSYSERR_NOERROR )
                return Result;

            Result = GetSoundDeviceCapabilities(Device, &Capabilities);
            if ( Result != MMSYSERR_NOERROR )
                return Result;

            CopyMemory((LPWAVEOUTCAPS)parameter1, &Capabilities.WaveOut, parameter2);

            return Result;
        }

        case WODM_OPEN :
        {
            WAVEOPENDESC* OpenParameters = (WAVEOPENDESC*) parameter1;

            SOUND_DEBUG(L"In WODM_OPEN");
            Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, device_id, &Device);
            if ( Result != MMSYSERR_NOERROR )
                return Result;

            if ( parameter2 & WAVE_FORMAT_QUERY )
            {
                Result = QueryWaveDeviceFormatSupport(Device,
                                               OpenParameters->lpFormat,
                                               sizeof(WAVEFORMATEX));

                return Result;
            }

            ASSERT(private_handle != 0);

            Result = CreateSoundDeviceInstance(Device, &Instance);
            if ( Result != MMSYSERR_NOERROR )
                return Result;

            Result = SetWaveDeviceFormat(Instance,
                                         OpenParameters->lpFormat,
                                         sizeof(WAVEFORMATEX));

            if ( Result != MMSYSERR_NOERROR )
            {
                DestroySoundDeviceInstance(Instance);
                return Result;
            }

            /* Start the wave handling thread */
            Result = StartWaveThread(Instance);
            if ( Result != MMSYSERR_NOERROR )
            {
                /* TODO: Do we need to do anything more */
                DestroySoundDeviceInstance(Instance);
                return Result;
            }

            /* Provide winmm with instance handle */
            *((PSOUND_DEVICE_INSTANCE*)private_handle) = Instance;

            /* TODO: Send callback... */

            return MMSYSERR_NOERROR;
        }

        case WODM_CLOSE :
        {
            //SOUND_DEBUG_HEX(Instance);
            SOUND_ASSERT(Instance != NULL);

            /* TODO: Ensure its OK to close */

            Result = StopWaveThread(Instance);
            SOUND_ASSERT(Result == MMSYSERR_NOERROR);

            Result = DestroySoundDeviceInstance(Instance);
            SOUND_DEBUG_HEX(Result);

            return Result;
            /* CloseSoundDevice() */
        }

        case WODM_WRITE :
        {
            SOUND_ASSERT(Instance != NULL);
            SOUND_ASSERT(parameter1 != 0);

            return QueueWaveDeviceBuffer(Instance, (PWAVEHDR) parameter1);
        }

        case WODM_PAUSE :
        case WODM_RESTART :
        case WODM_RESET :
        case WODM_BREAKLOOP :
            return MMSYSERR_INVALHANDLE;

        /* Let WINMM take care of these */
        case WODM_PREPARE :
        case WODM_UNPREPARE :
            return MMSYSERR_NOTSUPPORTED;

        default :
            return MMSYSERR_NOTSUPPORTED;
    }
}
