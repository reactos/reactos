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
#define MAX_SOUND_BUFFER_SIZE   65536



VOID
CompleteWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter,
    IN  DWORD BytesWritten);


MMRESULT
StreamWaveIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PWAVE_STREAM_INFO StreamInfo;
    DWORD BytesAvailable, BytesToStream;

    SOUND_TRACE("<== Streaming wave I/O ==>\n");

    SOUND_ASSERT(SoundDeviceInstance);

    /* Get the streaming info */
    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    BytesAvailable = MAX_SOUND_BUFFER_SIZE - StreamInfo->BytesOutstanding;

    if ( ! BytesAvailable )
    {
        SOUND_TRACE("NO BUFFER SPACE AVAILABLE\n");
    }

    if ( ! StreamInfo->CurrentBuffer )
    {
        SOUND_TRACE("NO CURRENT BUFFER\n");
    }

    while ( StreamInfo->CurrentBuffer && BytesAvailable )
    {
        /*
            Determine how much of the current buffer remains to be
            streamed.
        */
        BytesToStream = StreamInfo->CurrentBuffer->dwBufferLength -
                        StreamInfo->BufferOffset;

        SOUND_TRACE("Buffer %p, offset %d, length %d\nAvailable %d | BytesToStream %d | BytesOutstanding %d\n",
                StreamInfo->CurrentBuffer,
                (int) StreamInfo->BufferOffset,
                (int) StreamInfo->CurrentBuffer->dwBufferLength,
                (int) BytesAvailable,
                (int) BytesToStream,
                (int) StreamInfo->BytesOutstanding);


        /*
            We may receive a buffer with no bytes left to stream, for
            example as a result of an I/O completion triggering this
            routine, or if on a previous iteration of this loop we managed
            to finish sending a buffer.
        */
        if ( BytesToStream == 0 )
        {
            SOUND_TRACE("No bytes to stream\n");
            StreamInfo->CurrentBuffer = StreamInfo->CurrentBuffer->lpNext;
            StreamInfo->BufferOffset = 0;
            continue;
        }

        /*
            If the buffer can't be sent in its entirety to the sound driver,
            send a portion of it to fill the available space.
        */
        if ( BytesToStream > BytesAvailable )
        {
            BytesToStream = BytesAvailable;
        }

        SOUND_TRACE("Writing %d bytes from buffer %p, offset %d\n",
                    (int) BytesToStream,
                    StreamInfo->CurrentBuffer->lpData,
                    (int)StreamInfo->BufferOffset);

        /* TODO: Do the streaming */
        OverlappedSoundDeviceIo(SoundDeviceInstance,
                                (PCHAR) StreamInfo->CurrentBuffer->lpData +
                                    StreamInfo->BufferOffset,
                                BytesToStream,
                                CompleteWaveBuffer,
                                (PVOID) StreamInfo->CurrentBuffer);

        /*
            Keep track of the amount of data currently in the hands of the
            sound driver, so we know how much space is being used up so
            far, and how many bytes will be announced to the completion
            routines.
        */
        StreamInfo->BytesOutstanding += BytesToStream;

        /*
            Update the offset within the buffer to reflect the amount of
            data being transferred in this transaction.
        */
        StreamInfo->BufferOffset += BytesToStream;

        /*
            Update the number of bytes available for the next iteration.
        */
        BytesAvailable = MAX_SOUND_BUFFER_SIZE - StreamInfo->BytesOutstanding;

    }

    SOUND_TRACE("<== Done filling stream ==>\n");

#if 0
        /* TODO: Check result */
        OverlappedSoundDeviceIo(SoundDeviceInstance,
                                WaveHeader->lpData,
                                WaveHeader->dwBufferLength,
                                CompleteWaveBuffer,
                                (PVOID) WaveHeader);
#endif

    return MMSYSERR_NOERROR;
}


VOID
CompleteWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter,
    IN  DWORD BytesWritten)
{
    SOUND_TRACE("CompleteWaveBuffer called - wrote %d bytes\n", (int)BytesWritten);

    SoundDeviceInstance->Streaming.Wave.BytesOutstanding -= BytesWritten;

    StreamWaveIo(SoundDeviceInstance);
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

    SOUND_TRACE("QueueBuffer_Request\n");

    /* Initialise fields of interest to us */
    WaveHeader->lpNext = NULL;
    WaveHeader->reserved = 0;

    /*
        Is this the first buffer being queued? Streaming only needs to be
        done here if there's nothing else playing.
    */
    if ( ! StreamInfo->BufferQueueHead )
    {
        SOUND_TRACE("This is the first buffer being queued\n");

        /* Set head, tail and current to this buffer */
        StreamInfo->BufferQueueHead = WaveHeader;
        StreamInfo->BufferQueueTail = WaveHeader;
        StreamInfo->CurrentBuffer = WaveHeader;

        /* Initialise the stream state */
        StreamInfo->BufferOffset = 0;
        StreamInfo->BytesOutstanding = 0;

        /* Get the streaming started */
        StreamWaveIo(SoundDeviceInstance);
    }
    else
    {
        SOUND_TRACE("This is not the first buffer being queued\n");

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
