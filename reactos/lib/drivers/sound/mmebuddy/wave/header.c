/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/header.c
 *
 * PURPOSE:     Wave header preparation and submission routines
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
    This structure gets used locally within functions as a way to shuttle data
    to the sound thread. It's safe to use locally since CallSoundThread will
    not return until the operation has been carried out.
*/

typedef struct
{
    MMWAVEHEADER_FUNC Function;
    PWAVEHDR Header;
} THREADED_WAVEHEADER_PARAMETERS;


/*
    Helper routines to simplify the call to the sound thread for the header
    functions.
*/

MMRESULT
WaveHeaderOperationInSoundThread(
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    THREADED_WAVEHEADER_PARAMETERS* Parameters = (THREADED_WAVEHEADER_PARAMETERS*) Parameter;
    return Parameters->Function(SoundDeviceInstance, Parameters->Header);
}

MMRESULT
WaveHeaderOperation(
    MMWAVEHEADER_FUNC Function,
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    PWAVEHDR Header)
{
    THREADED_WAVEHEADER_PARAMETERS Parameters;

    Parameters.Function = Function;
    Parameters.Header = Header;

    return CallSoundThread(SoundDeviceInstance,
                           WaveHeaderOperationInSoundThread,
                           &Parameters);
}


/*
    SanitizeWaveHeader
        Clean up a header / reinitialize
*/

VOID
SanitizeWaveHeader(
    PWAVEHDR Header)
{
    PWAVEHDR_EXTENSION Extension = (PWAVEHDR_EXTENSION) Header->reserved;
    SND_ASSERT( Extension );

    Header->dwBytesRecorded = 0;

    Extension->BytesCommitted = 0;
    Extension->BytesCompleted = 0;
}


/*
    The following routines are basically handlers for:
    - WODM_PREPARE
    - WODM_UNPREPARE
    - WODM_WRITE

    All of these calls are ultimately dealt with in the context of the
    appropriate sound thread, so the implementation should expect itself to
    be running in this other thread when any of these operations take place.
*/

MMRESULT
PrepareWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;
    PWAVEHDR_EXTENSION Extension;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Header );

    SND_TRACE(L"Preparing wave header\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Extension = AllocateStruct(WAVEHDR_EXTENSION);
    if ( ! Extension )
        return MMSYSERR_NOMEM;

    Header->reserved = (DWORD_PTR) Extension;
    Extension->BytesCommitted = 0;
    Extension->BytesCompleted = 0;

    /* Configure the flags */
    Header->dwFlags |= WHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

MMRESULT
UnprepareWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;
    PWAVEHDR_EXTENSION Extension;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Header );

    SND_TRACE(L"Un-preparing wave header\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    SND_ASSERT( Header->reserved );
    Extension = (PWAVEHDR_EXTENSION) Header->reserved;
    FreeMemory(Extension);

    /* Configure the flags */
    Header->dwFlags &= ~WHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

MMRESULT
WriteWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PMMFUNCTION_TABLE FunctionTable;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Header );

    SND_TRACE(L"Submitting wave header\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    if ( ! FunctionTable->CommitWaveBuffer )
        return MMSYSERR_NOTSUPPORTED;

    /*
        A few minor sanity checks - any custom checks should've been carried
        out during wave header preparation etc.
    */
    VALIDATE_MMSYS_PARAMETER( Header->lpData != NULL );
    VALIDATE_MMSYS_PARAMETER( Header->dwBufferLength > 0 );
    VALIDATE_MMSYS_PARAMETER( Header->dwFlags & WHDR_PREPARED );
    VALIDATE_MMSYS_PARAMETER( ! (Header->dwFlags & WHDR_INQUEUE) );

    SanitizeWaveHeader(Header);

    /* Clear the "done" flag for the buffer */
    Header->dwFlags &= ~WHDR_DONE;

    Result = CallSoundThread(SoundDeviceInstance,
                             EnqueueWaveHeader,
                             Header);

    return Result;
}


/*
    EnqueueWaveHeader
        Put the header in the record/playback queue. This is performed within
        the context of the sound thread, it must NEVER be called from another
        thread.

    CompleteWaveHeader
        Set the header information to indicate that it has finished playing,
        and return it to the client application. This again must be called
        within the context of the sound thread.
*/

MMRESULT
EnqueueWaveHeader(
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter)
{
    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( Parameter );

    PWAVEHDR WaveHeader = (PWAVEHDR) Parameter;

    /* Initialise */
    WaveHeader->lpNext = NULL;

    /* Set the "in queue" flag */
    WaveHeader->dwFlags |= WHDR_INQUEUE;

    if ( ! SoundDeviceInstance->TailWaveHeader )
    {
        /* This is the first header in the queue */
        SND_TRACE(L"Enqueued first wave header\n");
        SoundDeviceInstance->HeadWaveHeader = WaveHeader;
        SoundDeviceInstance->TailWaveHeader = WaveHeader;

        DoWaveStreaming(SoundDeviceInstance);
    }
    else
    {
        /* There are already queued headers - make this one the tail */
        SND_TRACE(L"Enqueued next wave header\n");
        SoundDeviceInstance->TailWaveHeader->lpNext = WaveHeader;
        SoundDeviceInstance->TailWaveHeader = WaveHeader;

        DoWaveStreaming(SoundDeviceInstance);
    }

    DUMP_WAVEHDR_QUEUE(SoundDeviceInstance);

    return MMSYSERR_NOERROR;
}

VOID
CompleteWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header)
{
    PWAVEHDR PrevHdr = NULL, CurrHdr = NULL;
    PWAVEHDR_EXTENSION Extension;
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    MMRESULT Result;

    SND_TRACE(L"BUFFER COMPLETE :)\n");

    // TODO: Set header flags?
    // TODO: Call client
    // TODO: Streaming

    //DoWaveStreaming(SoundDeviceInstance);

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( MMSUCCESS(Result) );
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( MMSUCCESS(Result) );

    Extension = (PWAVEHDR_EXTENSION)Header->reserved;
    SND_ASSERT( Extension );

    /* Remove the header from the queue, like so */
    if ( SoundDeviceInstance->HeadWaveHeader == Header )
    {
        SoundDeviceInstance->HeadWaveHeader = Header->lpNext;

        SND_TRACE(L"Dropping head node\n");

        /* If nothing after the head, then there is no tail */
        if ( Header->lpNext == NULL )
        {
            SND_TRACE(L"Dropping tail node\n");
            SoundDeviceInstance->TailWaveHeader = NULL;
        }
    }
    else
    {
        PrevHdr = NULL;
        CurrHdr = SoundDeviceInstance->HeadWaveHeader;

        SND_TRACE(L"Relinking nodes\n");

        while ( CurrHdr != Header )
        {
            PrevHdr = CurrHdr;
            CurrHdr = CurrHdr->lpNext;
            SND_ASSERT( CurrHdr );
        }

        SND_ASSERT( PrevHdr );

        PrevHdr->lpNext = CurrHdr->lpNext;

        /* If this is the tail node, update the tail */
        if ( Header->lpNext == NULL )
        {
            SND_TRACE(L"Updating tail node\n");
            SoundDeviceInstance->TailWaveHeader = PrevHdr;
        }
    }

    /* Make sure we're not using this as the current buffer any more, either! */
    if ( SoundDeviceInstance->CurrentWaveHeader == Header )
    {
        SoundDeviceInstance->CurrentWaveHeader = Header->lpNext;
    }

    DUMP_WAVEHDR_QUEUE(SoundDeviceInstance);

    SND_TRACE(L"Returning buffer to client...\n");

    /* Update the header */
    Header->dwFlags &= ~WHDR_INQUEUE;
    Header->dwFlags |= WHDR_DONE;

    if ( DeviceType == WAVE_IN_DEVICE_TYPE )
    {
        // FIXME: We won't be called on incomplete buffer!
        Header->dwBytesRecorded = Extension->BytesCompleted;
    }

    /* Safe to do this without thread protection, as we're done with the header */
    NotifyMmeClient(SoundDeviceInstance,
                    DeviceType == WAVE_OUT_DEVICE_TYPE ? WOM_DONE : WIM_DATA,
                    (DWORD) Header);
}
