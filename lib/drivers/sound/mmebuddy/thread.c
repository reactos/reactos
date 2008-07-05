/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Sound device processing thread

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
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
    HANDLE Events[2];

    Instance = (PSOUND_DEVICE_INSTANCE) lpParameter;
    Thread = Instance->Thread;

    Events[0] = Thread->KillEvent;
    Events[1] = Thread->RequestEvent;

    Thread->Running = TRUE;

    MessageBox(0, L"Hi from thread!", L"Hi!", MB_OK | MB_TASKMODAL);

    while ( Thread->Running )
    {
        DWORD Signalled = 0;

        Signalled = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

        if ( Signalled == WAIT_OBJECT_0 )
        {
            Thread->Running = FALSE;
        }
        else if ( Signalled == WAIT_OBJECT_0 + 1 )
        {
            /* ... */
        }
    }

    MessageBox(0, L"Bye from thread!", L"Bye!", MB_OK | MB_TASKMODAL);

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

    /* Create the request completion event */
    Thread->RequestCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Thread->RequestCompletionEvent )
    {
        CloseHandle(Thread->RequestEvent);
        return MMSYSERR_NOMEM;
    }

    /* Create the kill event */
    Thread->KillEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Thread->KillEvent )
    {
        CloseHandle(Thread->RequestCompletionEvent);
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
    CloseHandle(Thread->RequestCompletionEvent);
    CloseHandle(Thread->KillEvent);

    return MMSYSERR_NOERROR;
}


MMRESULT
StartSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    PSOUND_THREAD SoundThread = NULL;

    /* Validate parameters */
    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    /* Only allowed one thread per instance */
    if ( Instance->Thread )
        return MMSYSERR_ERROR;

    /* Allocate memory for the thread info */
    SoundThread = (PSOUND_THREAD) HeapAlloc(GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            sizeof(SOUND_THREAD));

    if ( ! SoundThread )
        return MMSYSERR_NOMEM;

    /* Initialise */
    SoundThread->Running = FALSE;
    SoundThread->Handle = INVALID_HANDLE_VALUE;
    SoundThread->FirstOperation = NULL;

    /* Create the events */
    if ( CreateThreadEvents(SoundThread) != MMSYSERR_NOERROR )
    {
        HeapFree(GetProcessHeap(), 0, SoundThread);
        return MMSYSERR_NOMEM;
    }

    if ( ! SoundThread->RequestEvent )
    {
        CloseHandle(SoundThread->RequestEvent);
        HeapFree(GetProcessHeap(), 0, SoundThread);
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
        HeapFree(GetProcessHeap(), 0, SoundThread);
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

    /* Make the thread quit */
    Instance->Thread->Running = FALSE;

    /* Wait for the thread to respond to our gentle nudge */
    SetEvent(Instance->Thread->KillEvent);
    WaitForSingleObject(Instance->Thread, INFINITE);
    CloseHandle(Instance->Thread);  /* correct way? */

    /* TODO: A bunch of other stuff - WAIT for thread to die */
    /* Also clean up the events */

    DestroyThreadEvents(Instance->Thread);
    HeapFree(GetProcessHeap(), 0, Instance->Thread);

    return MMSYSERR_NOERROR;
}


/*
    Thread must be started before calling this!
*/

MMRESULT
AddSoundThreadOperation(
    PSOUND_DEVICE_INSTANCE Instance,
    DWORD OperationId,
    SOUND_THREAD_OPERATION OperationFunc)
{
    /*SOUND_THREAD_OPERATION OriginalFirstOp;*/

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    if ( ! OperationFunc )
        return MMSYSERR_INVALPARAM;

    if ( ! Instance->Thread )
        return MMSYSERR_ERROR;

/*
    OriginalFirstOp = Instance->Thread->FirstOperation;
    Instance->Thread->FirstOperation->Next = 
*/

    return MMSYSERR_NOTSUPPORTED;
}
