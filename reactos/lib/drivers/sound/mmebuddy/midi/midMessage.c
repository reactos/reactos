/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/midi/midMessage.c
 *
 * PURPOSE:     Provides the midMessage exported function, as required by
 *              the MME API, for MIDI input device support.
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
    Standard MME driver entry-point for messages relating to MIDI input.
*/
APIENTRY DWORD
midMessage(
    UINT DeviceId,
    UINT Message,
    DWORD_PTR PrivateHandle,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(MIDI_IN_DEVICE_TYPE);

    SND_TRACE(L"midMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case MIDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(MIDI_IN_DEVICE_TYPE);
            break;
        }

        case MIDM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(MIDI_IN_DEVICE_TYPE,
                                                   DeviceId,
                                                   Parameter1,
                                                   Parameter2);
            break;
        }
    }

    SND_TRACE(L"midMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIDI_IN_DEVICE_TYPE);

    return Result;
}
