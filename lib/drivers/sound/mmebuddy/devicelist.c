/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/devicelist.c
 *
 * PURPOSE:     Manages lists of sound devices.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

ULONG           SoundDeviceCounts[SOUND_DEVICE_TYPES];
PSOUND_DEVICE   SoundDeviceListHeads[SOUND_DEVICE_TYPES];
PSOUND_DEVICE   SoundDeviceListTails[SOUND_DEVICE_TYPES];

/*
    Handles the allocation and initialisation of a SOUND_DEVICE structure.
*/
MMRESULT
AllocateSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  PWSTR DevicePath OPTIONAL,
    OUT PSOUND_DEVICE* SoundDevice)
{
    PSOUND_DEVICE NewDevice;

    SND_ASSERT( IsValidSoundDeviceType(DeviceType) );
    SND_ASSERT( SoundDevice );

    SND_TRACE(L"Allocating SOUND_DEVICE for %wS\n", DevicePath);

    NewDevice = AllocateStruct(SOUND_DEVICE);

    if ( ! NewDevice )
        return MMSYSERR_NOMEM;

    NewDevice->Type = DeviceType;

    /* Only copy the device path if one was actually passed in */
    if ( DevicePath )
    {
        NewDevice->Path = AllocateWideString(wcslen(DevicePath));

        if ( ! NewDevice->Path )
        {
            FreeMemory(NewDevice);
            NewDevice = NULL;
            return MMSYSERR_NOMEM;
        }

        CopyWideString(NewDevice->Path, DevicePath);
    }

    /* TODO: Initialise function table */

    /* Return the new structure to the caller and report success */
    *SoundDevice = NewDevice;

    return MMSYSERR_NOERROR;
}

/*
    Handles the cleanup and freeing of a SOUND_DEVICE structure.
*/
VOID
FreeSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    SND_ASSERT( SoundDevice );

    SND_TRACE(L"Freeing SOUND_DEVICE for %wS\n", SoundDevice->Path);

    if ( SoundDevice->Path )
    {
        FreeMemory(SoundDevice->Path);
    }

    FreeMemory(SoundDevice);
}

/*
    Returns the number of devices of the specified type which have been added
    to the device lists. If an invalid device type is specified, the function
    returns zero.
*/
ULONG
GetSoundDeviceCount(
    IN  MMDEVICE_TYPE DeviceType)
{
    ULONG Index = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    if ( ! IsValidSoundDeviceType(DeviceType) )
    {
        return 0;
    }

    return SoundDeviceCounts[Index];
}

/*
    Determines if a sound device structure pointer is valid, firstly by
    ensuring that it is not NULL, and then by checking that the device itself
    exists in one of the device lists.
*/
BOOLEAN
IsValidSoundDevice(
    IN  PSOUND_DEVICE SoundDevice)
{
    if ( ! SoundDevice )
        return FALSE;

    /* TODO */
    return TRUE;
}

/*
    Informs the MME-Buddy library that it should take ownership of a device.
    The DevicePath is typically used for storing a device path (for subsequent
    opening using CreateFile) but it can be a wide-string representing any
    information that makes sense to your MME driver implementation.

    MME components which operate solely in user-mode (for example, MIDI
    loopback devices) won't need to communicate with a kernel-mode device,
    so in these situations DevicePath is likely to be NULL.

    Upon successful addition to the sound device list, the pointer to the new
    device's SOUND_DEVICE structure is returned via SoundDevice.
*/
MMRESULT
ListSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  LPWSTR DevicePath OPTIONAL,
    OUT PSOUND_DEVICE* SoundDevice OPTIONAL)
{
    MMRESULT Result;
    PSOUND_DEVICE NewDevice;
    UCHAR TypeIndex = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceType(DeviceType) );

    Result = AllocateSoundDevice(DeviceType, DevicePath, &NewDevice);

    if ( Result != MMSYSERR_NOERROR )
    {
        SND_ERR(L"Failed to allocate SOUND_DEVICE structure\n");
        return Result;
    }

    if ( ! SoundDeviceListHeads[TypeIndex] )
    {
        SND_TRACE(L"Putting first entry into device list %d\n", DeviceType);
        SoundDeviceListHeads[TypeIndex] = NewDevice;
        SoundDeviceListTails[TypeIndex] = NewDevice;
    }
    else
    {
        SND_TRACE(L"Putting another entry into device list %d\n", DeviceType);
        SoundDeviceListTails[TypeIndex]->Next = NewDevice;
        SoundDeviceListTails[TypeIndex] = NewDevice;
    }

    /* Add to the count */
    ++ SoundDeviceCounts[TypeIndex];

    /* Fill in the caller's PSOUND_DEVICE */
    if ( SoundDevice )
    {
        *SoundDevice = NewDevice;
    }

    return MMSYSERR_NOERROR;
}

/*
    Removes a sound device from the list, and frees the memory associated
    with its description.
*/
MMRESULT
UnlistSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  PSOUND_DEVICE SoundDevice)
{
    PSOUND_DEVICE CurrentDevice, PreviousDevice;

    UCHAR TypeIndex = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceType(DeviceType) );
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );

    PreviousDevice = NULL;
    CurrentDevice = SoundDeviceListHeads[TypeIndex];

    while ( CurrentDevice )
    {
        if ( CurrentDevice == SoundDevice )
        {
            if ( ! PreviousDevice )
            {
                /* This is the head node */
                SND_TRACE(L"Removing head node from device list %d\n", DeviceType);
                SoundDeviceListHeads[TypeIndex] =
                    SoundDeviceListHeads[TypeIndex]->Next;
            }
            else
            {
                SND_TRACE(L"Removing node from device list %d\n", DeviceType);
                /* There are nodes before this one - cut our device out */
                PreviousDevice->Next = CurrentDevice->Next;
            }

            if ( ! CurrentDevice->Next )
            {
                /* This is the tail node */
                SND_TRACE(L"Removing tail node from device list %d\n", DeviceType);
                SoundDeviceListTails[TypeIndex] = PreviousDevice;
            }
        }

        PreviousDevice = CurrentDevice;
        CurrentDevice = CurrentDevice->Next;
    }

    /* Subtract from the count */
    -- SoundDeviceCounts[TypeIndex];

    /* Finally, free up the deleted entry */
    FreeSoundDevice(SoundDevice);

    return MMSYSERR_NOERROR;
}

MMRESULT
UnlistSoundDevices(
    IN  MMDEVICE_TYPE DeviceType)
{
    UCHAR TypeIndex;
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceType(DeviceType) );

    SND_TRACE(L"Unlisting all sound devices of type %d\n", DeviceType);

    TypeIndex = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    /* Munch away at the head of the list until it's drained */
    while ( SoundDeviceCounts[TypeIndex] > 0 )
    {
        MMRESULT Result;
        Result = UnlistSoundDevice(DeviceType, SoundDeviceListHeads[TypeIndex]);
        SND_ASSERT( Result == MMSYSERR_NOERROR );
    }

    return MMSYSERR_NOERROR;
}

VOID
UnlistAllSoundDevices()
{
    MMDEVICE_TYPE Type;

    SND_TRACE(L"Unlisting all sound devices\n");

    for ( Type = MIN_SOUND_DEVICE_TYPE; Type <= MAX_SOUND_DEVICE_TYPE; ++ Type )
    {
        MMRESULT Result;
        Result = UnlistSoundDevices(Type);
        SND_ASSERT( Result == MMSYSERR_NOERROR );
    }
}

MMRESULT
GetSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceIndex,
    OUT PSOUND_DEVICE* Device OPTIONAL)
{
    return MMSYSERR_NOTSUPPORTED;
}
