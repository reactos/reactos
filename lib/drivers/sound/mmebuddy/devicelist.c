/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/devicelist.c
 *
 * PURPOSE:     Manages lists of sound devices.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

ULONG           SoundDeviceCounts[SOUND_DEVICE_TYPES];
PSOUND_DEVICE   SoundDeviceLists[SOUND_DEVICE_TYPES];

ULONG
GetSoundDeviceCount(
    IN  MMDEVICE_TYPE DeviceType)
{
    ULONG Index = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    if ( ! IsValidSoundDeviceType(DeviceType) )
    {
        return 0;
    }

    return SoundDeviceCounts[Index];
}

BOOLEAN
IsValidSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    return TRUE;
}

MMRESULT
ListSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  LPWSTR DevicePath,
    OUT PSOUND_DEVICE* SoundDevice)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
UnlistSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  PSOUND_DEVICE SoundDevice)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
UnlistSoundDevices(
    IN  MMDEVICE_TYPE DeviceType)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
UnlistAllSoundDevices()
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
GetSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceIndex,
    OUT PSOUND_DEVICE* Device OPTIONAL)
{
    return MMSYSERR_NOTSUPPORTED;
}
