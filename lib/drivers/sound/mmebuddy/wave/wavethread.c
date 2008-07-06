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

MMRESULT
WriteWaveBufferToSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVE_THREAD_DATA ThreadData,
    IN  PWAVEHDR WaveHeader);



VOID CALLBACK
WaveBufferCompleted(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    PWAVE_OVERLAPPED WaveOverlapped = (PWAVE_OVERLAPPED) lpOverlapped;
    /*PWAVE_THREAD_DATA ThreadData = WaveOverlapped->ThreadData;*/
    WCHAR msg[1024];

    wsprintf(msg, L"Buffer %x done\nWrote %d bytes\nErrCode %d",
             WaveOverlapped->Header,
             dwNumberOfBytesTransferred,
             dwErrorCode);

    MessageBox(0, msg, L"File IO Callback", MB_OK | MB_TASKMODAL);

    WriteWaveBufferToSoundDevice(WaveOverlapped->SoundDeviceInstance,
                                 WaveOverlapped->ThreadData,
                                 WaveOverlapped->Header);
}

MMRESULT
WriteWaveBufferToSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVE_THREAD_DATA ThreadData,
    IN  PWAVEHDR WaveHeader)
{
    SOUND_ASSERT(SoundDeviceInstance);
    SOUND_ASSERT(ThreadData);
    SOUND_ASSERT(WaveHeader);

    /* Prepare our overlapped data */
    ZeroMemory(&ThreadData->Overlapped, sizeof(WAVE_OVERLAPPED));
    ThreadData->Overlapped.SoundDeviceInstance = SoundDeviceInstance;
    ThreadData->Overlapped.ThreadData = ThreadData;
    ThreadData->Overlapped.Header = WaveHeader;

    return WriteSoundDeviceBuffer(SoundDeviceInstance,
                                  WaveHeader->lpData,
                                  WaveHeader->dwBufferLength,
                                  WaveBufferCompleted,
                                  (LPOVERLAPPED) &ThreadData->Overlapped);
}


/* Internal dispatch routines */

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


/* Thread callback */

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
