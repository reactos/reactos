/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/streaming.c
 *
 * PURPOSE:     Streams wave audio data to/from a sound driver, within
 *              a separate thread.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

/*
    TODO: If loops == 0 do we play once, and loops == 1 play twice?
*/

#include <windows.h>
#include <mmsystem.h>

#include <ntddk.h>
#include <ntddsnd.h>

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

MMRESULT
PerformWaveIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;
    PWAVE_STREAM_INFO StreamInfo;
    DWORD BytesToStream, BytesStreamed = 0;
    UCHAR DeviceType;

    TRACE_ENTRY();

    ASSERT( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    /* These shouldn't fail unless we pass them garbage */
    Result = GetSoundDeviceFromInstance(SoundDeviceInstance,
                                        &SoundDevice);

    ASSERT(Result == MMSYSERR_NOERROR);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    ASSERT(Result == MMSYSERR_NOERROR);
    ASSERT(IS_WAVE_DEVICE_TYPE(DeviceType));

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    /* If we're out of buffers, do nothing */
    if ( ! StreamInfo->CurrentBuffer )
    {
        TRACE_("*** NOTHING TO DO ***\n");

        TRACE_EXIT(MMSYSERR_NOERROR);
        return MMSYSERR_NOERROR;
    }

    /* Is this the beginning of a loop? */
    if ( StreamInfo->CurrentBuffer->dwFlags & WHDR_BEGINLOOP )
    {
        /* Avoid infinite looping where the beginning and end are the same */
        if ( StreamInfo->LoopHead != StreamInfo->CurrentBuffer )
        {
            TRACE_("Beginning of loop\n");
            StreamInfo->LoopHead = StreamInfo->CurrentBuffer;
            StreamInfo->LoopsRemaining = StreamInfo->CurrentBuffer->dwLoops;
        }
    }

    /* Work out how much buffer can be submitted */
    BytesToStream = MinimumOf(StreamInfo->CurrentBuffer->dwBufferLength -
                                StreamInfo->BufferOffset,
                              MAX_SOUND_BUFFER_SIZE);

    TRACE_("Writing hdr %p - %p + %d (%d bytes) - buffer length is %d bytes\n",
                StreamInfo->CurrentBuffer,
                StreamInfo->CurrentBuffer->lpData,
                (int) StreamInfo->BufferOffset,
                (int) BytesToStream,
                (int) StreamInfo->CurrentBuffer->dwBufferLength);

    /* Perform I/O */
    Result =  OverlappedSoundDeviceIo(
                        SoundDeviceInstance,
                        (PCHAR) StreamInfo->CurrentBuffer->lpData +
                                StreamInfo->BufferOffset,
                        BytesToStream,
                        CompleteWaveBuffer,
                        (PVOID) StreamInfo->CurrentBuffer);

    if ( Result != MMSYSERR_NOERROR )
    {
        ERR_("Failed to perform wave device I/O! MMSYS Error %d\n",
            (int) Result);
        TRACE_EXIT(Result);
        return Result;
    }

    BytesStreamed = BytesToStream;

    /* Advance the offset */
    StreamInfo->BufferOffset += BytesStreamed;

    /* If we've hit the end of the buffer, move to the next one */
    if ( StreamInfo->BufferOffset ==
            StreamInfo->CurrentBuffer->dwBufferLength )
    {

        if ( ( StreamInfo->CurrentBuffer->dwFlags & WHDR_ENDLOOP ) &&
             ( StreamInfo->LoopHead ) &&
             ( StreamInfo->LoopsRemaining > 0 ) )
        {
            TRACE_("Returning to loop head at %p\n", StreamInfo->LoopHead);
            /* Loop back to the head */
            StreamInfo->CurrentBuffer = StreamInfo->LoopHead;
            -- StreamInfo->LoopsRemaining;
            TRACE_("Now %d loops remaining\n", (int) StreamInfo->LoopsRemaining);
        }
        else
        {
            TRACE_("Advancing from wavehdr %p to %p\n",
                   StreamInfo->CurrentBuffer,
                   StreamInfo->CurrentBuffer->lpNext);

            /* Either not looping, or looping expired */
            StreamInfo->CurrentBuffer = StreamInfo->CurrentBuffer->lpNext;
        }

        /* Reset the writing offset */
        StreamInfo->BufferOffset = 0;

        if ( StreamInfo->CurrentBuffer )
        {
            TRACE_("Resetting offset of %p to zero\n", StreamInfo->CurrentBuffer);
            /* Reset the completion offset */
            StreamInfo->CurrentBuffer->reserved = 0;
        }
    }

    /* Increase the number of outstanding buffers */
    ++ StreamInfo->BuffersOutstanding;

    TRACE_EXIT(MMSYSERR_NOERROR);
    return MMSYSERR_NOERROR;
}

MMRESULT
StreamWaveBuffers(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    MMRESULT Result;
    PWAVE_STREAM_INFO StreamInfo;
    ASSERT(SoundDeviceInstance);

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    TRACE_("<== Streaming wave I/O ==>\n");
    while ( StreamReadyForData(StreamInfo) &&
            StreamHasBuffersQueued(StreamInfo) )
    {
        Result = PerformWaveIo(SoundDeviceInstance);

        if ( Result != MMSYSERR_NOERROR )
            return Result;
    }
    TRACE_("<== Done streaming ==>\n");

    return MMSYSERR_NOERROR;
}

VOID
CompleteWaveBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter,
    IN  DWORD BytesWritten)
{
    PWAVE_STREAM_INFO StreamInfo;
    PWAVEHDR WaveHeader = (PWAVEHDR) Parameter;

    TRACE_("CompleteWaveBuffer(%p, %p, %d)\n",
           SoundDeviceInstance,
           Parameter,
          (int) BytesWritten);

    ASSERT(SoundDeviceInstance);
    ASSERT(WaveHeader);

    WaveHeader->reserved += BytesWritten;

    TRACE_("Transferred %d bytes - offset is now %d of length %d\n",
           (int) BytesWritten, (int) WaveHeader->reserved, (int) WaveHeader->dwBufferLength);

    ASSERT(WaveHeader->reserved <= WaveHeader->dwBufferLength);

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    /* Did we complete a header sent by the client? */
    if ( WaveHeader->reserved == WaveHeader->dwBufferLength )
    {
        TRACE_("* Completed wavehdr %p (length %d)\n",
                WaveHeader,
                (int) WaveHeader->dwBufferLength);

        if ( StreamInfo->LoopsRemaining > 0 )
        {
            /* Let's go round again... */
            WaveHeader->reserved = 0;

            /*
                FIXME:
                I have no idea wtf happens with completions - tests with the NT4
                mmdrv indicate that they just randomly throw back completed
                buffers later on. Like, a lot later on... Like, when the device
                is being reset.
            */
        }
        else
        {
            /* Mark the header as done */
            WaveHeader->dwFlags |= WHDR_DONE;

            /* Notify the client */
            NotifySoundClient(SoundDeviceInstance, WOM_DONE, (DWORD) WaveHeader);
        }
    }
    else
    {
        TRACE_("* Partial wavehdr completion %p (%d/%d)\n",
                WaveHeader,
                (int) WaveHeader->reserved,
                (int) WaveHeader->dwBufferLength);
    }

    /* Decrease the number of outstanding buffers */
    ASSERT(StreamInfo->BuffersOutstanding > 0);
    -- StreamInfo->BuffersOutstanding;

    PerformWaveIo(SoundDeviceInstance);

    TRACE_EXIT(0);
}

MMRESULT
QueueWaveBuffer_Request(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    PWAVEHDR WaveHeader = (PWAVEHDR) Parameter;
    PWAVE_STREAM_INFO StreamInfo;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( WaveHeader );

    /* To avoid stupidly long variable names we alias this */
    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    TRACE_("QueueBuffer_Request - wavehdr %p\n", WaveHeader);

    /* Initialise fields of interest to us */
    WaveHeader->lpNext = NULL;
    WaveHeader->reserved = 0;

    /*
        Is this the first buffer being queued? Streaming only needs to be
        done here if there's nothing else playing.
    */
    if ( ! StreamInfo->BufferQueueHead )
    {
        TRACE_("This is the first buffer being queued\n");

        /* Set head and tail to this buffer (current gets set later) */
        StreamInfo->BufferQueueHead = WaveHeader;
        StreamInfo->BufferQueueTail = WaveHeader;
        StreamInfo->CurrentBuffer = NULL;

        /* Initialise the stream state */
        StreamInfo->BufferOffset = 0;
        //StreamInfo->BytesOutstanding = 0;
        StreamInfo->BuffersOutstanding = 0;

        StreamInfo->LoopHead = NULL;
        StreamInfo->LoopsRemaining = 0;

        /* Get the streaming started */
//        StreamWaveBuffers(SoundDeviceInstance);
    }
    else
    {
        TRACE_("This is not the first buffer being queued\n");

        /* Point the existing tail to the new buffer */
        StreamInfo->BufferQueueTail->lpNext = WaveHeader;
        /* ...and set the buffer as the new tail */
        StreamInfo->BufferQueueTail = WaveHeader;
    }

    /* Do we need to push the play button? */
    if ( ! StreamInfo->CurrentBuffer )
    {
        /* All buffers so far have been committed to the sound driver */
        StreamInfo->CurrentBuffer = WaveHeader;

        StreamWaveBuffers(SoundDeviceInstance);
    }

    return MMSYSERR_NOERROR;
}
