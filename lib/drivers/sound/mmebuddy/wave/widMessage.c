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

APIENTRY DWORD
widMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOERROR;

    AcquireEntrypointMutex(WAVE_IN_DEVICE_TYPE);

    switch ( Message )
    {
        case WIDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(WAVE_IN_DEVICE_TYPE);
            break;
        }
    }

    ReleaseEntrypointMutex(WAVE_IN_DEVICE_TYPE);

    return Result;
}
