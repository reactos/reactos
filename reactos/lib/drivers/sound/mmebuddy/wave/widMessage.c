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

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>

#include <mmebuddy.h>

/*
    Standard MME driver entry-point for messages relating to wave audio
    input.
*/
APIENTRY DWORD
widMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(WAVE_IN_DEVICE_TYPE);

    SND_TRACE(L"widMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case WIDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(WAVE_IN_DEVICE_TYPE);
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
    }

    SND_TRACE(L"widMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(WAVE_IN_DEVICE_TYPE);

    return Result;
}
