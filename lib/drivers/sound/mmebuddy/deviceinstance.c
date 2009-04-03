/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/deviceinstance.c
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
    /*
        Device is marked as invalid by now, but we can still do some sanity
        checking.
    */
    SND_ASSERT( SoundDeviceInstance->Thread == NULL );

    ZeroMemory(SoundDeviceInstance, sizeof(SOUND_DEVICE_INSTANCE));
    FreeMemory(SoundDeviceInstance);
}

BOOLEAN
IsValidSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE CurrentInstance;

    if ( ! SoundDeviceInstance )
        return FALSE;

    /* GetSoundDeviceFromInstance would send us into a recursive loop... */
    SoundDevice = SoundDeviceInstance->Device;
    SND_ASSERT(SoundDevice);

    CurrentInstance = SoundDevice->HeadInstance;

    while ( CurrentInstance )
    {
        if ( CurrentInstance == SoundDeviceInstance )
            return TRUE;

        CurrentInstance = CurrentInstance->Next;
    }

    return FALSE;
}

MMRESULT
ListSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );

    SND_TRACE(L"Listing sound device instance\n");

    if ( ! SoundDevice->HeadInstance )
    {
        /* First entry - assign to head and tail */
        SoundDevice->HeadInstance = SoundDeviceInstance;
        SoundDevice->TailInstance = SoundDeviceInstance;
    }
    else
    {
        /* Attach to the end */
        SoundDevice->TailInstance->Next = SoundDeviceInstance;
        SoundDevice->TailInstance = SoundDeviceInstance;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
UnlistSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE CurrentInstance, PreviousInstance;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    SND_TRACE(L"Unlisting sound device instance\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( MMSUCCESS(Result) );
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    PreviousInstance = NULL;
    CurrentInstance = SoundDevice->HeadInstance;

    while ( CurrentInstance )
    {
        if ( CurrentInstance == SoundDeviceInstance )
        {
            if ( ! PreviousInstance )
            {
                /* This is the head node */
                SoundDevice->HeadInstance = SoundDevice->HeadInstance->Next;
            }
            else
            {
                /* There are nodes before this one - cut ours out */
                PreviousInstance->Next = CurrentInstance->Next;
            }

            if ( ! CurrentInstance->Next )
            {
                /* This is the tail node */
                SoundDevice->TailInstance = PreviousInstance;
            }
        }

        PreviousInstance = CurrentInstance;
        CurrentInstance = CurrentInstance->Next;
    }

    return MMSYSERR_NOERROR;
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

    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Get the "open" routine from the function table, and validate it */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
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
    (*SoundDeviceInstance)->Next = NULL; 
    (*SoundDeviceInstance)->Device = SoundDevice;
    (*SoundDeviceInstance)->Handle = NULL;
    (*SoundDeviceInstance)->Thread = NULL;

    (*SoundDeviceInstance)->WinMM.Handle = INVALID_HANDLE_VALUE;
    (*SoundDeviceInstance)->WinMM.ClientCallback = 0;
    (*SoundDeviceInstance)->WinMM.ClientCallbackInstanceData = 0;
    (*SoundDeviceInstance)->WinMM.Flags = 0;

    /* Initialise the members of the struct used by the sound thread */
    (*SoundDeviceInstance)->HeadWaveHeader = NULL;
    (*SoundDeviceInstance)->TailWaveHeader = NULL;

    (*SoundDeviceInstance)->OutstandingBuffers = 0;

    (*SoundDeviceInstance)->LoopsRemaining = 0;

    /* Create the streaming thread (TODO - is this for wave only?) */
    Result = CreateSoundThread(&(*SoundDeviceInstance)->Thread);
    if ( ! MMSUCCESS(Result) )
    {
        FreeSoundDeviceInstance(*SoundDeviceInstance);
        return TranslateInternalMmResult(Result);
    }

    /* Add the instance to the list */
    Result = ListSoundDeviceInstance(SoundDevice, *SoundDeviceInstance);
    if ( ! MMSUCCESS(Result) )
    {
        FreeSoundDeviceInstance(*SoundDeviceInstance);
        return TranslateInternalMmResult(Result);
    }

    /* Try and open the device */
    Result = FunctionTable->Open(SoundDevice, (&(*SoundDeviceInstance)->Handle));
    if ( ! MMSUCCESS(Result) )
    {
        UnlistSoundDeviceInstance(*SoundDeviceInstance);
        FreeSoundDeviceInstance(*SoundDeviceInstance);
        return TranslateInternalMmResult(Result);
    }

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
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Get the "close" routine from the function table, and validate it */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    SND_ASSERT( FunctionTable->Close );
    if ( FunctionTable->Close == NULL )
    {
        /* This indicates bad practice, really! If you can open, why not close?! */
        return MMSYSERR_NOTSUPPORTED;
    }

    /* Stop the streaming thread (TODO - is this for wave only?) */
    Result = DestroySoundThread(SoundDeviceInstance->Thread);
    SND_ASSERT( MMSUCCESS(Result) );    /* It should succeed! */
    if ( ! MMSUCCESS(Result ) )
    {
        return TranslateInternalMmResult(Result);
    }

    /* Blank this out here */
    SoundDeviceInstance->Thread = NULL;

    /* Try and close the device */
    Result = FunctionTable->Close(SoundDeviceInstance, Handle);
    SND_ASSERT( MMSUCCESS(Result) );    /* It should succeed! */
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Drop it from the list */
    Result = UnlistSoundDeviceInstance(SoundDeviceInstance);
    SND_ASSERT( MMSUCCESS(Result) );    /* It should succeed! */
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    FreeSoundDeviceInstance(SoundDeviceInstance);

    return MMSYSERR_NOERROR;
}

MMRESULT
DestroyAllSoundDeviceInstances(
    IN  PSOUND_DEVICE SoundDevice)
{
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;

    SoundDeviceInstance = SoundDevice->HeadInstance;

    while ( SoundDeviceInstance )
    {
        Result = DestroySoundDeviceInstance(SoundDeviceInstance);
        SND_ASSERT( MMSUCCESS(Result) );
        SoundDeviceInstance = SoundDeviceInstance->Next;
    }

    return MMSYSERR_NOERROR;
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

MMRESULT
SetSoundDeviceInstanceMmeData(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  HDRVR MmeHandle,
    IN  DWORD ClientCallback,
    IN  DWORD ClientCallbackData,
    IN  DWORD Flags)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    SND_TRACE(L"Setting MME data - handle %x, callback %x, instance data %x, flags %x\n",
              MmeHandle, ClientCallback, ClientCallbackData, Flags);

    SoundDeviceInstance->WinMM.Handle = MmeHandle;
    SoundDeviceInstance->WinMM.ClientCallback = ClientCallback;
    SoundDeviceInstance->WinMM.ClientCallbackInstanceData = ClientCallbackData;
    SoundDeviceInstance->WinMM.Flags = Flags;

    return MMSYSERR_NOERROR;
}
