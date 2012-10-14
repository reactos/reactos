/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/streaming.c
 *
 * PURPOSE:     Wave streaming
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "precomp.h"


/*
    DoWaveStreaming
        Check if there is streaming to be done, and if so, do it.
*/

VOID
DoWaveStreaming(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;
    PWAVEHDR Header;
    PWAVEHDR_EXTENSION HeaderExtension;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( MMSUCCESS(Result) );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( MMSUCCESS(Result) );

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( MMSUCCESS(Result) );
    SND_ASSERT( FunctionTable );
    SND_ASSERT( FunctionTable->CommitWaveBuffer );

    /* No point in doing anything if no resources available to use */
    if ( SoundDeviceInstance->OutstandingBuffers >= SoundDeviceInstance->BufferCount )
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

    while ( ( SoundDeviceInstance->OutstandingBuffers < SoundDeviceInstance->BufferCount ) &&
            ( Header ) && SoundDeviceInstance->ResetInProgress == FALSE)
    {
        HeaderExtension = (PWAVEHDR_EXTENSION) Header->reserved;
        SND_ASSERT( HeaderExtension );

        /* Saniy checks */
        SND_ASSERT(Header->dwFlags & WHDR_PREPARED);
        SND_ASSERT(Header->dwFlags & WHDR_INQUEUE);

        /* Can never be *above* the length */
        SND_ASSERT( HeaderExtension->BytesCommitted <= Header->dwBufferLength );

        /* Is this header entirely committed? */
        if ( HeaderExtension->BytesCommitted == Header->dwBufferLength )
        {
            {
                /* Move on to the next header */
                SND_ASSERT(Header != Header->lpNext);
                Header = Header->lpNext;
            }
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
            BytesToCommit = BytesRemaining > SoundDeviceInstance->FrameSize ?
                            SoundDeviceInstance->FrameSize :
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

                /* Don't complete this header if it's part of a loop */
                Overlap->PerformCompletion = TRUE;
//                    ( SoundDeviceInstance->LoopsRemaining > 0 );

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
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_OVERLAPPED SoundOverlapped = (PSOUND_OVERLAPPED) lpOverlapped;
    PWAVEHDR WaveHdr;
    PWAVEHDR_EXTENSION HdrExtension;
    MMRESULT Result;
    DWORD Bytes;

    WaveHdr = (PWAVEHDR) SoundOverlapped->Header;
    SND_ASSERT( WaveHdr );

    HdrExtension = (PWAVEHDR_EXTENSION) WaveHdr->reserved;
    SND_ASSERT( HdrExtension );

    SoundDeviceInstance = SoundOverlapped->SoundDeviceInstance;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( MMSUCCESS(Result) );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( MMSUCCESS(Result) );

    do
	{

        /* We have an available buffer now */
        -- SoundDeviceInstance->OutstandingBuffers;

        /* Did we finish a WAVEHDR and aren't looping? */
        if ( HdrExtension->BytesCompleted + dwNumberOfBytesTransferred >= WaveHdr->dwBufferLength &&
            SoundOverlapped->PerformCompletion )
        {
            /* Wave buffer fully completed */
            Bytes = WaveHdr->dwBufferLength - HdrExtension->BytesCompleted;

            HdrExtension->BytesCompleted += Bytes;
            dwNumberOfBytesTransferred -= Bytes;

            CompleteWaveHeader(SoundDeviceInstance, WaveHdr);
            SND_TRACE(L"%d/%d bytes of wavehdr completed\n", HdrExtension->BytesCompleted, WaveHdr->dwBufferLength);
        }
		else
		{
            /* Partially completed */
            HdrExtension->BytesCompleted += dwNumberOfBytesTransferred;
            SND_TRACE(L"%d/%d bytes of wavehdr completed\n", HdrExtension->BytesCompleted, WaveHdr->dwBufferLength);
            break;
		}

        /* Move to next wave header */
        WaveHdr = WaveHdr->lpNext;

        if (!WaveHdr)
		{
            /* No following WaveHdr */
            SND_ASSERT(dwNumberOfBytesTransferred == 0);
            break;
		}

        HdrExtension = (PWAVEHDR_EXTENSION) WaveHdr->reserved;
        SND_ASSERT( HdrExtension );


	}while(dwNumberOfBytesTransferred);

    // AUDIO-BRANCH DIFF
    // completion callback is performed in a thread
    DoWaveStreaming(SoundDeviceInstance);

    //CompleteWavePortion(SoundDeviceInstance, dwNumberOfBytesTransferred);

    FreeMemory(lpOverlapped);
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


/*
    Stream control functions
    (External/internal thread pairs)

    TODO - Move elsewhere as these shouldn't be wave specific!
*/

MMRESULT
StopStreamingInSoundThread(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    MMDEVICE_TYPE DeviceType;
    PMMFUNCTION_TABLE FunctionTable;
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;

    /* set state reset in progress */
    SoundDeviceInstance->ResetInProgress = TRUE;

    /* Get sound device */
    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Obtain device instance type */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Check if reset function is supported */
    if (FunctionTable->ResetStream)
    {
         /* cancel all current audio buffers */
         FunctionTable->ResetStream(SoundDeviceInstance, DeviceType, TRUE);
    }
    while(SoundDeviceInstance->OutstandingBuffers)
    {
        SND_TRACE(L"StopStreamingInSoundThread OutStandingBufferCount %lu\n", SoundDeviceInstance->OutstandingBuffers);
        /* wait until pending i/o has completed */
        SleepEx(10, TRUE);
    }

    /* complete all current headers */
    while( SoundDeviceInstance->HeadWaveHeader )
    {
        SND_TRACE(L"StopStreamingInSoundThread: Completing Header %p\n", SoundDeviceInstance->HeadWaveHeader);
        CompleteWaveHeader( SoundDeviceInstance, SoundDeviceInstance->HeadWaveHeader );
    }

    /* there should be no oustanding buffers now */
    SND_ASSERT(SoundDeviceInstance->OutstandingBuffers == 0);


    /* Check if reset function is supported */
    if (FunctionTable->ResetStream)
    {
        /* finish the reset */
        FunctionTable->ResetStream(SoundDeviceInstance, DeviceType, FALSE);
    }

    /* clear state reset in progress */
    SoundDeviceInstance->ResetInProgress = FALSE;


    return MMSYSERR_NOERROR;
}

MMRESULT
StopStreaming(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;

    if ( ! IsValidSoundDeviceInstance(SoundDeviceInstance) )
        return MMSYSERR_INVALHANDLE;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( DeviceType != WAVE_OUT_DEVICE_TYPE && DeviceType != WAVE_IN_DEVICE_TYPE )
        return MMSYSERR_NOTSUPPORTED;

    return CallSoundThread(SoundDeviceInstance,
                           StopStreamingInSoundThread,
                           NULL);
}

MMRESULT
PerformWaveStreaming(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    DoWaveStreaming(SoundDeviceInstance);

    return MMSYSERR_NOERROR;
}

DWORD
WINAPI
WaveActivateSoundStreaming(
    IN PVOID lpParameter)
{
    CallSoundThread((PSOUND_DEVICE_INSTANCE)lpParameter,
                    PerformWaveStreaming,
                    NULL);

    ExitThread(0);
}

VOID
InitiateSoundStreaming(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance)
{
    HANDLE hThread;

    hThread = CreateThread(NULL, 0, WaveActivateSoundStreaming, (PVOID)SoundDeviceInstance, 0, NULL);

    if (hThread != NULL)
        CloseHandle(hThread);
}
