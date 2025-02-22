/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for service arguments
 * PROGRAMMER:      Jacek Caban for CodeWeavers
 *                  Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static char **argv;
static int argc;

static HANDLE pipe_handle = INVALID_HANDLE_VALUE;
static char service_nameA[100];
static WCHAR service_nameW[100];
static WCHAR named_pipe_name[100];

/* Test process global variables */
static SC_HANDLE scm_handle;
static SERVICE_STATUS_HANDLE service_handle;

static void send_msg(const char *type, const char *msg)
{
    DWORD written = 0;
    char buf[512];

    StringCbPrintfA(buf, sizeof(buf), "%s:%s", type, msg);
    WriteFile(pipe_handle, buf, lstrlenA(buf)+1, &written, NULL);
}

#if 0
static inline void service_trace(const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    StringCbVPrintfA(buf, sizeof(buf), msg, valist);
    va_end(valist);

    send_msg("TRACE", buf);
}
#endif

static void service_ok(int cnd, const char *msg, ...)
{
   va_list valist;
   char buf[512];

    va_start(valist, msg);
    StringCbVPrintfA(buf, sizeof(buf), msg, valist);
    va_end(valist);

    send_msg(cnd ? "OK" : "FAIL", buf);
}

static VOID WINAPI service_handler(DWORD ctrl)
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus(service_handle, &status);
    default:
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus(service_handle, &status);
    }
}

static void service_main_common(void)
{
    SERVICE_STATUS status;
    BOOL res;

    service_handle = RegisterServiceCtrlHandlerW(service_nameW, service_handler);
    service_ok(service_handle != NULL, "RegisterServiceCtrlHandler failed: %lu\n", GetLastError());
    if (!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %lu", GetLastError());

    Sleep(100);

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_STOPPED) failed: %lu", GetLastError());
}

/*
 * A/W argument layout XP/2003:
 * [argv array of pointers][parameter 1][parameter 2]...[service name]
 *
 * A/W argument layout Vista:
 * [argv array of pointers][align to 8 bytes][parameter 1][parameter 2]...[service name]
 *
 * A/W argument layout Win7/8:
 * [argv array of pointers][service name]
 * [parameter 1][align to 4 bytes][parameter 2][align to 4 bytes]...
 *
 * Space for parameters and service name is always enough to store
 * the WCHAR version including null terminator.
 */

static void WINAPI service_mainA(DWORD service_argc, char **service_argv)
{
    int i;
    char *next_arg;
    char *next_arg_aligned;

    service_ok(service_argc == argc - 3, "service_argc = %d, expected %d", service_argc, argc - 3);
    if (service_argc == argc - 3)
    {
        service_ok(!strcmp(service_argv[0], service_nameA), "service_argv[0] = %s, expected %s", service_argv[0], service_nameA);
        service_ok(service_argv[0] == (char *)&service_argv[service_argc] ||
                   service_argv[1] == (char *)&service_argv[service_argc] ||
                   service_argv[1] == (char *)(((ULONG_PTR)&service_argv[service_argc] + 7) & ~7), "service_argv[0] = %p, [1] = %p, expected one of them to be %p", service_argv[0], service_argv[1], &service_argv[service_argc]);
        //service_trace("service_argv[0] = %p", service_argv[0]);
        next_arg_aligned = next_arg = NULL;
        for (i = 1; i < service_argc; i++)
        {
            //service_trace("service_argv[%d] = %p", i, service_argv[i]);
            service_ok(!strcmp(service_argv[i], argv[i + 3]), "service_argv[%d] = %s, expected %s", i, service_argv[i], argv[i + 3]);
            service_ok(next_arg == NULL ||
                       service_argv[i] == next_arg ||
                       service_argv[i] == next_arg_aligned, "service_argv[%d] = %p, expected %p or %p", i, service_argv[i], next_arg, next_arg_aligned);
            next_arg = service_argv[i];
            next_arg += 2 * (strlen(next_arg) + 1);
            next_arg_aligned = (char *)(((ULONG_PTR)next_arg + 3) & ~3);
        }
    }

    service_main_common();
}

static void WINAPI service_mainW(DWORD service_argc, WCHAR **service_argv)
{
    int i;
    WCHAR argW[32];
    WCHAR *next_arg;
    WCHAR *next_arg_aligned;

    service_ok(service_argc == argc - 3, "service_argc = %d, expected %d", service_argc, argc - 3);
    if (service_argc == argc - 3)
    {
        service_ok(!wcscmp(service_argv[0], service_nameW), "service_argv[0] = %ls, expected %ls", service_argv[0], service_nameW);
        service_ok(service_argv[0] == (WCHAR *)&service_argv[service_argc] ||
                   service_argv[1] == (WCHAR *)&service_argv[service_argc] ||
                   service_argv[1] == (WCHAR *)(((ULONG_PTR)&service_argv[service_argc] + 7) & ~7), "service_argv[0] = %p, [1] = %p, expected one of them to be %p", service_argv[0], service_argv[1], &service_argv[service_argc]);
        //service_trace("service_argv[0] = %p", service_argv[0]);
        next_arg_aligned = next_arg = NULL;
        for (i = 1; i < service_argc; i++)
        {
            //service_trace("service_argv[%d] = %p", i, service_argv[i]);
            MultiByteToWideChar(CP_ACP, 0, argv[i + 3], -1, argW, _countof(argW));
            service_ok(!wcscmp(service_argv[i], argW), "service_argv[%d] = %ls, expected %ls", i, service_argv[i], argW);
            service_ok(next_arg == NULL ||
                       service_argv[i] == next_arg ||
                       service_argv[i] == next_arg_aligned, "service_argv[%d] = %p, expected %p or %p", i, service_argv[i], next_arg, next_arg_aligned);
            next_arg = service_argv[i];
            next_arg += wcslen(next_arg) + 1;
            next_arg_aligned = (WCHAR *)(((ULONG_PTR)next_arg + 3) & ~3);
        }
    }

    service_main_common();
}

static void service_process(BOOLEAN unicode)
{
    BOOL res;

    SERVICE_TABLE_ENTRYA servtblA[] =
    {
        { service_nameA, service_mainA },
        { NULL, NULL }
    };
    SERVICE_TABLE_ENTRYW servtblW[] =
    {
        { service_nameW, service_mainW },
        { NULL, NULL }
    };

    res = WaitNamedPipeW(named_pipe_name, NMPWAIT_USE_DEFAULT_WAIT);
    if (!res)
        return;

    pipe_handle = CreateFileW(named_pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (pipe_handle == INVALID_HANDLE_VALUE)
        return;

    //service_trace("Starting...");
    if (unicode)
    {
        res = StartServiceCtrlDispatcherW(servtblW);
        service_ok(res, "StartServiceCtrlDispatcherW failed: %lu\n", GetLastError());
    }
    else
    {
        res = StartServiceCtrlDispatcherA(servtblA);
        service_ok(res, "StartServiceCtrlDispatcherA failed: %lu\n", GetLastError());
    }

    CloseHandle(pipe_handle);
}

static SC_HANDLE register_service(PCWSTR extra_args)
{
    WCHAR service_cmd[MAX_PATH+150];
    SC_HANDLE service;

    GetModuleFileNameW(NULL, service_cmd, MAX_PATH);

    StringCbCatW(service_cmd, sizeof(service_cmd), L" ServiceArgs ");
    StringCbCatW(service_cmd, sizeof(service_cmd), service_nameW);
    StringCbCatW(service_cmd, sizeof(service_cmd), extra_args);

    trace("service_cmd \"%ls\"\n", service_cmd);

    service = CreateServiceW(scm_handle, service_nameW, service_nameW, SERVICE_ALL_ACCESS,
                             SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
                             service_cmd, NULL, NULL, NULL, NULL, NULL);
    if (!service && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough access right to create service\n");
        return NULL;
    }

    ok(service != NULL, "CreateService failed: %lu\n", GetLastError());
    return service;
}

static DWORD WINAPI pipe_thread(void *arg)
{
    char buf[512];
    DWORD read;
    BOOL res;

    res = ConnectNamedPipe(pipe_handle, NULL);
    ok(res || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe failed: %lu\n", GetLastError());

    while (1)
    {
        res = ReadFile(pipe_handle, buf, sizeof(buf), &read, NULL);
        if (!res)
        {
            ok(GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_INVALID_HANDLE,
               "ReadFile failed: %lu\n", GetLastError());
            break;
        }

        if (!strncmp(buf, "TRACE:", 6))
        {
            trace("service trace: %s\n", buf+6);
        }
        else if (!strncmp(buf, "OK:", 3))
        {
            ok(1, "service: %s\n", buf+3);
        }
        else if (!strncmp(buf, "FAIL:", 5))
        {
            ok(0, "service: %s\n", buf+5);
        }
        else
        {
            ok(0, "malformed service message: %s\n", buf);
        }
    }

    DisconnectNamedPipe(pipe_handle);
    //trace("pipe disconnected\n");
    return 0;
}

static void test_startA(SC_HANDLE service_handle, int service_argc, const char **service_argv)
{
    SERVICE_STATUS status;
    BOOL res;

    res = StartServiceA(service_handle, service_argc, service_argv);
    ok(res, "StartService failed: %lu\n", GetLastError());
    if (!res)
        return;

    do
    {
        Sleep(100);
        ZeroMemory(&status, sizeof(status));
        res = QueryServiceStatus(service_handle, &status);
    } while (res && status.dwCurrentState != SERVICE_STOPPED);
    ok(res, "QueryServiceStatus failed: %lu\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "status.dwCurrentState = %lx\n", status.dwCurrentState);
}

static void test_startW(SC_HANDLE service_handle, int service_argc, const WCHAR **service_argv)
{
    SERVICE_STATUS status;
    BOOL res;

    res = StartServiceW(service_handle, service_argc, service_argv);
    ok(res, "StartService failed: %lu\n", GetLastError());
    if (!res)
        return;

    do
    {
        Sleep(100);
        ZeroMemory(&status, sizeof(status));
        res = QueryServiceStatus(service_handle, &status);
    } while (res && status.dwCurrentState != SERVICE_STOPPED);
    ok(res, "QueryServiceStatus failed: %lu\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "status.dwCurrentState = %lx\n", status.dwCurrentState);
}

static void test_runner(BOOLEAN unicode, PCWSTR extra_args, int service_argc, void *service_argv)
{
    HANDLE thread;
    SC_HANDLE service_handle;
    BOOL res;

    StringCbPrintfW(service_nameW, sizeof(service_nameW), L"WineTestService%lu", GetTickCount());
    WideCharToMultiByte(CP_ACP, 0, service_nameW, -1, service_nameA, _countof(service_nameA), NULL, NULL);
    //trace("service_name: %ls\n", service_nameW);
    StringCbPrintfW(named_pipe_name, sizeof(named_pipe_name), L"\\\\.\\pipe\\%ls_pipe", service_nameW);

    pipe_handle = CreateNamedPipeW(named_pipe_name, PIPE_ACCESS_INBOUND,
                                   PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT, 10, 2048, 2048, 10000, NULL);
    ok(pipe_handle != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError());
    if (pipe_handle == INVALID_HANDLE_VALUE)
        return;

    thread = CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL);
    ok(thread != NULL, "CreateThread failed: %lu\n", GetLastError());
    if (!thread)
        goto Quit;

    service_handle = register_service(extra_args);
    if (!service_handle)
        goto Quit;

    //trace("starting...\n");

    if (unicode)
        test_startW(service_handle, service_argc, service_argv);
    else
        test_startA(service_handle, service_argc, service_argv);

    res = DeleteService(service_handle);
    ok(res, "DeleteService failed: %lu\n", GetLastError());

    CloseServiceHandle(service_handle);

    ok(WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0, "Timeout waiting for thread\n");

Quit:
    if (thread)
        CloseHandle(thread);

    if (pipe_handle != INVALID_HANDLE_VALUE)
        CloseHandle(pipe_handle);
}

START_TEST(ServiceArgs)
{
    argc = winetest_get_mainargs(&argv);

    scm_handle = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(scm_handle != NULL, "OpenSCManager failed: %lu\n", GetLastError());
    if (!scm_handle)
    {
        skip("Failed to open service control manager. Skipping test\n");
        return;
    }

    if (argc < 3)
    {
        char *service_argvA[10];
        WCHAR *service_argvW[10];

        test_runner(FALSE, L" A", 0, NULL);
        test_runner(FALSE, L" W", 0, NULL);
        test_runner(TRUE, L" A", 0, NULL);
        test_runner(TRUE, L" W", 0, NULL);

        service_argvA[0] = "x";
        service_argvW[0] = L"x";
        test_runner(FALSE, L" A x", 1, service_argvA);
        test_runner(FALSE, L" W x", 1, service_argvA);
        test_runner(TRUE, L" A x", 1, service_argvW);
        test_runner(TRUE, L" W x", 1, service_argvW);

        service_argvA[1] = "y";
        service_argvW[1] = L"y";
        test_runner(FALSE, L" A x y", 2, service_argvA);
        test_runner(FALSE, L" W x y", 2, service_argvA);
        test_runner(TRUE, L" A x y", 2, service_argvW);
        test_runner(TRUE, L" W x y", 2, service_argvW);

        service_argvA[0] = "ab";
        service_argvW[0] = L"ab";
        test_runner(FALSE, L" A ab y", 2, service_argvA);
        test_runner(FALSE, L" W ab y", 2, service_argvA);
        test_runner(TRUE, L" A ab y", 2, service_argvW);
        test_runner(TRUE, L" W ab y", 2, service_argvW);

        service_argvA[0] = "abc";
        service_argvW[0] = L"abc";
        test_runner(FALSE, L" A abc y", 2, service_argvA);
        test_runner(FALSE, L" W abc y", 2, service_argvA);
        test_runner(TRUE, L" A abc y", 2, service_argvW);
        test_runner(TRUE, L" W abc y", 2, service_argvW);

        service_argvA[0] = "abcd";
        service_argvW[0] = L"abcd";
        test_runner(FALSE, L" A abcd y", 2, service_argvA);
        test_runner(FALSE, L" W abcd y", 2, service_argvA);
        test_runner(TRUE, L" A abcd y", 2, service_argvW);
        test_runner(TRUE, L" W abcd y", 2, service_argvW);

        service_argvA[0] = "abcde";
        service_argvW[0] = L"abcde";
        test_runner(FALSE, L" A abcde y", 2, service_argvA);
        test_runner(FALSE, L" W abcde y", 2, service_argvA);
        test_runner(TRUE, L" A abcde y", 2, service_argvW);
        test_runner(TRUE, L" W abcde y", 2, service_argvW);

        service_argvA[0] = "abcdef";
        service_argvW[0] = L"abcdef";
        test_runner(FALSE, L" A abcdef y", 2, service_argvA);
        test_runner(FALSE, L" W abcdef y", 2, service_argvA);
        test_runner(TRUE, L" A abcdef y", 2, service_argvW);
        test_runner(TRUE, L" W abcdef y", 2, service_argvW);

        service_argvA[0] = "abcdefg";
        service_argvW[0] = L"abcdefg";
        test_runner(FALSE, L" A abcdefg y", 2, service_argvA);
        test_runner(FALSE, L" W abcdefg y", 2, service_argvA);
        test_runner(TRUE, L" A abcdefg y", 2, service_argvW);
        test_runner(TRUE, L" W abcdefg y", 2, service_argvW);

        service_argvA[0] = "";
        service_argvW[0] = L"";
        test_runner(FALSE, L" A \"\" y", 2, service_argvA);
        test_runner(FALSE, L" W \"\" y", 2, service_argvA);
        test_runner(TRUE, L" A \"\" y", 2, service_argvW);
        test_runner(TRUE, L" W \"\" y", 2, service_argvW);
    }
    else
    {
        strcpy(service_nameA, argv[2]);
        MultiByteToWideChar(CP_ACP, 0, service_nameA, -1, service_nameW, _countof(service_nameW));
        StringCbPrintfW(named_pipe_name, sizeof(named_pipe_name), L"\\\\.\\pipe\\%ls_pipe", service_nameW);
        if (!strcmp(argv[3], "A"))
            service_process(FALSE);
        else
            service_process(TRUE);
    }

    CloseServiceHandle(scm_handle);
}
