/*
 * Copyright 2021 Arkadiusz Hiler for CodeWeavers
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

#include <errno.h>
#include <stdarg.h>
#include <process.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

#include "threaddll.h"

enum beginthread_method
{
    use_beginthread,
    use_beginthreadex
};

static char *get_thread_dll_path(void)
{
    static char path[MAX_PATH];
    const char dll_name[] = "threaddll.dll";
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathA(ARRAY_SIZE(path), path);
    strcat(path, dll_name);

    file = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s: %lu.\n",
            debugstr_a(path), GetLastError());

    res = FindResourceA(NULL, dll_name, "TESTDLL");
    ok(!!res, "Failed to load resource: %lu\n", GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource( GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write resource\n");
    CloseHandle(file);

    return path;
}

static void set_thead_dll_detach_event(HANDLE dll, HANDLE event)
{
    void (WINAPI *_set_detach_event)(HANDLE event);
    _set_detach_event = (void*) GetProcAddress(dll, "set_detach_event");
    ok(_set_detach_event != NULL, "Failed to get set_detach_event: %lu\n", GetLastError());
    _set_detach_event(event);
}

static void test_thread_library_reference(char *thread_dll,
                                          enum beginthread_method beginthread_method,
                                          enum thread_exit_method exit_method)
{
    HANDLE detach_event;
    HMODULE dll;
    DWORD ret;
    uintptr_t thread_handle;
    struct threaddll_args args;

    args.exit_method = exit_method;

    detach_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(detach_event != NULL, "Failed to create an event: %lu\n", GetLastError());
    args.confirm_running = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(args.confirm_running != NULL, "Failed to create an event: %lu\n", GetLastError());
    args.past_free = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(args.past_free != NULL, "Failed to create an event: %lu\n", GetLastError());

    dll = LoadLibraryA(thread_dll);
    ok(dll != NULL, "Failed to load the test dll: %lu\n", GetLastError());

    set_thead_dll_detach_event(dll, detach_event);

    if (beginthread_method == use_beginthreadex)
    {
        _beginthreadex_start_routine_t proc = (void*) GetProcAddress(dll, "stdcall_thread_proc");
        ok(proc != NULL, "Failed to get stdcall_thread_proc: %lu\n", GetLastError());
        thread_handle = _beginthreadex(NULL, 0, proc, &args, 0, NULL);
    }
    else
    {
        _beginthread_start_routine_t proc = (void*) GetProcAddress(dll, "cdecl_thread_proc");
        ok(proc != NULL, "Failed to get stdcall_thread_proc: %lu\n", GetLastError());
        thread_handle = _beginthread(proc, 0, &args);
    }

    ok(thread_handle != -1 && thread_handle != 0, "Failed to begin thread: %u\n", errno);

    ret = FreeLibrary(dll);
    ok(ret, "Failed to free the library: %lu\n", GetLastError());

    ret = WaitForSingleObject(args.confirm_running, 200);
    ok(ret == WAIT_OBJECT_0, "Event was not signaled, ret: %lu, err: %lu\n", ret, GetLastError());

    ret = WaitForSingleObject(detach_event, 0);
    ok(ret == WAIT_TIMEOUT, "Thread detach happened unexpectedly signaling an event, ret: %ld, err: %lu\n", ret, GetLastError());

    ret = SetEvent(args.past_free);
    ok(ret, "Failed to signal event: %ld\n", GetLastError());

    if (beginthread_method == use_beginthreadex)
    {
        ret = WaitForSingleObject((HANDLE)thread_handle, 200);
        ok(ret == WAIT_OBJECT_0, "Thread has not exited, ret: %ld, err: %lu\n", ret, GetLastError());
    }

    ret = WaitForSingleObject(detach_event, 200);
    ok(ret == WAIT_OBJECT_0, "Detach event was not signaled, ret: %ld, err: %lu\n", ret, GetLastError());

    if (beginthread_method == use_beginthreadex)
        CloseHandle((HANDLE)thread_handle);

    CloseHandle(args.past_free);
    CloseHandle(args.confirm_running);
    CloseHandle(detach_event);
}

static BOOL handler_called;

void CDECL test_invalid_parameter_handler(const wchar_t *expression,
                                            const wchar_t *function_name,
                                            const wchar_t *file_name,
                                            unsigned line_number,
                                            uintptr_t reserved)
{
    handler_called = TRUE;
}

static void test_thread_invalid_params(void)
{
    uintptr_t hThread;
    _invalid_parameter_handler old = _set_invalid_parameter_handler(test_invalid_parameter_handler);

    errno = 0;
    handler_called = FALSE;
    hThread = _beginthreadex(NULL, 0, NULL, NULL, 0, NULL);
    ok(hThread == 0, "_beginthreadex unexpected ret: %Iu\n", hThread);
    ok(errno == EINVAL, "_beginthreadex unexpected errno: %d\n", errno);
    ok(handler_called, "Expected invalid_parameter_handler to be called\n");

    errno = 0;
    handler_called = FALSE;
    hThread = _beginthread(NULL, 0, NULL);
    ok(hThread == -1, "_beginthread unexpected ret: %Iu\n", hThread);
    ok(errno == EINVAL, "_beginthread unexpected errno: %d\n", errno);
    ok(handler_called, "Expected invalid_parameter_handler to be called\n");

    _set_invalid_parameter_handler(old);
}

START_TEST(thread)
{
    BOOL ret;
    char *thread_dll = get_thread_dll_path();

    test_thread_library_reference(thread_dll, use_beginthread, thread_exit_return);
    test_thread_library_reference(thread_dll, use_beginthread, thread_exit_endthread);
    test_thread_library_reference(thread_dll, use_beginthreadex, thread_exit_return);
    test_thread_library_reference(thread_dll, use_beginthreadex, thread_exit_endthreadex);

    ret = DeleteFileA(thread_dll);
    ok(ret, "Failed to remove the test dll, err: %lu\n", GetLastError());

    test_thread_invalid_params();
}
