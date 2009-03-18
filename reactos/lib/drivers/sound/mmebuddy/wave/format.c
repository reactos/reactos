/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/format.c
 *
 * PURPOSE:     Queries and sets wave device format (sample rate, etc.)
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <sndtypes.h>
#include <mmebuddy.h>

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

    if ( ! MMSUCCESS(Result) )
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
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Ensure we have a wave device (TODO: check if this applies to wavein as well) */
    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) );

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->SetWaveFormat )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->SetWaveFormat(SoundDeviceInstance, Format, FormatSize);
}
