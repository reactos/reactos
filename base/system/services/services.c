/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/services.c
 * PURPOSE:     Main SCM controller
 * COPYRIGHT:   Copyright 2001-2005 Eric Kohl
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

/* NOTE:
 * - Services.exe is NOT a native application, it is a GUI app.
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

int WINAPI RegisterServicesProcess(DWORD ServicesProcessId);

/* GLOBALS ******************************************************************/

#define PIPE_BUFSIZE 1024
#define PIPE_TIMEOUT 1000

BOOL ScmShutdown = FALSE;


/* FUNCTIONS *****************************************************************/

VOID
PrintString(LPCSTR fmt, ...)
{
#ifdef DBG
    CHAR buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buffer);
#endif
}


BOOL
ScmCreateStartEvent(PHANDLE StartEvent)
{
    HANDLE hEvent;

    hEvent = CreateEvent(NULL,
                         TRUE,
                         FALSE,
                         TEXT("SvcctrlStartEvent_A3752DX"));
    if (hEvent == NULL)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEvent(EVENT_ALL_ACCESS,
                               FALSE,
                               TEXT("SvcctrlStartEvent_A3752DX"));
            if (hEvent == NULL)
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    *StartEvent = hEvent;

    return TRUE;
}


BOOL
ScmNamedPipeHandleRequest(PVOID Request,
                          DWORD RequestSize,
                          PVOID Reply,
                          LPDWORD ReplySize)
{
    DbgPrint("SCM READ: %s\n", Request);

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

    DPRINT("ScmNamedPipeThread(%x) - Accepting SCM commands through named pipe\n", hPipe);

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

    DPRINT("ScmNamedPipeThread(%x) - Disconnecting named pipe connection\n", hPipe);

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    DPRINT("ScmNamedPipeThread(%x) - Done.\n", hPipe);

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

    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\Ntsvcs"),
              PIPE_ACCESS_DUPLEX,
              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
              PIPE_UNLIMITED_INSTANCES,
              PIPE_BUFSIZE,
              PIPE_BUFSIZE,
              PIPE_TIMEOUT,
              NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        DPRINT("CreateNamedPipe() failed (%d)\n", GetLastError());
        return FALSE;
    }

    DPRINT("CreateNamedPipe() - calling ConnectNamedPipe(%x)\n", hPipe);
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
            DPRINT("Could not create thread (%d)\n", GetLastError());
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            DPRINT("CreateNamedPipe() - returning FALSE\n");
            return FALSE;
        }
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
    DPRINT("ScmNamedPipeListenerThread(%x) - aka SCM.\n", Context);

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
    DPRINT("\n\nWARNING: ScmNamedPipeListenerThread(%x) - Aborted.\n\n", Context);
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

    return TRUE;
}


VOID FASTCALL
AcquireLoadDriverPrivilege(VOID)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    /* Get a token for this process */
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        /* Get the LUID for the debug privilege */
        LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &tkp.Privileges[0].Luid);

        /* One privilege to set */
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        /* Get the debug privilege for this process */
        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
    }
}


BOOL WINAPI
ShutdownHandlerRoutine(DWORD dwCtrlType)
{
    DPRINT1("ShutdownHandlerRoutine() called\n");

    if (dwCtrlType == CTRL_SHUTDOWN_EVENT)
    {
        DPRINT1("Shutdown event received!\n");
        ScmShutdown = TRUE;

        ScmAutoShutdownServices();
        ScmShutdownServiceDatabase();
    }

    return TRUE;
}


int STDCALL
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
    HANDLE hScmStartEvent;
    HANDLE hEvent;
    DWORD dwError;

    DPRINT("SERVICES: Service Control Manager\n");

    /* Acquire privileges to load drivers */
    AcquireLoadDriverPrivilege();

    /* Create start event */
    if (!ScmCreateStartEvent(&hScmStartEvent))
    {
        DPRINT1("SERVICES: Failed to create start event\n");
        ExitThread(0);
    }

    DPRINT("SERVICES: created start event with handle %x.\n", hScmStartEvent);

//    ScmInitThreadManager();

    /* FIXME: more initialization */


    /* Create the service database */
    dwError = ScmCreateServiceDatabase();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: failed to create SCM database (Error %lu)\n", dwError);
        ExitThread(0);
    }

    /* Update service database */
    ScmGetBootAndSystemDriverState();

    /* Start the RPC server */
    ScmStartRpcServer();

    /* Register service process with CSRSS */
    RegisterServicesProcess(GetCurrentProcessId());

    DPRINT("SERVICES: Initialized.\n");

    /* Signal start event */
    SetEvent(hScmStartEvent);

    /* Register event handler (used for system shutdown) */
    SetConsoleCtrlHandler(ShutdownHandlerRoutine, TRUE);

    /* Start auto-start services */
    ScmAutoStartServices();

    /* FIXME: more to do ? */


    DPRINT("SERVICES: Running.\n");

#if 1
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent)
        WaitForSingleObject(hEvent, INFINITE);
#else
    for (;;)
    {
        NtYieldExecution();
    }
#endif

    CloseHandle(hScmStartEvent);

    DPRINT("SERVICES: Finished.\n");

    ExitThread(0);

    return 0;
}

/* EOF */
