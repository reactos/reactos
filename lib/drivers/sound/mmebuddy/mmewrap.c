/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/mmewrap.c
 *
 * PURPOSE:     Interface between MME functions and MME Buddy's own.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
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

    /* FIXME: Validate device type and ID */
    VALIDATE_MMSYS_PARAMETER( Capabilities );
    VALIDATE_MMSYS_PARAMETER( CapabilitiesSize > 0 );

    /* Our parameter checks are done elsewhere */
    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);

    if ( Result != MMSYSERR_NOERROR )
        return Result;

    return GetSoundDeviceCapabilities(SoundDevice,
                                      Capabilities,
                                      CapabilitiesSize);
}

MMRESULT
MmeOpenWaveDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWAVEOPENDESC OpenParameters,
    IN  DWORD Flags,
    OUT DWORD* PrivateHandle)
{
    MMRESULT Result;

    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    LPWAVEFORMATEX Format;

    SND_TRACE(L"Opening wave device (WIDM_OPEN / WODM_OPEN)");

    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) );    /* FIXME? wave in too? */
    VALIDATE_MMSYS_PARAMETER( OpenParameters );

    Format = OpenParameters->lpFormat;

    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    /* Does this device support the format? */
    Result = QueryWaveDeviceFormatSupport(SoundDevice, Format, sizeof(WAVEFORMATEX));
    if ( Result != MMSYSERR_NOERROR )
    {
        SND_ERR(L"Format not supported\n");
        return TranslateInternalMmResult(Result);
    }

    /* If the caller just wanted to know if a format is supported, end here */
    if ( Flags & WAVE_FORMAT_QUERY )
        return MMSYSERR_NOERROR;

    /* Check that winmm gave us a private handle to fill */
    VALIDATE_MMSYS_PARAMETER( PrivateHandle );

    /* Create a sound device instance and open the sound device */
    Result = CreateSoundDeviceInstance(SoundDevice, &SoundDeviceInstance);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    Result = SetWaveDeviceFormat(SoundDeviceInstance, Format, sizeof(WAVEFORMATEX));
    if ( Result != MMSYSERR_NOERROR )
    {
        /* TODO: Destroy sound instance */
        return TranslateInternalMmResult(Result);
    }

    /* Store the device instance pointer in the private handle - is DWORD safe here? */
    *PrivateHandle = (DWORD) SoundDeviceInstance;

    /* TODO: Call the client application back to say the device is open */
    ReleaseEntrypointMutex(DeviceType);
    /* ... */
    AcquireEntrypointMutex(DeviceType);

    SND_TRACE(L"Wave device now open\n");

    return MMSYSERR_NOERROR;
}

MMRESULT
MmeCloseDevice(
    IN  DWORD PrivateHandle)
{
    /* FIXME - Where do we call the callback?? */
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;

    SND_TRACE(L"Closing wave device (WIDM_CLOSE / WODM_CLOSE)\n");

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    Result = DestroySoundDeviceInstance(SoundDeviceInstance);

    return Result;
}
