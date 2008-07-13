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


ULONG
GetSoundDeviceCount(
    IN  UCHAR DeviceType)
{
    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        return 0;
    }

    return SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE];
}


VOID
InitSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE Device,
    IN  PMMFUNCTION_TABLE SourceFunctionTable)
{
    Device->Functions.Constructor = DefaultInstanceConstructor;
    Device->Functions.Destructor = DefaultInstanceDestructor;

    Device->Functions.GetCapabilities = DefaultGetSoundDeviceCapabilities;

    /* Wave device specific */
    Device->Functions.QueryWaveFormat = DefaultQueryWaveDeviceFormatSupport;
    Device->Functions.SetWaveFormat = DefaultSetWaveDeviceFormat;

    Device->Functions.GetWaveDeviceState = DefaultGetWaveDeviceState;
    Device->Functions.PauseWaveDevice = DefaultPauseWaveDevice;
    Device->Functions.RestartWaveDevice = DefaultRestartWaveDevice;

    if ( ! SourceFunctionTable )
        return;

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
}


BOOLEAN
AddSoundDevice(
    IN  UCHAR DeviceType,
    IN  LPWSTR DevicePath,
    IN  PMMFUNCTION_TABLE FunctionTable)
{
    PSOUND_DEVICE NewDevice;
    UCHAR TypeIndex;

    TRACE_("Adding a sound device to list %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
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

    return TRUE;
}


MMRESULT
RemoveSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    ULONG TypeIndex;
    PSOUND_DEVICE CurrentDevice = NULL;
    PSOUND_DEVICE PreviousDevice = NULL;

    /*TRACE_("Removing a sound device from list %d\n", DeviceType);*/

    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    TypeIndex = SoundDevice->DeviceType - MIN_SOUND_DEVICE_TYPE;

    /* Clean up any instances */
    if ( SoundDevice->FirstInstance != NULL )
    {
        DestroyAllInstancesOfSoundDevice(SoundDevice);
    }

    if ( SoundDeviceLists[TypeIndex] == SoundDevice )
    {
        SoundDeviceLists[TypeIndex] = SoundDevice->Next;
    }
    else
    {
        /* Remove from list */
        CurrentDevice = SoundDeviceLists[TypeIndex];
        PreviousDevice = NULL;

        while ( CurrentDevice )
        {
            if ( CurrentDevice == SoundDevice )
            {
                ASSERT(PreviousDevice != NULL);
                PreviousDevice->Next = CurrentDevice->Next;

                break;
            }

            PreviousDevice = CurrentDevice;
            CurrentDevice = CurrentDevice->Next;
        }
    }

    /* Free the memory associated with the device info */
    FreeMemory(SoundDevice->DevicePath);
    FreeMemory(SoundDevice);

    return MMSYSERR_NOERROR;;
}


MMRESULT
RemoveSoundDevices(
    IN  UCHAR DeviceType)
{
    PSOUND_DEVICE CurrentDevice;

    TRACE_("Emptying device list for device type %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        ERR_("Invalid device type - %d\n", DeviceType);
        return MMSYSERR_INVALPARAM;
    }

    /*
        Clean out the device list. This works by repeatedly removing the
        first entry.
    */
    while ( (CurrentDevice =
             SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE]) )
    {
        RemoveSoundDevice(CurrentDevice);
    }

    /* Reset the list content and item count */
    SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE] = NULL;
    SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE] = 0;

    return MMSYSERR_NOERROR;
}


VOID
RemoveAllSoundDevices()
{
    ULONG i;

    TRACE_("Emptying all device lists\n");

    for ( i = MIN_SOUND_DEVICE_TYPE; i <= MAX_SOUND_DEVICE_TYPE; ++ i )
    {
        RemoveSoundDevices(i);
    }
}

BOOLEAN
IsValidSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    /* TODO */
    return ( SoundDevice != NULL );
}


MMRESULT
GetSoundDevice(
    IN  UCHAR DeviceType,
    IN  ULONG DeviceIndex,
    OUT PSOUND_DEVICE* Device)
{
    ULONG Count = 0;
    PSOUND_DEVICE CurrentDevice = NULL;

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
        return MMSYSERR_INVALPARAM;

    if ( DeviceIndex >= SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE] )
        return MMSYSERR_BADDEVICEID;

    if ( ! Device )
        return MMSYSERR_INVALPARAM;

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

    return MMSYSERR_NOERROR;
}


MMRESULT
GetSoundDevicePath(
    IN  PSOUND_DEVICE SoundDevice,
    OUT LPWSTR* DevicePath)
{
    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( ! DevicePath )
        return MMSYSERR_INVALPARAM;

    MessageBox(0, SoundDevice->DevicePath, L"Foo", MB_TASKMODAL | MB_OK);
    *DevicePath = SoundDevice->DevicePath;

    return MMSYSERR_NOERROR;
}


MMRESULT
GetSoundDeviceType(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PUCHAR DeviceType)
{
    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( ! DeviceType )
        return MMSYSERR_INVALPARAM;

    *DeviceType = SoundDevice->DeviceType;

    return MMSYSERR_NOERROR;
}

MMRESULT
GetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PMMFUNCTION_TABLE* FunctionTable)
{
    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( ! FunctionTable )
        return MMSYSERR_INVALPARAM;

    *FunctionTable = &SoundDevice->Functions;

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

    TRACE_("Default instance ctor");

    ASSERT(SoundDeviceInstance != NULL);
    GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    ASSERT(SoundDevice != NULL);

    /* If this fails, we have an internal error somewhere */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    if ( Result != MMSYSERR_NOERROR )
    {
        ASSERT(Result != MMSYSERR_NOERROR);
        return MMSYSERR_ERROR;
    }

    if ( DeviceType == WAVE_OUT_DEVICE_TYPE )
        AccessRights |= GENERIC_WRITE;

    Result = OpenKernelSoundDevice(SoundDevice,
                                   AccessRights,
                                   &SoundDeviceInstance->Handle);
    if ( Result != MMSYSERR_NOERROR )
        return Result;

    TRACE_("Returning from default ctor");

    return MMSYSERR_NOERROR;
}


VOID
DefaultInstanceDestructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;

    TRACE_("Default instance dtor");

    ASSERT(SoundDeviceInstance);
    GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    ASSERT(SoundDevice);

    Result = CloseKernelSoundDevice(SoundDevice);

    ASSERT(Result == MMSYSERR_NOERROR);
}
