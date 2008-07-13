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

    TRACE_("PerformWaveIo\n");

    ASSERT(SoundDeviceInstance);

    /* These shouldn't fail unless we pass them garbage */
    Result = GetSoundDeviceFromInstance(SoundDeviceInstance,
                                        &SoundDevice);

    ASSERT(Result == MMSYSERR_NOERROR);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    ASSERT(Result == MMSYSERR_NOERROR);
    ASSERT(IS_WAVE_DEVICE_TYPE(DeviceType));

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    /* If we're out of buffers, mark stream as stopped and do nothing */
    if ( ! StreamInfo->CurrentBuffer )
    {
        TRACE_("*** NOTHING TO DO - state is now WAVE_DD_IDLE ***\n");

        /* The stream is idle */
        StreamInfo->State = WAVE_DD_IDLE;

        return 0;
    }

    /* Work out how much buffer can be submitted */
    BytesToStream = MinimumOf(StreamInfo->CurrentBuffer->dwBufferLength -
                                StreamInfo->CurrentBuffer->reserved,
                              MAX_SOUND_BUFFER_SIZE);

    TRACE_("Writing %p + %d (%d bytes) - buffer length is %d bytes\n",
                StreamInfo->CurrentBuffer->lpData,
                (int) StreamInfo->CurrentBuffer->reserved,
                (int) BytesToStream,
                (int) StreamInfo->CurrentBuffer->dwBufferLength);

    /* Perform I/O */
    Result =  OverlappedSoundDeviceIo(
                        SoundDeviceInstance,
                        (PCHAR) StreamInfo->CurrentBuffer->lpData +
                                StreamInfo->CurrentBuffer->reserved,
                        BytesToStream,
                        CompleteWaveBuffer,
                        (PVOID) StreamInfo->CurrentBuffer);

    if ( Result != MMSYSERR_NOERROR )
    {
        ERR_("Failed to perform wave device I/O! MMSYS Error %d\n",
            (int) Result);
        return Result;
    }

    /* TODO: Deal with INPUT as well */
    if ( DeviceType == WAVE_OUT_DEVICE_TYPE )
    {
        TRACE_("Streamed data - state is now WAVE_DD_PLAYING\n");
        StreamInfo->State = WAVE_DD_PLAYING;
    }
    else
    {
    /* NOTE - MME wavein recording does not begin immediately!! */
    /*
        TRACE_("Streamed data - state is now WAVE_DD_RECORDING\n");
        StreamInfo->State = WAVE_DD_RECORDING;
    */
    }

    BytesStreamed = BytesToStream;

    /* Advance the offset */
    StreamInfo->CurrentBuffer->reserved += BytesStreamed;

    /* If we've hit the end of the buffer, move to the next one */
    if ( StreamInfo->CurrentBuffer->reserved ==
            StreamInfo->CurrentBuffer->dwBufferLength )
    {
        TRACE_("Advancing to next buffer\n");
        StreamInfo->CurrentBuffer = StreamInfo->CurrentBuffer->lpNext;
    }

    /* Increase the number of outstanding buffers */
    ++ StreamInfo->BuffersOutstanding;

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
        TRACE_("Performing wave I/O ...\n");
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

    TRACE_("CompleteWaveBuffer(%p, %p, %d)\n",
           SoundDeviceInstance,
           Parameter,
          (int) BytesWritten);

    ASSERT(SoundDeviceInstance);

    StreamInfo = &SoundDeviceInstance->Streaming.Wave;

    /* Decrease the number of outstanding buffers */
    ASSERT(StreamInfo->BuffersOutstanding > 0);
    -- StreamInfo->BuffersOutstanding;

    PerformWaveIo(SoundDeviceInstance);

    TRACE_("Wave completion routine done\n");
}

MMRESULT
QueueWaveBuffer_Request(
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

    TRACE_("QueueBuffer_Request\n");

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
        TRACE_("This is not the first buffer being queued\n");

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
GetWaveDeviceState_Request(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PVOID Parameter)
{
    PUCHAR State = (PUCHAR) Parameter;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! State )
        return MMSYSERR_INVALPARAM;

    *State = SoundDeviceInstance->Streaming.Wave.State;

    return MMSYSERR_NOERROR;
}

MMRESULT
PauseWaveDevice_Request(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    /* TODO */

    return MMSYSERR_NOERROR;
}

MMRESULT
ContinueWaveDevice_Request(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    /* TODO */

    return MMSYSERR_NOERROR;
}
