/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/thread.c
 *
 * PURPOSE:     Multimedia thread management
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

DWORD WINAPI
SoundThreadMain(
    IN  LPVOID lpParameter OPTIONAL)
{
    PSOUND_THREAD Thread = (PSOUND_THREAD) lpParameter;

    SND_TRACE(L"SoundThread running :)\n");

    /* Callers will wait for us to be ready */
    Thread->Running = TRUE;
    SetEvent(Thread->Events.Ready);

    while ( Thread->Running )
    {
        DWORD WaitResult;

        /* Wait for a request, or an I/O completion */
        WaitResult = WaitForSingleObjectEx(Thread->Events.Request, INFINITE, TRUE);
        SND_TRACE(L"SoundThread - Came out of waiting\n");

        if ( WaitResult == WAIT_OBJECT_0 )
        {
            SND_TRACE(L"SoundThread - Processing request\n");

            if ( Thread->Request.Handler )
            {
                Thread->Request.Result = Thread->Request.Handler(Thread->Request.SoundDeviceInstance,
                                                                 Thread->Request.Parameter);
            }
            else
            {
                Thread->Request.Result = MMSYSERR_ERROR;
            }

            /* Announce completion of the request */
            SetEvent(Thread->Events.Done);
            /* Accept new requests */
            SetEvent(Thread->Events.Ready);
        }
        else if ( WaitResult == WAIT_IO_COMPLETION )
        {
            SND_TRACE(L"SoundThread - Processing IO completion\n");
            /* TODO */
        }
        else
        {
            /* This should not happen! */
            SND_ASSERT(FALSE);
        }

    }

    return 0;
}

MMRESULT
CreateSoundThreadEvents(
    OUT HANDLE* ReadyEvent,
    OUT HANDLE* RequestEvent,
    OUT HANDLE* DoneEvent)
{
    BOOL ok;

    VALIDATE_MMSYS_PARAMETER( ReadyEvent );
    VALIDATE_MMSYS_PARAMETER( RequestEvent );
    VALIDATE_MMSYS_PARAMETER( DoneEvent );

    SND_TRACE(L"Creating thread events\n");

    /* Initialise these so we can identify them upon failure */
    *ReadyEvent = *RequestEvent = *DoneEvent = INVALID_HANDLE_VALUE;

    ok = (*ReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) != INVALID_HANDLE_VALUE;
    ok &= (*RequestEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) != INVALID_HANDLE_VALUE;
    ok &= (*DoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) != INVALID_HANDLE_VALUE;

    /* If something went wrong, clean up */
    if ( ! ok )
    {
        if ( *ReadyEvent != INVALID_HANDLE_VALUE )
            CloseHandle(*ReadyEvent);

        if ( *RequestEvent != INVALID_HANDLE_VALUE )
            CloseHandle(*RequestEvent);

        if ( *DoneEvent != INVALID_HANDLE_VALUE )
            CloseHandle(*DoneEvent);

        return MMSYSERR_NOMEM;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
DestroySoundThreadEvents(
    IN  HANDLE ReadyEvent,
    IN  HANDLE RequestEvent,
    IN  HANDLE DoneEvent)
{
    VALIDATE_MMSYS_PARAMETER( ReadyEvent != INVALID_HANDLE_VALUE );
    VALIDATE_MMSYS_PARAMETER( RequestEvent != INVALID_HANDLE_VALUE );
    VALIDATE_MMSYS_PARAMETER( DoneEvent != INVALID_HANDLE_VALUE );

    SND_TRACE(L"Destroying thread events\n");

    CloseHandle(ReadyEvent);
    CloseHandle(RequestEvent);
    CloseHandle(DoneEvent);

    return MMSYSERR_NOERROR;
}

MMRESULT
CreateSoundThread(
    OUT PSOUND_THREAD* Thread)
{
    MMRESULT Result;
    PSOUND_THREAD NewThread;

    VALIDATE_MMSYS_PARAMETER( Thread );

    NewThread = AllocateStruct(SOUND_THREAD);
    if ( ! NewThread )
        return MMSYSERR_NOMEM;

    /* Prepare the events we'll be using to sync. everything */
    Result = CreateSoundThreadEvents(&NewThread->Events.Ready,
                                     &NewThread->Events.Request,
                                     &NewThread->Events.Done);

    if ( ! MMSUCCESS(Result) )
    {
        FreeMemory(NewThread);
        return TranslateInternalMmResult(Result);
    }

    SND_TRACE(L"Creating a sound thread\n");
    NewThread->Handle = CreateThread(NULL,
                                     0,
                                     &SoundThreadMain,
                                     (LPVOID) NewThread,
                                     CREATE_SUSPENDED,
                                     NULL);

    /* Something went wrong, bail out! */
    if ( NewThread->Handle == INVALID_HANDLE_VALUE )
    {
        SND_ERR(L"Sound thread creation failed!\n");
        DestroySoundThreadEvents(NewThread->Events.Ready,
                                 NewThread->Events.Request,
                                 NewThread->Events.Done);

        FreeMemory(NewThread);

        return Win32ErrorToMmResult(GetLastError());
    }

    /* Wake the thread up */
    if ( ResumeThread(NewThread->Handle) == -1 )
    {
        SND_ERR(L"Failed to resume thread!\n");
        CloseHandle(NewThread->Handle);
        DestroySoundThreadEvents(NewThread->Events.Ready,
                                 NewThread->Events.Request,
                                 NewThread->Events.Done);

        FreeMemory(NewThread);
        return Win32ErrorToMmResult(GetLastError());
    }

    /* If all is well we can now give the thread to the caller */
    *Thread = NewThread;
    return MMSYSERR_NOERROR;
}

MMRESULT
DestroySoundThread(
    IN  PSOUND_THREAD Thread)
{
    VALIDATE_MMSYS_PARAMETER( Thread );
    SND_TRACE(L"Terminating sound thread\n");
    Thread->Running = FALSE;
    /* TODO: Implement me! Wait for thread to have finished? */
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
CallSoundThread(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  SOUND_THREAD_REQUEST_HANDLER RequestHandler,
    IN  PVOID Parameter OPTIONAL)
{
    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( RequestHandler );

    /* TODO: Don't call this directly? */
    PSOUND_THREAD Thread = SoundDeviceInstance->Thread;

    SND_TRACE(L"Waiting for READY event\n");
    WaitForSingleObject(Thread->Events.Ready, INFINITE);

    Thread->Request.Result = MMSYSERR_NOTSUPPORTED;
    Thread->Request.Handler = RequestHandler;
    Thread->Request.SoundDeviceInstance = SoundDeviceInstance;
    Thread->Request.Parameter = Parameter;

    /* Notify the thread it has work to do */
    SND_TRACE(L"Setting REQUEST event\n");
    SetEvent(Thread->Events.Request);

    /* Wait for the work to be done */
    SND_TRACE(L"Waiting for DONE event\n");
    WaitForSingleObject(Thread->Events.Done, INFINITE);

    return Thread->Request.Result;
}
