/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/header.c
 *
 * PURPOSE:     Wave header preparation routines
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

MMRESULT
PrepareWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Header );

    SND_TRACE(L"Preparing wave header\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->PrepareWaveHeader )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->PrepareWaveHeader(SoundDeviceInstance, Header);
}

MMRESULT
UnprepareWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Header );

    SND_TRACE(L"Un-preparing wave header\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->UnprepareWaveHeader )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->UnprepareWaveHeader(SoundDeviceInstance, Header);
}

MMRESULT
SubmitWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Header );

    SND_TRACE(L"Submitting wave header\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->SubmitWaveHeader )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->SubmitWaveHeader(SoundDeviceInstance, Header);
}

