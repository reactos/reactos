/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Sound device processing thread

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
        5 July 2008 - Implemented basic request processing
        6 July 2008 - Added I/O completion handling

    Possible improvements:
        Spawn *one* thread to deal with requests and I/O completion, rather
        than have a thread per sound device instance. This wouldn't be too
        hard to do but not worth doing right now.
*/

/*
    This is used internally by the internal routines
    (my, how recursive of you...)
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>


/*
    This is the sound thread's processing loop. Its job is to aid in
    asynchronous streaming of sound data with a kernel-mode driver. It's
    basically a loop in which we wait for a request from the client, or
    completed data from the kernel-mode driver.

    When either of these events occur, the relevant details get passed on
    to one of the routines specified when the thread was started.
*/
DWORD WINAPI
SoundThreadProc(
    IN  LPVOID lpParameter)
{
    PSOUND_DEVICE_INSTANCE Instance;
    PSOUND_THREAD Thread;

    Instance = (PSOUND_DEVICE_INSTANCE) lpParameter;
    Thread = Instance->Thread;

    Thread->Running = TRUE;

    /* We're ready to do work */
    SetEvent(Thread->ReadyEvent);

    /*MessageBox(0, L"Hi from thread!", L"Hi!", MB_OK | MB_TASKMODAL);*/

    while ( Thread->Running )
    {
        DWORD WaitResult;

        /* Wait for some work, or I/O completion */
        WaitResult = WaitForSingleObjectEx(Thread->RequestEvent, INFINITE, TRUE);

        if ( WaitResult == WAIT_OBJECT_0 )
        {
            /* Do the work (request 0 kills the thread) */
            Thread->Request.Result =
                Thread->RequestHandler(Instance,
                                       Thread->PrivateData,
                                       Thread->Request.RequestId,
                                       Thread->Request.Data);

            if ( Thread->Request.RequestId == 0 )
            {
                Thread->Running = FALSE;
                Thread->Request.Result = MMSYSERR_NOERROR;
            }

            /* Notify the caller that the work is done */
            SetEvent(Thread->ReadyEvent);
            SetEvent(Thread->DoneEvent);
        }
        else if ( WaitResult == WAIT_IO_COMPLETION )
        {
            /* This gets called after I/O completion */
            PSOUND_THREAD_COMPLETED_IO CompletionData;
            SOUND_ASSERT(Thread->FirstCompletedIo);

            SOUND_DEBUG(L"Outside I/O completion APC");

            /*
                Purge the completed data queue
                FIXME? This will be done in the WRONG ORDER!
                Is this such a problem? The caller won't care. We'll need
                to remove them from our queue though!
            */
            while ( (CompletionData = Thread->FirstCompletedIo) )
            {
                /* Call high-level custom I/O completion routine */
                Thread->IoCompletionHandler(Instance,
                                            CompletionData->ContextData,
                                            CompletionData->BytesTransferred);

                /* TODO: I'm sure I've forgotten something ... */

                Thread->FirstCompletedIo = CompletionData->Next;
                FreeMemory(CompletionData);
            }
        }
    }

    /*MessageBox(0, L"Bye from thread!", L"Bye!", MB_OK | MB_TASKMODAL);*/

    ExitThread(0);
    return 0;
}

MMRESULT
CreateThreadEvents(
    IN  PSOUND_THREAD Thread)
{
    /* Create the request event */
    Thread->RequestEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Thread->RequestEvent )
    {
        return MMSYSERR_NOMEM;
    }

    /* Create the 'ready' event */
    Thread->ReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Thread->ReadyEvent )
    {
        CloseHandle(Thread->RequestEvent);
        return MMSYSERR_NOMEM;
    }

    /* Create the 'done' event */
    Thread->DoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Thread->DoneEvent )
    {
        CloseHandle(Thread->ReadyEvent);
        CloseHandle(Thread->RequestEvent);
        return MMSYSERR_NOMEM;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
DestroyThreadEvents(
    IN  PSOUND_THREAD Thread)
{
    CloseHandle(Thread->RequestEvent);
    Thread->RequestEvent = INVALID_HANDLE_VALUE;

    CloseHandle(Thread->ReadyEvent);
    Thread->ReadyEvent = INVALID_HANDLE_VALUE;

    CloseHandle(Thread->DoneEvent);
    Thread->DoneEvent = INVALID_HANDLE_VALUE;

    return MMSYSERR_NOERROR;
}


MMRESULT
StartSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  SOUND_THREAD_REQUEST_HANDLER RequestHandler,
    IN  SOUND_THREAD_IO_COMPLETION_HANDLER IoCompletionHandler,
    IN  LPVOID PrivateThreadData)
{
    PSOUND_THREAD SoundThread = NULL;

    /* Validate parameters */
    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    if ( ! RequestHandler )
        return MMSYSERR_INVALPARAM;

    /* Only allowed one thread per instance */
    if ( Instance->Thread )
        return MMSYSERR_ERROR;

    /* Allocate memory for the thread info */
    SoundThread = AllocateMemoryFor(SOUND_THREAD);

    if ( ! SoundThread )
        return MMSYSERR_NOMEM;

    /* Initialise */
    SoundThread->PrivateData = PrivateThreadData;
    SoundThread->Running = FALSE;
    SoundThread->Handle = INVALID_HANDLE_VALUE;

    SoundThread->RequestHandler = RequestHandler;
    SoundThread->ReadyEvent = INVALID_HANDLE_VALUE;
    SoundThread->RequestEvent = INVALID_HANDLE_VALUE;
    SoundThread->DoneEvent = INVALID_HANDLE_VALUE;

    SoundThread->IoCompletionHandler = IoCompletionHandler;
    SoundThread->FirstCompletedIo = NULL;

    /* No need to initialise the requests */

    /* Create the events */
    if ( CreateThreadEvents(SoundThread) != MMSYSERR_NOERROR )
    {
        FreeMemory(SoundThread);
        return MMSYSERR_NOMEM;
    }

    if ( ! SoundThread->RequestEvent )
    {
        CloseHandle(SoundThread->RequestEvent);
        FreeMemory(SoundThread);
        return MMSYSERR_NOMEM;
    }

    /* Do the creation thang */
    SoundThread->Handle = CreateThread(NULL,
                                       0,
                                       &SoundThreadProc,
                                       (LPVOID) Instance,
                                       CREATE_SUSPENDED,
                                       NULL);

    if (SoundThread->Handle == INVALID_HANDLE_VALUE )
    {
        FreeMemory(SoundThread);
        return Win32ErrorToMmResult(GetLastError());
    }

    /* Assign the thread to the instance */
    Instance->Thread = SoundThread;

    /* Go! */
    ResumeThread(SoundThread->Handle);

    return MMSYSERR_NOERROR;
}

MMRESULT
StopSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    if ( ! Instance->Thread )
        return MMSYSERR_ERROR;

    /* Send request zero to ask the thread to end */
    CallSoundThread(Instance, 0, NULL);

    /* Wait for the thread to exit */
    WaitForSingleObject(Instance->Thread->Handle, INFINITE);

    /* Finish with the thread */
    CloseHandle(Instance->Thread->Handle);
    Instance->Thread->Handle = INVALID_HANDLE_VALUE;

    /* Clean up the thread events */
    DestroyThreadEvents(Instance->Thread);

    /* Free memory associated with the thread */
    FreeMemory(Instance->Thread);
    Instance->Thread = NULL;

    return MMSYSERR_NOERROR;
}

MMRESULT
CallSoundThread(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD RequestId,
    IN  PVOID RequestData)
{
    MMRESULT Result;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! SoundDeviceInstance->Thread )
        return MMSYSERR_ERROR;

    /* Wait for the thread to be ready */
    WaitForSingleObject(SoundDeviceInstance->Thread->ReadyEvent, INFINITE);

    /* Load the request */
    SoundDeviceInstance->Thread->Request.DeviceInstance = SoundDeviceInstance;
    SoundDeviceInstance->Thread->Request.RequestId = RequestId;
    SoundDeviceInstance->Thread->Request.Data = RequestData;
    SoundDeviceInstance->Thread->Request.Result = MMSYSERR_NOTSUPPORTED;

    /* Notify the thread that there's a request to be processed */
    SetEvent(SoundDeviceInstance->Thread->RequestEvent);

    /* Wait for the thread to be ready (request complete) */
    WaitForSingleObject(SoundDeviceInstance->Thread->DoneEvent, INFINITE);

    /* Grab the result */
    Result = SoundDeviceInstance->Thread->Request.Result;

    return Result;
}

MMRESULT
GetSoundThreadPrivateData(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PVOID* PrivateData)
{
    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! SoundDeviceInstance->Thread )
        return MMSYSERR_ERROR;

    if ( ! PrivateData )
        return MMSYSERR_INVALPARAM;

    *PrivateData = SoundDeviceInstance->Thread->PrivateData;

    return MMSYSERR_NOERROR;
}

VOID CALLBACK
CompleteSoundThreadIo(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance;
    PSOUND_THREAD_OVERLAPPED SoundThreadOverlapped;
    PSOUND_THREAD_COMPLETED_IO CompletionData;

    SoundThreadOverlapped = (PSOUND_THREAD_OVERLAPPED) lpOverlapped;
    SOUND_ASSERT(SoundThreadOverlapped);

    CompletionData = SoundThreadOverlapped->CompletionData;
    SOUND_ASSERT(CompletionData);

    SoundDeviceInstance = SoundThreadOverlapped->SoundDeviceInstance;
    SOUND_ASSERT(SoundDeviceInstance);
    SOUND_ASSERT(SoundDeviceInstance->Thread);

    SOUND_DEBUG(L"New I/O Completion Callback Called");

    /* This is going at the start of the list */
    CompletionData->Next = SoundDeviceInstance->Thread->FirstCompletedIo;
    SoundDeviceInstance->Thread->FirstCompletedIo = CompletionData;

    /* Whatever information was supplied to us originally by the caller */
    CompletionData->ContextData = SoundThreadOverlapped->ContextData;

    /* How much data was transferred */
    CompletionData->BytesTransferred = dwNumberOfBytesTransferred;

    /* Overlapped structure gets freed now, but we still need the completion */
    FreeMemory(SoundThreadOverlapped);

    SOUND_DEBUG(L"New I/O Completion Callback Done");
}

MMRESULT
OverlappedWriteToSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID ContextData,
    IN  PVOID Buffer,
    IN  DWORD BufferSize)
{
    PSOUND_THREAD_OVERLAPPED Overlapped;
    PSOUND_THREAD_COMPLETED_IO CompletedIo;

    if ( ! SoundDeviceInstance )
        return MMSYSERR_INVALPARAM;

    if ( ! SoundDeviceInstance->Thread )
        return MMSYSERR_ERROR;  /* FIXME - better return code? */

    if ( ! Buffer )
        return MMSYSERR_INVALPARAM;

    /* This will contain information about the write operation */
    Overlapped = AllocateMemoryFor(SOUND_THREAD_OVERLAPPED);
    if ( ! Overlapped )
        return MMSYSERR_NOMEM;

    /* We collect this on I/O completion */
    CompletedIo = AllocateMemoryFor(SOUND_THREAD_COMPLETED_IO);
    if ( ! CompletedIo )
    {
        FreeMemory(Overlapped);
        return MMSYSERR_NOMEM;
    }

    ZeroMemory(Overlapped, sizeof(SOUND_THREAD_OVERLAPPED));
    ZeroMemory(CompletedIo, sizeof(SOUND_THREAD_COMPLETED_IO));

    /* We'll want to know which device to queue completion data to */
    Overlapped->SoundDeviceInstance = SoundDeviceInstance;

    /* Caller-supplied data, gets passed back on completion */
    Overlapped->ContextData = ContextData;

    /* The completion data buffer which will be filled later */
    Overlapped->CompletionData = CompletedIo;

    return WriteSoundDeviceBuffer(SoundDeviceInstance,
                                  Buffer,
                                  BufferSize,
                                  CompleteSoundThreadIo,
                                  (LPOVERLAPPED) Overlapped);
}
