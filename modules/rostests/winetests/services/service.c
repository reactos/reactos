/*
 * Copyright 2012 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include <windef.h>
#include <winsvc.h>
#include <stdio.h>
#include <winbase.h>
#include <winuser.h>

#include "wine/test.h"

static SERVICE_STATUS_HANDLE (WINAPI *pRegisterServiceCtrlHandlerExA)(LPCSTR,LPHANDLER_FUNCTION_EX,LPVOID);

static HANDLE pipe_handle = INVALID_HANDLE_VALUE;
static char service_name[100], named_pipe_name[100];
static SERVICE_STATUS_HANDLE service_handle;

/* Service process global variables */
static HANDLE service_stop_event;

static int monitor_count;

static void send_msg(const char *type, const char *msg)
{
    DWORD written = 0;
    char buf[512];

    sprintf(buf, "%s:%s", type, msg);
    WriteFile(pipe_handle, buf, strlen(buf)+1, &written, NULL);
}

static inline void service_trace(const char *msg)
{
    send_msg("TRACE", msg);
}

static inline void service_event(const char *event)
{
    send_msg("EVENT", event);
}

static void service_ok(int cnd, const char *msg, ...)
{
   va_list valist;
   char buf[512];

    va_start(valist, msg);
    vsprintf(buf, msg, valist);
    va_end(valist);

    send_msg(cnd ? "OK" : "FAIL", buf);
}

static void test_winstation(void)
{
    HWINSTA winstation;
    USEROBJECTFLAGS flags;
    BOOL r;

    winstation = GetProcessWindowStation();
    service_ok(winstation != NULL, "winstation = NULL\n");

    r = GetUserObjectInformationA(winstation, UOI_FLAGS, &flags, sizeof(flags), NULL);
    service_ok(r, "GetUserObjectInformation(UOI_NAME) failed: %u\n", GetLastError());
    service_ok(!(flags.dwFlags & WSF_VISIBLE), "winstation has flags %x\n", flags.dwFlags);
}

/*
 * Test creating window in a service process. Although services run in non-interactive,
 * they may create windows that will never be visible.
 */
static void test_create_window(void)
{
    DWORD style;
    ATOM class;
    HWND hwnd;
    BOOL r;

    static WNDCLASSEXA wndclass = {
        sizeof(WNDCLASSEXA),
        0,
        DefWindowProcA,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        "service_test",
        NULL
    };

    hwnd = GetDesktopWindow();
    service_ok(IsWindow(hwnd), "GetDesktopWindow returned invalid window %p\n", hwnd);

    class = RegisterClassExA(&wndclass);
    service_ok(class, "RegisterClassFailed\n");

    hwnd = CreateWindowA("service_test", "service_test",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            515, 530, NULL, NULL, NULL, NULL);
    service_ok(hwnd != NULL, "CreateWindow failed: %u\n", GetLastError());

    style = GetWindowLongW(hwnd, GWL_STYLE);
    service_ok(!(style & WS_VISIBLE), "style = %x, expected invisible\n", style);

    r = ShowWindow(hwnd, SW_SHOW);
    service_ok(!r, "ShowWindow returned %x\n", r);

    style = GetWindowLongW(hwnd, GWL_STYLE);
    service_ok(style & WS_VISIBLE, "style = %x, expected visible\n", style);

    r = ShowWindow(hwnd, SW_SHOW);
    service_ok(r, "ShowWindow returned %x\n", r);

    r = DestroyWindow(hwnd);
    service_ok(r, "DestroyWindow failed: %08x\n", GetLastError());
}

static BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC hdc, LPRECT lprc, LPARAM lparam)
{
    BOOL r;
    MONITORINFOEXA mi;

    service_ok(hmon != NULL, "Unexpected hmon=%#x\n", hmon);

    monitor_count++;

    mi.cbSize = sizeof(mi);

    SetLastError(0xdeadbeef);
    r = GetMonitorInfoA(NULL, (MONITORINFO*)&mi);
    service_ok(GetLastError() == ERROR_INVALID_MONITOR_HANDLE, "Unexpected GetLastError: %#x.\n", GetLastError());
    service_ok(!r, "GetMonitorInfo with NULL HMONITOR succeeded.\n");

    r = GetMonitorInfoA(hmon, (MONITORINFO*)&mi);
    service_ok(r, "GetMonitorInfo failed.\n");

    service_ok(mi.rcMonitor.left == 0 && mi.rcMonitor.top == 0 && mi.rcMonitor.right >= 640 && mi.rcMonitor.bottom >= 480,
               "Unexpected monitor rcMonitor values: {%d,%d,%d,%d}\n",
               mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);

    service_ok(mi.rcWork.left == 0 && mi.rcWork.top == 0 && mi.rcWork.right >= 640 && mi.rcWork.bottom >= 480,
               "Unexpected monitor rcWork values: {%d,%d,%d,%d}\n",
               mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);

    service_ok(!strcmp(mi.szDevice, "WinDisc") || !strcmp(mi.szDevice, "\\\\.\\DISPLAY1"),
               "Unexpected szDevice received: %s\n", mi.szDevice);

    service_ok(mi.dwFlags == MONITORINFOF_PRIMARY, "Unexpected secondary monitor info.\n");

    return TRUE;
}

/* query monitor information, even in non-interactive services */
static void test_monitors(void)
{
    BOOL r;

    r = EnumDisplayMonitors(0, 0, monitor_enum_proc, 0);
    service_ok(r, "EnumDisplayMonitors failed.\n");
    service_ok(monitor_count == 1, "Callback got called less or more than once. %d\n", monitor_count);
}

static DWORD WINAPI service_handler(DWORD ctrl, DWORD event_type, void *event_data, void *context)
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
    case SERVICE_CONTROL_SHUTDOWN:
        service_event("STOP");
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus(service_handle, &status);
        SetEvent(service_stop_event);
        return NO_ERROR;
    case 128:
        test_winstation();
        test_create_window();
        test_monitors();
        service_event("CUSTOM");
        return 0xdeadbeef;
    default:
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI service_main(DWORD argc, char **argv)
{
    SERVICE_STATUS status;
    char buf[64];
    BOOL res;

    service_ok(argc == 3, "argc = %u, expected 3\n", argc);
    service_ok(!strcmp(argv[0], service_name), "argv[0] = '%s', expected '%s'\n", argv[0], service_name);
    service_ok(!strcmp(argv[1], "param1"), "argv[1] = '%s', expected 'param1'\n", argv[1]);
    service_ok(!strcmp(argv[2], "param2"), "argv[2] = '%s', expected 'param2'\n", argv[2]);

    buf[0] = 0;
    GetEnvironmentVariableA("PATHEXT", buf, sizeof(buf));
    service_ok(buf[0], "did not find PATHEXT environment variable\n");

    service_handle = pRegisterServiceCtrlHandlerExA(service_name, service_handler, NULL);
    service_ok(service_handle != NULL, "RegisterServiceCtrlHandlerEx failed: %u\n", GetLastError());
    if(!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %u\n", GetLastError());

    service_event("RUNNING");

    WaitForSingleObject(service_stop_event, INFINITE);

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_STOPPED) failed: %u\n", GetLastError());
}

static void service_process(void (WINAPI *p_service_main)(DWORD, char **))
{
    BOOL res;

    SERVICE_TABLE_ENTRYA servtbl[] = {
        {service_name, p_service_main},
        {NULL, NULL}
    };

    res = WaitNamedPipeA(named_pipe_name, NMPWAIT_USE_DEFAULT_WAIT);
    if(!res)
        return;

    pipe_handle = CreateFileA(named_pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(pipe_handle == INVALID_HANDLE_VALUE)
        return;

    service_trace("Starting...\n");

    service_stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    service_ok(service_stop_event != NULL, "Could not create event: %u\n", GetLastError());
    if(!service_stop_event)
        return;

    res = StartServiceCtrlDispatcherA(servtbl);
    service_ok(res, "StartServiceCtrlDispatcher failed: %u\n", GetLastError());

    /* Let service thread terminate */
    Sleep(50);

    CloseHandle(service_stop_event);
    CloseHandle(pipe_handle);
}

static DWORD WINAPI no_stop_handler(DWORD ctrl, DWORD event_type, void *event_data, void *context)
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
        case SERVICE_CONTROL_SHUTDOWN:
            service_event("STOP");
            status.dwCurrentState     = SERVICE_STOPPED;
            status.dwControlsAccepted = 0;
            SetServiceStatus(service_handle, &status);
            SetEvent(service_stop_event);
            return NO_ERROR;
        default:
            status.dwCurrentState = SERVICE_RUNNING;
            SetServiceStatus( service_handle, &status );
            return NO_ERROR;
    }
}

static void WINAPI no_stop_main(DWORD argc, char **argv)
{
    SERVICE_STATUS status;
    BOOL res;

    service_ok(argc == 1, "argc = %u, expected 1\n", argc);
    service_ok(!strcmp(argv[0], service_name), "argv[0] = '%s', expected '%s'\n", argv[0], service_name);

    service_handle = pRegisterServiceCtrlHandlerExA(service_name, no_stop_handler, NULL);
    service_ok(service_handle != NULL, "RegisterServiceCtrlHandlerEx failed: %u\n", GetLastError());
    if(!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %u\n", GetLastError());

    service_event("RUNNING");
}

/* Test process global variables */
static SC_HANDLE scm_handle;

static char current_event[32];
static HANDLE event_handle = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION event_cs;

static SC_HANDLE register_service(const char *test_name)
{
    char service_cmd[MAX_PATH+150], *ptr;
    SC_HANDLE service;

    ptr = service_cmd + GetModuleFileNameA(NULL, service_cmd, MAX_PATH);

    /* If the file doesn't exist, assume we're using built-in exe and append .so to the path */
    if(GetFileAttributesA(service_cmd) == INVALID_FILE_ATTRIBUTES) {
        strcpy(ptr, ".so");
        ptr += 3;
    }

    strcpy(ptr, " service ");
    ptr += strlen(ptr);
    sprintf(ptr, "%s ", test_name);
    ptr += strlen(ptr);
    strcpy(ptr, service_name);

    trace("service_cmd \"%s\"\n", service_cmd);

    service = CreateServiceA(scm_handle, service_name, service_name, GENERIC_ALL,
                             SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
                             service_cmd, NULL, NULL, NULL, NULL, NULL);
    if(!service && GetLastError() == ERROR_ACCESS_DENIED) {
        skip("Not enough access right to create service\n");
        return NULL;
    }

    ok(service != NULL, "CreateService failed: %u\n", GetLastError());
    return service;
}

static void expect_event(const char *event_name)
{
    char evt[32];
    DWORD res;

    trace("waiting for %s\n", event_name);

    res = WaitForSingleObject(event_handle, 30000);
    ok(res == WAIT_OBJECT_0, "WaitForSingleObject failed: %u\n", res);
    if(res != WAIT_OBJECT_0)
        return;

    EnterCriticalSection(&event_cs);
    strcpy(evt, current_event);
    *current_event = 0;
    LeaveCriticalSection(&event_cs);

    ok(!strcmp(evt, event_name), "Unexpected event: %s, expected %s\n", evt, event_name);
}

static DWORD WINAPI pipe_thread(void *arg)
{
    char buf[512], *ptr;
    DWORD read;
    BOOL res;

    res = ConnectNamedPipe(pipe_handle, NULL);
    ok(res || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe failed: %u\n", GetLastError());

    while(1) {
        res = ReadFile(pipe_handle, buf, sizeof(buf), &read, NULL);
        if(!res) {
            ok(GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_INVALID_HANDLE,
               "ReadFile failed: %u\n", GetLastError());
            break;
        }

        for(ptr = buf; ptr < buf+read; ptr += strlen(ptr)+1) {
            if(!strncmp(ptr, "TRACE:", 6)) {
                trace("service trace: %s", ptr+6);
            }else if(!strncmp(ptr, "OK:", 3)) {
                ok(1, "service: %s", ptr+3);
            }else if(!strncmp(ptr, "FAIL:", 5)) {
                ok(0, "service: %s", ptr+5);
            }else if(!strncmp(ptr, "EVENT:", 6)) {
                trace("service event: %s\n", ptr+6);
                EnterCriticalSection(&event_cs);
                ok(!current_event[0], "event %s still queued\n", current_event);
                strcpy(current_event, ptr+6);
                LeaveCriticalSection(&event_cs);
                SetEvent(event_handle);
            }else {
                ok(0, "malformed service message: %s\n", ptr);
            }
        }
    }

    DisconnectNamedPipe(pipe_handle);
    trace("pipe disconnected\n");
    return 0;
}

static void test_service(void)
{
    static const char *argv[2] = {"param1", "param2"};
    SC_HANDLE service_handle = register_service("simple_service");
    SERVICE_STATUS_PROCESS status2;
    SERVICE_STATUS status;
    DWORD bytes;
    BOOL res;

    if(!service_handle)
        return;

    trace("starting...\n");
    res = StartServiceA(service_handle, 2, argv);
    ok(res, "StartService failed: %u\n", GetLastError());
    if(!res) {
        DeleteService(service_handle);
        CloseServiceHandle(service_handle);
        return;
    }
    expect_event("RUNNING");

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %d\n", GetLastError());
    todo_wine ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %x\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_RUNNING, "status.dwCurrentState = %x\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN),
            "status.dwControlsAccepted = %x\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %d\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %d\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %d\n", status.dwCheckPoint);
    todo_wine ok(status.dwWaitHint == 0, "status.dwWaitHint = %d\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status2.dwCurrentState == SERVICE_RUNNING, "status2.dwCurrentState = %x\n", status2.dwCurrentState);
    ok(status2.dwProcessId != 0, "status2.dwProcessId = %d\n", status2.dwProcessId);

    res = ControlService(service_handle, 128, &status);
    ok(res, "ControlService failed: %u\n", GetLastError());
    expect_event("CUSTOM");

    res = ControlService(service_handle, SERVICE_CONTROL_STOP, &status);
    ok(res, "ControlService failed: %u\n", GetLastError());
    expect_event("STOP");

    res = DeleteService(service_handle);
    ok(res, "DeleteService failed: %u\n", GetLastError());

    CloseServiceHandle(service_handle);
}

static inline void test_no_stop(void)
{
    SC_HANDLE service_handle = register_service("no_stop");
    SERVICE_STATUS_PROCESS status2;
    SERVICE_STATUS status;
    DWORD bytes;
    BOOL res;

    if(!service_handle)
        return;

    trace("starting...\n");
    res = StartServiceA(service_handle, 0, NULL);
    ok(res, "StartService failed: %u\n", GetLastError());
    if(!res) {
        DeleteService(service_handle);
        CloseServiceHandle(service_handle);
        return;
    }
    expect_event("RUNNING");

    /* Let service thread terminate */
    Sleep(1000);

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %d\n", GetLastError());
    todo_wine ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %x\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_RUNNING, "status.dwCurrentState = %x\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN),
            "status.dwControlsAccepted = %x\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %d\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %d\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %d\n", status.dwCheckPoint);
    todo_wine ok(status.dwWaitHint == 0, "status.dwWaitHint = %d\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status2.dwCurrentState == SERVICE_RUNNING, "status2.dwCurrentState = %x\n", status2.dwCurrentState);
    ok(status2.dwProcessId != 0, "status2.dwProcessId = %d\n", status2.dwProcessId);

    res = ControlService(service_handle, SERVICE_CONTROL_STOP, &status);
    ok(res, "ControlService failed: %u\n", GetLastError());
    expect_event("STOP");

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %d\n", GetLastError());
    todo_wine ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %x\n", status.dwServiceType);
    ok(status.dwCurrentState==SERVICE_STOPPED || status.dwCurrentState==SERVICE_STOP_PENDING,
            "status.dwCurrentState = %x\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == 0, "status.dwControlsAccepted = %x\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %d\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %d\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %d\n", status.dwCheckPoint);
    ok(status.dwWaitHint == 0, "status.dwWaitHint = %d\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status2.dwProcessId == 0 || broken(status2.dwProcessId != 0),
       "status2.dwProcessId = %d\n", status2.dwProcessId);

    res = DeleteService(service_handle);
    ok(res, "DeleteService failed: %u\n", GetLastError());

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %d\n", GetLastError());
    todo_wine ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %x\n", status.dwServiceType);
    ok(status.dwCurrentState==SERVICE_STOPPED || status.dwCurrentState==SERVICE_STOP_PENDING,
            "status.dwCurrentState = %x\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == 0, "status.dwControlsAccepted = %x\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %d\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %d\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %d\n", status.dwCheckPoint);
    ok(status.dwWaitHint == 0, "status.dwWaitHint = %d\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status2.dwProcessId == 0 || broken(status2.dwProcessId != 0),
       "status2.dwProcessId = %d\n", status2.dwProcessId);

    CloseServiceHandle(service_handle);

    res = QueryServiceStatus(service_handle, &status);
    ok(!res, "QueryServiceStatus should have failed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError = %d\n", GetLastError());
}

static void test_runner(void (*p_run_test)(void))
{
    HANDLE thread;

    sprintf(service_name, "WineTestService%d", GetTickCount());
    trace("service_name: %s\n", service_name);
    sprintf(named_pipe_name, "\\\\.\\pipe\\%s_pipe", service_name);

    pipe_handle = CreateNamedPipeA(named_pipe_name, PIPE_ACCESS_INBOUND,
                                   PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT, 10, 2048, 2048, 10000, NULL);
    ok(pipe_handle != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %u\n", GetLastError());
    if(pipe_handle == INVALID_HANDLE_VALUE)
        return;

    event_handle = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(event_handle != INVALID_HANDLE_VALUE, "CreateEvent failed: %u\n", GetLastError());
    if(event_handle == INVALID_HANDLE_VALUE)
        return;

    thread = CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL);
    ok(thread != NULL, "CreateThread failed: %u\n", GetLastError());
    if(!thread)
        return;

    p_run_test();

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(event_handle);
    CloseHandle(pipe_handle);
    CloseHandle(thread);
}

START_TEST(service)
{
    char **argv;
    int argc;

    InitializeCriticalSection(&event_cs);

    pRegisterServiceCtrlHandlerExA = (void*)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegisterServiceCtrlHandlerExA");
    if(!pRegisterServiceCtrlHandlerExA) {
        win_skip("RegisterServiceCtrlHandlerExA not available, skipping tests\n");
        return;
    }

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    ok(scm_handle != NULL || GetLastError() == ERROR_ACCESS_DENIED, "OpenSCManager failed: %u\n", GetLastError());
    if(!scm_handle) {
        skip("OpenSCManager failed, skipping tests\n");
        return;
    }

    argc = winetest_get_mainargs(&argv);

    if(argc < 3) {
        test_runner(test_service);
        test_runner(test_no_stop);
    }else {
        strcpy(service_name, argv[3]);
        sprintf(named_pipe_name, "\\\\.\\pipe\\%s_pipe", service_name);

        if(!strcmp(argv[2], "simple_service"))
            service_process(service_main);
        else if(!strcmp(argv[2], "no_stop"))
            service_process(no_stop_main);
    }

    CloseServiceHandle(scm_handle);
}
