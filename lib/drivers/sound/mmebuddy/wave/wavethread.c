/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Wave thread operations

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
        6 July 2008 - Restructured to hide some of the low-level threading

    TODO:
        Track if a buffer has already been inserted?
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>


/*
    Just a neat wrapper around the writing routine.
*/

MMRESULT
WriteWaveBufferToSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVE_THREAD_DATA ThreadData,
    IN  PWAVEHDR WaveHeader)
{
    SOUND_ASSERT(SoundDeviceInstance);
    SOUND_ASSERT(ThreadData);
    SOUND_ASSERT(WaveHeader);

    return OverlappedWriteToSoundDevice(SoundDeviceInstance,
                                        (PVOID) WaveHeader,
                                        WaveHeader->lpData,
                                        WaveHeader->dwBufferLength);
}


/*
    Private thread dispatch routines
*/

MMRESULT
SubmitWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVE_THREAD_DATA ThreadData,
    IN  PWAVEHDR Buffer)
{
    SOUND_ASSERT(Instance != NULL);
    SOUND_ASSERT(Buffer != NULL);
    SOUND_ASSERT(Instance->Thread != NULL);

    /* Store the device instance */
    Buffer->reserved = (DWORD) Instance;

    /* Set the head of the buffer list if this is the first buffer */
    if ( ! ThreadData->FirstBuffer )
    {
        ThreadData->FirstBuffer = Buffer;
    }

    /* Attach the buffer to the end of the list, unless this is the first */
    if ( ThreadData->LastBuffer )
    {
        ThreadData->LastBuffer->lpNext = Buffer;
    }

    /* Update our record of the last buffer */
    ThreadData->LastBuffer = Buffer;

    /* Increment the number of buffers queued */
    ++ ThreadData->BufferCount;

    /* HACK */
    ThreadData->CurrentBuffer = ThreadData->FirstBuffer;

    WriteWaveBufferToSoundDevice(Instance, ThreadData, Buffer);

/*
    Result = WriteSoundDeviceBuffer(Instance,
                                    ThreadData->CurrentBuffer->lpData,
                                    ThreadData->CurrentBuffer->dwBufferLength,
                                    WaveBufferCompleted,
                                    (LPOVERLAPPED) &Thread->Wave.Overlapped);
*/
    return MMSYSERR_NOERROR;
}


/*
    Thread request dispatcher
*/

MMRESULT
ProcessWaveThreadRequest(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PVOID PrivateThreadData,
    IN  DWORD RequestId,
    IN  PVOID Data)
{
    PWAVE_THREAD_DATA WaveThreadData =
        (PWAVE_THREAD_DATA) PrivateThreadData;

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

            return SubmitWaveBuffer(Instance, WaveThreadData, Buffer);
        }
    }

    return MMSYSERR_NOTSUPPORTED;
}


/*
    I/O completion
    Called from outside the overlapped I/O completion APC
*/

VOID
ProcessWaveIoCompletion(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  PVOID ContextData,
    IN  DWORD BytesTransferred)
{
    LPWAVEHDR WaveHeader = (LPWAVEHDR) ContextData;

    SOUND_DEBUG(L"ProcessWaveIoCompletion called :)");
    SOUND_DEBUG_HEX(WaveHeader);

    /*
        At this point we know:
        - The sound device instance involved in the transaction
        - The wave header for the buffer which just completed
        - How much data was transferred

        The next task is to figure out how much of the buffer
        got played, and enqueue the next buffer if the current
        one has been completed.

        Basically, this routine is responsible for keeping the
        stream of audio data to/from the sound driver going.

        When no more WAVEHDRs are queued (ie, WaveHeader->lpNext
        is NULL), no more buffers are submitted and the sound
        thread will wait for more requests from the client before
        it does anything else.
    */
}


/*
    Wave thread start/stop routines
*/

MMRESULT
StartWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    MMRESULT Result = MMSYSERR_NOERROR;
    PWAVE_THREAD_DATA WaveThreadData = NULL;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    WaveThreadData = AllocateMemoryFor(WAVE_THREAD_DATA);

    if ( ! WaveThreadData )
        return MMSYSERR_NOMEM;

    /* Initialise our data */
    WaveThreadData->CurrentBuffer = NULL;
    WaveThreadData->FirstBuffer = NULL;
    WaveThreadData->LastBuffer = NULL;
    WaveThreadData->BufferCount = 0;
    /* TODO: More */

    /* Kick off the thread */
    Result = StartSoundThread(Instance,
                              ProcessWaveThreadRequest,
                              ProcessWaveIoCompletion,
                              WaveThreadData);
    if ( Result != MMSYSERR_NOERROR )
    {
        FreeMemory(WaveThreadData);
        return Result;
    }

    /* AddSoundThreadOperation(Instance, 69, SayHello); */
    return MMSYSERR_NOERROR;
}

MMRESULT
StopWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    MMRESULT Result;
    PWAVE_THREAD_DATA WaveThreadData = NULL;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    Result = GetSoundThreadPrivateData(Instance, (PVOID*) &WaveThreadData);
    SOUND_ASSERT( Result == MMSYSERR_NOERROR );

    /* This shouldn't fail... */
    Result = StopSoundThread(Instance);
    SOUND_ASSERT( Result == MMSYSERR_NOERROR );

    FreeMemory(WaveThreadData);

    return MMSYSERR_NOERROR;
}
