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
    How much we can feed to the driver at a time.
    This is deliberately set low at the moment for testing purposes, and
    intentionally set to be "one out" from 16384 so that writing 65536 bytes
    results in 4x16383 byte buffers followed by 1x4 byte buffer.

    Should it be configurable? Some sound drivers let you set buffer size...
*/
#define MAX_SOUND_BUFFER_SIZE   16383


/*
    Audio buffer I/O streaming handler. Slices 'n dices your buffers and
    serves them up on a platter with a side dressing of your choice.

    It's safe to update the buffer information post-submission within this
    routine since any completion will be done in an APC for the same thread,
    so we're not likely to be rudely interrupted by a completion routine
    bursting through the door.
*/
MMRESULT
StreamWaveData(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVE_THREAD_DATA ThreadData)
{
    PWAVEHDR Buffer = ThreadData->CurrentBuffer;
    DWORD BufferOffset, BytesToStream, BytesAvailable;
    MMRESULT Result;

    SOUND_ASSERT(ThreadData->RemainingBytes <= MAX_SOUND_BUFFER_SIZE);
    SOUND_ASSERT(Buffer);

    /* The 'reserved' member of Buffer contains the offset */
    BufferOffset = Buffer->reserved;

    /* Work out how much data can be streamed with the driver */
    BytesAvailable = MAX_SOUND_BUFFER_SIZE - ThreadData->RemainingBytes;

    /*
        If no space available, don't do anything. The routine will be revisited
        as buffers complete, at which point the backlog will start being
        dealt with.
    */
    if ( BytesAvailable == 0 )
        return MMSYSERR_NOERROR;

    /*
        Adjust the amount of data to be streamed based on how much of the
        current buffer has actually been dealt with already.
    */
    BytesToStream = Buffer->dwBufferLength - BufferOffset;

    /*
        No point in doing work unless we have to... This is, however, a
        completed buffer, so it should be dealt with!!
    */
    if ( BytesToStream == 0 )
    {
        /* TODO */
        return MMSYSERR_NOERROR;
    }

    /*
        Now we know how much buffer is waiting to be provided to the driver,
        we need to consider how much the driver can accept.
    */
    if ( BytesToStream > BytesAvailable )
    {
        /* Buffer can't fit entirely - fill the available space */
        BytesToStream = BytesAvailable;
    }

    /*
        Now the audio buffer sets sail on its merry way to the sound driver...
        NOTE: Will need to implement a 'read' function, too.
    */
    Result = OverlappedWriteToSoundDevice(SoundDeviceInstance,
                                          (PVOID) ThreadData,
                                          Buffer->lpData + BufferOffset,
                                          BytesToStream);

    if ( Result != MMSYSERR_NOERROR )
        return Result;

    /* Update the offset ready for the next streaming operation */
    BufferOffset += BytesToStream;
    Buffer->reserved = BufferOffset;

    /* More data has been sent to the driver, so this is updated, too */
    ThreadData->RemainingBytes += BytesToStream;

    return MMSYSERR_NOERROR;
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
/*    DWORD BytesSubmitted = 0;*/

    SOUND_ASSERT(Instance != NULL);
    SOUND_ASSERT(Buffer != NULL);
    SOUND_ASSERT(Instance->Thread != NULL);

    /* This is used to mark the offset within the buffer */
    Buffer->reserved = 0;
    /* TODO: Clear completion flag */

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

    /* If nothing is playing, we'll need to start things off */
    if ( ThreadData->RemainingBytes == 0 )
    {
        ThreadData->CurrentBuffer = ThreadData->FirstBuffer;
        StreamWaveData(Instance, ThreadData);
    }

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
    PWAVE_THREAD_DATA ThreadData = (PWAVE_THREAD_DATA) ContextData;
    SOUND_ASSERT(ThreadData);
    WCHAR msg[1024];

    /*SOUND_DEBUG(L"ProcessWaveIoCompletion called :)");*/
    /*SOUND_DEBUG_HEX(WaveHeader);*/

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

    /* Discount the amount of buffer which has been processed */
    SOUND_ASSERT(BytesTransferred <= ThreadData->RemainingBytes);
    ThreadData->RemainingBytes -= BytesTransferred;

    wsprintf(msg, L"Wave header: %x\nOffset: %d\nTransferred: %d bytes\nRemaining: %d bytes",
             ThreadData->CurrentBuffer,
             ThreadData->CurrentBuffer->reserved,
             BytesTransferred,
             ThreadData->RemainingBytes);

    MessageBox(0, msg, L"I/O COMPLETE", MB_OK | MB_TASKMODAL);

    /* TODO: Check return value */
    StreamWaveData(Instance, ThreadData);
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
