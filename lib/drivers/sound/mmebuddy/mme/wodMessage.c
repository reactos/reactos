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

    TRACE_("wodMessageStub called\n");

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

            TRACE_("In WODM_OPEN\n");
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

            /* Provide winmm with instance handle */
            *((PSOUND_DEVICE_INSTANCE*)private_handle) = Instance;

            /* TODO: Send callback... */

            return MMSYSERR_NOERROR;
        }

        case WODM_CLOSE :
        {
            //SOUND_DEBUG_HEX(Instance);
            ASSERT(Instance != NULL);

            /* TODO: Ensure its OK to close */

            Result = DestroySoundDeviceInstance(Instance);
            /*SOUND_DEBUG_HEX(Result);*/

            /* TODO: When do we send the callback? */

            return Result;
            /* CloseSoundDevice() */
        }

        case WODM_WRITE :
        {
            ASSERT(Instance != NULL);
            ASSERT(parameter1 != 0);

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
