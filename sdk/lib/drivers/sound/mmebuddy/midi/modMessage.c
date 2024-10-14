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

#include "precomp.h"

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

        case DRV_QUERYDEVICEINTERFACESIZE :
        {
            Result = MmeGetDeviceInterfaceString(MIDI_OUT_DEVICE_TYPE, DeviceId, NULL, 0, (DWORD*)Parameter1); //FIXME DWORD_PTR
            break;
        }

        case DRV_QUERYDEVICEINTERFACE :
        {
            Result = MmeGetDeviceInterfaceString(MIDI_OUT_DEVICE_TYPE, DeviceId, (LPWSTR)Parameter1, Parameter2, NULL); //FIXME DWORD_PTR
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

    SND_TRACE(L"modMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIDI_OUT_DEVICE_TYPE);

    return Result;
}
