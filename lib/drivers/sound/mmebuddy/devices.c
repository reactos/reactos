/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/devices.c
 *
 * PURPOSE:     Manages lists of sound devices.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

/*
    TODO:
        The removal of devices from the list needs to be separated from
        the destruction of the device structure.
*/

#include <windows.h>
#include <ntddsnd.h>

#include <mmebuddy.h>

/* Device Lists */
ULONG SoundDeviceTotals[SOUND_DEVICE_TYPES];
PSOUND_DEVICE SoundDeviceLists[SOUND_DEVICE_TYPES];

#define DEVICE_TYPE_TO_INDEX(device_type) \
    ( device_type - MIN_SOUND_DEVICE_TYPE )


ULONG
GetSoundDeviceCount(
    IN  UCHAR DeviceType)
{
    ULONG Count;
    TRACE_ENTRY();

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        TRACE_EXIT(0);
        return 0;
    }

    Count = SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE];

    TRACE_EXIT(Count);
    return Count;
}


VOID
InitSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE Device,
    IN  PMMFUNCTION_TABLE SourceFunctionTable)
{
    TRACE_ENTRY();

    Device->Functions.Constructor = DefaultInstanceConstructor;
    Device->Functions.Destructor = DefaultInstanceDestructor;

    Device->Functions.GetCapabilities = DefaultGetSoundDeviceCapabilities;

    /* Wave device specific */
    Device->Functions.QueryWaveFormat = DefaultQueryWaveDeviceFormatSupport;
    Device->Functions.SetWaveFormat = DefaultSetWaveDeviceFormat;

    Device->Functions.GetWaveDeviceState = DefaultGetWaveDeviceState;
    Device->Functions.PauseWaveDevice = DefaultPauseWaveDevice;
    Device->Functions.RestartWaveDevice = DefaultRestartWaveDevice;
    Device->Functions.ResetWaveDevice = DefaultResetWaveDevice;

    if ( ! SourceFunctionTable )
    {
        TRACE_EXIT(0);
        return;
    }

    /* If we get here, the function table is being over-ridden */

    if ( SourceFunctionTable->Constructor )
    {
        Device->Functions.Constructor =
            SourceFunctionTable->Constructor;
    }

    if ( SourceFunctionTable->Destructor )
    {
        Device->Functions.Destructor =
            SourceFunctionTable->Destructor;
    }

    if ( SourceFunctionTable->GetCapabilities )
    {
        Device->Functions.GetCapabilities =
            SourceFunctionTable->GetCapabilities;
    }

    if ( SourceFunctionTable->QueryWaveFormat )
    {
        Device->Functions.QueryWaveFormat =
            SourceFunctionTable->QueryWaveFormat;
    }

    if ( SourceFunctionTable->SetWaveFormat )
    {
        Device->Functions.SetWaveFormat =
            SourceFunctionTable->SetWaveFormat;
    }

    if ( SourceFunctionTable->GetWaveDeviceState )
    {
        Device->Functions.GetWaveDeviceState =
            SourceFunctionTable->GetWaveDeviceState;
    }

    if ( SourceFunctionTable->PauseWaveDevice )
    {
        Device->Functions.PauseWaveDevice =
            SourceFunctionTable->PauseWaveDevice;
    }

    if ( SourceFunctionTable->RestartWaveDevice )
    {
        Device->Functions.RestartWaveDevice =
            SourceFunctionTable->RestartWaveDevice;
    }

    if ( SourceFunctionTable->ResetWaveDevice )
    {
        Device->Functions.ResetWaveDevice =
            SourceFunctionTable->ResetWaveDevice;
    }

    TRACE_EXIT(0);
}


BOOLEAN
AddSoundDevice(
    IN  UCHAR DeviceType,
    IN  LPWSTR DevicePath,
    IN  PMMFUNCTION_TABLE FunctionTable)
{
    PSOUND_DEVICE NewDevice;
    UCHAR TypeIndex;

    TRACE_ENTRY();

    TRACE_("Adding a sound device to list %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        TRACE_EXIT(FALSE);
        return FALSE;
    }

    TypeIndex = DeviceType - MIN_SOUND_DEVICE_TYPE;

    NewDevice = AllocateMemoryFor(SOUND_DEVICE);
/*
    NewDevice = (PSOUND_DEVICE)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOUND_DEVICE));
*/

    if ( ! NewDevice )
    {
        TRACE_EXIT(FALSE);
        return FALSE;
    }

    NewDevice->Next = NULL;
    NewDevice->FirstInstance = NULL;
    NewDevice->DeviceType = DeviceType;

    NewDevice->DevicePath = AllocateWideString(wcslen(DevicePath));
/*
    NewDevice->DevicePath = (LPWSTR)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DevicePathSize);
*/

    if ( ! NewDevice->DevicePath )
    {
        FreeMemory(NewDevice);
        /*HeapFree(GetProcessHeap(), 0, NewDevice);*/
        TRACE_EXIT(FALSE);
        return FALSE;
    }

    CopyWideString(NewDevice->DevicePath, DevicePath);
    /*CopyMemory(NewDevice->DevicePath, DevicePath, DevicePathSize);*/

    /* Set up function table */
    InitSoundDeviceFunctionTable(NewDevice, FunctionTable);

    /* Start or add to list */
    if ( ! SoundDeviceLists[TypeIndex] )
    {
        TRACE_("Starting device list\n");
        SoundDeviceLists[TypeIndex] = NewDevice;
    }
    else
    {
        PSOUND_DEVICE CurrentDevice = SoundDeviceLists[TypeIndex];

        TRACE_("Adding to device list\n");

        while ( CurrentDevice != NULL )
        {
            if ( ! CurrentDevice->Next )
            {
                CurrentDevice->Next = NewDevice;
                break;
            }

            CurrentDevice = CurrentDevice->Next;
        }
    }

    ++ SoundDeviceTotals[TypeIndex];
    TRACE_("Now %d devices of type %d\n", (int) SoundDeviceTotals[TypeIndex], DeviceType);

    TRACE_EXIT(TRUE);
    return TRUE;
}


MMRESULT
RemoveSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    ULONG TypeIndex;
    BOOLEAN Done = FALSE;
    PSOUND_DEVICE CurrentDevice = NULL;
    PSOUND_DEVICE PreviousDevice = NULL;

    /*TRACE_("Removing a sound device from list %d\n", DeviceType);*/
    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );

    TypeIndex = SoundDevice->DeviceType - MIN_SOUND_DEVICE_TYPE;

    /* Clean up any instances */
    if ( SoundDevice->FirstInstance != NULL )
    {
        TRACE_("About to destroy all instances of this sound device\n");
        DestroyAllInstancesOfSoundDevice(SoundDevice);
    }

    if ( SoundDeviceLists[TypeIndex] == SoundDevice )
    {
        TRACE_("Removing head of list\n");
        SoundDeviceLists[TypeIndex] = SoundDevice->Next;
        Done = TRUE;
    }
    else
    {
        /* Remove from list */
        CurrentDevice = SoundDeviceLists[TypeIndex];
        PreviousDevice = NULL;

        TRACE_("Removing from list\n");

        while ( CurrentDevice )
        {
            if ( CurrentDevice == SoundDevice )
            {
                ASSERT(PreviousDevice != NULL);
                PreviousDevice->Next = CurrentDevice->Next;
                Done = TRUE;

                break;
            }

            PreviousDevice = CurrentDevice;
            CurrentDevice = CurrentDevice->Next;
        }
    }

    ASSERT(Done);

    TRACE_("Freeing path at %p\n", SoundDevice->DevicePath);
    /* Free the memory associated with the device info */
    FreeMemory(SoundDevice->DevicePath);

    TRACE_("Freeing struct\n");
    FreeMemory(SoundDevice);

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;;
}


MMRESULT
RemoveSoundDevices(
    IN  UCHAR DeviceType)
{
    MMRESULT Result;
    PSOUND_DEVICE CurrentDevice;

    TRACE_ENTRY();

    TRACE_("Emptying device list for device type %d\n", DeviceType);

    VALIDATE_MMSYS_PARAMETER( VALID_SOUND_DEVICE_TYPE(DeviceType) );

    /*
        Clean out the device list. This works by repeatedly removing the
        first entry.
    */
    while ( (CurrentDevice =
             SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE]) )
    {
        Result = RemoveSoundDevice(CurrentDevice);
        ASSERT(Result == MMSYSERR_NOERROR);
    }

    /* Reset the list content and item count */
    SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE] = NULL;
    SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE] = 0;

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}


VOID
RemoveAllSoundDevices()
{
    ULONG i;
    TRACE_ENTRY();

    TRACE_("RemoveAllSoundDevices\n");

    for ( i = MIN_SOUND_DEVICE_TYPE; i <= MAX_SOUND_DEVICE_TYPE; ++ i )
    {
        RemoveSoundDevices(i);
    }

    TRACE_EXIT(0);
}

BOOLEAN
IsValidSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    UCHAR DeviceType;
    PSOUND_DEVICE CurrentDevice;

    /* TRACE_ENTRY(); */

    if ( ! SoundDevice )
    {
        TRACE_EXIT(FALSE);
        return FALSE;
    }

    for ( DeviceType = MIN_SOUND_DEVICE_TYPE;
          DeviceType <= MAX_SOUND_DEVICE_TYPE;
          ++DeviceType )
    {
        CurrentDevice = SoundDeviceLists[DEVICE_TYPE_TO_INDEX(DeviceType)];

        while ( CurrentDevice )
        {
            if ( CurrentDevice == SoundDevice )
            {
                /* TRACE_EXIT(TRUE); */
                return TRUE;
            }

            CurrentDevice = CurrentDevice->Next;
        }
    }

    /* Not found in list */
    TRACE_EXIT(FALSE);
    return FALSE;
}


MMRESULT
GetSoundDevice(
    IN  UCHAR DeviceType,
    IN  ULONG DeviceIndex,
    OUT PSOUND_DEVICE* Device)
{
    ULONG Count = 0;
    PSOUND_DEVICE CurrentDevice = NULL;

    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( VALID_SOUND_DEVICE_TYPE(DeviceType) );
    VALIDATE_MMSYS_PARAMETER( Device );

    if ( DeviceIndex >= SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE] )
    {
        TRACE_EXIT(MMSYSERR_BADDEVICEID);
        return MMSYSERR_BADDEVICEID;
    }

    /*
        We know by now that a device at the index should exist
        so just loop around until we reach that index.
    */

    CurrentDevice = SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE];

    for ( Count = 0; Count <= DeviceIndex; ++ Count )
    {
        *Device = CurrentDevice;
        CurrentDevice = CurrentDevice->Next;
    }

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}


MMRESULT
GetSoundDevicePath(
    IN  PSOUND_DEVICE SoundDevice,
    OUT LPWSTR* DevicePath)
{
    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( DevicePath );

    MessageBox(0, SoundDevice->DevicePath, L"Foo", MB_TASKMODAL | MB_OK);
    *DevicePath = SoundDevice->DevicePath;

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}


MMRESULT
GetSoundDeviceType(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PUCHAR DeviceType)
{
    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( DeviceType );

    *DeviceType = SoundDevice->DeviceType;

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}

MMRESULT
GetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PMMFUNCTION_TABLE* FunctionTable)
{
    TRACE_ENTRY();

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( FunctionTable );

    *FunctionTable = &SoundDevice->Functions;

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}


#include <ntddk.h>      /* How do I avoid this? */

/* Should these go somewhere else? */

MMRESULT
DefaultInstanceConstructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    UCHAR DeviceType;
    DWORD AccessRights = GENERIC_READ;
    MMRESULT Result;

    TRACE_ENTRY();

    ASSERT(SoundDeviceInstance != NULL);
    GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    ASSERT(SoundDevice != NULL);

    /* If this fails, we have an internal error somewhere */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    ASSERT(Result == MMSYSERR_NOERROR);
    if ( Result != MMSYSERR_NOERROR )
    {
        Result = TranslateInternalMmResult(Result);
        TRACE_EXIT(Result);
        return Result;
    }

    if ( DeviceType == WAVE_OUT_DEVICE_TYPE )
        AccessRights |= GENERIC_WRITE;

    Result = OpenKernelSoundDevice(SoundDevice,
                                   AccessRights,
                                   &SoundDeviceInstance->Handle);

    if ( Result != MMSYSERR_NOERROR )
    {
        Result = TranslateInternalMmResult(Result);
        TRACE_EXIT(MMSYSERR_NOERROR);
        return Result;
    }

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}


VOID
DefaultInstanceDestructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;

    TRACE_ENTRY();

    ASSERT(SoundDeviceInstance);
    GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    ASSERT(SoundDevice);

    Result = CloseKernelSoundDevice(SoundDevice);
    ASSERT(Result == MMSYSERR_NOERROR);

    TRACE_EXIT(0);
}
