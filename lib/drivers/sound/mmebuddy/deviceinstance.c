/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/deviceinstance.c
 *
 * PURPOSE:     Manages instances of sound devices.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmebuddy.h>

MMRESULT
AllocateSoundDeviceInstance(
    OUT PSOUND_DEVICE_INSTANCE* SoundDeviceInstance)
{
    PSOUND_DEVICE_INSTANCE NewInstance;

    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );

    /* Allocate memory for the new instance */
    NewInstance = AllocateStruct(SOUND_DEVICE_INSTANCE);

    if ( ! NewInstance )
        return MMSYSERR_NOMEM;

    /* Provide the caller with the new instance pointer */
    *SoundDeviceInstance = NewInstance;

    return MMSYSERR_NOERROR;
}

VOID
FreeSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    SND_ASSERT( SoundDeviceInstance );
    FreeMemory(SoundDeviceInstance);
}

BOOLEAN
IsValidSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    /* TODO - do this better */
    return ( SoundDeviceInstance != NULL );
}

MMRESULT
ListSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
UnlistSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
CreateSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PSOUND_DEVICE_INSTANCE* SoundDeviceInstance)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
DestroyAllSoundDeviceInstances(
    IN  PSOUND_DEVICE SoundDevice)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
GetSoundDeviceFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PSOUND_DEVICE* SoundDevice)
{
    return MMSYSERR_NOTSUPPORTED;
}
