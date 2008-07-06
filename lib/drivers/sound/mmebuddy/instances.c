/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Device instance manager

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
        6 July 2008 - Re-structured
*/

#include <windows.h>
#include <ntddsnd.h>
#include <debug.h>

#include <mmebuddy.h>

/*
    Init / New / Delete handlers
*/

VOID
InitSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    /* Initialise */
    SoundDeviceInstance->Next = NULL;
    SoundDeviceInstance->Device = NULL;
    SoundDeviceInstance->Thread = NULL;
}

PSOUND_DEVICE_INSTANCE
AllocateSoundDeviceInstance()
{
    PSOUND_DEVICE_INSTANCE ptr;
    ptr = AllocateMemoryFor(SOUND_DEVICE_INSTANCE);

    if ( ! ptr )
        return NULL;

    InitSoundDeviceInstance(ptr);

    return ptr;
}

VOID
FreeSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    FreeMemory(SoundDeviceInstance);
}


/*
    List management
*/

VOID
ListSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE_INSTANCE CurrentInstance = NULL;

    SOUND_ASSERT(SoundDevice != NULL);
    SOUND_ASSERT(SoundDeviceInstance != NULL);
    SOUND_ASSERT(SoundDeviceInstance->Device == NULL);

    SoundDeviceInstance->Device = SoundDevice;

    /* Search for an appropriate place in the list to put this instance */
    if ( ! SoundDevice->FirstInstance )
    {
        /* This is going to be the first instance */
        SoundDevice->FirstInstance = SoundDeviceInstance;
    }
    else
    {
        /* There is already one or more instances */
        CurrentInstance = SoundDevice->FirstInstance;

        while ( CurrentInstance )
        {
            if ( ! CurrentInstance->Next )
            {
                /* Add to the end and get outta here */
                CurrentInstance->Next = SoundDeviceInstance;
                break;
            }

            CurrentInstance = CurrentInstance->Next;
        }
    }
}

VOID
UnlistSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE CurrentInstance;

    ASSERT(SoundDeviceInstance != NULL);
    ASSERT(SoundDeviceInstance->Device != NULL);

    SoundDevice = SoundDeviceInstance->Device;

    if ( SoundDevice->FirstInstance == SoundDeviceInstance )
    {
        /* Removing the first instance */
        SoundDevice->FirstInstance = NULL;
    }
    else
    {
        /* Removing an instance beyond the first */
        CurrentInstance = SoundDevice->FirstInstance;

        /* If we hit the end of the list, evidently there's a bug */
        while ( CurrentInstance->Next != SoundDeviceInstance )
        {
            CurrentInstance = CurrentInstance->Next;
            ASSERT(CurrentInstance != NULL);
        }

        /* This is actually the one before the one we want to remove */
        CurrentInstance->Next = SoundDeviceInstance->Next;
    }
}


/*
    Public routines
*/

MMRESULT
CreateSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PSOUND_DEVICE_INSTANCE* SoundDeviceInstance)
{
    PSOUND_DEVICE_INSTANCE CreatedInstance = NULL;
    MMRESULT Result;

    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    CreatedInstance = AllocateSoundDeviceInstance();
    if ( ! CreatedInstance )
        return MMSYSERR_NOMEM;

    /* Add the new instance to the device's instance list */
    ListSoundDeviceInstance(SoundDevice, CreatedInstance);

    /* Consult the custom construction function */
    Result = SoundDevice->Functions.Constructor(CreatedInstance);
    if ( Result != MMSYSERR_NOERROR )
    {
        SOUND_DEBUG(L"Custom ctor returned failure - unlisting");
        UnlistSoundDeviceInstance(CreatedInstance);
        SOUND_DEBUG(L"Freeing");
        FreeSoundDeviceInstance(CreatedInstance);
        CreatedInstance = NULL;
        //DestroySoundDeviceInstance(CreatedInstance);
    }

    /* Fill the output parameter with this */
    *SoundDeviceInstance = CreatedInstance;

    return Result;
}

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice = NULL;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    SoundDevice = SoundDeviceInstance->Device;

    /* TODO - Perform cleanup, stop playback etc. */

    /* Call the custom destructor */
    SoundDevice->Functions.Destructor(SoundDeviceInstance);

    /* Remove the isntance from the device's instance list */
    UnlistSoundDeviceInstance(SoundDeviceInstance);

    /* Kill it! */
    FreeSoundDeviceInstance(SoundDeviceInstance);
    /*HeapFree(GetProcessHeap(), 0, Instance);*/

    return MMSYSERR_NOERROR;
}

MMRESULT
DestroyAllInstancesOfSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    PSOUND_DEVICE_INSTANCE CurrentInstance = NULL;
    PSOUND_DEVICE_INSTANCE NextInstance = NULL;
    MMRESULT Result;

    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    CurrentInstance = SoundDevice->FirstInstance;

    while ( CurrentInstance )
    {
        NextInstance = CurrentInstance->Next;

        Result = DestroySoundDeviceInstance(CurrentInstance);
        ASSERT(Result == MMSYSERR_NOERROR);
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
GetSoundDeviceFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PSOUND_DEVICE* SoundDevice)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    *SoundDevice = SoundDeviceInstance->Device;

    return MMSYSERR_NOERROR;
}
