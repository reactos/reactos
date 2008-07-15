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
            TRACE_("GetSoundDevice == %d\n", (int) Result);


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
            TRACE_("CreateSoundDeviceInstance == %d\n", (int) Result);

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
            MMRESULT Result;
            DWORD State;
            //SOUND_DEBUG_HEX(Instance);
            ASSERT(Instance != NULL);

            /* Ensure its OK to close - is this the right state to check? */
            Result = GetWaveDeviceState(Instance, &State);
            ASSERT(Result == MMSYSERR_NOERROR);

            /* Must not be playing or paused */
            if ( State != WAVE_DD_IDLE )
                return WAVERR_STILLPLAYING;

            Result = DestroySoundDeviceInstance(Instance);
            ASSERT(Result == MMSYSERR_NOERROR);

            /* TODO: Send the callback */

            return Result;
        }

        case WODM_WRITE :
        {
            ASSERT(Instance != NULL);

            if ( ! parameter1 )
                return MMSYSERR_INVALPARAM;

            return QueueWaveDeviceBuffer(Instance, (PWAVEHDR) parameter1);
        }

        case WODM_PAUSE :
        {
            ASSERT(Instance != NULL);

            return PauseWaveDevice(Instance);
        }

        case WODM_RESTART :
        {
            ASSERT(Instance != NULL);

            return RestartWaveDevice(Instance);
        }

        case WODM_RESET :
        {
            ASSERT(Instance != NULL);

            return ResetWaveDevice(Instance);
        }

        case WODM_BREAKLOOP :
        {
            ASSERT(Instance != NULL);

            return BreakWaveDeviceLoop(Instance);
        }

        /* Let WINMM take care of these */
        case WODM_PREPARE :
        case WODM_UNPREPARE :
            return MMSYSERR_NOTSUPPORTED;

        /* TODO */
        case WODM_SETVOLUME :
        case WODM_GETVOLUME :
        case WODM_SETPITCH :
        case WODM_GETPITCH :
            return MMSYSERR_NOTSUPPORTED;

        default :
            return MMSYSERR_NOTSUPPORTED;
    }
}
