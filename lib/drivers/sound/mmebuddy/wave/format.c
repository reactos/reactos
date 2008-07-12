/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/format.c
 *
 * PURPOSE:     Queries/sets format for wave audio devices.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddk.h>      /* for IOCTL stuff */
#include <ntddsnd.h>

#include <mmebuddy.h>

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
