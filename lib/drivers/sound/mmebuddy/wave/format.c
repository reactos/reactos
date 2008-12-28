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
#include <ntddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

MMRESULT
QueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    MMFUNCTION_TABLE FunctionTable;

    SND_TRACE(L"Querying wave format support\n");

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Format );
    VALIDATE_MMSYS_PARAMETER( FormatSize >= sizeof(WAVEFORMATEX) );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Ensure we have a wave device (TODO: check if wavein as well) */
    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) );

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( Result != MMSYSERR_NOERROR )
        return TranslateInternalMmResult(Result);

    SND_ASSERT( FunctionTable->QueryWaveFormatSupport );

    return FunctionTable->QueryWaveFormatSupport(SoundDevice, Format, FormatSize);
}

MMRESULT
SetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize)
{
    SND_TRACE(L"Setting wave format\n");
    return MMSYSERR_NOTSUPPORTED;
}
