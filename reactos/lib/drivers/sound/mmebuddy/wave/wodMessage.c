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

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>
#include <sndtypes.h>

#include <mmebuddy.h>

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
            Result = MmeEnqueueWaveHeader(PrivateHandle, Parameter1);
            break;
        }

        case WODM_GETPOS :
        {
#if 0
            /* Hacky code to test the threading */
            PSOUND_DEVICE_INSTANCE Instance = (PSOUND_DEVICE_INSTANCE)PrivateHandle;
            CallSoundThread(Instance->Thread, HelloWorld, Instance, L"Hello World!");
            CallSoundThread(Instance->Thread, HelloWorld, Instance, L"Hello Universe!");
#endif
            break;
        }
    }

    SND_TRACE(L"wodMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_OUT_DEVICE_TYPE);

    return Result;
}
