/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/auxiliary/auxMessage.c
 *
 * PURPOSE:     Provides the auxMessage exported function, as required by
 *              the MME API, for auxiliary device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>

#include <mmebuddy.h>

APIENTRY DWORD
auxMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOERROR;

    AcquireEntrypointMutex(AUX_DEVICE_TYPE);

    switch ( Message )
    {
        case AUXDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(AUX_DEVICE_TYPE);
            break;
        }
    }

    ReleaseEntrypointMutex(AUX_DEVICE_TYPE);

    return Result;
}
