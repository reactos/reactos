/*
 * Unit test suite for process functions
 *
 * Copyright 2017 Michael Müller
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

#include <stdio.h>

#include "ntdll_test.h"

#include "windef.h"
#include "winbase.h"

static NTSTATUS (WINAPI *pNtResumeProcess)(HANDLE);
static NTSTATUS (WINAPI *pNtSuspendProcess)(HANDLE);
static NTSTATUS (WINAPI *pNtSuspendThread)(HANDLE,PULONG);
static NTSTATUS (WINAPI *pNtResumeThread)(HANDLE);

static void test_NtSuspendProcess(char *process_name)
{
    PROCESS_INFORMATION info;
    DEBUG_EVENT ev;
    STARTUPINFOA startup;
    NTSTATUS status;
    HANDLE event;
    char buffer[MAX_PATH];
    ULONG count;
    DWORD ret;

    status = pNtResumeProcess(GetCurrentProcess());
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    event = CreateEventA(NULL, TRUE, FALSE, "wine_suspend_event");
    ok(!!event, "Failed to create event: %u\n", GetLastError());

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);

    sprintf(buffer, "%s tests/process.c dummy_process wine_suspend_event", process_name);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess failed with error %u\n", GetLastError());

    ret = WaitForSingleObject(event, 500);
    ok(ret == WAIT_OBJECT_0, "Event was not signaled: %d\n", ret);

    status = pNtSuspendProcess(info.hProcess);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    ResetEvent(event);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    status = NtResumeThread(info.hThread, &count);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);
    ok(count == 1, "Expected count 1, got %d\n", count);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_OBJECT_0, "Event was not signaled: %d\n", ret);

    status = pNtResumeProcess(info.hProcess);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    status = pNtSuspendThread(info.hThread, &count);
    ok(status == STATUS_SUCCESS, "NtSuspendThread failed: %x\n", status);
    ok(count == 0, "Expected count 0, got %d\n", count);

    ResetEvent(event);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    status = pNtResumeProcess(info.hProcess);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_OBJECT_0, "Event was not signaled: %d\n", ret);

    status = pNtSuspendThread(info.hThread, &count);
    ok(status == STATUS_SUCCESS, "NtSuspendThread failed: %x\n", status);
    ok(count == 0, "Expected count 0, got %d\n", count);

    status = pNtSuspendThread(info.hThread, &count);
    ok(status == STATUS_SUCCESS, "NtSuspendThread failed: %x\n", status);
    ok(count == 1, "Expected count 1, got %d\n", count);

    ResetEvent(event);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    status = pNtResumeProcess(info.hProcess);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    status = pNtResumeProcess(info.hProcess);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_OBJECT_0, "Event was not signaled: %d\n", ret);

    ret = DebugActiveProcess(info.dwProcessId);
    ok(ret, "Failed to debug process: %d\n", GetLastError());

    ResetEvent(event);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    for (;;)
    {
        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed, last error %#x.\n", GetLastError());
        if (!ret) break;

        if (ev.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) break;

        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed, last error %#x.\n", GetLastError());
        if (!ret) break;
    }

    ResetEvent(event);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    status = pNtResumeProcess(info.hProcess);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    status = NtResumeThread(info.hThread, &count);
    ok(status == STATUS_SUCCESS, "NtResumeProcess failed: %x\n", status);
    ok(count == 0, "Expected count 0, got %d\n", count);

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, got: %d\n", ret);

    ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
    ok(ret, "ContinueDebugEvent failed, last error %#x.\n", GetLastError());

    ret = WaitForSingleObject(event, 200);
    ok(ret == WAIT_OBJECT_0, "Event was not signaled: %d\n", ret);

    TerminateProcess(info.hProcess, 0);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
}

static void dummy_process(char *event_name)
{
    HANDLE event = OpenEventA(EVENT_ALL_ACCESS, FALSE, event_name);

    while (TRUE)
    {
        SetEvent(event);
        OutputDebugStringA("test");
        Sleep(5);
    }
}

START_TEST(process)
{
    HMODULE mod;
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);
    if (argc >= 4 && strcmp(argv[2], "dummy_process") == 0)
    {
        dummy_process(argv[3]);
        return;
    }

    mod = GetModuleHandleA("ntdll.dll");
    if (!mod)
    {
        win_skip("Not running on NT, skipping tests\n");
        return;
    }

    pNtResumeProcess  = (void*)GetProcAddress(mod, "NtResumeProcess");
    pNtSuspendProcess = (void*)GetProcAddress(mod, "NtSuspendProcess");
    pNtResumeThread   = (void*)GetProcAddress(mod, "NtResumeThread");
    pNtSuspendThread  = (void*)GetProcAddress(mod, "NtSuspendThread");

    test_NtSuspendProcess(argv[0]);
}
