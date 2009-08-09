/*
 * PROJECT:     ReactOS Sound System "MME Buddy" NT4 Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mment4/control.c
 *
 * PURPOSE:     Device control for NT4 audio devices
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#define NDEBUG

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <sndtypes.h>
#include <mmebuddy.h>
#include <mment4.h>

/*
    Convenience routine for getting the path of a device and opening it.
*/
MMRESULT
OpenNt4KernelSoundDevice(
    IN  PSOUND_DEVICE SoundDevice,
    IN  BOOLEAN ReadOnly,
    OUT PHANDLE Handle)
{
    PWSTR Path;
    MMRESULT Result;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Handle );

    Result = GetSoundDeviceIdentifier(SoundDevice, (PVOID*) &Path);
    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Unable to get sound device path");
        return TranslateInternalMmResult(Result);
    }

    SND_ASSERT( Path );

    return OpenKernelSoundDeviceByName(Path, ReadOnly, Handle);
}

/*
    Device open/close. These are basically wrappers for the MME-Buddy
    open and close routines, which provide a Windows device handle.
    These may seem simple but as you can return pretty much anything
    as the handle, we could just as easily return a structure etc.
*/
MMRESULT
OpenNt4SoundDevice(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID* Handle)
{
    SND_TRACE(L"Opening NT4 style sound device\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Handle );

    return OpenNt4KernelSoundDevice(SoundDevice, FALSE, Handle);
}

MMRESULT
CloseNt4SoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Handle)
{
    SND_TRACE(L"Closing NT4 style sound device\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    return CloseKernelSoundDevice((HANDLE) Handle);
}

/*
    Provides an implementation for the "get capabilities" request,
    using the standard IOCTLs used by NT4 sound drivers.
*/
MMRESULT
GetNt4SoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    DWORD IoCtl;
    HANDLE DeviceHandle;

    /* If these are bad there's an internal error with MME-Buddy! */
    SND_ASSERT( SoundDevice );
    SND_ASSERT( Capabilities );
    SND_ASSERT( CapabilitiesSize > 0 );

    SND_TRACE(L"NT4 get-capabilities routine called\n");

    /* Get the device type */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( ! MMSUCCESS(Result) );
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
        /* FIXME - need to support AUX and mixer devices */
        SND_ASSERT( FALSE );
    }

    /* Get the capabilities information from the driver */
    Result = OpenNt4KernelSoundDevice(SoundDevice, TRUE, &DeviceHandle);

    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Failed to open device");
        return TranslateInternalMmResult(Result);
    }

    Result = SyncOverlappedDeviceIoControl(DeviceHandle,
                                           IoCtl,
                                           Capabilities,
                                           CapabilitiesSize,
                                           NULL,
                                           0,
                                           NULL);

    CloseKernelSoundDevice(DeviceHandle);

    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Retrieval of capabilities information failed\n");
        Result = TranslateInternalMmResult(Result);
    }

    return Result;
}

/*
    Querying/setting the format of a wave device. Querying format support
    requires us to first open the device, whereas setting format is done
    on an already opened device.
*/
MMRESULT
QueryNt4WaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize)
{
    MMRESULT Result;
    HANDLE Handle;

    SND_TRACE(L"NT4 wave format support querying routine called\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Format );
    VALIDATE_MMSYS_PARAMETER( FormatSize >= sizeof(WAVEFORMATEX) );

    /* Get the device path */
    Result = OpenNt4KernelSoundDevice(SoundDevice,
                                      FALSE,
                                      &Handle);

    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Unable to open kernel sound device\n");
        return TranslateInternalMmResult(Result);
    }

    Result = SyncOverlappedDeviceIoControl(Handle,
                                           IOCTL_WAVE_QUERY_FORMAT,
                                           (LPVOID) Format,
                                           FormatSize,
                                           NULL,
                                           0,
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Sync overlapped I/O failed - MMSYS_ERROR %d\n", Result);
        Result = TranslateInternalMmResult(Result);
    }

    CloseKernelSoundDevice(Handle);

    return MMSYSERR_NOERROR;
}

MMRESULT
SetNt4WaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize)
{
    MMRESULT Result;
    HANDLE Handle;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Format );
    VALIDATE_MMSYS_PARAMETER( FormatSize >= sizeof(WAVEFORMATEX) );

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);

    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    SND_TRACE(L"Setting wave device format on handle %x\n", Handle);

    Result = SyncOverlappedDeviceIoControl(Handle,
                                           IOCTL_WAVE_SET_FORMAT,
                                           (LPVOID) Format,
                                           FormatSize,
                                           NULL,
                                           0,
                                           NULL);

    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    return MMSYSERR_NOERROR;
}

#if 0
MMRESULT
SubmitNt4WaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR WaveHeader)
{
    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( WaveHeader );

    SND_TRACE(L"Submitting wave header %p (in sound thread)\n", WaveHeader);

    /* TODO: This should only submit the header to the device, nothing more! */
}
#endif
