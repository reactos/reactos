/* Unit test suite for Ntdll NamedPipe API functions
 *
 * Copyright 2011 Bernhard Loos
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
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/test.h"
#include "wine/winternl.h"
#include "winioctl.h"

#ifndef __WINE_WINTERNL_H

typedef struct {
  ULONG ReadMode;
  ULONG CompletionMode;
} FILE_PIPE_INFORMATION;

typedef struct {
  ULONG NamedPipeType;
  ULONG NamedPipeConfiguration;
  ULONG MaximumInstances;
  ULONG CurrentInstances;
  ULONG InboundQuota;
  ULONG ReadDataAvailable;
  ULONG OutboundQuota;
  ULONG WriteQuotaAvailable;
  ULONG NamedPipeState;
  ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION;

#ifndef FILE_SYNCHRONOUS_IO_ALERT
#define FILE_SYNCHRONOUS_IO_ALERT 0x10
#endif

#ifndef FILE_SYNCHRONOUS_IO_NONALERT
#define FILE_SYNCHRONOUS_IO_NONALERT 0x20
#endif

#ifndef FSCTL_PIPE_LISTEN
#define FSCTL_PIPE_LISTEN CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#endif

static NTSTATUS (WINAPI *pNtFsControlFile) (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, PVOID apc_context, PIO_STATUS_BLOCK io, ULONG code, PVOID in_buffer, ULONG in_size, PVOID out_buffer, ULONG out_size);
static NTSTATUS (WINAPI *pNtCreateNamedPipeFile) (PHANDLE handle, ULONG access,
                                        POBJECT_ATTRIBUTES attr, PIO_STATUS_BLOCK iosb,
                                        ULONG sharing, ULONG dispo, ULONG options,
                                        ULONG pipe_type, ULONG read_mode,
                                        ULONG completion_mode, ULONG max_inst,
                                        ULONG inbound_quota, ULONG outbound_quota,
                                        PLARGE_INTEGER timeout);
static NTSTATUS (WINAPI *pNtQueryInformationFile) (IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length, IN FILE_INFORMATION_CLASS FileInformationClass);
static NTSTATUS (WINAPI *pNtSetInformationFile) (HANDLE handle, PIO_STATUS_BLOCK io, PVOID ptr, ULONG len, FILE_INFORMATION_CLASS class);
static NTSTATUS (WINAPI *pNtCancelIoFile) (HANDLE hFile, PIO_STATUS_BLOCK io_status);
static void (WINAPI *pRtlInitUnicodeString) (PUNICODE_STRING target, PCWSTR source);

static HANDLE (WINAPI *pOpenThread)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId);
static DWORD (WINAPI *pQueueUserAPC)(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData);


static BOOL init_func_ptrs(void)
{
    HMODULE module = GetModuleHandleA("ntdll.dll");

#define loadfunc(name)  if (!(p##name = (void *)GetProcAddress(module, #name))) { \
                            trace("GetProcAddress(%s) failed\n", #name); \
                            return FALSE; \
                        }

    loadfunc(NtFsControlFile)
    loadfunc(NtCreateNamedPipeFile)
    loadfunc(NtQueryInformationFile)
    loadfunc(NtSetInformationFile)
    loadfunc(NtCancelIoFile)
    loadfunc(RtlInitUnicodeString)

    /* not fatal */
    module = GetModuleHandleA("kernel32.dll");
    pOpenThread = (void *)GetProcAddress(module, "OpenThread");
    pQueueUserAPC = (void *)GetProcAddress(module, "QueueUserAPC");
    return TRUE;
}

static const WCHAR testpipe[] = { '\\', '\\', '.', '\\', 'p', 'i', 'p', 'e', '\\',
                                  't', 'e', 's', 't', 'p', 'i', 'p', 'e', 0 };
static const WCHAR testpipe_nt[] = { '\\', '?', '?', '\\', 'p', 'i', 'p', 'e', '\\',
                                     't', 'e', 's', 't', 'p', 'i', 'p', 'e', 0 };

static NTSTATUS create_pipe(PHANDLE handle, ULONG sharing, ULONG options)
{
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    LARGE_INTEGER timeout;
    NTSTATUS res;

    pRtlInitUnicodeString(&name, testpipe_nt);

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &name;
    attr.Attributes               = 0x40; /*case insensitive */
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    timeout.QuadPart = -100000000000ll;

    res = pNtCreateNamedPipeFile(handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb, sharing,  2 /*FILE_CREATE*/,
                                 options, 1, 0, 0, 0xFFFFFFFF, 500, 500, &timeout);
    return res;
}

static void test_create_invalid(void)
{
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    LARGE_INTEGER timeout;
    NTSTATUS res;
    HANDLE handle, handle2;
    FILE_PIPE_LOCAL_INFORMATION info;

    pRtlInitUnicodeString(&name, testpipe_nt);

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &name;
    attr.Attributes               = 0x40; /*case insensitive */
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    timeout.QuadPart = -100000000000ll;

/* create a pipe with FILE_OVERWRITE */
    res = pNtCreateNamedPipeFile(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb, FILE_SHARE_READ, 4 /*FILE_OVERWRITE*/,
                                 0, 1, 0, 0, 0xFFFFFFFF, 500, 500, &timeout);
    todo_wine ok(res == STATUS_INVALID_PARAMETER, "NtCreateNamedPipeFile returned %x\n", res);
    if (!res)
        CloseHandle(handle);

/* create a pipe with FILE_OVERWRITE_IF */
    res = pNtCreateNamedPipeFile(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb, FILE_SHARE_READ, 5 /*FILE_OVERWRITE_IF*/,
                                 0, 1, 0, 0, 0xFFFFFFFF, 500, 500, &timeout);
    todo_wine ok(res == STATUS_INVALID_PARAMETER, "NtCreateNamedPipeFile returned %x\n", res);
    if (!res)
        CloseHandle(handle);

/* create a pipe with sharing = 0 */
    res = pNtCreateNamedPipeFile(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb, 0, 2 /*FILE_CREATE*/,
                                 0, 1, 0, 0, 0xFFFFFFFF, 500, 500, &timeout);
    ok(res == STATUS_INVALID_PARAMETER, "NtCreateNamedPipeFile returned %x\n", res);
    if (!res)
        CloseHandle(handle);

/* create a pipe without r/w access */
    res = pNtCreateNamedPipeFile(&handle, SYNCHRONIZE, &attr, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, 2 /*FILE_CREATE*/,
                                 0, 1, 0, 0, 0xFFFFFFFF, 500, 500, &timeout);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

    res = pNtQueryInformationFile(handle, &iosb, &info, sizeof(info), (FILE_INFORMATION_CLASS)24);
    ok(res == STATUS_ACCESS_DENIED, "NtQueryInformationFile returned %x\n", res);

/* test FILE_CREATE creation disposition */
    res = pNtCreateNamedPipeFile(&handle2, SYNCHRONIZE, &attr, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, 2 /*FILE_CREATE*/,
                                 0, 1, 0, 0, 0xFFFFFFFF, 500, 500, &timeout);
    todo_wine ok(res == STATUS_ACCESS_DENIED, "NtCreateNamedPipeFile returned %x\n", res);
    if (!res)
        CloseHandle(handle2);

    CloseHandle(handle);
}

static void test_create(void)
{
    HANDLE hserver;
    NTSTATUS res;
    int j, k;
    FILE_PIPE_LOCAL_INFORMATION info;
    IO_STATUS_BLOCK iosb;

    static const DWORD access[] = { 0, GENERIC_READ, GENERIC_WRITE, GENERIC_READ | GENERIC_WRITE};
    static const DWORD sharing[] =    { FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE };
    static const DWORD pipe_config[]= {               1,                0,                                  2 };

    for (j = 0; j < sizeof(sharing) / sizeof(DWORD); j++) {
        for (k = 0; k < sizeof(access) / sizeof(DWORD); k++) {
            HANDLE hclient;
            BOOL should_succeed = TRUE;

            res  = create_pipe(&hserver, sharing[j], FILE_SYNCHRONOUS_IO_NONALERT);
            if (res) {
                ok(0, "NtCreateNamedPipeFile returned %x, sharing: %x\n", res, sharing[j]);
                continue;
            }

            res = pNtQueryInformationFile(hserver, &iosb, &info, sizeof(info), (FILE_INFORMATION_CLASS)24);
            ok(!res, "NtQueryInformationFile for server returned %x, sharing: %x\n", res, sharing[j]);
            ok(info.NamedPipeConfiguration == pipe_config[j], "wrong duplex status for pipe: %d, expected %d\n",
               info.NamedPipeConfiguration, pipe_config[j]);

            hclient = CreateFileW(testpipe, access[k], 0, 0, OPEN_EXISTING, 0, 0);
            if (hclient != INVALID_HANDLE_VALUE) {
                res = pNtQueryInformationFile(hclient, &iosb, &info, sizeof(info), (FILE_INFORMATION_CLASS)24);
                ok(!res, "NtQueryInformationFile for client returned %x, access: %x, sharing: %x\n",
                   res, access[k], sharing[j]);
                ok(info.NamedPipeConfiguration == pipe_config[j], "wrong duplex status for pipe: %d, expected %d\n",
                   info.NamedPipeConfiguration, pipe_config[j]);
                CloseHandle(hclient);
            }

            if (access[k] & GENERIC_WRITE)
                should_succeed &= !!(sharing[j] & FILE_SHARE_WRITE);
            if (access[k] & GENERIC_READ)
                should_succeed &= !!(sharing[j] & FILE_SHARE_READ);

            if (should_succeed)
                ok(hclient != INVALID_HANDLE_VALUE, "CreateFile failed for sharing %x, access: %x, GetLastError: %d\n",
                   sharing[j], access[k], GetLastError());
            else
                ok(hclient == INVALID_HANDLE_VALUE, "CreateFile succeeded for sharing %x, access: %x\n", sharing[j], access[k]);

            CloseHandle(hserver);
        }
    }
}

static BOOL ioapc_called;
static void CALLBACK ioapc(void *arg, PIO_STATUS_BLOCK io, ULONG reserved)
{
    ioapc_called = TRUE;
}

static NTSTATUS listen_pipe(HANDLE hPipe, HANDLE hEvent, PIO_STATUS_BLOCK iosb, BOOL use_apc)
{
    int dummy;

    ioapc_called = FALSE;

    return pNtFsControlFile(hPipe, hEvent, use_apc ? &ioapc: NULL, use_apc ? &dummy: NULL, iosb, FSCTL_PIPE_LISTEN, 0, 0, 0, 0);
}

static void test_overlapped(void)
{
    IO_STATUS_BLOCK iosb;
    HANDLE hEvent;
    HANDLE hPipe;
    HANDLE hClient;
    NTSTATUS res;

    hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(hEvent != INVALID_HANDLE_VALUE, "can't create event, GetLastError: %x\n", GetLastError());

    res = create_pipe(&hPipe, FILE_SHARE_READ | FILE_SHARE_WRITE, 0 /* OVERLAPPED */);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

    memset(&iosb, 0x55, sizeof(iosb));

/* try with event and apc */
    res = listen_pipe(hPipe, hEvent, &iosb, TRUE);
    ok(res == STATUS_PENDING, "NtFsControlFile returned %x\n", res);

    hClient = CreateFileW(testpipe, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    ok(hClient != INVALID_HANDLE_VALUE, "can't open pipe, GetLastError: %x\n", GetLastError());

    ok(U(iosb).Status == 0, "Wrong iostatus %x\n", U(iosb).Status);
    ok(WaitForSingleObject(hEvent, 0) == 0, "hEvent not signaled\n");

    ok(!ioapc_called, "IOAPC ran too early\n");

    SleepEx(0, TRUE); /* alertable wait state */

    ok(ioapc_called, "IOAPC didn't run\n");

    CloseHandle(hEvent);
    CloseHandle(hPipe);
    CloseHandle(hClient);
}

static BOOL userapc_called;
static void CALLBACK userapc(ULONG_PTR dwParam)
{
    userapc_called = TRUE;
}

static BOOL open_succeeded;
static DWORD WINAPI thread(PVOID main_thread)
{
    HANDLE h;

    Sleep(400);

    if (main_thread) {
        DWORD ret;
        userapc_called = FALSE;
        ret = pQueueUserAPC(&userapc, main_thread, 0);
        ok(ret, "can't queue user apc, GetLastError: %x\n", GetLastError());
        CloseHandle(main_thread);
    }

    Sleep(400);

    h = CreateFileW(testpipe, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

    if (h != INVALID_HANDLE_VALUE) {
        open_succeeded = TRUE;
        Sleep(100);
        CloseHandle(h);
    } else
        open_succeeded = FALSE;

    return 0;
}

static void test_alertable(void)
{
    IO_STATUS_BLOCK iosb;
    HANDLE hEvent;
    HANDLE hPipe;
    NTSTATUS res;
    HANDLE hThread;
    DWORD ret;

    memset(&iosb, 0x55, sizeof(iosb));

    hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(hEvent != INVALID_HANDLE_VALUE, "can't create event, GetLastError: %x\n", GetLastError());

    res = create_pipe(&hPipe, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

/* queue an user apc before calling listen */
    userapc_called = FALSE;
    ret = pQueueUserAPC(&userapc, GetCurrentThread(), 0);
    ok(ret, "can't queue user apc, GetLastError: %x\n", GetLastError());

    res = listen_pipe(hPipe, hEvent, &iosb, TRUE);
    todo_wine ok(res == STATUS_CANCELLED, "NtFsControlFile returned %x\n", res);

    todo_wine ok(userapc_called, "user apc didn't run\n");
    ok(U(iosb).Status == 0x55555555, "iosb.Status got changed to %x\n", U(iosb).Status);
    todo_wine ok(WaitForSingleObjectEx(hEvent, 0, TRUE) == WAIT_TIMEOUT, "hEvent signaled\n");
    ok(!ioapc_called, "IOAPC ran\n");

/* queue an user apc from a different thread */
    hThread = CreateThread(NULL, 0, &thread, pOpenThread(MAXIMUM_ALLOWED, FALSE, GetCurrentThreadId()), 0, 0);
    ok(hThread != INVALID_HANDLE_VALUE, "can't create thread, GetLastError: %x\n", GetLastError());

    /* wine_todo: the earlier NtFsControlFile call gets cancelled after the pipe gets set into listen state
                  instead of before, so this NtFsControlFile will fail STATUS_INVALID_HANDLE */
    res = listen_pipe(hPipe, hEvent, &iosb, TRUE);
    todo_wine ok(res == STATUS_CANCELLED, "NtFsControlFile returned %x\n", res);

    ok(userapc_called, "user apc didn't run\n");
    todo_wine ok(U(iosb).Status == 0x55555555, "iosb.Status got changed to %x\n", U(iosb).Status);
    ok(WaitForSingleObjectEx(hEvent, 0, TRUE) == WAIT_TIMEOUT, "hEvent signaled\n");
    ok(!ioapc_called, "IOAPC ran\n");

    WaitForSingleObject(hThread, INFINITE);

    SleepEx(0, TRUE); /* get rid of the userapc, if NtFsControlFile failed */

    ok(open_succeeded, "couldn't open client side pipe\n");

    CloseHandle(hThread);
    DisconnectNamedPipe(hPipe);

/* finally try without an apc */
    hThread = CreateThread(NULL, 0, &thread, 0, 0, 0);
    ok(hThread != INVALID_HANDLE_VALUE, "can't create thread, GetLastError: %x\n", GetLastError());

    res = listen_pipe(hPipe, hEvent, &iosb, TRUE);
    todo_wine ok(!res, "NtFsControlFile returned %x\n", res);

    ok(open_succeeded, "couldn't open client side pipe\n");
    ok(!U(iosb).Status, "Wrong iostatus %x\n", U(iosb).Status);
    todo_wine ok(WaitForSingleObject(hEvent, 0) == 0, "hEvent not signaled\n");

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hEvent);
    CloseHandle(hPipe);
}

static void test_nonalertable(void)
{
    IO_STATUS_BLOCK iosb;
    HANDLE hEvent;
    HANDLE hPipe;
    NTSTATUS res;
    HANDLE hThread;
    DWORD ret;

    memset(&iosb, 0x55, sizeof(iosb));

    hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(hEvent != INVALID_HANDLE_VALUE, "can't create event, GetLastError: %x\n", GetLastError());

    res = create_pipe(&hPipe, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

    hThread = CreateThread(NULL, 0, &thread, 0, 0, 0);
    ok(hThread != INVALID_HANDLE_VALUE, "can't create thread, GetLastError: %x\n", GetLastError());

    userapc_called = FALSE;
    ret = pQueueUserAPC(&userapc, GetCurrentThread(), 0);
    ok(ret, "can't queue user apc, GetLastError: %x\n", GetLastError());

    res = listen_pipe(hPipe, hEvent, &iosb, TRUE);
    todo_wine ok(!res, "NtFsControlFile returned %x\n", res);

    ok(open_succeeded, "couldn't open client side pipe\n");
    todo_wine ok(!U(iosb).Status, "Wrong iostatus %x\n", U(iosb).Status);
    todo_wine ok(WaitForSingleObject(hEvent, 0) == 0, "hEvent not signaled\n");

    ok(!ioapc_called, "IOAPC ran too early\n");
    ok(!userapc_called, "user apc ran too early\n");

    SleepEx(0, TRUE); /* alertable wait state */

    ok(ioapc_called, "IOAPC didn't run\n");
    ok(userapc_called, "user apc didn't run\n");

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hEvent);
    CloseHandle(hPipe);
}

static void test_cancelio(void)
{
    IO_STATUS_BLOCK iosb;
    IO_STATUS_BLOCK cancel_sb;
    HANDLE hEvent;
    HANDLE hPipe;
    NTSTATUS res;

    hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(hEvent != INVALID_HANDLE_VALUE, "can't create event, GetLastError: %x\n", GetLastError());

    res = create_pipe(&hPipe, FILE_SHARE_READ | FILE_SHARE_WRITE, 0 /* OVERLAPPED */);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

    memset(&iosb, 0x55, sizeof(iosb));

    res = listen_pipe(hPipe, hEvent, &iosb, TRUE);
    ok(res == STATUS_PENDING, "NtFsControlFile returned %x\n", res);

    res = pNtCancelIoFile(hPipe, &cancel_sb);
    ok(!res, "NtCancelIoFile returned %x\n", res);

    ok(U(iosb).Status == STATUS_CANCELLED, "Wrong iostatus %x\n", U(iosb).Status);
    ok(WaitForSingleObject(hEvent, 0) == 0, "hEvent not signaled\n");

    ok(!ioapc_called, "IOAPC ran too early\n");

    SleepEx(0, TRUE); /* alertable wait state */

    ok(ioapc_called, "IOAPC didn't run\n");

    CloseHandle(hEvent);
    CloseHandle(hPipe);
}

static void _check_pipe_handle_state(int line, HANDLE handle, ULONG read, ULONG completion)
{
    IO_STATUS_BLOCK iosb;
    FILE_PIPE_INFORMATION fpi;
    NTSTATUS res;
    if (handle != INVALID_HANDLE_VALUE)
    {
        memset(&fpi, 0x55, sizeof(fpi));
        res = pNtQueryInformationFile(handle, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok_(__FILE__, line)(!res, "NtQueryInformationFile returned %x\n", res);
        ok_(__FILE__, line)(fpi.ReadMode == read, "Unexpected ReadMode, expected %x, got %x\n",
                            read, fpi.ReadMode);
        ok_(__FILE__, line)(fpi.CompletionMode == completion, "Unexpected CompletionMode, expected %x, got %x\n",
                            completion, fpi.CompletionMode);
    }
}
#define check_pipe_handle_state(handle, r, c) _check_pipe_handle_state(__LINE__, handle, r, c)

static void test_filepipeinfo(void)
{
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    LARGE_INTEGER timeout;
    HANDLE hServer, hClient;
    FILE_PIPE_INFORMATION fpi;
    NTSTATUS res;

    pRtlInitUnicodeString(&name, testpipe_nt);

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &name;
    attr.Attributes               = 0x40; /* case insensitive */
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    timeout.QuadPart = -100000000000ll;

    /* test with INVALID_HANDLE_VALUE */
    res = pNtQueryInformationFile(INVALID_HANDLE_VALUE, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(res == STATUS_OBJECT_TYPE_MISMATCH, "NtQueryInformationFile returned %x\n", res);

    fpi.ReadMode = 0;
    fpi.CompletionMode = 0;
    res = pNtSetInformationFile(INVALID_HANDLE_VALUE, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(res == STATUS_OBJECT_TYPE_MISMATCH, "NtSetInformationFile returned %x\n", res);

    /* server end with read-only attributes */
    res = pNtCreateNamedPipeFile(&hServer, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,  2 /* FILE_CREATE */,
                                 0, 0, 0, 1, 0xFFFFFFFF, 500, 500, &timeout);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

    check_pipe_handle_state(hServer, 0, 1);

    hClient = CreateFileW(testpipe, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    ok(hClient != INVALID_HANDLE_VALUE, "can't open pipe, GetLastError: %x\n", GetLastError());

    check_pipe_handle_state(hServer, 0, 1);
    check_pipe_handle_state(hClient, 0, 0);

    fpi.ReadMode = 0;
    fpi.CompletionMode = 0;
    res = pNtSetInformationFile(hServer, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(res == STATUS_ACCESS_DENIED, "NtSetInformationFile returned %x\n", res);

    check_pipe_handle_state(hServer, 0, 1);
    check_pipe_handle_state(hClient, 0, 0);

    fpi.ReadMode = 1; /* invalid on a byte stream pipe */
    fpi.CompletionMode = 1;
    res = pNtSetInformationFile(hServer, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(res == STATUS_ACCESS_DENIED, "NtSetInformationFile returned %x\n", res);

    check_pipe_handle_state(hServer, 0, 1);
    check_pipe_handle_state(hClient, 0, 0);

    if (hClient != INVALID_HANDLE_VALUE)
    {
        fpi.ReadMode = 1; /* invalid on a byte stream pipe */
        fpi.CompletionMode = 1;
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(res == STATUS_INVALID_PARAMETER, "NtSetInformationFile returned %x\n", res);
    }

    check_pipe_handle_state(hServer, 0, 1);
    check_pipe_handle_state(hClient, 0, 0);

    if (hClient != INVALID_HANDLE_VALUE)
    {
        fpi.ReadMode = 0;
        fpi.CompletionMode = 1;
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(!res, "NtSetInformationFile returned %x\n", res);
    }

    check_pipe_handle_state(hServer, 0, 1);
    check_pipe_handle_state(hClient, 0, 1);

    if (hClient != INVALID_HANDLE_VALUE)
    {
        fpi.ReadMode = 0;
        fpi.CompletionMode = 2; /* not in range 0-1 */
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(res == STATUS_INVALID_PARAMETER || broken(!res) /* < Vista */, "NtSetInformationFile returned %x\n", res);

        fpi.ReadMode = 2; /* not in range 0-1 */
        fpi.CompletionMode = 0;
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(res == STATUS_INVALID_PARAMETER || broken(!res) /* < Vista */, "NtSetInformationFile returned %x\n", res);
    }

    CloseHandle(hClient);

    check_pipe_handle_state(hServer, 0, 1);

    fpi.ReadMode = 0;
    fpi.CompletionMode = 0;
    res = pNtSetInformationFile(hServer, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(res == STATUS_ACCESS_DENIED, "NtSetInformationFile returned %x\n", res);

    CloseHandle(hServer);

    /* message mode server with read/write attributes */
    res = pNtCreateNamedPipeFile(&hServer, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,  2 /* FILE_CREATE */,
                                 0, 1, 1, 0, 0xFFFFFFFF, 500, 500, &timeout);
    ok(!res, "NtCreateNamedPipeFile returned %x\n", res);

    check_pipe_handle_state(hServer, 1, 0);

    hClient = CreateFileW(testpipe, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    ok(hClient != INVALID_HANDLE_VALUE, "can't open pipe, GetLastError: %x\n", GetLastError());

    check_pipe_handle_state(hServer, 1, 0);
    check_pipe_handle_state(hClient, 0, 0);

    if (hClient != INVALID_HANDLE_VALUE)
    {
        fpi.ReadMode = 1;
        fpi.CompletionMode = 1;
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(!res, "NtSetInformationFile returned %x\n", res);
    }

    check_pipe_handle_state(hServer, 1, 0);
    check_pipe_handle_state(hClient, 1, 1);

    fpi.ReadMode = 0;
    fpi.CompletionMode = 1;
    res = pNtSetInformationFile(hServer, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(!res, "NtSetInformationFile returned %x\n", res);

    check_pipe_handle_state(hServer, 0, 1);
    check_pipe_handle_state(hClient, 1, 1);

    if (hClient != INVALID_HANDLE_VALUE)
    {
        fpi.ReadMode = 0;
        fpi.CompletionMode = 2; /* not in range 0-1 */
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(res == STATUS_INVALID_PARAMETER || broken(!res) /* < Vista */, "NtSetInformationFile returned %x\n", res);

        fpi.ReadMode = 2; /* not in range 0-1 */
        fpi.CompletionMode = 0;
        res = pNtSetInformationFile(hClient, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
        ok(res == STATUS_INVALID_PARAMETER || broken(!res) /* < Vista */, "NtSetInformationFile returned %x\n", res);
    }

    CloseHandle(hClient);

    check_pipe_handle_state(hServer, 0, 1);

    fpi.ReadMode = 1;
    fpi.CompletionMode = 0;
    res = pNtSetInformationFile(hServer, &iosb, &fpi, sizeof(fpi), (FILE_INFORMATION_CLASS)23);
    ok(!res, "NtSetInformationFile returned %x\n", res);

    check_pipe_handle_state(hServer, 1, 0);

    CloseHandle(hServer);
}

START_TEST(pipe)
{
    if (!init_func_ptrs())
        return;

    trace("starting invalid create tests\n");
    test_create_invalid();

    trace("starting create tests\n");
    test_create();

    trace("starting overlapped tests\n");
    test_overlapped();

    trace("starting FILE_PIPE_INFORMATION tests\n");
    test_filepipeinfo();

    if (!pOpenThread || !pQueueUserAPC)
        return;

    trace("starting alertable tests\n");
    test_alertable();

    trace("starting nonalertable tests\n");
    test_nonalertable();

    trace("starting cancelio tests\n");
    test_cancelio();
}
