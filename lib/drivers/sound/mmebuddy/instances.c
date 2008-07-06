/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Device instance manager

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <ntddsnd.h>
#include <debug.h>

#include <mmebuddy.h>

/*
    Instances
*/

MMRESULT
CreateSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PSOUND_DEVICE_INSTANCE* Instance)
{
    PSOUND_DEVICE_INSTANCE NewInstance = NULL;
    PSOUND_DEVICE_INSTANCE CurrentInstance = NULL;

    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    NewInstance = AllocateMemoryFor(SOUND_DEVICE_INSTANCE);
/*
    NewInstance = (PSOUND_DEVICE_INSTANCE)
        HeapAlloc(GetProcessHeap(),
                  HEAP_ZERO_MEMORY,
                  sizeof(SOUND_DEVICE_INSTANCE));
*/

    if ( ! NewInstance )
        return MMSYSERR_NOMEM;

    /* Initialise */
    NewInstance->Next = NULL;
    NewInstance->Device = SoundDevice;
    NewInstance->Thread = NULL;

    /* Search for an appropriate place in the list to put this instance */
    CurrentInstance = SoundDevice->FirstInstance;

    if ( ! CurrentInstance )
    {
        /* This is going to be the first instance */
        SoundDevice->FirstInstance = CurrentInstance;
    }
    else
    {
        /* There is already one or more instances */
        while ( CurrentInstance )
        {
            if ( ! CurrentInstance->Next )
            {
                /* Add to the end and get outta here */
                CurrentInstance->Next = NewInstance;
                break;
            }

            CurrentInstance = CurrentInstance->Next;
        }
    }

    /* Fill the output parameter with this */
    *Instance = NewInstance;

    return MMSYSERR_NOERROR;
}

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    PSOUND_DEVICE_INSTANCE CurrentInstance = NULL;
    PSOUND_DEVICE SoundDevice = NULL;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    SoundDevice = Instance->Device;

    /* TODO - Perform cleanup, stop playback etc. */

    if ( SoundDevice->FirstInstance == Instance )
    {
        /* Deleting the first instance */
        SoundDevice->FirstInstance = NULL;
    }
    else
    {
        /* Deleting an instance beyond the first */
        CurrentInstance = SoundDevice->FirstInstance;

        /* If we hit the end of the list, evidently there's a bug */
        while ( CurrentInstance->Next != Instance )
        {
            CurrentInstance = CurrentInstance->Next;
            ASSERT(CurrentInstance);
        }

        /* This is actually the one before the one we want to remove */
        CurrentInstance->Next = Instance->Next;
    }

    /* Kill it! */
    FreeMemory(Instance);
    /*HeapFree(GetProcessHeap(), 0, Instance);*/

    return MMSYSERR_NOTSUPPORTED;
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

