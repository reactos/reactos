/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/format.c
 *
 * PURPOSE:     Queries and sets wave device format (sample rate, etc.)
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

/* Adapted from MmeQueryWaveFormat... TODO: Move elsewhere */
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

    SND_TRACE(L"Opening wave device (WIM_OPEN / WOM_OPEN)");

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
QueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    PMMFUNCTION_TABLE FunctionTable;

    SND_TRACE(L"Querying wave format support\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Format );
    VALIDATE_MMSYS_PARAMETER( FormatSize >= sizeof(WAVEFORMATEX) );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Ensure we have a wave device (TODO: check if this applies to wavein as well) */
    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) );

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->QueryWaveFormatSupport )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->QueryWaveFormatSupport(SoundDevice, Format, FormatSize);
}

MMRESULT
SetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    PMMFUNCTION_TABLE FunctionTable;
    PSOUND_DEVICE SoundDevice;

    SND_TRACE(L"Setting wave format\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Format );
    VALIDATE_MMSYS_PARAMETER( FormatSize >= sizeof(WAVEFORMATEX) );

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Ensure we have a wave device (TODO: check if this applies to wavein as well) */
    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) );

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->SetWaveFormat )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->SetWaveFormat(SoundDeviceInstance, Format, FormatSize);
}
