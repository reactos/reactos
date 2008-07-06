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
*/

/*
    This is used internally by the internal routines
    (my, how recursive of you...)
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>


DWORD WINAPI
SoundThreadProc(
    IN  LPVOID lpParameter)
{
    PSOUND_DEVICE_INSTANCE Instance;
    PSOUND_THREAD Thread;
    /*HANDLE Events[2];*/

    Instance = (PSOUND_DEVICE_INSTANCE) lpParameter;
    Thread = Instance->Thread;

/*
    Events[0] = Thread->KillEvent;
    Events[1] = Thread->RequestEvent;
*/

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
            /* Do we need to do anything special here? */
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
