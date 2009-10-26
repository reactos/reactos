/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/mixer/mxdMessage.c
 *
 * PURPOSE:     Provides the mxdMessage exported function, as required by
 *              the MME API, for mixer device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ntddsnd.h>
#include <sndtypes.h>
#undef NDEBUG
#include <mmebuddy.h>

MMRESULT
MmeGetLineInfo(
    IN  DWORD Message,
    IN  DWORD PrivateHandle,
    IN  DWORD Parameter1,
    IN  DWORD Parameter2)
{
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    SND_TRACE(L"Getting mixer info %u\n", Message);

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->QueryMixerInfo )
        return MMSYSERR_NOTSUPPORTED;

    Result = FunctionTable->QueryMixerInfo(SoundDeviceInstance, Message, (LPVOID)Parameter1, Parameter2);

    return Result;
}


MMRESULT
MmeCloseMixerDevice(
    IN  DWORD PrivateHandle)
{
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_DEVICE SoundDevice;

    SND_TRACE(L"Closing mixer device \n");

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    SoundDeviceInstance = (PSOUND_DEVICE_INSTANCE) PrivateHandle;

    if ( ! IsValidSoundDeviceInstance(SoundDeviceInstance) )
        return MMSYSERR_INVALHANDLE;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);


    Result = DestroySoundDeviceInstance(SoundDeviceInstance);

    return Result;
}

MMRESULT
MmeOpenMixerDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPMIXEROPENDESC OpenParameters,
    IN  DWORD Flags,
    OUT DWORD* PrivateHandle)
{
    MMRESULT Result;
    PMMFUNCTION_TABLE FunctionTable;
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;

    SND_TRACE(L"Opening mixer device");

    VALIDATE_MMSYS_PARAMETER( OpenParameters );

    Result = GetSoundDevice(DeviceType, DeviceId, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Check that winmm gave us a private handle to fill */
    VALIDATE_MMSYS_PARAMETER( PrivateHandle );

    /* Create a sound device instance and open the sound device */
    Result = CreateSoundDeviceInstance(SoundDevice, &SoundDeviceInstance);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->SetWaveFormat )
        return MMSYSERR_NOTSUPPORTED;

    Result = FunctionTable->SetWaveFormat(SoundDeviceInstance, DeviceId, NULL, 0);
    if ( ! MMSUCCESS(Result) )
    {
        /* TODO: Destroy sound instance */
        return TranslateInternalMmResult(Result);
    }

    /* Store the device instance pointer in the private handle - is DWORD safe here? */
    *PrivateHandle = (DWORD) SoundDeviceInstance;

    /* Store the additional information we were given - FIXME: Need flags! */
    SetSoundDeviceInstanceMmeData(SoundDeviceInstance,
                                  (HDRVR)OpenParameters->hmx,
                                  OpenParameters->dwCallback,
                                  OpenParameters->dwInstance,
                                  Flags);

    /* Let the application know the device is open */
    ReleaseEntrypointMutex(DeviceType);
#if 0
    NotifyMmeClient(SoundDeviceInstance,
                    DeviceType == WAVE_OUT_DEVICE_TYPE ? WOM_OPEN : WIM_OPEN,
                    0);
#endif
    AcquireEntrypointMutex(DeviceType);

    SND_TRACE(L"Mixer device now open\n");

    return MMSYSERR_NOERROR;
}

/*
    Standard MME driver entry-point for messages relating to mixers.
*/
DWORD
APIENTRY
mxdMessage(
    DWORD DeviceId,
    DWORD Message,
    DWORD PrivateHandle,
    DWORD Parameter1,
    DWORD Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(MIXER_DEVICE_TYPE);

    SND_TRACE(L"mxdMessage - Message type %d\n", Message);

    switch ( Message )
    {
        case MXDM_GETNUMDEVS :
        {
            Result = GetSoundDeviceCount(MIXER_DEVICE_TYPE);
            break;
        }

        case MXDM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(MIXER_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }

        case MXDM_INIT :
        {
            Result = MMSYSERR_NOERROR;
            break;
        }

        case MXDM_OPEN :
        {
            Result = MmeOpenMixerDevice(MIXER_DEVICE_TYPE,
                                       DeviceId,
                                       (LPMIXEROPENDESC) Parameter1,
                                       Parameter2,
                                       (DWORD*) PrivateHandle);

            break;
        }

        case MXDM_CLOSE :
        {
            Result = MmeCloseMixerDevice(PrivateHandle);

            break;
        }

        case MXDM_GETCONTROLDETAILS :
        {
            Result = MmeGetLineInfo(Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case MXDM_SETCONTROLDETAILS :
        {
            Result = MmeGetLineInfo(Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case MXDM_GETLINECONTROLS :
        {
            Result = MmeGetLineInfo(Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case MXDM_GETLINEINFO :
        {
            Result = MmeGetLineInfo(Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }
    }

    SND_TRACE(L"mxdMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIXER_DEVICE_TYPE);

    return Result;
}
