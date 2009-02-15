/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/header.c
 *
 * PURPOSE:     Wave header preparation routines
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>


/*
    This structure gets used locally within functions as a way to shuttle data
    to the sound thread. It's safe to use locally since CallSoundThread will
    not return until the operation has been carried out.
*/

typedef struct
{
    MMWAVEHEADER_FUNC Function;
    PWAVEHDR Header;
} THREADED_WAVEHEADER_PARAMETERS;


/*
    Helper routines to simplify the call to the sound thread for the header
    functions.
*/

MMRESULT
WaveHeaderOperationInSoundThread(
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    THREADED_WAVEHEADER_PARAMETERS* Parameters = (THREADED_WAVEHEADER_PARAMETERS*) Parameter;
    return Parameters->Function(SoundDeviceInstance, Parameters->Header);
}

MMRESULT
WaveHeaderOperation(
    MMWAVEHEADER_FUNC Function,
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    PWAVEHDR Header)
{
    THREADED_WAVEHEADER_PARAMETERS Parameters;

    Parameters.Function = Function;
    Parameters.Header = Header;

    return CallSoundThread(SoundDeviceInstance,
                           WaveHeaderOperationInSoundThread,
                           &Parameters);
}


/*
    The following routines are basically handlers for:
    - WODM_PREPARE
    - WODM_UNPREPARE
    - WODM_WRITE

    All of these calls are ultimately dealt with in the context of the
    appropriate sound thread, so the implementation should expect itself to
    be running in this other thread when any of these operations take place.
*/

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

    return WaveHeaderOperation(FunctionTable->PrepareWaveHeader,
                               SoundDeviceInstance,
                               Header);
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

    return WaveHeaderOperation(FunctionTable->UnprepareWaveHeader,
                               SoundDeviceInstance,
                               Header);
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

    return WaveHeaderOperation(FunctionTable->SubmitWaveHeader,
                               SoundDeviceInstance,
                               Header);
}

