/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/capabilities.c
 *
 * PURPOSE:     Queries sound devices for their capabilities.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddk.h>      /* needed for ioctl stuff */
#include <ntddsnd.h>

#include <mmebuddy.h>

MMRESULT
GetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE Device,
    OUT PUNIVERSAL_CAPS Capabilities)
{
    if ( ! Device )
        return MMSYSERR_INVALPARAM;

    if ( ! Capabilities )
        return MMSYSERR_INVALPARAM;

    return Device->Functions.GetCapabilities(Device, Capabilities);
}

MMRESULT
DefaultGetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE Device,
    OUT PUNIVERSAL_CAPS Capabilities)
{
    PVOID RawCapsPtr = NULL;
    ULONG CapsSize = 0;
    DWORD Ioctl;
    MMRESULT Result;
    DWORD BytesReturned;

    ZeroMemory(Capabilities, sizeof(UNIVERSAL_CAPS));

    if ( ! Device )
        return MMSYSERR_INVALPARAM;

    if ( ! Capabilities )
        return MMSYSERR_INVALPARAM;

    /* Select appropriate IOCTL and capabilities structure */
    switch ( Device->DeviceType )
    {
        case WAVE_OUT_DEVICE_TYPE :
            Ioctl = IOCTL_WAVE_GET_CAPABILITIES;
            RawCapsPtr = (PVOID) &Capabilities->WaveOut;
            CapsSize = sizeof(WAVEOUTCAPS);
            break;

        case WAVE_IN_DEVICE_TYPE :
            Ioctl = IOCTL_WAVE_GET_CAPABILITIES;
            RawCapsPtr = (PVOID) &Capabilities->WaveIn;
            CapsSize = sizeof(WAVEINCAPS);
            break;

        case MIDI_OUT_DEVICE_TYPE :
            Ioctl = IOCTL_MIDI_GET_CAPABILITIES;
            RawCapsPtr = (PVOID) &Capabilities->MidiOut;
            CapsSize = sizeof(MIDIOUTCAPS);
            break;

        case MIDI_IN_DEVICE_TYPE :
            Ioctl = IOCTL_MIDI_GET_CAPABILITIES;
            RawCapsPtr = (PVOID) &Capabilities->MidiIn;
            CapsSize = sizeof(MIDIINCAPS);
            break;

        case MIXER_DEVICE_TYPE :
            /* TODO */
            /*Ioctl = IOCTL_MIX_GET_CAPABILITIES;*/
            return MMSYSERR_NOTSUPPORTED;

        case AUX_DEVICE_TYPE :
            /* TODO */
            Ioctl = IOCTL_AUX_GET_CAPABILITIES;
            return MMSYSERR_NOTSUPPORTED;

        default :
            return MMSYSERR_NOTSUPPORTED;
    }

    /* Call the driver */
    Result = ReadSoundDevice(
        Device,
        Ioctl,
        (LPVOID) RawCapsPtr,
        CapsSize,
        &BytesReturned,
        NULL);

    return Result;
}
