/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Device list manager

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/


/*
    TODO: Free up devicepath on exit
*/

#include <windows.h>
#include <ntddsnd.h>
#include <debug.h>

#include <mmebuddy.h>

/* Device Lists */
ULONG SoundDeviceTotals[SOUND_DEVICE_TYPES];
PSOUND_DEVICE SoundDeviceLists[SOUND_DEVICE_TYPES];


ULONG
GetSoundDeviceCount(
    UCHAR DeviceType)
{
    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        return 0;
    }

    return SoundDeviceTotals[DeviceType];
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
        return MMSYSERR_INVALPARAM;

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


VOID
DestroyAllSoundDevices()
{
    ULONG i;

    DPRINT("Emptying all device lists\n");

    for ( i = 0; i < SOUND_DEVICE_TYPES; ++ i )
    {
        DestroySoundDevices(i);
    }
}


BOOLEAN
DestroySoundDevices(
    UCHAR DeviceType)
{
    PSOUND_DEVICE CurrentDevice;
    PSOUND_DEVICE NextDevice;

    DPRINT("Emptying device list for device type %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        DPRINT("Invalid device type - %d\n", DeviceType);
        return FALSE;
    }

    /* Clean out the device list */
    CurrentDevice = SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE];

    while ( CurrentDevice )
    {
        /* Save the next device pointer so we can reference it later */
        NextDevice = CurrentDevice->Next;

        HeapFree(GetProcessHeap(), 0, CurrentDevice);
        CurrentDevice = NextDevice;
    }

    /* Reset the list content and item count */
    SoundDeviceLists[DeviceType - MIN_SOUND_DEVICE_TYPE] = NULL;
    SoundDeviceTotals[DeviceType - MIN_SOUND_DEVICE_TYPE] = 0;

    return TRUE;
}


VOID
InitSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE Device)
{
    Device->Functions.GetCapabilities = DefaultGetSoundDeviceCapabilities;
}


BOOLEAN
CreateSoundDevice(
    UCHAR DeviceType,
    LPWSTR DevicePath)
{
    PSOUND_DEVICE NewDevice;
    ULONG DevicePathSize = 0;
    UCHAR TypeIndex;

    DPRINT("Adding a sound device to list %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        return FALSE;
    }

    TypeIndex = DeviceType - MIN_SOUND_DEVICE_TYPE;

    NewDevice = (PSOUND_DEVICE)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOUND_DEVICE));

    if ( ! NewDevice )
    {
        return FALSE;
    }

    DevicePathSize = (wcslen(DevicePath) + 1) * sizeof(WCHAR);

    NewDevice->Next = NULL;
    NewDevice->FirstInstance = NULL;
    NewDevice->DeviceType = DeviceType;
    NewDevice->Handle = INVALID_HANDLE_VALUE;

    NewDevice->DevicePath = (LPWSTR)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DevicePathSize);

    if ( ! NewDevice->DevicePath )
    {
        HeapFree(GetProcessHeap(), 0, NewDevice);
        return FALSE;
    }

    CopyMemory(NewDevice->DevicePath, DevicePath, DevicePathSize);

    /* Set up function table */
    InitSoundDeviceFunctionTable(NewDevice);

    /* Start or add to list */
    if ( ! SoundDeviceLists[TypeIndex] )
    {
        DPRINT("Starting device list\n");
        SoundDeviceLists[TypeIndex] = NewDevice;
    }
    else
    {
        PSOUND_DEVICE CurrentDevice = SoundDeviceLists[TypeIndex];

        DPRINT("Adding to device list\n");

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
    DPRINT("Now %d devices of type %d\n", SoundDeviceTotals[TypeIndex], DeviceType);

    return TRUE;
}


BOOLEAN
DestroySoundDevice(
    UCHAR DeviceType,
    ULONG Index)
{
    ULONG Counter = 0;
    ULONG TypeIndex;
    PSOUND_DEVICE CurrentDevice = NULL;
    PSOUND_DEVICE PreviousDevice = NULL;

    DPRINT("Removing a sound device from list %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        return FALSE;
    }

    TypeIndex = DeviceType - MIN_SOUND_DEVICE_TYPE;

    CurrentDevice = SoundDeviceLists[TypeIndex];
    PreviousDevice = NULL;

    while ( CurrentDevice )
    {
        if ( Counter == Index )
        {
            /* Clean up any instances */
            if ( CurrentDevice->FirstInstance != NULL )
            {
                DestroyAllInstancesOfSoundDevice(CurrentDevice);
            }

            /* Close handle (if open) */
            if ( CurrentDevice->Handle != INVALID_HANDLE_VALUE )
            {
                CloseHandle(CurrentDevice->Handle);
                CurrentDevice->Handle = INVALID_HANDLE_VALUE;
            }

            if ( ! PreviousDevice )
            {
                /* Head of list */
                SoundDeviceLists[TypeIndex] = CurrentDevice->Next;
            }
            else
            {
                /* Not the head of list */
                PreviousDevice->Next = CurrentDevice->Next;
            }

            /* Free the memory associated with the device info */
            HeapFree(GetProcessHeap(), 0, CurrentDevice);
            CurrentDevice = NULL;

            DPRINT("Removal succeeded\n");

            return TRUE;
        }

        PreviousDevice = CurrentDevice;
        ++ Counter;
    }

    DPRINT("Not found\n");
    /* Not found */
    return FALSE;
}



/* Should this go somewhere else? */

MMRESULT
GetSoundDeviceCapabilities(
    PSOUND_DEVICE SoundDevice,
    PUNIVERSAL_CAPS Capabilities)
{
    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( ! Capabilities )
        return MMSYSERR_INVALPARAM;

    return SoundDevice->Functions.GetCapabilities(SoundDevice, Capabilities);
}

MMRESULT
IsSoundDeviceFormatSupported(
    IN  PSOUND_DEVICE SoundDevice /* what else? */)
{
    /* TODO */
    return MMSYSERR_NOTSUPPORTED;
}

