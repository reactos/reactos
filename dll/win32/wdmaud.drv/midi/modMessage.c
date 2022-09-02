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

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

/*
    Standard MME driver entry-point for messages relating to MIDI output.
*/
DWORD
APIENTRY
modMessage(
    UINT DeviceId,
    UINT Message,
    DWORD_PTR PrivateHandle,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(MIDI_OUT_DEVICE_TYPE);

    DPRINT("modMessage - Message type %d\n", Message);

    switch ( Message )
    {
#ifndef USE_MMIXER_LIB
        case MODM_INIT:
        {
            Result = WdmAudAddRemoveDeviceNode(MIDI_OUT_DEVICE_TYPE, TRUE);
            break;
        }

        case DRVM_EXIT:
        {
            Result = WdmAudAddRemoveDeviceNode(MIDI_OUT_DEVICE_TYPE, FALSE);
            break;
        }
#endif

        case MODM_GETNUMDEVS :
        {
            Result = MmeGetNumDevs(MIDI_OUT_DEVICE_TYPE);
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

        case MODM_OPEN :
        {
            Result = MmeOpenDevice(MIDI_OUT_DEVICE_TYPE,
                                   DeviceId,
                                   (LPWAVEOPENDESC) Parameter1, /* unused */
                                   Parameter2,
                                   (DWORD_PTR*)PrivateHandle);
            break;
        }

        case MODM_CLOSE :
        {
            Result = MmeCloseDevice(PrivateHandle);

            break;
        }

    }

    DPRINT("modMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIDI_OUT_DEVICE_TYPE);

    return Result;
}
