/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/mixer/mxdMessage.c
 *
 * PURPOSE:     Provides the mxdMessage exported function, as required by
 *              the MME API, for mixer device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>

#include <mmebuddy.h>

/*
    Standard MME driver entry-point for messages relating to mixers.
*/
APIENTRY DWORD
mxdMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(MIXER_DEVICE_TYPE);

    SND_TRACE(L"mxdMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case MXDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(MIXER_DEVICE_TYPE);
            break;
        }

        case MXDM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(MIXER_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }
    }

    SND_TRACE(L"mxdMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIXER_DEVICE_TYPE);

    return Result;
}
