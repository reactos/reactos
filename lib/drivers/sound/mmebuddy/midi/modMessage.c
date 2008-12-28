/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/midi/modMessage.c
 *
 * PURPOSE:     Provides the modMessage exported function, as required by
 *              the MME API, for MIDI output device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>

#include <mmebuddy.h>

/*
    Standard MME driver entry-point for messages relating to MIDI output.
*/
APIENTRY DWORD
modMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(MIDI_OUT_DEVICE_TYPE);

    SND_TRACE(L"modMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case MODM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(MIDI_OUT_DEVICE_TYPE);
            break;
        }

        case MODM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(MIDI_OUT_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }
    }

    SND_TRACE(L"modMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIDI_OUT_DEVICE_TYPE);

    return Result;
}
