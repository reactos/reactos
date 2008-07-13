/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/thread.c
 *
 * PURPOSE:     Manages a thread to assist with the streaming of sound data.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>

static BOOLEAN ThreadRunning = FALSE;
static HANDLE SoundThread = INVALID_HANDLE_VALUE;
static HANDLE ReadyEvent = INVALID_HANDLE_VALUE;
static HANDLE RequestEvent = INVALID_HANDLE_VALUE;
static HANDLE DoneEvent = INVALID_HANDLE_VALUE;

static SOUND_THREAD_REQUEST CurrentRequest =
{
    NULL,
    NULL,
    NULL,
    MMSYSERR_NOERROR
};

static SOUND_THREAD_COMPLETED_IO* CompletedIoListHead = NULL;
static SOUND_THREAD_COMPLETED_IO* CompletedIoListTail = NULL;


VOID
CleanupThreadEvents();

VOID CALLBACK
CompleteSoundThreadIo(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    PSOUND_THREAD_COMPLETED_IO CompletionData;
    PSOUND_THREAD_OVERLAPPED SoundOverlapped;

    TRACE_("** I/O has completed\n");
    TRACE_("** Returned overlapped at %p\n", lpOverlapped);

    SoundOverlapped = (PSOUND_THREAD_OVERLAPPED) lpOverlapped;
    ASSERT(SoundOverlapped);

    CompletionData = SoundOverlapped->CompletionData;
    ASSERT(CompletionData);

    CompletionData->BytesTransferred = dwNumberOfBytesTransferred;

    /* Is this the first completion? */
    if ( ! CompletedIoListHead )
    {
        TRACE_("First completion - making new head and tail\n");
        /* This is the first completion */
        ASSERT(! CompletedIoListTail);

        CompletionData->Previous = NULL;
        CompletionData->Next = NULL;

        CompletedIoListHead = CompletionData;
        CompletedIoListTail = CompletionData;
    }
    else
    {
        TRACE_("Not the first completion - making new tail\n");
        ASSERT(CompletionData);
        /* This is not the first completion */
        CompletionData->Previous = CompletedIoListTail;
        CompletionData->Next = NULL;

        /* Completion data gets made the new tail */
        ASSERT(CompletedIoListTail);
        CompletedIoListTail->Next = CompletionData;
        CompletedIoListTail = CompletionData;
    }

    /* We keep the completion data, but no longer need this: */
    FreeMemory(SoundOverlapped);

    TRACE_("** Leaving completion routine\n");
}

MMRESULT
OverlappedSoundDeviceIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Buffer,
    IN  DWORD BufferSize,
    IN  SOUND_THREAD_IO_COMPLETION_HANDLER IoCompletionHandler,
    IN  PVOID CompletionParameter OPTIONAL)
{
    PSOUND_THREAD_OVERLAPPED Overlapped;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Buffer );
    VALIDATE_MMSYS_PARAMETER( BufferSize > 0 );
    VALIDATE_MMSYS_PARAMETER( IoCompletionHandler );

    /* Allocate memory for the overlapped I/O structure (auto-zeroed) */
    Overlapped = AllocateMemoryFor(SOUND_THREAD_OVERLAPPED);

    if ( ! Overlapped )
        return MMSYSERR_NOMEM;

    /* We also need memory for the completion data (auto-zeroed) */
    Overlapped->CompletionData = AllocateMemoryFor(SOUND_THREAD_COMPLETED_IO);

    if ( ! Overlapped->CompletionData )
    {
        FreeMemory(Overlapped);
        return MMSYSERR_NOMEM;
    }

    /* Information to be passed to the completion routine */
    Overlapped->CompletionData->SoundDeviceInstance = SoundDeviceInstance;
    Overlapped->CompletionData->CompletionHandler = IoCompletionHandler;
    Overlapped->CompletionData->Parameter = CompletionParameter;
    Overlapped->CompletionData->BytesTransferred = 0;

    TRACE_("Performing overlapped I/O\n");
    return WriteSoundDeviceBuffer(SoundDeviceInstance,
                                  Buffer,
                                  BufferSize,
                                  CompleteSoundThreadIo,
                                  (LPOVERLAPPED) Overlapped);
}

MMRESULT
ProcessThreadExitRequest(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance OPTIONAL,
    IN  PVOID Parameter OPTIONAL)
{
    TRACE_("ProcessThreadExitRequest called\n");
    ThreadRunning = FALSE;
    return MMSYSERR_NOERROR;
}

DWORD WINAPI
SoundThreadProc(
    IN  LPVOID lpParameter OPTIONAL)
{
    ThreadRunning = TRUE;

    /* We're ready to do work */
    SetEvent(ReadyEvent);

    while ( ThreadRunning )
    {
        DWORD WaitResult;

        /* Wait for a request, or an I/O completion */
        WaitResult = WaitForSingleObjectEx(RequestEvent, INFINITE, TRUE);
        TRACE_("Came out of waiting\n");

        if ( WaitResult == WAIT_OBJECT_0 )
        {
            /* Process the request */

            TRACE_("Processing request\n");

            ASSERT(CurrentRequest.RequestHandler);
            if ( CurrentRequest.RequestHandler )
            {
                TRACE_("Calling function %p\n", CurrentRequest.RequestHandler);
                CurrentRequest.ReturnValue = CurrentRequest.RequestHandler(
                    CurrentRequest.SoundDeviceInstance,
                    CurrentRequest.Parameter);
            }
            else
            {
                CurrentRequest.ReturnValue = MMSYSERR_ERROR;
            }

            /* Announce completion of the request */
            SetEvent(DoneEvent);
            /* Accept new requests */
            SetEvent(ReadyEvent);
        }
        else if ( WaitResult == WAIT_IO_COMPLETION )
        {
            PSOUND_THREAD_COMPLETED_IO CurrentCompletion;

            /* Process the I/O Completion */
            TRACE_("Returned from I/O completion APC\n");

            CurrentCompletion = CompletedIoListHead;
            ASSERT(CurrentCompletion);

            TRACE_("Beginning enumeration of completions\n");
            while ( CurrentCompletion )
            {
                PSOUND_THREAD_COMPLETED_IO PreviousCompletion;

                TRACE_("Calling completion handler\n");
                /* Call the completion handler */
                CurrentCompletion->CompletionHandler(
                    CurrentCompletion->SoundDeviceInstance,
                    CurrentCompletion->Parameter,
                    CurrentCompletion->BytesTransferred);

                TRACE_("Advancing to next completion\n");
                /* Get the next completion but destroy the previous */
                PreviousCompletion = CurrentCompletion;
                CurrentCompletion = CurrentCompletion->Next;
                /*CompletedIoListHead = CurrentCompletion;*/

                FreeMemory(PreviousCompletion);
            }

            /* Nothing in the completion queue/list now */
            CompletedIoListHead = NULL;
            CompletedIoListTail = NULL;
            TRACE_("Ended enumeration of completions\n");
        }
        else
        {
            /* Shouldn't happen! */
            ASSERT(FALSE);
        }
    }

    TRACE_("THREAD: Exiting\n");

    ExitThread(0);
    return 0;
}

MMRESULT
CreateThreadEvents()
{
    ReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ReadyEvent == INVALID_HANDLE_VALUE )
    {
        CleanupThreadEvents();
        return MMSYSERR_NOMEM;
    }

    RequestEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( RequestEvent == INVALID_HANDLE_VALUE )
    {
        CleanupThreadEvents();
        return MMSYSERR_NOMEM;
    }

    DoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( DoneEvent == INVALID_HANDLE_VALUE )
    {
        CleanupThreadEvents();
        return MMSYSERR_NOMEM;
    }

    return MMSYSERR_NOERROR;
}

VOID
CleanupThreadEvents()
{
    if ( ReadyEvent != INVALID_HANDLE_VALUE )
    {
        CloseHandle(ReadyEvent);
        ReadyEvent = INVALID_HANDLE_VALUE;
    }

    if ( RequestEvent != INVALID_HANDLE_VALUE )
    {
        CloseHandle(RequestEvent);
        RequestEvent = INVALID_HANDLE_VALUE;
    }

    if ( DoneEvent != INVALID_HANDLE_VALUE )
    {
        CloseHandle(DoneEvent);
        DoneEvent = INVALID_HANDLE_VALUE;
    }
}

MMRESULT
StartSoundThread()
{
    MMRESULT Result;

    /* Create the thread events */
    Result = CreateThreadEvents();

    if ( Result != MMSYSERR_NOERROR )
    {
        return Result;
    }

    /* Create the thread */
    SoundThread = CreateThread(NULL,
                               0,
                               &SoundThreadProc,
                               (LPVOID) NULL,   /* Parameter */
                               CREATE_SUSPENDED,
                               NULL);

    if ( SoundThread == INVALID_HANDLE_VALUE )
    {
        CleanupThreadEvents();
        return Win32ErrorToMmResult(GetLastError());
    }

    TRACE_("Starting sound thread\n");

    /* We're all set to go, so let's start the thread up */
    ResumeThread(SoundThread);

    return MMSYSERR_NOERROR;
}

MMRESULT
CallUsingSoundThread(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance OPTIONAL,
    IN  SOUND_THREAD_REQUEST_HANDLER RequestHandler,
    IN  PVOID Parameter OPTIONAL)
{
    VALIDATE_MMSYS_PARAMETER( RequestHandler );

    ASSERT(SoundThread != INVALID_HANDLE_VALUE);
    if ( SoundThread == INVALID_HANDLE_VALUE )
    {
        return MMSYSERR_ERROR;
    }

    /* Wait for the sound thread to be ready for a request */
    WaitForSingleObject(ReadyEvent, INFINITE);

    /* Fill in the information about the request */
    CurrentRequest.SoundDeviceInstance = SoundDeviceInstance;
    CurrentRequest.RequestHandler = RequestHandler;
    CurrentRequest.Parameter = Parameter;
    CurrentRequest.ReturnValue = MMSYSERR_ERROR;

    /* Tell the sound thread there is a request waiting */
    SetEvent(RequestEvent);

    /* Wait for our request to be dealt with */
    WaitForSingleObject(DoneEvent, INFINITE);

    return CurrentRequest.ReturnValue;
}

MMRESULT
StopSoundThread()
{
    MMRESULT Result;

    ASSERT(SoundThread != INVALID_HANDLE_VALUE);
    if ( SoundThread == INVALID_HANDLE_VALUE )
    {
        return MMSYSERR_ERROR;
    }

    TRACE_("Calling thread shutdown function\n");

    Result = CallUsingSoundThread(NULL,
                                  ProcessThreadExitRequest,
                                  NULL);

    /* Our request didn't get processed? */
    ASSERT(Result == MMSYSERR_NOERROR);
    if ( Result != MMSYSERR_NOERROR )
        return Result;

    WaitForSingleObject(SoundThread, INFINITE);

    TRACE_("Sound thread has quit\n");

    /* Clean up */
    CloseHandle(SoundThread);
    CleanupThreadEvents();

    return MMSYSERR_NOERROR;
}
