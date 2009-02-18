/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/streaming.c
 *
 * PURPOSE:     Wave streaming
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>
#include <sndtypes.h>


/*
    Restrain ourselves from flooding the kernel device!
*/

#define SOUND_KERNEL_BUFFER_COUNT       10
#define SOUND_KERNEL_BUFFER_SIZE        16384


/*
    DoWaveStreaming
        Check if there is streaming to be done, and if so, do it.
*/

VOID
DoWaveStreaming(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;
    PWAVEHDR Header;
    PWAVEHDR_EXTENSION HeaderExtension;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( MMSUCCESS(Result) );

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( MMSUCCESS(Result) );
    SND_ASSERT( FunctionTable );
    SND_ASSERT( FunctionTable->CommitWaveBuffer );

    /* No point in doing anything if no resources available to use */
    if ( SoundDeviceInstance->OutstandingBuffers >= SOUND_KERNEL_BUFFER_COUNT )
    {
        SND_TRACE(L"DoWaveStreaming: No available buffers to stream with - doing nothing\n");
        return;
    }

    /* Is there any work to do? */
    Header = SoundDeviceInstance->HeadWaveHeader;

    if ( ! Header )
    {
        SND_TRACE(L"DoWaveStreaming: No work to do - doing nothing\n");
        return;
    }

    while ( ( SoundDeviceInstance->OutstandingBuffers < SOUND_KERNEL_BUFFER_COUNT ) &&
            ( Header ) )
    {
        HeaderExtension = (PWAVEHDR_EXTENSION) Header->reserved;
        SND_ASSERT( HeaderExtension );

        /* Can never be *above* the length */
        SND_ASSERT( HeaderExtension->BytesCommitted <= Header->dwBufferLength );

        if ( HeaderExtension->BytesCommitted == Header->dwBufferLength )
        {
            Header = Header->lpNext;
        }
        else
        {
            PSOUND_OVERLAPPED Overlap;
            LPVOID OffsetPtr;
            DWORD BytesRemaining, BytesToCommit;
            BOOL OK;

            /* Where within the header buffer to stream from */
            OffsetPtr = Header->lpData + HeaderExtension->BytesCommitted;

            /* How much of this header has not been committed */
            BytesRemaining = Header->dwBufferLength - HeaderExtension->BytesCommitted;

            /* We can commit anything up to the buffer size limit */
            BytesToCommit = BytesRemaining > SOUND_KERNEL_BUFFER_SIZE ?
                            SOUND_KERNEL_BUFFER_SIZE :
                            BytesRemaining;

            /* Should always have something to commit by this point */
            SND_ASSERT( BytesToCommit > 0 );

            /* We need a new overlapped info structure for each buffer */
            Overlap = AllocateStruct(SOUND_OVERLAPPED);

            if ( Overlap )
            {
                ZeroMemory(Overlap, sizeof(SOUND_OVERLAPPED));
                Overlap->SoundDeviceInstance = SoundDeviceInstance;
                Overlap->Header = Header;

                /* Adjust the commit-related counters */
                HeaderExtension->BytesCommitted += BytesToCommit;
                ++ SoundDeviceInstance->OutstandingBuffers;

                OK = MMSUCCESS(FunctionTable->CommitWaveBuffer(SoundDeviceInstance,
                                                               OffsetPtr,
                                                               BytesToCommit,
                                                               Overlap,
                                                               CompleteIO));

                if ( ! OK )
                {
                    /* Clean-up and try again on the next iteration (is this OK?) */
                    SND_WARN(L"FAILED\n");

                    FreeMemory(Overlap);
                    HeaderExtension->BytesCommitted -= BytesToCommit;
                    -- SoundDeviceInstance->OutstandingBuffers;
                }
            }
        }
    }


#if 0

    // HACK
    SND_TRACE(L"Calling buffer submit routine\n");

    if ( ! SoundDeviceInstance->CurrentWaveHeader )
    {
        /* Start from the beginning (always a good idea) */
        SoundDeviceInstance->CurrentWaveHeader = SoundDeviceInstance->HeadWaveHeader;
    }

    if ( SoundDeviceInstance->CurrentWaveHeader )
    {
        /* Stream or continue streaming this header */

        Result = CommitWaveHeaderToKernelDevice(SoundDeviceInstance,
                                                SoundDeviceInstance->CurrentWaveHeader,
                                                FunctionTable->CommitWaveBuffer);
    }
    else
    {
        SND_TRACE(L"NOTHING TO DO - REC/PLAY STOPPED\n");
    }

    return Result;
#endif
}


/*
    CompleteIO
        An APC called as a result of a call to CommitWaveHeaderToKernelDevice.
        This will count up the number of bytes which have been dealt with,
        and when the entire wave header has been dealt with, will call
        CompleteWaveHeader to have the wave header returned to the client.

    CommitWaveHeaderToKernelDevice
        Sends portions of the buffer described by the wave header to a kernel
        device. This must only be called from within the context of the sound
        thread. The caller supplies either their own commit routine, or uses
        WriteFileEx_Committer. The committer is called with portions of the
        buffer specified in the wave header.

    WriteFileEx_Committer
        Commit buffers using the WriteFileEx API.
*/

VOID CALLBACK
CompleteIO(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_OVERLAPPED SoundOverlapped = (PSOUND_OVERLAPPED) lpOverlapped;
    PWAVEHDR WaveHdr;
    PWAVEHDR_EXTENSION HdrExtension;

    WaveHdr = (PWAVEHDR) SoundOverlapped->Header;
    SND_ASSERT( WaveHdr );

    HdrExtension = (PWAVEHDR_EXTENSION) WaveHdr->reserved;
    SND_ASSERT( HdrExtension );

    SoundDeviceInstance = SoundOverlapped->SoundDeviceInstance;

    HdrExtension->BytesCompleted += dwNumberOfBytesTransferred;
    SND_TRACE(L"%d/%d bytes of wavehdr completed\n", HdrExtension->BytesCompleted, WaveHdr->dwBufferLength);

    /* We have an available buffer now */
    -- SoundDeviceInstance->OutstandingBuffers;

    if ( HdrExtension->BytesCompleted == WaveHdr->dwBufferLength )
    {
        CompleteWaveHeader(SoundDeviceInstance, WaveHdr);
    }

    DoWaveStreaming(SoundDeviceInstance);

    //CompleteWavePortion(SoundDeviceInstance, dwNumberOfBytesTransferred);

    FreeMemory(lpOverlapped);
}

MMRESULT
CommitWaveHeaderToKernelDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header,
    IN  WAVE_COMMIT_FUNC CommitFunction)
{
    PSOUND_OVERLAPPED Overlap;
    DWORD BytesToWrite, BytesRemaining;
    PWAVEHDR_EXTENSION HdrExtension;
    LPVOID Offset;

    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( Header );
    VALIDATE_MMSYS_PARAMETER( CommitFunction );

    HdrExtension = (PWAVEHDR_EXTENSION) Header->reserved;
    VALIDATE_MMSYS_PARAMETER( HdrExtension );

    /* Loop whilst there is data and sufficient available buffers */
    while ( ( SoundDeviceInstance->OutstandingBuffers < SOUND_KERNEL_BUFFER_COUNT ) &&
            ( HdrExtension->BytesCommitted < Header->dwBufferLength ) )
    {
        /* Is this the start of a loop? */
        SoundDeviceInstance->WaveLoopStart = Header;

        /* Where to start pulling the data from within the buffer */
        Offset = Header->lpData + HdrExtension->BytesCommitted;

        /* How much of this header is not committed? */
        BytesRemaining = Header->dwBufferLength - HdrExtension->BytesCommitted;

        /* We can write anything up to the buffer size limit */
        BytesToWrite = BytesRemaining > SOUND_KERNEL_BUFFER_SIZE ?
                       SOUND_KERNEL_BUFFER_SIZE :
                       BytesRemaining;

        /* If there's nothing left in the current header, move to the next */
        if ( BytesToWrite == 0 )
        {
            Header = Header->lpNext;
            HdrExtension = (PWAVEHDR_EXTENSION) Header->reserved;
            SND_ASSERT( HdrExtension );
            SND_ASSERT( HdrExtension->BytesCommitted == 0 );
            SND_ASSERT( HdrExtension->BytesCompleted == 0 );
            continue;
        }

        HdrExtension->BytesCommitted += BytesToWrite;

        /* We're using up a buffer so update this */
        ++ SoundDeviceInstance->OutstandingBuffers;

        SND_TRACE(L"COMMIT: Offset 0x%x amount %d remain %d totalcommit %d",
                  Offset, BytesToWrite, BytesRemaining, HdrExtension->BytesCommitted);

        /* We need a new overlapped info structure for each buffer */
        Overlap = AllocateStruct(SOUND_OVERLAPPED);

        if ( Overlap )
        {
            ZeroMemory(Overlap, sizeof(SOUND_OVERLAPPED));
            Overlap->SoundDeviceInstance = SoundDeviceInstance;
            Overlap->Header = Header;


            if ( ! MMSUCCESS(CommitFunction(SoundDeviceInstance, Offset, BytesToWrite, Overlap, CompleteIO)) )
            {
                /* Just pretend it played if we fail... Show must go on, etc. etc. */
                SND_WARN(L"FAILED\n");
                HdrExtension->BytesCompleted += BytesToWrite;
            }
        }
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WriteFileEx_Committer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine)
{
    HANDLE Handle;

    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( OffsetPtr );
    VALIDATE_MMSYS_PARAMETER( Overlap );
    VALIDATE_MMSYS_PARAMETER( CompletionRoutine );

    GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);

    if ( ! WriteFileEx(Handle, OffsetPtr, Length, (LPOVERLAPPED)Overlap, CompletionRoutine) )
    {
        // TODO
    }

    return MMSYSERR_NOERROR;
}
