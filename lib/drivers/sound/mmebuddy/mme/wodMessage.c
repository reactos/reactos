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
    PSOUND_DEVICE_INSTANCE Instance = NULL;
    DPRINT("wodMessageStub called\n");

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

            /* TODO: Provide winmm with instance handle */
            /* TODO: Send callback... */

            return MMSYSERR_NOERROR;
        }

        case WODM_CLOSE :
            /* CloseSoundDevice() */

        case WODM_WRITE :
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
