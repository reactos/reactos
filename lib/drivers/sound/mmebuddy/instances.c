/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/instances.c
 *
 * PURPOSE:     Handles construction/destruction of sound device instances,
 *              along with tracking of existing instances.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <ntddsnd.h>

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
    /* TODO: WinMM callback entry */
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
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE CurrentInstance = NULL;

    TRACE_ENTRY();

    ASSERT(SoundDevice != NULL);
    ASSERT(SoundDeviceInstance != NULL);
    ASSERT(SoundDeviceInstance->Device == NULL);

    SoundDeviceInstance->Device = SoundDevice;

    if ( IS_WAVE_DEVICE_TYPE(SoundDevice->DeviceType) )
    {
        Result = InitWaveStreamData(SoundDeviceInstance);
        ASSERT(Result == MMSYSERR_NOERROR);
    }
    else if ( IS_MIDI_DEVICE_TYPE(SoundDevice->DeviceType) )
    {
        /* TODO ... */
        ASSERT(FALSE);
    }
    else if ( IS_MIXER_DEVICE_TYPE(SoundDevice->DeviceType) )
    {
        /* TODO ... */
        ASSERT(FALSE);
    }
    else if ( IS_AUX_DEVICE_TYPE(SoundDevice->DeviceType) )
    {
        /* TODO ... */
        ASSERT(FALSE);
    }
    else
    {
        /* What kind of device do we have, then?!?! */
        ASSERT(FALSE);
    }

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

    TRACE_EXIT(0);
}

VOID
UnlistSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE CurrentInstance;

    TRACE_ENTRY();

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

    TRACE_EXIT(0);
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

    TRACE_ENTRY();

    TRACE_("Creating instance of PSOUND_DEVICE %p\n", SoundDevice);

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );

    CreatedInstance = AllocateSoundDeviceInstance();

    if ( ! CreatedInstance )
    {
        TRACE_EXIT(MMSYSERR_NOMEM);
        return MMSYSERR_NOMEM;
    }

    /* Add the new instance to the device's instance list */
    ListSoundDeviceInstance(SoundDevice, CreatedInstance);

    /* Consult the custom construction function */
    Result = SoundDevice->Functions.Constructor(CreatedInstance);
    if ( Result != MMSYSERR_NOERROR )
    {
        ERR_("Custom ctor returned failure - unlisting");
        UnlistSoundDeviceInstance(CreatedInstance);
        TRACE_("Freeing");
        FreeSoundDeviceInstance(CreatedInstance);
        CreatedInstance = NULL;
        //DestroySoundDeviceInstance(CreatedInstance);

        Result = TranslateInternalMmResult(Result);
    }

    /* Fill the output parameter with this */
    *SoundDeviceInstance = CreatedInstance;

    TRACE_EXIT(Result);

    return Result;
}

MMRESULT
GetSoundDeviceFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PSOUND_DEVICE* SoundDevice)
{
    ASSERT(SoundDeviceInstance);
    ASSERT(SoundDevice);

    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( SoundDevice );

    *SoundDevice = SoundDeviceInstance->Device;

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice = NULL;

    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    SoundDevice = SoundDeviceInstance->Device;

    /* TODO - Perform cleanup, stop playback etc. */

    /* Call the custom destructor */
    SoundDevice->Functions.Destructor(SoundDeviceInstance);

    /* Remove the isntance from the device's instance list */
    UnlistSoundDeviceInstance(SoundDeviceInstance);

    /* Kill it! */
    FreeSoundDeviceInstance(SoundDeviceInstance);
    /*HeapFree(GetProcessHeap(), 0, Instance);*/

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}

MMRESULT
DestroyAllInstancesOfSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    PSOUND_DEVICE_INSTANCE CurrentInstance = NULL;
    MMRESULT Result;

    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );

    /* Just munch away at the first item repeatedly */
    while ( (CurrentInstance = SoundDevice->FirstInstance) )
    {
        Result = DestroySoundDeviceInstance(CurrentInstance);
        ASSERT(Result == MMSYSERR_NOERROR);
    }

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}

BOOLEAN
IsValidSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    /* TRACE_ENTRY(); */

    if ( ! SoundDeviceInstance )
    {
        TRACE_EXIT(FALSE);
        return FALSE;
    }

    /* Now what? */
    return TRUE;

    /* TRACE_EXIT(12345678); */

   TRACE_EXIT(FALSE);
    return ( FALSE );
}

MMRESULT
GetSoundDeviceTypeFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PUCHAR DeviceType)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;

    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( DeviceType );

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( Result != MMSYSERR_NOERROR )
    {
        TRACE_EXIT(Result);
        return Result;
    }

    Result = GetSoundDeviceType(SoundDevice, DeviceType);
    Result = TranslateInternalMmResult(Result);

    TRACE_EXIT(Result);
    return Result;
}
