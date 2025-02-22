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

#include "precomp.h"

MMRESULT
MmeGetLineInfo(
    IN UINT DeviceId,
    IN  UINT Message,
    IN  DWORD_PTR PrivateHandle,
    IN  DWORD_PTR Parameter1,
    IN  DWORD_PTR Parameter2)
{
    MMRESULT Result;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    //SND_TRACE(L"Getting mixer info %u\n", Message);

    if ( PrivateHandle == 0 )
    {
        Result = GetSoundDevice(MIXER_DEVICE_TYPE, DeviceId, &SoundDevice);

        if ( ! MMSUCCESS(Result) )
            return TranslateInternalMmResult(Result);

         Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
         if ( ! MMSUCCESS(Result) )
            return TranslateInternalMmResult(Result);

         Result = FunctionTable->QueryMixerInfo(NULL, DeviceId, Message, (LPVOID)Parameter1, Parameter2);
         return Result;
    }

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

    Result = FunctionTable->QueryMixerInfo(SoundDeviceInstance, DeviceId, Message, (LPVOID)Parameter1, Parameter2);

    return Result;
}


/*
    Standard MME driver entry-point for messages relating to mixers.
*/
DWORD
APIENTRY
mxdMessage(
    UINT DeviceId,
    UINT Message,
    DWORD_PTR PrivateHandle,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(MIXER_DEVICE_TYPE);

    //SND_TRACE(L"mxdMessage - Message type %d\n", Message);

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
            Result = MmeOpenDevice(MIXER_DEVICE_TYPE,
                                   DeviceId,
                                   (LPWAVEOPENDESC) Parameter1, /* unused */
                                   Parameter2,
                                   (DWORD_PTR*) PrivateHandle);
            VALIDATE_MMSYS_PARAMETER(*(DWORD_PTR*)PrivateHandle);
            break;
        }

        case MXDM_CLOSE :
        {
            Result = MmeCloseDevice(PrivateHandle);

            break;
        }

        case MXDM_GETCONTROLDETAILS :
        {
            Result = MmeGetLineInfo(DeviceId,
                                    Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case MXDM_SETCONTROLDETAILS :
        {
            Result = MmeGetLineInfo(DeviceId,
                                    Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case MXDM_GETLINECONTROLS :
        {
            Result = MmeGetLineInfo(DeviceId,
                                    Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case MXDM_GETLINEINFO :
        {
            Result = MmeGetLineInfo(DeviceId,
                                    Message,
                                    PrivateHandle,
                                    Parameter1,
                                    Parameter2);

            break;
        }

        case DRV_QUERYDEVICEINTERFACESIZE :
        {
            Result = MmeGetDeviceInterfaceString(MIXER_DEVICE_TYPE, DeviceId, NULL, 0, (DWORD*)Parameter1); //FIXME DWORD_PTR
            break;
        }

        case DRV_QUERYDEVICEINTERFACE :
        {
            Result = MmeGetDeviceInterfaceString(MIXER_DEVICE_TYPE, DeviceId, (LPWSTR)Parameter1, Parameter2, NULL); //FIXME DWORD_PTR
            break;
        }

    }

    //SND_TRACE(L"mxdMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(MIXER_DEVICE_TYPE);

    return Result;
}
