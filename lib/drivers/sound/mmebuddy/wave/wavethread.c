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
#include <debug.h>

#include <mmebuddy.h>


/*
    How much we can feed to the driver at a time. For example, 2 buffers
    of 65536 would mean we can send 65536 bytes in a single I/O operation,
    and a total of 2 buffers (not necessarily full).

    If a single WAVEHDR is larger than MAX_SOUND_BUFFER_SIZE then a second
    buffer will be used.
*/
#define MAX_SOUND_BUFFER_SIZE   65536
#define MAX_SOUND_BUFFERS       2

VOID
CompleteWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter,
    IN  DWORD BytesWritten);

BOOLEAN
StreamReadyForData(
    IN  PWAVE_STREAM_INFO StreamInfo)
{
    ASSERT(StreamInfo);

    return (StreamInfo->BuffersOutstanding < MAX_SOUND_BUFFERS);
}

BOOLEAN
StreamHasBuffersQueued(
    IN  PWAVE_STREAM_INFO StreamInfo)
{
    ASSERT(StreamInfo);

    return (StreamInfo->CurrentBuffer != NULL);
}

DWORD
PerformWaveIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PWAVE_STREAM_INFO StreamInfo;
    DWORD BytesToStream, BytesStreamed = 0;

    DPRINT("PerformWaveIo\n");

    ASSERT(SoundDeviceInstance);

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    /* If we're out of buffers, mark stream as stopped and do nothing */
    if ( ! StreamInfo->CurrentBuffer )
    {
        DPRINT("*** NOTHING TO DO ***\n");
        return 0;
    }

    /* Work out how much buffer can be submitted */
    BytesToStream = MinimumOf(StreamInfo->CurrentBuffer->dwBufferLength -
                                StreamInfo->CurrentBuffer->reserved,
                              MAX_SOUND_BUFFER_SIZE);

    DPRINT("About to report what I'm about to write...\n");

    DPRINT("Writing %p + %d (%d bytes) - buffer length is %d bytes\n",
                StreamInfo->CurrentBuffer->lpData,
                (int) StreamInfo->CurrentBuffer->reserved,
                (int) BytesToStream,
                (int) StreamInfo->CurrentBuffer->dwBufferLength);

    /* TODO: Error checking */
    OverlappedSoundDeviceIo(SoundDeviceInstance,
                            (PCHAR) StreamInfo->CurrentBuffer->lpData +
                                StreamInfo->CurrentBuffer->reserved,
                            BytesToStream,
                            CompleteWaveBuffer,
                            (PVOID) StreamInfo->CurrentBuffer);

    /* FIXME? - find out how much was actually sent? */
    BytesStreamed = BytesToStream;

    /* Advance the offset */
    StreamInfo->CurrentBuffer->reserved += BytesStreamed;

    /* If we've hit the end of the buffer, move to the next one */
    if ( StreamInfo->CurrentBuffer->reserved ==
            StreamInfo->CurrentBuffer->dwBufferLength )
    {
        DPRINT("Advancing to next buffer\n");
        StreamInfo->CurrentBuffer = StreamInfo->CurrentBuffer->lpNext;
    }

    /* Increase the number of outstanding buffers */
    ++ StreamInfo->BuffersOutstanding;

    return BytesStreamed;
}

MMRESULT
StreamWaveBuffers(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PWAVE_STREAM_INFO StreamInfo;
    ASSERT(SoundDeviceInstance);

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    DPRINT("<== Streaming wave I/O ==>\n");
    while ( StreamReadyForData(StreamInfo) &&
            StreamHasBuffersQueued(StreamInfo) )
    {
        DPRINT("Performing wave I/O ...\n");
        PerformWaveIo(SoundDeviceInstance);
    }
    DPRINT("<== Done streaming ==>\n");

    return MMSYSERR_NOERROR;
}

VOID
CompleteWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter,
    IN  DWORD BytesWritten)
{
    PWAVE_STREAM_INFO StreamInfo;

    DPRINT("CompleteWaveBuffer(%p, %p, %d)\n",
           SoundDeviceInstance,
           Parameter,
          (int) BytesWritten);

    ASSERT(SoundDeviceInstance);

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    /* Decrease the number of outstanding buffers */
    ASSERT(StreamInfo->BuffersOutstanding > 0);
    -- StreamInfo->BuffersOutstanding;

    PerformWaveIo(SoundDeviceInstance);

    DPRINT("Wave completion routine done\n");
}

MMRESULT
QueueBuffer_Request(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    PWAVEHDR WaveHeader = (PWAVEHDR) Parameter;
    PWAVE_STREAM_INFO StreamInfo;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! WaveHeader )
        return MMSYSERR_INVALPARAM;

    /* To avoid stupidly long variable names we alias this */
    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    DPRINT("QueueBuffer_Request\n");

    /* Initialise fields of interest to us */
    WaveHeader->lpNext = NULL;
    WaveHeader->reserved = 0;

    /*
        Is this the first buffer being queued? Streaming only needs to be
        done here if there's nothing else playing.
    */
    if ( ! StreamInfo->BufferQueueHead )
    {
        DPRINT("This is the first buffer being queued\n");

        /* Set head, tail and current to this buffer */
        StreamInfo->BufferQueueHead = WaveHeader;
        StreamInfo->BufferQueueTail = WaveHeader;
        StreamInfo->CurrentBuffer = WaveHeader;

        /* Initialise the stream state */
        //StreamInfo->BufferOffset = 0;
        //StreamInfo->BytesOutstanding = 0;
        StreamInfo->BuffersOutstanding = 0;

        /* Get the streaming started */
        StreamWaveBuffers(SoundDeviceInstance);
    }
    else
    {
        DPRINT("This is not the first buffer being queued\n");

        /* Point the existing tail to the new buffer */
        StreamInfo->BufferQueueTail->lpNext = WaveHeader;
        /* ...and set the buffer as the new tail */
        StreamInfo->BufferQueueTail = WaveHeader;

        if ( ! StreamInfo->CurrentBuffer )
        {
            /* All buffers so far have been committed to the sound driver */
            StreamInfo->CurrentBuffer = WaveHeader;
        }
    }

    return MMSYSERR_NOERROR;
}


MMRESULT
QueueWaveDeviceBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR BufferHeader)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! BufferHeader )
        return MMSYSERR_INVALPARAM;

    if ( ! BufferHeader->lpData )
        return MMSYSERR_INVALPARAM;

    if ( ! BufferHeader->dwBufferLength )
        return MMSYSERR_INVALPARAM;

    if ( ! (BufferHeader->dwFlags & WHDR_PREPARED ) )
        return WAVERR_UNPREPARED;

    /* TODO: WHDR_INQUEUE */

    BufferHeader->dwFlags &= ~WHDR_DONE;
    BufferHeader->lpNext = NULL;

    return CallUsingSoundThread(SoundDeviceInstance,
                                QueueBuffer_Request,
                                BufferHeader);
}
