/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 *                  Hermes Belusca-Maito
 */

#include "client_server.h"
#include <strsafe.h>

/*
 * Params:
 * IN LPCTSTR PackageName); // Example: _T("NTLM"), or _T("Negotiate")
 */
int server2_main(int argc, WCHAR** argv);

/*
 * Params:
 * IN LPCTSTR ServerName,   // ServerName must be defined as the name of the computer running the server sample.    Example: _T("127.0.0.1")
 * IN LPCTSTR TargetName,   // TargetName must be defined as the logon name of the user running the server program. Example: _T("")
 * IN LPCTSTR PackageName); // Example: _T("NTLM"), or _T("Negotiate")
 */
int client2_main(int argc, WCHAR** argv);

typedef int (*TESTPROC)(int argc, WCHAR** argv);

typedef struct _TESTDATA
{
    TESTPROC server_main;
    int svr_argc;
    WCHAR* svr_argv[1];
    TESTPROC client_main;
    int cli_argc;
    WCHAR* cli_argv[3];
} TESTDATA, *PTESTDATA;

TESTDATA testdata[] =
{
    {   // Test 0
        server2_main, 1, { L"NTLM" },
        client2_main, 3, { L"127.0.0.1", L"", L"NTLM" }
    }
};

CRITICAL_SECTION sync_msg_cs;

void sync_msg_enter()
{
    EnterCriticalSection(&sync_msg_cs);
}

void sync_msg_leave()
{
    LeaveCriticalSection(&sync_msg_cs);
}

static DWORD WINAPI server_thread(void *param)
{
    PTESTDATA td = (PTESTDATA)param;

    td->server_main(td->svr_argc, td->svr_argv);

    return 0;
}

void test_runner(USHORT testidx)
{
    HANDLE hThreadServer;
    WCHAR FileName[MAX_PATH];
    HANDLE hClientServer[1];
    PTESTDATA td = &testdata[testidx];

    /* Retrieve our full path */
    if (!GetModuleFileNameW(NULL, FileName, _countof(FileName)))
    {
        skip("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return;
    }

    hThreadServer = CreateThread(NULL, 0, server_thread, td, 0, NULL);
    ok(hThreadServer != NULL, "CreateThread failed: %lu\n", GetLastError());
    if (!hThreadServer)
        goto Quit;

    /* Wait 500 millisecs for server initialization */
    Sleep(500);

    /*
     * Run client
     */
    td->client_main(td->cli_argc, td->cli_argv);

    /* Wait on client & server */
    // FIXME: Server can wait forever?! WTF
    WaitForSingleObject(hClientServer[0], INFINITE);

    ok(WaitForSingleObject(hThreadServer, 10000) == WAIT_OBJECT_0, "Timeout waiting for thread\n");

Quit:
    if (hThreadServer)
    {
        /* Be sure to kill the thread if it did not already terminate */
        TerminateThread(hThreadServer, 0);
        CloseHandle(hThreadServer);
    }
}

/******************************************************************************/

START_TEST(ClientServer)
{
    InitializeCriticalSection(&sync_msg_cs);
    NtlmCheckInit();

    test_runner(0);

    NtlmCheckFini();
    DeleteCriticalSection(&sync_msg_cs);
}
