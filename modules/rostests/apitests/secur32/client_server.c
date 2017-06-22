/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 *                  Hermes Belusca-Maito
 */

#include "client_server.h"
#include <strsafe.h>

#ifdef SAMUEL_TESTS
// NOTE: If enabled, re-include both client1.c and server1.c in the compilation.

int client1_main(
    char *server,
    char *portstr,
    char *tokenSource,
    char *user,
    char *pwd,
    char *domain);

int server1_main(
    char *portstr);

#endif

DWORD
server2_main(
    IN LPCTSTR PackageName); // Example: _T("NTLM"), or _T("Negotiate")

DWORD
client2_main(
    IN LPCTSTR ServerName,   // ServerName must be defined as the name of the computer running the server sample.    Example: _T("127.0.0.1")
    IN LPCTSTR TargetName,   // TargetName must be defined as the logon name of the user running the server program. Example: _T("")
    IN LPCTSTR PackageName); // Example: _T("NTLM"), or _T("Negotiate")




/*************  F O R K   ( C L I E N T )   M O D U L E   S I D E  ************/

static HANDLE hClientPipe = INVALID_HANDLE_VALUE;
static WCHAR named_pipe_name[100]; // Shared: FIXME!

void send_msg(const char *type, const char *msg)
{
    DWORD written = 0;
    char buf[512];

    StringCbPrintfA(buf, sizeof(buf), "%s:%s", type, msg);
    WriteFile(hClientPipe, buf, strlen(buf)+1, &written, NULL);
}

void client_trace(const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    StringCbVPrintfA(buf, sizeof(buf), msg, valist);
    va_end(valist);

    send_msg("TRACE", buf);
}

void client_ok(int cnd, const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    StringCbVPrintfA(buf, sizeof(buf), msg, valist);
    va_end(valist);

    send_msg(cnd ? "OK" : "FAIL", buf);
}

void client_process(BOOL (*start_client)(PCSTR, PCWSTR), int argc, char** argv)
{
    BOOL res;

    StringCbCopyA(service_nameA, sizeof(service_nameA), argv[2]);
    MultiByteToWideChar(CP_ACP, 0, service_nameA, -1, service_nameW, _countof(service_nameW));
    StringCbPrintfW(named_pipe_name, sizeof(named_pipe_name), L"\\\\.\\pipe\\%ls_pipe", service_nameW);

    res = WaitNamedPipeW(named_pipe_name, NMPWAIT_USE_DEFAULT_WAIT);
    if (!res)
        return;

    hClientPipe = CreateFileW(named_pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hClientPipe == INVALID_HANDLE_VALUE)
        return;

    client_trace("Client process starting...\n");
    res = start_client(service_nameA, service_nameW);
    client_trace("Client process stopped.\n");

    CloseHandle(hClientPipe);
}


/***********  T E S T E R   ( S E R V E R )   M O D U L E   S I D E  **********/

static DWORD WINAPI pipe_thread(void *param)
{
    HANDLE hServerPipe = (HANDLE)param;
    DWORD read;
    BOOL res;
    char buf[512];

    // printf("pipe_thread -- ConnectNamedPipe...\n");
    res = ConnectNamedPipe(hServerPipe, NULL);
    ok(res || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe failed: %lu\n", GetLastError());
    // printf("pipe_thread -- ConnectNamedPipe ok\n");

    while (1)
    {
        res = ReadFile(hServerPipe, buf, sizeof(buf), &read, NULL);
        if (!res)
        {
            ok(GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_INVALID_HANDLE,
               "ReadFile failed: %lu\n", GetLastError());
            // printf("pipe_thread -- break loop\n");
            break;
        }

        if (!strncmp(buf, "TRACE:", 6))
        {
            trace("client trace: %s", buf+6);
        }
        else if (!strncmp(buf, "OK:", 3))
        {
            ok(1, "client: %s", buf+3);
        }
        else if (!strncmp(buf, "FAIL:", 5))
        {
            ok(0, "client: %s", buf+5);
        }
        else
        {
            ok(0, "malformed client message: %s\n", buf);
        }
    }

    // printf("pipe_thread -- DisconnectNamedPipe\n");

    /*
     * Flush the pipe to allow the client to read
     * the pipe's contents before disconnecting.
     */
    FlushFileBuffers(hServerPipe); 

    DisconnectNamedPipe(hServerPipe);
    trace("pipe disconnected\n");
    return 0;
}

void test_runner(void (*run_test)(PCSTR, PCWSTR, void*), void *param)
{
    HANDLE hServerPipe = INVALID_HANDLE_VALUE;
    HANDLE hThread;

    StringCbPrintfW(service_nameW, sizeof(service_nameW), L"RosTestClient%lu", GetTickCount());
    WideCharToMultiByte(CP_ACP, 0, service_nameW, -1, service_nameA, _countof(service_nameA), NULL, NULL);
    //trace("service_name: %ls\n", service_nameW);
    StringCbPrintfW(named_pipe_name, sizeof(named_pipe_name), L"\\\\.\\pipe\\%ls_pipe", service_nameW);

    hServerPipe = CreateNamedPipeW(named_pipe_name, PIPE_ACCESS_INBOUND,
                                   PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT, 10, 2048, 2048, 10000, NULL);
    ok(hServerPipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError());
    if (hServerPipe == INVALID_HANDLE_VALUE)
        return;

    hThread = CreateThread(NULL, 0, pipe_thread, (LPVOID)hServerPipe, 0, NULL);
    ok(hThread != NULL, "CreateThread failed: %lu\n", GetLastError());
    if (!hThread)
        goto Quit;

    run_test(service_nameA, service_nameW, param);

    ok(WaitForSingleObject(hThread, 10000) == WAIT_OBJECT_0, "Timeout waiting for thread\n");

Quit:
    if (hThread)
    {
        /* Be sure to kill the thread if it did not already terminate */
        TerminateThread(hThread, 0);
        CloseHandle(hThread);
    }

    if (hServerPipe != INVALID_HANDLE_VALUE)
        CloseHandle(hServerPipe);
}



/******************************************************************************/

static HANDLE
StartChildProcess(
    IN PWSTR CommandLine)
{
    BOOL Success;
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    // /* HACK: running the test under rosautotest seems to keep another reference
    // * to the child process around until the test finishes (on both ROS and
    // * Windows)... I'm too lazy to investigate very much so let's just redirect
    // * the child std handles to nowhere. ok() is useless in half the child
    // * processes anyway.
    // */
    // StartupInfo.dwFlags = STARTF_USESTDHANDLES;

    Success = CreateProcessW(NULL, // FileName,
                             CommandLine,
                             NULL,
                             NULL,
                             TRUE, // FALSE,
                             0,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInfo);
    if (!Success)
    {
        skip("CreateProcess failed with %lu\n", GetLastError());
        return NULL;
    }
    CloseHandle(ProcessInfo.hThread);
    return ProcessInfo.hProcess;
}



VOID
StartTest(int argc, char** argv)
{
    if (stricmp(argv[2], "client") == 0) /* Client */
    {
#ifdef SAMUEL_TESTS
        if (stricmp(argv[3], "1") == 0)
            client1_main(argc-4, &argv[4]);
        else
#endif
        if (stricmp(argv[3], "2") == 0)
            client2_main(_T("127.0.0.1"), _T(""), _T("NTLM"));
        else
            printf("Unknown test client %s\n", argv[3]);
    }
    else if (stricmp(argv[2], "server") == 0) /* Server */
    {
#ifdef SAMUEL_TESTS
        if (stricmp(argv[3], "1") == 0)
            server1_main(argc-4, &argv[4]);
        else
#endif
        if (stricmp(argv[3], "2") == 0)
            server2_main(_T("NTLM"));
        else
            printf("Unknown test server %s\n", argv[3]);
    }
    else
    {
        printf("Unknown option %s (valid ones: 'client', or 'server')\n", argv[2]);
    }

    return;
}

START_TEST(ClientServer)
{
    int argc;
    char** argv;

    USHORT i;
    WCHAR FileName[MAX_PATH];
    WCHAR CommandLine[MAX_PATH];
    HANDLE hClientServer[2];

    /* Check whether this test is started as a separate process */
    argc = winetest_get_mainargs(&argv);
    if (argc >= 4)
    {
        client_process(StartTest, argc, argv);
        return;
    }

    /* We are started as the real test */
    test_runner(..., NULL);

    /* Retrieve our full path */
    if (!GetModuleFileNameW(NULL, FileName, _countof(FileName))
    {
        skip("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return;
    }

#ifdef SAMUEL_TESTS
    for (i = 1; i <= 2; ++i)
#else
    for (i = 2; i <= 2; ++i)
#endif
    {
        /*
         * Build the server command line and start it
         */
        StringCbPrintfW(CommandLine,
                        sizeof(CommandLine),
                        L"\"%s\" ClientServer server %d",
                        FileName, i);
        hClientServer[0] = StartChildProcess(CommandLine);

        /* Wait 500 millisecs for server initialization */
        Sleep(500);

        /*
         * Build the client command line and start it
         */
        StringCbPrintfW(CommandLine,
                        sizeof(CommandLine),
                        L"\"%s\" ClientServer client %d",
                        FileName, i);
        hClientServer[1] = StartChildProcess(CommandLine);

        /* Wait on client & server */
        // FIXME: Server can wait forever?! WTF
        WaitForMultipleObjects(_countof(hClientServer), hClientServer, TRUE, INFINITE);
        hClientServer[0] = hClientServer[0];
    }
}
