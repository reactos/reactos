/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Support helpers for embedded services inside api tests.
 * PROGRAMMERS:     Jacek Caban for CodeWeavers
 *                  Thomas Faber <thomas.faber@reactos.org>
 *                  Hermes Belusca-Maito
 *
 * NOTE: Room for improvements:
 * - One test_runner managing 1 pipe for 1 service process that is shared
 *   by multiple services of type SERVICE_WIN32_SHARE_PROCESS.
 * - Find a way to elegantly determine the registered service name inside
 *   the service process, without really passing it
 */

#include "precomp.h"

static HANDLE hClientPipe = INVALID_HANDLE_VALUE;
static WCHAR named_pipe_name[100]; // Shared: FIXME!

static CHAR  service_nameA[100];
static WCHAR service_nameW[100];


/**********  S E R V I C E   ( C L I E N T )   M O D U L E   S I D E  *********/

void send_msg(const char *type, const char *msg)
{
    DWORD written = 0;
    char buf[512];

    StringCbPrintfA(buf, sizeof(buf), "%s:%s", type, msg);
    WriteFile(hClientPipe, buf, lstrlenA(buf)+1, &written, NULL);
}

void service_trace(const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    StringCbVPrintfA(buf, sizeof(buf), msg, valist);
    va_end(valist);

    send_msg("TRACE", buf);
}

void service_ok(int cnd, const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    StringCbVPrintfA(buf, sizeof(buf), msg, valist);
    va_end(valist);

    send_msg(cnd ? "OK" : "FAIL", buf);
}

void service_process(BOOL (*start_service)(PCSTR, PCWSTR), int argc, char** argv)
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

    service_trace("Service process starting...\n");
    res = start_service(service_nameA, service_nameW);
    service_trace("Service process stopped.\n");

    CloseHandle(hClientPipe);
}


/***********  T E S T E R   ( S E R V E R )   M O D U L E   S I D E  **********/

SC_HANDLE register_service_exA(
    SC_HANDLE scm_handle,
    PCSTR test_name,
    PCSTR service_name, // LPCSTR lpServiceName,
    PCSTR extra_args OPTIONAL,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPCSTR lpLoadOrderGroup OPTIONAL,
    LPDWORD lpdwTagId OPTIONAL,
    LPCSTR lpDependencies OPTIONAL,
    LPCSTR lpServiceStartName OPTIONAL,
    LPCSTR lpPassword OPTIONAL)
{
    SC_HANDLE service;
    CHAR service_cmd[MAX_PATH+150];

    /* Retrieve our full path */
    if (!GetModuleFileNameA(NULL, service_cmd, MAX_PATH))
    {
        skip("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return NULL;
    }

    /*
     * Build up our custom command line. The first parameter is the test name,
     * the second parameter is the flag used to decide whether we should start
     * as a service.
     */
    StringCbCatA(service_cmd, sizeof(service_cmd), " ");
    StringCbCatA(service_cmd, sizeof(service_cmd), test_name);
    StringCbCatA(service_cmd, sizeof(service_cmd), " ");
    StringCbCatA(service_cmd, sizeof(service_cmd), service_name);
    if (extra_args)
    {
        StringCbCatA(service_cmd, sizeof(service_cmd), " ");
        StringCbCatA(service_cmd, sizeof(service_cmd), extra_args);
    }

    trace("service_cmd \"%s\"\n", service_cmd);

    service = CreateServiceA(scm_handle, service_name, service_name,
                             dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
                             service_cmd, lpLoadOrderGroup, lpdwTagId, lpDependencies,
                             lpServiceStartName, lpPassword);
    if (!service && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough access right to create service.\n");
        return NULL;
    }

    ok(service != NULL, "CreateService failed: %lu\n", GetLastError());
    return service;
}

SC_HANDLE register_service_exW(
    SC_HANDLE scm_handle,
    PCWSTR test_name,
    PCWSTR service_name, // LPCWSTR lpServiceName,
    PCWSTR extra_args OPTIONAL,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPCWSTR lpLoadOrderGroup OPTIONAL,
    LPDWORD lpdwTagId OPTIONAL,
    LPCWSTR lpDependencies OPTIONAL,
    LPCWSTR lpServiceStartName OPTIONAL,
    LPCWSTR lpPassword OPTIONAL)
{
    SC_HANDLE service;
    WCHAR service_cmd[MAX_PATH+150];

    /* Retrieve our full path */
    if (!GetModuleFileNameW(NULL, service_cmd, MAX_PATH))
    {
        skip("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return NULL;
    }

    /*
     * Build up our custom command line. The first parameter is the test name,
     * the second parameter is the flag used to decide whether we should start
     * as a service.
     */
    StringCbCatW(service_cmd, sizeof(service_cmd), L" ");
    StringCbCatW(service_cmd, sizeof(service_cmd), test_name);
    StringCbCatW(service_cmd, sizeof(service_cmd), L" ");
    StringCbCatW(service_cmd, sizeof(service_cmd), service_name);
    if (extra_args)
    {
        StringCbCatW(service_cmd, sizeof(service_cmd), L" ");
        StringCbCatW(service_cmd, sizeof(service_cmd), extra_args);
    }

    trace("service_cmd \"%ls\"\n", service_cmd);

    service = CreateServiceW(scm_handle, service_name, service_name,
                             dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
                             service_cmd, lpLoadOrderGroup, lpdwTagId, lpDependencies,
                             lpServiceStartName, lpPassword);
    if (!service && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough access right to create service.\n");
        return NULL;
    }

    ok(service != NULL, "CreateService failed: %lu\n", GetLastError());
    return service;
}

SC_HANDLE register_serviceA(
    SC_HANDLE scm_handle,
    PCSTR test_name,
    PCSTR service_name,
    PCSTR extra_args OPTIONAL)
{
    return register_service_exA(scm_handle, test_name, service_name, extra_args,
                                SERVICE_ALL_ACCESS,
                                SERVICE_WIN32_OWN_PROCESS,
                                SERVICE_DEMAND_START,
                                SERVICE_ERROR_IGNORE,
                                NULL, NULL, NULL, NULL, NULL);
}

SC_HANDLE register_serviceW(
    SC_HANDLE scm_handle,
    PCWSTR test_name,
    PCWSTR service_name,
    PCWSTR extra_args OPTIONAL)
{
    return register_service_exW(scm_handle, test_name, service_name, extra_args,
                                SERVICE_ALL_ACCESS,
                                SERVICE_WIN32_OWN_PROCESS,
                                SERVICE_DEMAND_START,
                                SERVICE_ERROR_IGNORE,
                                NULL, NULL, NULL, NULL, NULL);
}

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
            trace("service trace: %s", buf+6);
        }
        else if (!strncmp(buf, "OK:", 3))
        {
            ok(1, "service: %s", buf+3);
        }
        else if (!strncmp(buf, "FAIL:", 5))
        {
            ok(0, "service: %s", buf+5);
        }
        else
        {
            ok(0, "malformed service message: %s\n", buf);
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

    StringCbPrintfW(service_nameW, sizeof(service_nameW), L"WineTestService%lu", GetTickCount());
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
