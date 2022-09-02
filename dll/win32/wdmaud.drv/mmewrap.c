/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/mmewrap.c
 *
 * PURPOSE:     Interface between MME functions and MME Buddy's own.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

/*
    Sets the device into running or stopped state
*/

MMRESULT
MmeSetState(
    IN  DWORD_PTR PrivateHandle,
    IN  BOOL bStart)
{
    PWDMAUD_DEVICE_INFO DeviceInfo;

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    DeviceInfo = (PWDMAUD_DEVICE_INFO)PrivateHandle;

    VALIDATE_MMSYS_PARAMETER( DeviceInfo );

    /* Try change state */
    return FUNC_NAME(WdmAudSetWaveState)(DeviceInfo, bStart);
}

/*
    Call the client application when something interesting happens (MME API
    defines "interesting things" as device open, close, and buffer
    completion.)
*/
VOID
NotifyMmeClient(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  UINT Message,
    IN  DWORD_PTR Parameter)
{
    ASSERT( DeviceInfo );

    DPRINT("MME client callback - message %d, parameter %d\n",
              (int) Message,
              (int) Parameter);

    if ( DeviceInfo->dwCallback )
    {
        DriverCallback(DeviceInfo->dwCallback,
                       HIWORD(DeviceInfo->Flags),
                       DeviceInfo->hDevice,
                       Message,
                       DeviceInfo->dwInstance,
                       Parameter,
                       0);
    }
}

DWORD
MmeGetNumDevs(
    IN SOUND_DEVICE_TYPE DeviceType)
{
    MMRESULT Result;
    DWORD DeviceCount;

    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );

    Result = FUNC_NAME(WdmAudGetNumWdmDevs)(DeviceType, &DeviceCount);

    if ( ! MMSUCCESS(Result) )
    {
        DPRINT1("Error %d while obtaining number of devices\n", GetLastError());
        return 0;
    }

    DPRINT("%d devices of type %d found\n", DeviceCount, DeviceType);

    return DeviceCount;
}

/*
    Obtains the capabilities of a sound device. This routine ensures that the
    supplied CapabilitiesSize parameter at least meets the minimum size of the
    relevant capabilities structure.

    Ultimately, it will call the GetCapabilities function specified in the
    sound device's function table. Note that there are several of these, in a
    union. This is simply to avoid manually typecasting when implementing the
    functions.
*/
MMRESULT
MmeGetSoundDeviceCapabilities(
    IN  SOUND_DEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    PWDMAUD_DEVICE_INFO DeviceInfo;
    BOOL GoodSize = FALSE;
    MMRESULT Result;

    DPRINT("MME *_GETCAPS for device %d of type %d\n", DeviceId, DeviceType);

    /* FIXME: Validate device ID */
    VALIDATE_MMSYS_PARAMETER( Capabilities );
    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );

    /* Check that the capabilities structure is of a valid size */
    if ( DeviceType == WAVE_OUT_DEVICE_TYPE )
        GoodSize = CapabilitiesSize >= sizeof(WAVEOUTCAPSW);
    else if ( DeviceType == WAVE_IN_DEVICE_TYPE )
        GoodSize = CapabilitiesSize >= sizeof(WAVEINCAPSW);
    else if ( DeviceType == MIDI_OUT_DEVICE_TYPE )
        GoodSize = CapabilitiesSize >= sizeof(MIDIOUTCAPSW);
    else if ( DeviceType == MIDI_IN_DEVICE_TYPE )
        GoodSize = CapabilitiesSize >= sizeof(MIDIINCAPSW);
    else if ( DeviceType == AUX_DEVICE_TYPE )
        GoodSize = CapabilitiesSize >= sizeof(AUXCAPSW);
    else if ( DeviceType == MIXER_DEVICE_TYPE )
        GoodSize = CapabilitiesSize >= sizeof(MIXERCAPSW);

    if ( ! GoodSize )
    {
        DPRINT1("Device capabilities structure too small\n");
        return MMSYSERR_INVALPARAM;
    }

    DeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
    if (!DeviceInfo)
    {
        /* No memory */
        return MMSYSERR_NOMEM;
    }

    DeviceInfo->DeviceType = DeviceType;
    DeviceInfo->DeviceIndex = DeviceId;

    Result = FUNC_NAME(WdmAudGetCapabilities)(DeviceInfo,
                                              Capabilities,
                                              CapabilitiesSize);

    HeapFree(GetProcessHeap(), 0, DeviceInfo);

    return Result;
}

MMRESULT
MmeOpenDevice(
    IN  SOUND_DEVICE_TYPE DeviceType,
    IN  UINT DeviceId,
    IN  LPWAVEOPENDESC OpenParameters,
    IN  DWORD Flags,
    OUT DWORD_PTR* PrivateHandle)
{
    MMRESULT Result;
    UINT Message;
    PWDMAUD_DEVICE_INFO DeviceInfo;

    DPRINT("Opening device\n");

    VALIDATE_MMSYS_PARAMETER( IS_WAVE_DEVICE_TYPE(DeviceType) || IS_MIXER_DEVICE_TYPE(DeviceType) || IS_MIDI_DEVICE_TYPE(DeviceType) );    /* FIXME? wave in too? */
    VALIDATE_MMSYS_PARAMETER( OpenParameters );

    /* Check that winmm gave us a private handle to fill */
    VALIDATE_MMSYS_PARAMETER( PrivateHandle );

    /* Allocate a new WDMAUD_DEVICE_INFO structure */
    DeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
    if (!DeviceInfo)
    {
        /* No memory */
        return MMSYSERR_NOMEM;
    }

    /* Allocate a new WDMAUD_DEVICE_STATE structure */
    DeviceInfo->DeviceState = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_STATE));
    if (!DeviceInfo->DeviceState)
    {
        /* No memory */
        HeapFree(GetProcessHeap(), 0, DeviceInfo);
        return MMSYSERR_NOMEM;
    }

    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        /* Check if the caller just wanted to know if a format is supported */
        if ( Flags & WAVE_FORMAT_QUERY )
        {
#if 0
            DeviceInfo->DeviceType = DeviceType;
            DeviceInfo->DeviceIndex = DeviceId;
            DeviceInfo->Flags = Flags;

            Result = FUNC_NAME(WdmAudOpenSoundDevice)(DeviceInfo, OpenParameters->lpFormat);
            if (!MMSUCCESS(Result))
            {
                DPRINT1("Audio format is not supported\n");
                Result = WAVERR_BADFORMAT;
            }

            HeapFree(GetProcessHeap(), 0, DeviceInfo->DeviceState);
            HeapFree(GetProcessHeap(), 0, DeviceInfo);
            return Result;
#else
            return MMSYSERR_NOERROR;
#endif
        }
    }

    /* Allocate a new queue critical section */
    DeviceInfo->DeviceState->QueueCriticalSection = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CRITICAL_SECTION));
    if (!DeviceInfo->DeviceState->QueueCriticalSection)
    {
        /* No memory */
        HeapFree(GetProcessHeap(), 0, DeviceInfo->DeviceState);
        HeapFree(GetProcessHeap(), 0, DeviceInfo);
        return MMSYSERR_NOMEM;
    }

    /* Initialize it */
    InitializeCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

    DeviceInfo->DeviceType = DeviceType;
    DeviceInfo->DeviceIndex = DeviceId;
    DeviceInfo->hDevice = OpenParameters->hWave;
    DeviceInfo->dwCallback = OpenParameters->dwCallback;
    DeviceInfo->dwInstance = OpenParameters->dwInstance;
    DeviceInfo->Flags = Flags;
    DeviceInfo->hMixer = NULL;
    DeviceInfo->NotificationType = 0;
    DeviceInfo->Value = 0;
    DeviceInfo->DeviceState->WaveQueue = NULL;
    DeviceInfo->DeviceState->bStart = FALSE;
    DeviceInfo->DeviceState->bStartInThread = FALSE;
    DeviceInfo->DeviceState->hNotifyEvent = NULL;
    DeviceInfo->DeviceState->hStopEvent = NULL; 
    DeviceInfo->DeviceState->hThread = NULL;

    /* Open sound device */
    Result = FUNC_NAME(WdmAudOpenSoundDevice)(DeviceInfo, OpenParameters->lpFormat);
    if ( ! MMSUCCESS(Result) )
    {
        DeleteCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
        HeapFree(GetProcessHeap(), 0, DeviceInfo->DeviceState->QueueCriticalSection);
        HeapFree(GetProcessHeap(), 0, DeviceInfo->DeviceState);
        HeapFree(GetProcessHeap(), 0, DeviceInfo);
        return TranslateInternalMmResult(Result);
    }

    /* Store the device info pointer in the private handle */
    *PrivateHandle = (DWORD_PTR)DeviceInfo;

    if (DeviceType == WAVE_OUT_DEVICE_TYPE || DeviceType == WAVE_IN_DEVICE_TYPE ||
        DeviceType == MIDI_OUT_DEVICE_TYPE || DeviceType == MIDI_IN_DEVICE_TYPE)
    {
        /* Let the application know the device is open */

        if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
            Message = WOM_OPEN;
        else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
            Message = WIM_OPEN;
        else if (DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE)
            Message = MIM_OPEN;
        else if (DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE)
            Message = MOM_OPEN;

        ReleaseEntrypointMutex(DeviceType);

        NotifyMmeClient(DeviceInfo,
                        Message,
                        0);

        AcquireEntrypointMutex(DeviceType);
    }

    DPRINT("Device is now opened\n");

    return MMSYSERR_NOERROR;
}

MMRESULT
MmeCloseDevice(
    IN  DWORD_PTR PrivateHandle)
{
    MMRESULT Result;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    UINT Message = 0;

    DPRINT("Closing wave device (WIDM_CLOSE / WODM_CLOSE)\n");

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    DeviceInfo = (PWDMAUD_DEVICE_INFO)PrivateHandle;

    if ( ! DeviceInfo )
        return MMSYSERR_INVALHANDLE;

    /* TODO: Check device is stopped! */

    if (DeviceInfo->DeviceType != MIXER_DEVICE_TYPE)
    {
        ReleaseEntrypointMutex(DeviceInfo->DeviceType);

        if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
            Message = WOM_CLOSE;
        else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
            Message = WIM_CLOSE;
        else if (DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE)
            Message = MIM_CLOSE;
        else if (DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE)
            Message = MOM_CLOSE;

        /* TODO: Work with MIDI devices too */
        NotifyMmeClient(DeviceInfo,
                        Message,
                        0);

        AcquireEntrypointMutex(DeviceInfo->DeviceType);
    }

    /* Clode sound device */
    Result = FUNC_NAME(WdmAudCloseSoundDevice)(DeviceInfo, DeviceInfo->hDevice);

    /* Delete queue critical section */
    DeleteCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

    /* Free it */
    HeapFree(GetProcessHeap(), 0, DeviceInfo->DeviceState->QueueCriticalSection);

    /* Free device state */
    HeapFree(GetProcessHeap(), 0, DeviceInfo->DeviceState);

    /* Free device info */
    HeapFree(GetProcessHeap(), 0, DeviceInfo);

    return Result;
}

MMRESULT
MmeResetWavePlayback(
    IN  DWORD_PTR PrivateHandle)
{
    PWDMAUD_DEVICE_INFO DeviceInfo;

    DPRINT("Resetting wave device (WODM_RESET)\n");

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    DeviceInfo = (PWDMAUD_DEVICE_INFO) PrivateHandle;

    return StopStreaming(DeviceInfo);
}


MMRESULT
MmeGetPosition(
    IN  DWORD_PTR PrivateHandle,
    IN  MMTIME* Time,
    IN  DWORD Size)
{
    PWDMAUD_DEVICE_INFO DeviceInfo;

    VALIDATE_MMSYS_PARAMETER( PrivateHandle );
    DeviceInfo = (PWDMAUD_DEVICE_INFO) PrivateHandle;

    if ( ! DeviceInfo )
        return MMSYSERR_INVALHANDLE;

    if ( Size != sizeof(MMTIME) )
        return MMSYSERR_INVALPARAM;

    /* Call the driver */
    return FUNC_NAME(WdmAudGetWavePosition)(DeviceInfo, Time);
}

