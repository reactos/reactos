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
#include <mmddk.h>
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
    SND_ASSERT( IsValidSoundDeviceInstance(SoundDeviceInstance) );
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
    MMRESULT Result;
    PMMFUNCTION_TABLE FunctionTable;

    SND_TRACE(L"Creating a sound device instance\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance != NULL );

    Result = AllocateSoundDeviceInstance(SoundDeviceInstance);

    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    /* Get the "open" routine from the function table, and validate it */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( Result != MMSYSERR_NOERROR )
    {
        FreeSoundDeviceInstance(*SoundDeviceInstance);
        return TranslateInternalMmResult(Result);
    }

    if ( FunctionTable->Open == NULL )
    {
        FreeSoundDeviceInstance(*SoundDeviceInstance);
        return MMSYSERR_NOTSUPPORTED;
    }

    /* Set up the members of the structure */
    (*SoundDeviceInstance)->Device = SoundDevice;

    /* Try and open the device */
    Result = FunctionTable->Open(SoundDevice, (&(*SoundDeviceInstance)->Handle));
    if ( Result != MMSYSERR_NOERROR )
    {
        FreeSoundDeviceInstance(*SoundDeviceInstance);
        return TranslateInternalMmResult(Result);
    }

    /* TODO: List device */

    return MMSYSERR_NOERROR;
}

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    MMRESULT Result;
    PMMFUNCTION_TABLE FunctionTable;
    PSOUND_DEVICE SoundDevice;
    PVOID Handle;

    SND_TRACE(L"Destroying a sound device instance\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    /* TODO: Unlist */

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    /* Get the "close" routine from the function table, and validate it */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    SND_ASSERT( FunctionTable->Close );
    if ( FunctionTable->Close == NULL )
    {
        /* Bad practice, really! If you can open, why not close?! */
        return MMSYSERR_NOTSUPPORTED;
    }

    /* Try and close the device */
    Result = FunctionTable->Close(SoundDeviceInstance, Handle);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    FreeSoundDeviceInstance(SoundDeviceInstance);

    return MMSYSERR_NOERROR;
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
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( SoundDevice );

    *SoundDevice = SoundDeviceInstance->Device;

    return MMSYSERR_NOERROR;
}

MMRESULT
GetSoundDeviceInstanceHandle(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PVOID* Handle)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Handle );

    *Handle = SoundDeviceInstance->Handle;

    return MMSYSERR_NOERROR;
}
