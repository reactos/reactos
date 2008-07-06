/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Wave thread operations

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created

    TODO:
        Track if a buffer has already been inserted?
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>

VOID CALLBACK
WaveBufferCompleted(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    MessageBox(0, L"Job done!", L"File IO Callback", MB_OK | MB_TASKMODAL);
}

/* Internal dispatch routines */

MMRESULT
SubmitWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEHDR Buffer)
{
    MMRESULT Result;

    SOUND_ASSERT(Instance != NULL);
    SOUND_ASSERT(Buffer != NULL);

    /* Set the head of the buffer list if this is the first buffer */
    if ( ! Instance->Wave.FirstBuffer )
    {
        Instance->Wave.FirstBuffer = Buffer;
    }

    /* Attach the buffer to the end of the list, unless this is the first */
    if ( Instance->Wave.LastBuffer )
    {
        Instance->Wave.LastBuffer->lpNext = Buffer;
    }

    /* Update our record of the last buffer */
    Instance->Wave.LastBuffer = Buffer;

    /* Increment the number of buffers queued */
    ++ Instance->Wave.BufferCount;

    /* HACK */
    Instance->Wave.CurrentBuffer = Instance->Wave.FirstBuffer;

    Result = WriteSoundDeviceBuffer(Instance,
                                    Instance->Wave.CurrentBuffer->lpData,
                                    Instance->Wave.CurrentBuffer->dwBufferLength,
                                    WaveBufferCompleted);

    return MMSYSERR_NOERROR;
}


/* Thread callback */

MMRESULT
ProcessWaveThreadRequest(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD RequestId,
    IN  PVOID Data)
{
    /* Just some temporary testing code for now */
    WCHAR msg[128];
    wsprintf(msg, L"Request %d received", RequestId);

    MessageBox(0, msg, L"Request", MB_OK | MB_TASKMODAL);

    SOUND_ASSERT(Instance != NULL);

    switch ( RequestId )
    {
        case WAVEREQUEST_QUEUE_BUFFER :
        {
            PWAVEHDR Buffer = (PWAVEHDR) Data;

            return SubmitWaveBuffer(Instance, Buffer);
        }
    }

    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
StartWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    MMRESULT Result = MMSYSERR_NOERROR;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    /* Initialise our data */
    Instance->Wave.CurrentBuffer = NULL;
    Instance->Wave.FirstBuffer = NULL;
    Instance->Wave.LastBuffer = NULL;
    Instance->Wave.BufferCount = 0;

    /* Kick off the thread */
    Result = StartSoundThread(Instance, ProcessWaveThreadRequest);
    if ( Result != MMSYSERR_NOERROR )
    {
        return Result;
    }

    /* AddSoundThreadOperation(Instance, 69, SayHello); */
    return MMSYSERR_NOERROR;
}

MMRESULT
StopWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    return StopSoundThread(Instance);
}
