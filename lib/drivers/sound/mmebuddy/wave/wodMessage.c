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

    AcquireEntrypointMutex();

    TRACE_("wodMessageStub called\n");
//    MessageBox(0, L"wodMessage", L"wodMessage", MB_OK | MB_TASKMODAL);

    switch ( message )
    {
        case WODM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(WAVE_OUT_DEVICE_TYPE);
            break;
        }

        case WODM_GETDEVCAPS :
        {
            UNIVERSAL_CAPS Capabilities;

            if ( parameter2 < sizeof(WAVEOUTCAPS) )
            {
                Result = MMSYSERR_INVALPARAM;
                break;
            }

            Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, device_id, &Device);
            if ( Result != MMSYSERR_NOERROR )
                break;

            Result = GetSoundDeviceCapabilities(Device, &Capabilities);
            if ( Result != MMSYSERR_NOERROR )
                break;

            CopyMemory((LPWAVEOUTCAPS)parameter1, &Capabilities.WaveOut, parameter2);

            break;
        }

        case WODM_OPEN :
        {
            WAVEOPENDESC* OpenParameters = (WAVEOPENDESC*) parameter1;

            TRACE_("In WODM_OPEN\n");
            Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, device_id, &Device);
            TRACE_("GetSoundDevice == %d\n", (int) Result);

            if ( Result != MMSYSERR_NOERROR )
                break;

            if ( parameter2 & WAVE_FORMAT_QUERY )
            {
                Result = QueryWaveDeviceFormatSupport(Device,
                                               OpenParameters->lpFormat,
                                               sizeof(WAVEFORMATEX));

                break;
            }

            ASSERT(private_handle != 0);

            // HACK
            StartSoundThread();

            Result = CreateSoundDeviceInstance(Device, &Instance);
            TRACE_("CreateSoundDeviceInstance == %d\n", (int) Result);

            if ( Result != MMSYSERR_NOERROR )
                break;

            Result = SetWaveDeviceFormat(Instance,
                                         OpenParameters->lpFormat,
                                         sizeof(WAVEFORMATEX));

            if ( Result != MMSYSERR_NOERROR )
            {
                DestroySoundDeviceInstance(Instance);
                break;
            }

            /* Set up the callback - TODO: Put this somewhere else? */
            Instance->WinMM.ClientCallback = OpenParameters->dwCallback;
            Instance->WinMM.ClientCallbackInstanceData =
                OpenParameters->dwInstance;
            Instance->WinMM.Handle = (HDRVR) OpenParameters->hWave;
            Instance->WinMM.Flags = parameter2;

            /* Provide winmm with instance handle */
            *((PSOUND_DEVICE_INSTANCE*)private_handle) = Instance;

            /* Notify the client */
            NotifySoundClient(Instance, WOM_OPEN, 0);

            Result = MMSYSERR_NOERROR;
            break;
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
            {
                Result = WAVERR_STILLPLAYING;
                break;
            }

            /* Notify the client */
            NotifySoundClient(Instance, WOM_CLOSE, 0);

            Result = DestroySoundDeviceInstance(Instance);
            ASSERT(Result == MMSYSERR_NOERROR);

            // HACK
            StopSoundThread();

            return Result;
        }

        case WODM_WRITE :
        {
            ASSERT(Instance != NULL);

            if ( ! parameter1 )
                Result = MMSYSERR_INVALPARAM;
            else
                Result = QueueWaveDeviceBuffer(Instance, (PWAVEHDR) parameter1);

            break;
        }

        case WODM_PAUSE :
        {
            ASSERT(Instance != NULL);

            Result = PauseWaveDevice(Instance);
            break;
        }

        case WODM_RESTART :
        {
            ASSERT(Instance != NULL);

            Result = RestartWaveDevice(Instance);
            break;
        }

        case WODM_RESET :
        {
            ASSERT(Instance != NULL);

            Result = ResetWaveDevice(Instance);
            break;
        }

        case WODM_BREAKLOOP :
        {
            ASSERT(Instance != NULL);

            Result = BreakWaveDeviceLoop(Instance);
            break;
        }

        /* Let WINMM take care of these */
        case WODM_PREPARE :
        case WODM_UNPREPARE :
            Result = MMSYSERR_NOTSUPPORTED;
            break;

        /* TODO */
        case WODM_SETVOLUME :
        case WODM_GETVOLUME :
        case WODM_SETPITCH :
        case WODM_GETPITCH :
            Result = MMSYSERR_NOTSUPPORTED;
            break;

        default :
            Result = MMSYSERR_NOTSUPPORTED;
            break;
    }

    ReleaseEntrypointMutex();

    return Result;
}
