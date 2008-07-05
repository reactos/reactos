/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Legacy (NT4) sound device support

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

/*
    A better way of detecting sound devices...
    Search the appropriate registry key!
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddsnd.h>
#include <debug.h>

#include <mmebuddy.h>

/*
    Open the parameters key of a sound driver.
    NT4 only.
*/
MMRESULT
OpenSoundDriverParametersRegKey(
    IN  LPWSTR ServiceName,
    OUT PHKEY KeyHandle)
{
    ULONG KeyLength;
    PWCHAR ParametersKeyName;

    if ( ! ServiceName )
        return MMSYSERR_INVALPARAM;

    if ( ! KeyHandle )
        return MMSYSERR_INVALPARAM;

    /* Work out how long the string will be */
    KeyLength = wcslen(REG_SERVICES_KEY_NAME_U) + 1
              + wcslen(ServiceName) + 1
              + wcslen(REG_PARAMETERS_KEY_NAME_U) + 1;

    KeyLength *= sizeof(WCHAR);

    /* Allocate memory for the string */
    ParametersKeyName = (PWCHAR) HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           KeyLength);

    if ( ! ParametersKeyName )
        return MMSYSERR_NOMEM;

    /* Construct the registry path */
    wsprintf(ParametersKeyName,
             L"%s\\%s\\%s",
             REG_SERVICES_KEY_NAME_U,
             ServiceName,
             REG_PARAMETERS_KEY_NAME_U);

    MessageBox(0, ParametersKeyName, L"Parameters key is...", MB_OK | MB_TASKMODAL);

    /* Perform the open */
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      ParametersKeyName,
                      0,
                      KEY_READ,
                      KeyHandle) != ERROR_SUCCESS )
    {
        /* Couldn't open the key */
        HeapFree(GetProcessHeap(), 0, ParametersKeyName);
        return MMSYSERR_ERROR;
    }

    HeapFree(GetProcessHeap(), 0, ParametersKeyName);

    return MMSYSERR_NOERROR;
}

/*
    Open one of the Device sub-keys belonging to the sound driver.
    NT4 only.
*/
MMRESULT
OpenSoundDeviceRegKey(
    IN  LPWSTR ServiceName,
    IN  DWORD DeviceIndex,
    OUT PHKEY KeyHandle)
{
    DWORD PathSize;
    PWCHAR RegPath;

    if ( ! ServiceName )
        return MMSYSERR_INVALPARAM;

    if ( ! KeyHandle )
        return MMSYSERR_INVALPARAM;

    /*
        Work out the space required to hold the path:

        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\
            sndblst\
                Parameters\
                    Device123\
    */
    PathSize = wcslen(REG_SERVICES_KEY_NAME_U) + 1
             + wcslen(ServiceName) + 1
             + wcslen(REG_PARAMETERS_KEY_NAME_U) + 1
             + wcslen(REG_DEVICE_KEY_NAME_U)
             + GetDigitCount(DeviceIndex) + 1;

    PathSize *= sizeof(WCHAR);

    /* Allocate storage for the string */
    RegPath = (PWCHAR) HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 PathSize);

    if ( ! RegPath )
    {
        return MMSYSERR_NOMEM;
    }

    /* Write the path */
    wsprintf(RegPath,
             L"%ls\\%ls\\%ls\\%ls%d",
             REG_SERVICES_KEY_NAME_U,
             ServiceName,
             REG_PARAMETERS_KEY_NAME_U,
             REG_DEVICE_KEY_NAME_U,
             DeviceIndex);

    MessageBox(0, RegPath, L"Opening registry path", MB_OK | MB_TASKMODAL);

    /* Perform the open */
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      RegPath,
                      0,
                      KEY_READ,
                      KeyHandle) != ERROR_SUCCESS )
    {
        /* Couldn't open the key */
        HeapFree(GetProcessHeap(), 0, RegPath);
        return MMSYSERR_ERROR;
    }

    HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, RegPath);

    return MMSYSERR_NOERROR;
}

/*
    This is the "nice" way to discover audio devices in NT4 - go into the
    service registry key and enumerate the Parameters\Device*\Devices
    values. The value names represent the device name, whereas the data
    assigned to them identifies the type of device.
*/
MMRESULT
EnumerateNt4ServiceSoundDevices(
    LPWSTR ServiceName,
    UCHAR DeviceType,
    SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc)
{
    HKEY Key;
    DWORD KeyIndex = 0;

    /* Validate parameters */
    if ( ! ServiceName )
        return MMSYSERR_INVALPARAM;

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
        return MMSYSERR_INVALPARAM;

    while ( OpenSoundDeviceRegKey(ServiceName, KeyIndex, &Key) == MMSYSERR_NOERROR )
    {
        HKEY DevicesKey;
        DWORD ValueType = REG_NONE, ValueIndex = 0;
        DWORD MaxNameLength = 0, ValueNameLength = 0;
        PWSTR DevicePath = NULL, ValueName = NULL;
        DWORD ValueDataLength = sizeof(DWORD);
        DWORD ValueData;

        if ( RegOpenKeyEx(Key,
                          REG_DEVICES_KEY_NAME_U,
                          0,
                          KEY_READ,
                          &DevicesKey) == ERROR_SUCCESS )
        {
            /* Find out how much memory is needed for the key name */
            if ( RegQueryInfoKey(DevicesKey,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 &MaxNameLength,
                                 NULL, NULL, NULL) != ERROR_SUCCESS )
            {
                RegCloseKey(DevicesKey);
                RegCloseKey(Key);

                return MMSYSERR_ERROR;
            }

            /* Account for terminating NULL */
            ++ MaxNameLength;

            DevicePath = (PWSTR) HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           (MaxNameLength +
                                            strlen("\\\\.\\")) *
                                            sizeof(WCHAR));

            /* Check that the memory allocation was successful */
            if ( ! DevicePath )
            {
                /* There's no point in going further */
                RegCloseKey(DevicesKey);
                RegCloseKey(Key);

                return MMSYSERR_NOMEM;
            }

            /* Insert the device path prefix */
            wsprintf(DevicePath, L"\\\\.\\");

            /* The offset of the string following this prefix */
            ValueName = DevicePath + strlen("\\\\.\\");

            /* Copy this so that it may be overwritten */
            ValueNameLength = MaxNameLength;

            while ( RegEnumValue(DevicesKey,
                                 ValueIndex,
                                 ValueName,
                                 &ValueNameLength,
                                 NULL,
                                 &ValueType,
                                 (LPBYTE) &ValueData,
                                 &ValueDataLength) == ERROR_SUCCESS )
            {
                /* Device types are stored as DWORDs */
                if ( ( ValueType == REG_DWORD ) &&
                     ( ValueDataLength == sizeof(DWORD) ) )
                {
                    SoundDeviceDetectedProc(
                        DeviceType,
                        DevicePath,
                        INVALID_HANDLE_VALUE);
                }

                /* Reset variables for the next iteration */
                ValueNameLength = MaxNameLength;
                ZeroMemory(ValueName, MaxNameLength);
                ValueDataLength = sizeof(DWORD);
                ValueData = 0;
                ValueType = REG_NONE;

                ++ ValueIndex;
            }

            HeapFree(GetProcessHeap(), 0, DevicePath);

            RegCloseKey(DevicesKey);
        }

        ++ KeyIndex;

        RegCloseKey(Key);
    }

    return MMSYSERR_NOERROR;
}

/*
    Brute-force device detection, using a base device name (eg: \\.\WaveOut).

    This will add the device number as a suffix to the end of the string and
    attempt to open the device based on that name. On success, it will
    increment the device number and repeat this process.

    When it runs out of devices, it will give up.
*/
MMRESULT
DetectNt4SoundDevices(
    UCHAR DeviceType,
    PWSTR BaseDeviceName,
    SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc)
{
    ULONG DeviceNameLength = 0;
    ULONG DeviceNameSize = 0;
    PWSTR DeviceName = NULL;
    ULONG Index = 0, Count = 0;
    HANDLE DeviceHandle;
    BOOLEAN DoSearch = TRUE;

    DPRINT("Detecting NT4 style sound devices of type %d\n", DeviceType);

    if ( ! VALID_SOUND_DEVICE_TYPE(DeviceType) )
    {
        return MMSYSERR_INVALPARAM;
    }

    DeviceNameLength = wcslen(BaseDeviceName);
    /* Consider the length of the number */
    DeviceNameLength += GetDigitCount(Index);
    /* ...and the terminating NULL */
    DeviceNameLength += 1;
    /* Finally, this is a wide string, so... */
    DeviceNameSize = DeviceNameLength * sizeof(WCHAR);

    DeviceName = (PWSTR)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DeviceNameSize);

    if ( ! DeviceName )
    {
        return MMSYSERR_NOMEM;
    }

    while ( DoSearch )
    {
        /* Nothing like a nice clean device name */
        ZeroMemory(DeviceName, DeviceNameSize);
        wsprintf(DeviceName, L"%ls%d", BaseDeviceName, Index);

        if ( OpenKernelSoundDeviceByName(DeviceName,
                                         GENERIC_READ,
                                         &DeviceHandle) == MMSYSERR_NOERROR )
        {
            DPRINT("Found device %d\n", Index);

            /* Notify the callback function */
            if ( SoundDeviceDetectedProc(DeviceType, DeviceName, DeviceHandle) )
            {
                ++ Count;
            }

            CloseHandle(DeviceHandle);

            ++ Index;
        }
        else
        {
            DoSearch = FALSE;
        }
    }

    HeapFree(GetProcessHeap(), 0, DeviceName);

    return MMSYSERR_NOERROR;
}


#include <ntddk.h>      /* How do I avoid this? */

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
