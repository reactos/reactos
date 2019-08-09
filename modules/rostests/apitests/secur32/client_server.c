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
    WCHAR* cli_argv[6];
} TESTDATA, *PTESTDATA;

//#deinfe TEST_AUTO
#define TEST_NO_AUTO

TESTDATA testdata[] =
{
#ifdef TEST_AUTO
    {   // Test 0
        server2_main, 2, { L"2000", L"NTLM" },
        client2_main, 4, { L"127.0.0.1", L"2000", L"", L"NTLM" }
    }
#endif
#ifdef TEST_NO_AUTO
    {   // Test 1 - not for automation
        // give params per cmd line <ip> <usr> <pwd>
        NULL, 0, {0},
        // Port 139 / 445 (smb)
        client2_main, 6, { L"<ip>", L"445", L"", L"NTLM", L"<usr>", L"<pwd>" }
    }
#endif
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
    HANDLE hThreadServer = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    LPWSTR *argv, parIp, parUsr, parPwd;
    int argc;
    PTESTDATA td = &testdata[testidx];

    /* replace <pwd> client-param if given .... */
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 5)
    {
        int i1;
        parIp = argv[2];
        parUsr = argv[3];
        parPwd = argv[4];
        for (i1 = 0; i1 < td->cli_argc; i1++)
        {
            if (wcscmp(td->cli_argv[i1], L"<ip>") == 0)
                td->cli_argv[i1] = parIp;
            if (wcscmp(td->cli_argv[i1], L"<usr>") == 0)
                td->cli_argv[i1] = parUsr;
            if (wcscmp(td->cli_argv[i1], L"<pwd>") == 0)
                td->cli_argv[i1] = parPwd;
        }
    }

    /* Retrieve our full path */
    if (!GetModuleFileNameW(NULL, FileName, _countof(FileName)))
    {
        skip("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return;
    }

    if (td->server_main != NULL)
    {
        hThreadServer = CreateThread(NULL, 0, server_thread, td, 0, NULL);
        ok(hThreadServer != NULL, "CreateThread failed: %lu\n", GetLastError());
        if (!hThreadServer)
            goto Quit;
    }

    /* Wait 500 millisecs for server initialization */
    Sleep(500);

    /*
     * Run client
     */

    td->client_main(td->cli_argc, td->cli_argv);

    if (hThreadServer != INVALID_HANDLE_VALUE)
        ok(WaitForSingleObject(hThreadServer, 10000) == WAIT_OBJECT_0, "Timeout waiting for thread\n");

Quit:
    if (hThreadServer != INVALID_HANDLE_VALUE)
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
