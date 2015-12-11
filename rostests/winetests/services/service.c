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

#include <windows.h>
#include <stdio.h>

#include "wine/test.h"

static SERVICE_STATUS_HANDLE (WINAPI *pRegisterServiceCtrlHandlerExA)(LPCSTR,LPHANDLER_FUNCTION_EX,LPVOID);

static HANDLE pipe_handle = INVALID_HANDLE_VALUE;
static char service_name[100], named_pipe_name[100];
static SERVICE_STATUS_HANDLE service_handle;

/* Service process global variables */
static HANDLE service_stop_event;

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
    default:
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI service_main(DWORD argc, char **argv)
{
    SERVICE_STATUS status;
    BOOL res;

    service_ok(argc == 1, "argc = %d", argc);
    if(argc)
        service_ok(!strcmp(argv[0], service_name), "argv[0] = %s, expected %s", argv[0], service_name);

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
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %u", GetLastError());

    service_event("RUNNING");

    WaitForSingleObject(service_stop_event, INFINITE);

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_STOPPED) failed: %u", GetLastError());
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

    service_trace("Starting...");

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

    service_ok(argc == 1, "argc = %d", argc);
    if(argc)
        service_ok(!strcmp(argv[0], service_name), "argv[0] = %s, expected %s", argv[0], service_name);

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
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %u", GetLastError());

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
    char buf[257], *ptr;
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
                trace("service trace: %s\n", ptr+6);
            }else if(!strncmp(ptr, "OK:", 3)) {
                ok(1, "service: %s\n", ptr+3);
            }else if(!strncmp(ptr, "FAIL:", 5)) {
                ok(0, "service: %s\n", ptr+5);
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
    SC_HANDLE service_handle = register_service("simple_service");
    SERVICE_STATUS status;
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
    SERVICE_STATUS status;
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
                                   PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT, 10, 2048, 2048, 10000, NULL);
    ok(pipe_handle != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %u\n", GetLastError());
    if(pipe_handle == INVALID_HANDLE_VALUE)
        return;

    InitializeCriticalSection(&event_cs);
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
}

START_TEST(service)
{
    char **argv;
    int argc;

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
