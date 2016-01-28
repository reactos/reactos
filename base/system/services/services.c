/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/services.c
 * PURPOSE:     Main SCM controller
 * COPYRIGHT:   Copyright 2001-2005 Eric Kohl
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#include <wincon.h>

#define NDEBUG
#include <debug.h>

int WINAPI RegisterServicesProcess(DWORD ServicesProcessId);

/* GLOBALS ******************************************************************/

#define PIPE_BUFSIZE 1024
#define PIPE_TIMEOUT 1000

/* Defined in include/reactos/services/services.h */
// #define SCM_START_EVENT             L"SvcctrlStartEvent_A3752DX"
#define SCM_AUTOSTARTCOMPLETE_EVENT L"SC_AutoStartComplete"
#define LSA_RPC_SERVER_ACTIVE       L"LSA_RPC_SERVER_ACTIVE"

BOOL ScmInitialize = FALSE;
BOOL ScmShutdown = FALSE;
static HANDLE hScmShutdownEvent = NULL;


/* FUNCTIONS *****************************************************************/

VOID
PrintString(LPCSTR fmt, ...)
{
#if DBG
    CHAR buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buffer);
#endif
}


VOID
ScmLogEvent(DWORD dwEventId,
            WORD wType,
            WORD wStrings,
            LPCWSTR *lpStrings)
{
    HANDLE hLog;

    hLog = RegisterEventSourceW(NULL,
                                L"Service Control Manager");
    if (hLog == NULL)
    {
        DPRINT1("ScmLogEvent: RegisterEventSourceW failed %lu\n", GetLastError());
        return;
    }

    if (!ReportEventW(hLog,
                      wType,
                      0,
                      dwEventId,
                      NULL,
                      wStrings,
                      0,
                      lpStrings,
                      NULL))
    {
        DPRINT1("ScmLogEvent: ReportEventW failed %lu\n", GetLastError());
    }

    DeregisterEventSource(hLog);
}


VOID
ScmWaitForLsa(VOID)
{
    HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, LSA_RPC_SERVER_ACTIVE);
    if (hEvent == NULL)
    {
        DPRINT1("Failed to create the notification event (Error %lu)\n", GetLastError());
    }
    else
    {
        DPRINT("Wait for the LSA server!\n");
        WaitForSingleObject(hEvent, INFINITE);
        DPRINT("LSA server running!\n");
        CloseHandle(hEvent);
    }

    DPRINT("ScmWaitForLsa() done\n");
}


BOOL
ScmNamedPipeHandleRequest(PVOID Request,
                          DWORD RequestSize,
                          PVOID Reply,
                          LPDWORD ReplySize)
{
    DbgPrint("SCM READ: %p\n", Request);

    *ReplySize = 0;
    return FALSE;
}


DWORD WINAPI
ScmNamedPipeThread(LPVOID Context)
{
    CHAR chRequest[PIPE_BUFSIZE];
    CHAR chReply[PIPE_BUFSIZE];
    DWORD cbReplyBytes;
    DWORD cbBytesRead;
    DWORD cbWritten;
    BOOL bSuccess;
    HANDLE hPipe;

    hPipe = (HANDLE)Context;

    DPRINT("ScmNamedPipeThread(%p) - Accepting SCM commands through named pipe\n", hPipe);

    for (;;)
    {
        bSuccess = ReadFile(hPipe,
                            &chRequest,
                            PIPE_BUFSIZE,
                            &cbBytesRead,
                            NULL);
        if (!bSuccess || cbBytesRead == 0)
        {
            break;
        }

        if (ScmNamedPipeHandleRequest(&chRequest, cbBytesRead, &chReply, &cbReplyBytes))
        {
            bSuccess = WriteFile(hPipe,
                                 &chReply,
                                 cbReplyBytes,
                                 &cbWritten,
                                 NULL);
            if (!bSuccess || cbReplyBytes != cbWritten)
            {
                break;
            }
        }
    }

    DPRINT("ScmNamedPipeThread(%p) - Disconnecting named pipe connection\n", hPipe);

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    DPRINT("ScmNamedPipeThread(%p) - Done.\n", hPipe);

    return ERROR_SUCCESS;
}


BOOL
ScmCreateNamedPipe(VOID)
{
    DWORD dwThreadId;
    BOOL bConnected;
    HANDLE hThread;
    HANDLE hPipe;

    DPRINT("ScmCreateNamedPipe() - CreateNamedPipe(\"\\\\.\\pipe\\Ntsvcs\")\n");

    hPipe = CreateNamedPipeW(L"\\\\.\\pipe\\Ntsvcs",
              PIPE_ACCESS_DUPLEX,
              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
              PIPE_UNLIMITED_INSTANCES,
              PIPE_BUFSIZE,
              PIPE_BUFSIZE,
              PIPE_TIMEOUT,
              NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        DPRINT("CreateNamedPipe() failed (%lu)\n", GetLastError());
        return FALSE;
    }

    DPRINT("CreateNamedPipe() - calling ConnectNamedPipe(%p)\n", hPipe);
    bConnected = ConnectNamedPipe(hPipe,
                                  NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    DPRINT("CreateNamedPipe() - ConnectNamedPipe() returned %d\n", bConnected);

    if (bConnected)
    {
        DPRINT("Pipe connected\n");
        hThread = CreateThread(NULL,
                               0,
                               ScmNamedPipeThread,
                               (LPVOID)hPipe,
                               0,
                               &dwThreadId);
        if (!hThread)
        {
            DPRINT("Could not create thread (%lu)\n", GetLastError());
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            DPRINT("CreateNamedPipe() - returning FALSE\n");
            return FALSE;
        }

        CloseHandle(hThread);
    }
    else
    {
        DPRINT("Pipe not connected\n");
        CloseHandle(hPipe);
        DPRINT("CreateNamedPipe() - returning FALSE\n");
        return FALSE;
    }
    DPRINT("CreateNamedPipe() - returning TRUE\n");
    return TRUE;
}


DWORD WINAPI
ScmNamedPipeListenerThread(LPVOID Context)
{
//    HANDLE hPipe;
    DPRINT("ScmNamedPipeListenerThread(%p) - aka SCM.\n", Context);

//    hPipe = (HANDLE)Context;
    for (;;)
    {
        DPRINT("SCM: Waiting for new connection on named pipe...\n");
        /* Create named pipe */
        if (!ScmCreateNamedPipe())
        {
            DPRINT1("\nSCM: Failed to create named pipe\n");
            break;
            //ExitThread(0);
        }
        DPRINT("\nSCM: named pipe session created.\n");
        Sleep(10);
    }
    DPRINT("\n\nWARNING: ScmNamedPipeListenerThread(%p) - Aborted.\n\n", Context);
    return ERROR_SUCCESS;
}


BOOL
StartScmNamedPipeThreadListener(VOID)
{
    DWORD dwThreadId;
    HANDLE hThread;

    hThread = CreateThread(NULL,
                           0,
                           ScmNamedPipeListenerThread,
                           NULL, /*(LPVOID)hPipe,*/
                           0,
                           &dwThreadId);
    if (!hThread)
    {
        DPRINT1("SERVICES: Could not create thread (Status %lx)\n", GetLastError());
        return FALSE;
    }

    CloseHandle(hThread);

    return TRUE;
}


BOOL WINAPI
ShutdownHandlerRoutine(DWORD dwCtrlType)
{
    DPRINT1("ShutdownHandlerRoutine() called\n");

    if (dwCtrlType & (CTRL_SHUTDOWN_EVENT | CTRL_LOGOFF_EVENT))
    {
        DPRINT1("Shutdown event received!\n");
        ScmShutdown = TRUE;

        ScmAutoShutdownServices();
        ScmShutdownServiceDatabase();

        /* Set the shutdwon event */
        SetEvent(hScmShutdownEvent);
    }

    return TRUE;
}


int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nShowCmd)
{
    HANDLE hScmStartEvent = NULL;
    HANDLE hScmAutoStartCompleteEvent = NULL;
    SC_RPC_LOCK Lock = NULL;
    BOOL bCanDeleteNamedPipeCriticalSection = FALSE;
    DWORD dwError;

    DPRINT("SERVICES: Service Control Manager\n");

    /* Make us critical */
    RtlSetProcessIsCritical(TRUE, NULL, TRUE);

    /* We are initializing ourselves */
    ScmInitialize = TRUE;

    /* Create the start event */
    hScmStartEvent = CreateEventW(NULL, TRUE, FALSE, SCM_START_EVENT);
    if (hScmStartEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the start event\n");
        goto done;
    }
    DPRINT("SERVICES: Created start event with handle %p.\n", hScmStartEvent);

    /* Create the auto-start complete event */
    hScmAutoStartCompleteEvent = CreateEventW(NULL, TRUE, FALSE, SCM_AUTOSTARTCOMPLETE_EVENT);
    if (hScmAutoStartCompleteEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the auto-start complete event\n");
        goto done;
    }
    DPRINT("SERVICES: created auto-start complete event with handle %p.\n", hScmAutoStartCompleteEvent);

    /* Create the shutdown event */
    hScmShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (hScmShutdownEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the shutdown event\n");
        goto done;
    }

    /* Initialize our communication named pipe's critical section */
    ScmInitNamedPipeCriticalSection();
    bCanDeleteNamedPipeCriticalSection = TRUE;

//    ScmInitThreadManager();

    /* FIXME: more initialization */

    /* Read the control set values */
    if (!ScmGetControlSetValues())
    {
        DPRINT1("SERVICES: Failed to read the control set values\n");
        goto done;
    }

    /* Create the services database */
    dwError = ScmCreateServiceDatabase();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to create SCM database (Error %lu)\n", dwError);
        goto done;
    }

    /* Wait for the LSA server */
    ScmWaitForLsa();

    /* Update the services database */
    ScmGetBootAndSystemDriverState();

    /* Register the Service Control Manager process with the ReactOS Subsystem */
    if (!RegisterServicesProcess(GetCurrentProcessId()))
    {
        DPRINT1("SERVICES: Could not register SCM process\n");
        goto done;
    }

    /*
     * Acquire the user service start lock until
     * auto-start services have been started.
     */
    dwError = ScmAcquireServiceStartLock(TRUE, &Lock);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to acquire the service start lock (Error %lu)\n", dwError);
        goto done;
    }

    /* Start the RPC server */
    ScmStartRpcServer();

    /* Signal start event */
    SetEvent(hScmStartEvent);

    DPRINT("SERVICES: Initialized.\n");

    /* Register event handler (used for system shutdown) */
    SetConsoleCtrlHandler(ShutdownHandlerRoutine, TRUE);

    /*
     * Set our shutdown parameters: we want to shutdown after the maintained
     * services (that inherit the default shutdown level of 640).
     */
    SetProcessShutdownParameters(480, SHUTDOWN_NORETRY);

    /* Start auto-start services */
    ScmAutoStartServices();

    /* Signal auto-start complete event */
    SetEvent(hScmAutoStartCompleteEvent);

    /* FIXME: more to do ? */

    /* Release the service start lock */
    ScmReleaseServiceStartLock(&Lock);

    /* Initialization finished */
    ScmInitialize = FALSE;

    DPRINT("SERVICES: Running.\n");

    /* Wait until the shutdown event gets signaled */
    WaitForSingleObject(hScmShutdownEvent, INFINITE);

done:
    /* Delete our communication named pipe's critical section */
    if (bCanDeleteNamedPipeCriticalSection == TRUE)
        ScmDeleteNamedPipeCriticalSection();

    /* Close the shutdown event */
    if (hScmShutdownEvent != NULL)
        CloseHandle(hScmShutdownEvent);

    /* Close the auto-start complete event */
    if (hScmAutoStartCompleteEvent != NULL)
        CloseHandle(hScmAutoStartCompleteEvent);

    /* Close the start event */
    if (hScmStartEvent != NULL)
        CloseHandle(hScmStartEvent);

    DPRINT("SERVICES: Finished.\n");

    ExitThread(0);
    return 0;
}

/* EOF */
