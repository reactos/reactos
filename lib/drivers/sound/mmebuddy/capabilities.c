/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/capabilities.c
 *
 * PURPOSE:     Queries sound devices for their capabilities.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

/*
    This is a helper function to alleviate some of the repetition involved with
    implementing the various MME message functions.
*/
MMRESULT
MmeGetSoundDeviceCapabilities(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;

    SND_TRACE(L"MME *_GETCAPS for device %d of type %d\n", DeviceId, DeviceType);

    /* Our parameter checks are done elsewhere */
    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);

    if ( Result != MMSYSERR_NOERROR )
        return Result;

    return GetSoundDeviceCapabilities(SoundDevice,
                                      Capabilities,
                                      CapabilitiesSize);
}

/*
    Obtains the capabilities of a sound device. This routine ensures that the
    supplied CapabilitiesSize parameter at least meets the minimum size of the
    relevant capabilities structure.

    Ultimately, it will call the GetCapabilities function specified in the
    sound device's function table. Note that there are several of these, in a
    union. This is simply to avoid manually typecasting when implementing the
    functions.
*/
MMRESULT
GetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMDEVICE_TYPE DeviceType;
    PMMFUNCTION_TABLE FunctionTable;
    BOOLEAN GoodSize = FALSE;
    MMRESULT Result;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Capabilities );
    VALIDATE_MMSYS_PARAMETER( CapabilitiesSize > 0 );

    /* Obtain the device type */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    /* Check that the capabilities structure is of a valid size */
    switch ( DeviceType )
    {
        case WAVE_OUT_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(WAVEOUTCAPS);
            break;
        }
        case WAVE_IN_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(WAVEINCAPS);
            break;
        }
        case MIDI_OUT_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(MIDIOUTCAPS);
            break;
        }
        case MIDI_IN_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(MIDIINCAPS);
            break;
        }
        /* TODO: Others... */
        default :
        {
            SND_ASSERT(FALSE);
        }
    };

    if ( ! GoodSize )
    {
        SND_ERR(L"Device capabilities structure too small\n");
        return MMSYSERR_INVALPARAM;
    }

    /* Call the "get capabilities" function within the function table */
    SND_ASSERT( FunctionTable->GetCapabilities );

    if ( ! FunctionTable->GetCapabilities )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->GetCapabilities(SoundDevice,
                                          Capabilities,
                                          CapabilitiesSize);
}

/*
    Provides a default implementation for the "get capabilities" request,
    using the standard IOCTLs used by NT4 sound drivers.
*/
MMRESULT
DefaultGetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    PWSTR DevicePath;
    DWORD IoCtl;
    DWORD BytesTransferred;
    HANDLE DeviceHandle;

    /* If these are bad there's an internal error with MME-Buddy! */
    SND_ASSERT( SoundDevice );
    SND_ASSERT( Capabilities );
    SND_ASSERT( CapabilitiesSize > 0 );

    SND_TRACE(L"Default get-capabilities routine called\n");

    /* Get the device type */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR );
        return TranslateInternalMmResult(Result);

    /* Get the device path */
    Result = GetSoundDevicePath(SoundDevice, &DevicePath);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR );
        return TranslateInternalMmResult(Result);

    /* Choose the appropriate IOCTL */
    if ( IS_WAVE_DEVICE_TYPE(DeviceType) )
    {
        IoCtl = IOCTL_WAVE_GET_CAPABILITIES;
    }
    else if ( IS_MIDI_DEVICE_TYPE(DeviceType) )
    {
        IoCtl = IOCTL_MIDI_GET_CAPABILITIES;
    }
    else
    {
        /* TODO */
        SND_ASSERT( FALSE );
    }

    /* Get the capabilities information from the driver */
    Result = OpenKernelSoundDeviceByName(DevicePath, TRUE, &DeviceHandle);

    if ( Result != MMSYSERR_NOERROR )
    {
        SND_ERR(L"Failed to open %wS\n", DevicePath);
        return TranslateInternalMmResult(Result);
    }

    Result = QueryDevice(DeviceHandle,
                         IoCtl,
                         Capabilities,
                         CapabilitiesSize,
                         &BytesTransferred,
                         NULL);

    CloseKernelSoundDevice(DeviceHandle);

    if ( Result != MMSYSERR_NOERROR )
    {
        SND_ERR(L"Retrieval of capabilities information failed\n");
    }

    return Result;
}
