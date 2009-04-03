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
#include <sndtypes.h>

#include <mmebuddy.h>

/*
    Standard MME driver entry-point for messages relating to auxiliary devices.
*/
APIENTRY DWORD
auxMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(AUX_DEVICE_TYPE);

    SND_TRACE(L"auxMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case AUXDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(AUX_DEVICE_TYPE);
            break;
        }

        case AUXDM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(AUX_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }
    }

    SND_TRACE(L"auxMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(AUX_DEVICE_TYPE);

    return Result;
}
