/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/mmewrap.c
 *
 * PURPOSE:     Interface between MME functions and MME Buddy's own.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "precomp.h"


/*
    Sets the device into running or stopped state
*/

MMRESULT
MmeSetState(
    IN  DWORD_PTR PrivateHandle,
    IN  BOOL bStart)
{
    MMRESULT Result;
    PMMFUNCTION_TABLE FunctionTable;
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    BOOL OldState;

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Get the function table, and validate it */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    SND_ASSERT( FunctionTable->SetState );
    if ( FunctionTable->SetState == NULL )
    {
        /* FIXME */
        return MMSYSERR_NOTSUPPORTED;
    }
    /* Try change state */
    Result = FunctionTable->SetState(SoundDeviceInstance, bStart);

    if ( MMSUCCESS(Result) )
    {
        /* Get old audio stream state */
        OldState = SoundDeviceInstance->bPaused;

        /* Store audio stream pause state */
        SoundDeviceInstance->bPaused = !bStart;

        if (SoundDeviceInstance->bPaused == FALSE && OldState)
        {
            InitiateSoundStreaming(SoundDeviceInstance);
        }
    }

    return Result;
}

/*
    Call the client application when something interesting happens (MME API
    defines "interesting things" as device open, close, and buffer
    completion.)
*/
VOID
NotifyMmeClient(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  UINT Message,
    IN  DWORD_PTR Parameter)
{
    SND_ASSERT( SoundDeviceInstance );

    SND_TRACE(L"MME client callback - message %d, parameter %d\n",
              (int) Message,
              (int) Parameter);

    if ( SoundDeviceInstance->WinMM.ClientCallback )
    {
        DriverCallback(SoundDeviceInstance->WinMM.ClientCallback,
                       HIWORD(SoundDeviceInstance->WinMM.Flags),
                       SoundDeviceInstance->WinMM.Handle,
                       Message,
                       SoundDeviceInstance->WinMM.ClientCallbackInstanceData,
                       Parameter,
                       0);
    }
}

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

    /* FIXME: Validate device ID */
    VALIDATE_MMSYS_PARAMETER( Capabilities );
    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );

    /* Our parameter checks are done elsewhere */

    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
        return Result;

    return GetSoundDeviceCapabilities(SoundDevice,
                                      DeviceId,
                                      Capabilities,
                                      CapabilitiesSize);
}

MMRESULT
MmeOpenDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  UINT DeviceId,
    IN  LPWAVEOPENDESC OpenParameters,
    IN  DWORD Flags,
    OUT DWORD_PTR* PrivateHandle)
{
    MMRESULT Result;
    UINT Message;
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    LPWAVEFORMATEX Format = NULL;

    SND_TRACE(L"Opening device");

    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) || IS_MIXER_DEVICE_TYPE(DeviceType) || IS_MIDI_DEVICE_TYPE(DeviceType) );    /* FIXME? wave in too? */
    VALIDATE_MMSYS_PARAMETER( OpenParameters );

    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        Format = OpenParameters->lpFormat;

        /* Does this device support the format? */
        Result = QueryWaveDeviceFormatSupport(SoundDevice, Format, sizeof(WAVEFORMATEX));
        if ( ! MMSUCCESS(Result) )
        {
            SND_ERR(L"Format not supported\n");
            return TranslateInternalMmResult(Result);
        }

        /* If the caller just wanted to know if a format is supported, end here */
        if ( Flags & WAVE_FORMAT_QUERY )
            return MMSYSERR_NOERROR;
    }

    /* Check that winmm gave us a private handle to fill */
    VALIDATE_MMSYS_PARAMETER( PrivateHandle );

    /* Create a sound device instance and open the sound device */
    Result = CreateSoundDeviceInstance(SoundDevice, &SoundDeviceInstance);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = SetWaveDeviceFormat(SoundDeviceInstance, DeviceId, Format, sizeof(WAVEFORMATEX));
    if ( ! MMSUCCESS(Result) )
    {
        /* TODO: Destroy sound instance */
        return TranslateInternalMmResult(Result);
    }

    /* Store the device instance pointer in the private handle */
    *PrivateHandle = (DWORD_PTR)SoundDeviceInstance;

    /* Store the additional information we were given - FIXME: Need flags! */
    SetSoundDeviceInstanceMmeData(SoundDeviceInstance,
                                  (HDRVR)OpenParameters->hWave, /* works because LPMIXEROPENDESC/etc has also the handle as first member */
                                  OpenParameters->dwCallback,
                                  OpenParameters->dwInstance,
                                  Flags);

    if (DeviceType == WAVE_OUT_DEVICE_TYPE || DeviceType == WAVE_IN_DEVICE_TYPE ||
        DeviceType == MIDI_OUT_DEVICE_TYPE || DeviceType == MIDI_IN_DEVICE_TYPE)
    {
        /* Let the application know the device is open */

        if (DeviceType == WAVE_OUT_DEVICE_TYPE)
            Message = WOM_OPEN;
        else if (DeviceType == WAVE_IN_DEVICE_TYPE)
            Message = WIM_OPEN;
        else if (DeviceType == MIDI_IN_DEVICE_TYPE)
            Message = MIM_OPEN;
        else
            Message = MOM_OPEN;

        ReleaseEntrypointMutex(DeviceType);

        NotifyMmeClient(SoundDeviceInstance,
                        Message,
                        0);

        AcquireEntrypointMutex(DeviceType);
    }

    SND_TRACE(L"device now open\n");

    return MMSYSERR_NOERROR;
}

MMRESULT
MmeCloseDevice(
    IN  DWORD_PTR PrivateHandle)
{
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    UINT Message = 0;

    SND_TRACE(L"Closing wave device (WIDM_CLOSE / WODM_CLOSE)\n");

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    if ( ! IsValidSoundDeviceInstance(SoundDeviceInstance) )
        return MMSYSERR_INVALHANDLE;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);


    /* TODO: Check device is stopped! */


    if (DeviceType != MIXER_DEVICE_TYPE)
    {
        ReleaseEntrypointMutex(DeviceType);

        if (DeviceType == WAVE_OUT_DEVICE_TYPE)
            Message = WOM_CLOSE;
        else if (DeviceType == WAVE_IN_DEVICE_TYPE)
            Message = WIM_CLOSE;
        else if (DeviceType == MIDI_IN_DEVICE_TYPE)
            Message = MIM_CLOSE;
        else if (DeviceType == MIDI_OUT_DEVICE_TYPE)
            Message = MOM_CLOSE;

        /* TODO: Work with MIDI devices too */
        NotifyMmeClient(SoundDeviceInstance,
                        Message,
                        0);
        AcquireEntrypointMutex(DeviceType);
    }

    Result = DestroySoundDeviceInstance(SoundDeviceInstance);

    return Result;
}

MMRESULT
MmeResetWavePlayback(
    IN  DWORD_PTR PrivateHandle)
{
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;

    SND_TRACE(L"Resetting wave device (WODM_RESET)\n");

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    return StopStreaming(SoundDeviceInstance);
}

MMRESULT
MmeGetDeviceInterfaceString(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWSTR Interface,
    IN  DWORD  InterfaceLength,
    OUT  DWORD * InterfaceSize)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( FunctionTable->GetDeviceInterfaceString == NULL )
    {
        /* querying device interface string / size not supported */
        return MMSYSERR_NOTSUPPORTED;
    }

    /* Call the driver */
    Result = FunctionTable->GetDeviceInterfaceString(DeviceType, DeviceId, Interface, InterfaceLength, InterfaceSize);

    return Result;
}


MMRESULT
MmeGetPosition(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  DWORD_PTR PrivateHandle,
    IN  MMTIME* Time,
    IN  DWORD Size)
{
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    if ( ! IsValidSoundDeviceInstance(SoundDeviceInstance) )
        return MMSYSERR_INVALHANDLE;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( Size != sizeof(MMTIME) )
        return MMSYSERR_INVALPARAM;

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( FunctionTable->GetPos == NULL )
    {
        /* This indicates bad practice, really! If you can open, why not close?! */
        return MMSYSERR_NOTSUPPORTED;
    }

    /* Call the driver */
    Result = FunctionTable->GetPos(SoundDeviceInstance, Time);

    return Result;
}

