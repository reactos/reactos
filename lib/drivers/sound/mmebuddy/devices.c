/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Device list manager

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
        5 July 2008 - Implemented format support
        6 July 2008 - Added default instance constructor/destructor
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
    /* Defaults... TODO - Make all these over-rideable! */
    Device->Functions.Constructor = DefaultInstanceConstructor;
    Device->Functions.Destructor = DefaultInstanceDestructor;

    Device->Functions.GetCapabilities = DefaultGetSoundDeviceCapabilities;
    Device->Functions.QueryWaveFormat = DefaultQueryWaveDeviceFormatSupport;
    Device->Functions.SetWaveFormat = DefaultSetWaveDeviceFormat;

    if ( ! SourceFunctionTable )
        return;

    /* If we get here, the function table is being over-ridden */
    if ( SourceFunctionTable->GetCapabilities )
    {
        Device->Functions.GetCapabilities =
            SourceFunctionTable->GetCapabilities;
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
    NewDevice->Handle = INVALID_HANDLE_VALUE;

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

    /* Close handle (if open) */
    if ( SoundDevice->Handle != INVALID_HANDLE_VALUE )
    {
        CloseHandle(SoundDevice->Handle);
        SoundDevice->Handle = INVALID_HANDLE_VALUE;
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
    IN  PSOUND_DEVICE Device,
    OUT PUCHAR DeviceType)
{
    if ( ! Device )
        return MMSYSERR_INVALPARAM;

    if ( ! DeviceType )
        return MMSYSERR_INVALPARAM;

    *DeviceType = Device->DeviceType;

    return MMSYSERR_NOERROR;
}


#include <ntddk.h>      /* How do I avoid this? */

/* Should these go somewhere else? */

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

MMRESULT
QueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    if ( ! Device )
        return MMSYSERR_INVALPARAM;

    if ( ! WaveFormat )
        return MMSYSERR_INVALPARAM;

    /* TODO: Should we check the size? */

    return Device->Functions.QueryWaveFormat(Device, WaveFormat, WaveFormatSize);
}

MMRESULT
DefaultQueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMRESULT Result;
    DWORD BytesReturned = 0;

    if ( ! Device )
        return MMSYSERR_INVALPARAM;

    if ( ! WaveFormat )
        return MMSYSERR_INVALPARAM;

    /* Make sure we have a wave device */
    if ( ! IS_WAVE_DEVICE_TYPE(Device->DeviceType) )
    {
        return MMSYSERR_INVALPARAM;
    }

    Result = WriteSoundDevice(Device,
                              IOCTL_WAVE_QUERY_FORMAT,
                              (LPVOID) WaveFormat,
                              WaveFormatSize,
                              &BytesReturned,
                              NULL);

    return Result;
}

MMRESULT
SetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    if ( ! WaveFormat )
        return MMSYSERR_INVALPARAM;

    /* TODO: Should we check the size? */

    return Instance->Device->Functions.SetWaveFormat(Instance, WaveFormat, WaveFormatSize);
}

MMRESULT
DefaultSetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMRESULT Result;
    DWORD BytesReturned = 0;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    if ( ! WaveFormat )
        return MMSYSERR_INVALPARAM;

    /* Make sure we have a wave device */
    if ( ! IS_WAVE_DEVICE_TYPE(Instance->Device->DeviceType) )
    {
        return MMSYSERR_INVALPARAM;
    }

    Result = WriteSoundDevice(Instance->Device,
                              IOCTL_WAVE_SET_FORMAT,
                              (LPVOID) WaveFormat,
                              WaveFormatSize,
                              &BytesReturned,
                              NULL);

    return Result;
}

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

    Result = OpenKernelSoundDevice(SoundDevice, AccessRights);
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
