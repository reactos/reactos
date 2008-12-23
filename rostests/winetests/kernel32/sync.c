/*
 * Synchronization tests
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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
#include <stdlib.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>

#include "wine/test.h"

static BOOL   (WINAPI *pChangeTimerQueueTimer)(HANDLE, HANDLE, ULONG, ULONG);
static HANDLE (WINAPI *pCreateTimerQueue)(void);
static BOOL   (WINAPI *pCreateTimerQueueTimer)(PHANDLE, HANDLE, WAITORTIMERCALLBACK,
                                               PVOID, DWORD, DWORD, ULONG);
static HANDLE (WINAPI *pCreateWaitableTimerA)(SECURITY_ATTRIBUTES*,BOOL,LPCSTR);
static BOOL   (WINAPI *pDeleteTimerQueueEx)(HANDLE, HANDLE);
static BOOL   (WINAPI *pDeleteTimerQueueTimer)(HANDLE, HANDLE, HANDLE);
static HANDLE (WINAPI *pOpenWaitableTimerA)(DWORD,BOOL,LPCSTR);

static void test_signalandwait(void)
{
    DWORD (WINAPI *pSignalObjectAndWait)(HANDLE, HANDLE, DWORD, BOOL);
    HMODULE kernel32;
    DWORD r;
    int i;
    HANDLE event[2], maxevents[MAXIMUM_WAIT_OBJECTS], semaphore[2], file;

    kernel32 = GetModuleHandle("kernel32");
    pSignalObjectAndWait = (void*) GetProcAddress(kernel32, "SignalObjectAndWait");

    if (!pSignalObjectAndWait)
        return;

    /* invalid parameters */
    r = pSignalObjectAndWait(NULL, NULL, 0, 0);
    if (r == ERROR_INVALID_FUNCTION)
    {
        skip("SignalObjectAndWait is not implemented\n");
        return; /* Win98/ME */
    }
    ok( r == WAIT_FAILED, "should fail\n");

    event[0] = CreateEvent(NULL, 0, 0, NULL);
    event[1] = CreateEvent(NULL, 1, 1, NULL);

    ok( event[0] && event[1], "failed to create event flags\n");

    r = pSignalObjectAndWait(event[0], NULL, 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");

    r = pSignalObjectAndWait(NULL, event[0], 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");


    /* valid parameters */
    r = pSignalObjectAndWait(event[0], event[1], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    /* event[0] is now signalled */
    r = pSignalObjectAndWait(event[0], event[0], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    /* event[0] is not signalled */
    r = WaitForSingleObject(event[0], 0);
    ok( r == WAIT_TIMEOUT, "event was signalled\n");

    r = pSignalObjectAndWait(event[0], event[0], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    /* clear event[1] and check for a timeout */
    ok(ResetEvent(event[1]), "failed to clear event[1]\n");
    r = pSignalObjectAndWait(event[0], event[1], 0, FALSE);
    ok( r == WAIT_TIMEOUT, "should timeout\n");

    CloseHandle(event[0]);
    CloseHandle(event[1]);

    /* create the maximum number of events and make sure 
     * we can wait on that many */
    for (i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
    {
        maxevents[i] = CreateEvent(NULL, 1, 1, NULL);
        ok( maxevents[i] != 0, "should create enough events\n");
    }
    r = WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, maxevents, 0, 0);
    ok( r != WAIT_FAILED && r != WAIT_TIMEOUT, "should succeed\n");

    for (i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
        if (maxevents[i]) CloseHandle(maxevents[i]);

    /* semaphores */
    semaphore[0] = CreateSemaphore( NULL, 0, 1, NULL );
    semaphore[1] = CreateSemaphore( NULL, 1, 1, NULL );
    ok( semaphore[0] && semaphore[1], "failed to create semaphore\n");

    r = pSignalObjectAndWait(semaphore[0], semaphore[1], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    r = pSignalObjectAndWait(semaphore[0], semaphore[1], 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");

    r = ReleaseSemaphore(semaphore[0],1,NULL);
    ok( r == FALSE, "should fail\n");

    r = ReleaseSemaphore(semaphore[1],1,NULL);
    ok( r == TRUE, "should succeed\n");

    CloseHandle(semaphore[0]);
    CloseHandle(semaphore[1]);

    /* try a registry key */
    file = CreateFile("x", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    r = pSignalObjectAndWait(file, file, 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");
    ok( ERROR_INVALID_HANDLE == GetLastError(), "should return invalid handle error\n");
    CloseHandle(file);
}

static void test_mutex(void)
{
    DWORD wait_ret;
    BOOL ret;
    HANDLE hCreated;
    HANDLE hOpened;

    hCreated = CreateMutex(NULL, FALSE, "WineTestMutex");
    ok(hCreated != NULL, "CreateMutex failed with error %d\n", GetLastError());
    wait_ret = WaitForSingleObject(hCreated, INFINITE);
    ok(wait_ret == WAIT_OBJECT_0, "WaitForSingleObject failed with error 0x%08x\n", wait_ret);

    /* yes, opening with just READ_CONTROL access allows us to successfully
     * call ReleaseMutex */
    hOpened = OpenMutex(READ_CONTROL, FALSE, "WineTestMutex");
    ok(hOpened != NULL, "OpenMutex failed with error %d\n", GetLastError());
    ret = ReleaseMutex(hOpened);
    todo_wine ok(ret, "ReleaseMutex failed with error %d\n", GetLastError());
    ret = ReleaseMutex(hCreated);
    todo_wine ok(!ret && (GetLastError() == ERROR_NOT_OWNER),
        "ReleaseMutex should have failed with ERROR_NOT_OWNER instead of %d\n", GetLastError());

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    hOpened = OpenMutex(READ_CONTROL, FALSE, "WINETESTMUTEX");
    ok(!hOpened, "OpenMutex succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_NAME, /* win9x */
       "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    hOpened = OpenMutex(READ_CONTROL, FALSE, "winetestmutex");
    ok(!hOpened, "OpenMutex succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_NAME, /* win9x */
       "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    hOpened = CreateMutex(NULL, FALSE, "WineTestMutex");
    ok(hOpened != NULL, "CreateMutex failed with error %d\n", GetLastError());
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %u\n", GetLastError());
    CloseHandle(hOpened);

    SetLastError(0xdeadbeef);
    hOpened = CreateMutex(NULL, FALSE, "WINETESTMUTEX");
    ok(hOpened != NULL, "CreateMutex failed with error %d\n", GetLastError());
    ok(GetLastError() == 0, "wrong error %u\n", GetLastError());
    CloseHandle(hOpened);

    CloseHandle(hCreated);
}

static void test_slist(void)
{
    struct item
    {
        SLIST_ENTRY entry;
        int value;
    } item1, item2, item3, *pitem;

    SLIST_HEADER slist_header, test_header;
    PSLIST_ENTRY entry;
    USHORT size;

    VOID (WINAPI *pInitializeSListHead)(PSLIST_HEADER);
    USHORT (WINAPI *pQueryDepthSList)(PSLIST_HEADER);
    PSLIST_ENTRY (WINAPI *pInterlockedFlushSList)(PSLIST_HEADER);
    PSLIST_ENTRY (WINAPI *pInterlockedPopEntrySList)(PSLIST_HEADER);
    PSLIST_ENTRY (WINAPI *pInterlockedPushEntrySList)(PSLIST_HEADER,PSLIST_ENTRY);
    HMODULE kernel32;

    kernel32 = GetModuleHandle("KERNEL32.DLL");
    pInitializeSListHead = (void*) GetProcAddress(kernel32, "InitializeSListHead");
    pQueryDepthSList = (void*) GetProcAddress(kernel32, "QueryDepthSList");
    pInterlockedFlushSList = (void*) GetProcAddress(kernel32, "InterlockedFlushSList");
    pInterlockedPopEntrySList = (void*) GetProcAddress(kernel32, "InterlockedPopEntrySList");
    pInterlockedPushEntrySList = (void*) GetProcAddress(kernel32, "InterlockedPushEntrySList");
    if (pInitializeSListHead == NULL ||
        pQueryDepthSList == NULL ||
        pInterlockedFlushSList == NULL ||
        pInterlockedPopEntrySList == NULL ||
        pInterlockedPushEntrySList == NULL)
    {
        skip("some required slist entrypoints were not found, skipping tests\n");
        return;
    }

    memset(&test_header, 0, sizeof(test_header));
    memset(&slist_header, 0xFF, sizeof(slist_header));
    pInitializeSListHead(&slist_header);
    ok(memcmp(&test_header, &slist_header, sizeof(SLIST_HEADER)) == 0,
        "InitializeSListHead didn't zero-fill list header\n");
    size = pQueryDepthSList(&slist_header);
    ok(size == 0, "initially created slist has size %d, expected 0\n", size);

    item1.value = 1;
    ok(pInterlockedPushEntrySList(&slist_header, &item1.entry) == NULL,
        "previous entry in empty slist wasn't NULL\n");
    size = pQueryDepthSList(&slist_header);
    ok(size == 1, "slist with 1 item has size %d\n", size);

    item2.value = 2;
    entry = pInterlockedPushEntrySList(&slist_header, &item2.entry);
    ok(entry != NULL, "previous entry in non-empty slist was NULL\n");
    if (entry != NULL)
    {
        pitem = (struct item*) entry;
        ok(pitem->value == 1, "previous entry in slist wasn't the one added\n");
    }
    size = pQueryDepthSList(&slist_header);
    ok(size == 2, "slist with 2 items has size %d\n", size);

    item3.value = 3;
    entry = pInterlockedPushEntrySList(&slist_header, &item3.entry);
    ok(entry != NULL, "previous entry in non-empty slist was NULL\n");
    if (entry != NULL)
    {
        pitem = (struct item*) entry;
        ok(pitem->value == 2, "previous entry in slist wasn't the one added\n");
    }
    size = pQueryDepthSList(&slist_header);
    ok(size == 3, "slist with 3 items has size %d\n", size);

    entry = pInterlockedPopEntrySList(&slist_header);
    ok(entry != NULL, "entry shouldn't be NULL\n");
    if (entry != NULL)
    {
        pitem = (struct item*) entry;
        ok(pitem->value == 3, "unexpected entry removed\n");
    }
    size = pQueryDepthSList(&slist_header);
    ok(size == 2, "slist with 2 items has size %d\n", size);

    entry = pInterlockedFlushSList(&slist_header);
    size = pQueryDepthSList(&slist_header);
    ok(size == 0, "flushed slist should be empty, size is %d\n", size);
    if (size == 0)
    {
        ok(pInterlockedPopEntrySList(&slist_header) == NULL,
            "popping empty slist didn't return NULL\n");
    }
    ok(((struct item*)entry)->value == 2, "item 2 not in front of list\n");
    ok(((struct item*)entry->Next)->value == 1, "item 1 not at the back of list\n");
}

static void test_event(void)
{
    HANDLE handle, handle2;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    ACL acl;

    /* no sd */
    handle = CreateEventA(NULL, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    /* blank sd */
    handle = CreateEventA(&sa, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    /* sd with NULL dacl */
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    handle = CreateEventA(&sa, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    /* sd with empty dacl */
    InitializeAcl(&acl, sizeof(acl), ACL_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, &acl, FALSE);
    handle = CreateEventA(&sa, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    handle = CreateEventA(NULL, FALSE, FALSE, __FILE__ ": Test Event");
    ok( handle != NULL, "CreateEvent failed with error %u\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle2 = CreateEventA(NULL, FALSE, FALSE, __FILE__ ": Test Event");
    ok( handle2 != NULL, "CreateEvent failed with error %d\n", GetLastError());
    ok( GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = CreateEventA(NULL, FALSE, FALSE, __FILE__ ": TEST EVENT");
    ok( handle2 != NULL, "CreateEvent failed with error %d\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenEventA( EVENT_ALL_ACCESS, FALSE, __FILE__ ": Test Event");
    ok( handle2 != NULL, "OpenEvent failed with error %d\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenEventA( EVENT_ALL_ACCESS, FALSE, __FILE__ ": TEST EVENT");
    ok( !handle2, "OpenEvent succeeded\n");
    ok( GetLastError() == ERROR_FILE_NOT_FOUND ||
        GetLastError() == ERROR_INVALID_NAME, /* win9x */
        "wrong error %u\n", GetLastError());

    CloseHandle( handle );
}

static void test_semaphore(void)
{
    HANDLE handle, handle2;

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    handle = CreateSemaphoreA(NULL, 0, 1, __FILE__ ": Test Semaphore");
    ok(handle != NULL, "CreateSemaphore failed with error %u\n", GetLastError());
    ok(GetLastError() == 0, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle2 = CreateSemaphoreA(NULL, 0, 1, __FILE__ ": Test Semaphore");
    ok( handle2 != NULL, "CreateSemaphore failed with error %d\n", GetLastError());
    ok( GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = CreateSemaphoreA(NULL, 0, 1, __FILE__ ": TEST SEMAPHORE");
    ok( handle2 != NULL, "CreateSemaphore failed with error %d\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenSemaphoreA( SEMAPHORE_ALL_ACCESS, FALSE, __FILE__ ": Test Semaphore");
    ok( handle2 != NULL, "OpenSemaphore failed with error %d\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenSemaphoreA( SEMAPHORE_ALL_ACCESS, FALSE, __FILE__ ": TEST SEMAPHORE");
    ok( !handle2, "OpenSemaphore succeeded\n");
    ok( GetLastError() == ERROR_FILE_NOT_FOUND ||
        GetLastError() == ERROR_INVALID_NAME, /* win9x */
        "wrong error %u\n", GetLastError());

    CloseHandle( handle );
}

static void test_waitable_timer(void)
{
    HANDLE handle, handle2;

    if (!pCreateWaitableTimerA || !pOpenWaitableTimerA)
    {
        skip("{Create,Open}WaitableTimerA() is not available\n");
        return;
    }

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    handle = pCreateWaitableTimerA(NULL, FALSE, __FILE__ ": Test WaitableTimer");
    ok(handle != NULL, "CreateWaitableTimer failed with error %u\n", GetLastError());
    ok(GetLastError() == 0, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle2 = pCreateWaitableTimerA(NULL, FALSE, __FILE__ ": Test WaitableTimer");
    ok( handle2 != NULL, "CreateWaitableTimer failed with error %d\n", GetLastError());
    ok( GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = pCreateWaitableTimerA(NULL, FALSE, __FILE__ ": TEST WAITABLETIMER");
    ok( handle2 != NULL, "CreateWaitableTimer failed with error %d\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = pOpenWaitableTimerA( TIMER_ALL_ACCESS, FALSE, __FILE__ ": Test WaitableTimer");
    ok( handle2 != NULL, "OpenWaitableTimer failed with error %d\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = pOpenWaitableTimerA( TIMER_ALL_ACCESS, FALSE, __FILE__ ": TEST WAITABLETIMER");
    ok( !handle2, "OpenWaitableTimer succeeded\n");
    ok( GetLastError() == ERROR_FILE_NOT_FOUND ||
        GetLastError() == ERROR_INVALID_NAME, /* win98 */
        "wrong error %u\n", GetLastError());

    CloseHandle( handle );
}

static HANDLE sem = 0;

static void CALLBACK iocp_callback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)
{
    ReleaseSemaphore(sem, 1, NULL);
}

static BOOL (WINAPI *p_BindIoCompletionCallback)( HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags) = NULL;

static void test_iocp_callback(void)
{
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    DWORD ret;
    BOOL retb;
    static const char prefix[] = "pfx";
    HANDLE hFile;
    HMODULE hmod = GetModuleHandleA("kernel32.dll");
    DWORD bytesWritten;
    const char *buffer = "12345678123456781234567812345678";
    OVERLAPPED overlapped;

    p_BindIoCompletionCallback = (void*)GetProcAddress(hmod, "BindIoCompletionCallback");
    if(!p_BindIoCompletionCallback) {
        skip("BindIoCompletionCallback not found in this DLL\n");
        return;
    }

    sem = CreateSemaphore(NULL, 0, 1, NULL);
    ok(sem != INVALID_HANDLE_VALUE, "Creating a semaphore failed\n");

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());

    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded on a file that wasn't created with FILE_FLAG_OVERLAPPED\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error is %d\n", GetLastError());

    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());

    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 0);
    ok(retb == TRUE, "BindIoCompletionCallback failed\n");

    memset(&overlapped, 0, sizeof(overlapped));
    retb = WriteFile(hFile, (const void *) buffer, 4, &bytesWritten, &overlapped);
    ok(retb == TRUE || GetLastError() == ERROR_IO_PENDING, "WriteFile failed, lastError = %d\n", GetLastError());

    ret = WaitForSingleObject(sem, 5000);
    ok(ret == WAIT_OBJECT_0, "Wait for the IO completion callback failed\n");
    CloseHandle(sem);

    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded when setting the same callback on the file again\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error is %d\n", GetLastError());
    retb = p_BindIoCompletionCallback(hFile, NULL, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded when setting the callback to NULL\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error is %d\n", GetLastError());

    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    /* win2k3 requires the Flags parameter to be zero */
    SetLastError(0xdeadbeef);
    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());
    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 12345);
    if (!retb)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    else
        ok(retb == TRUE, "BindIoCompletionCallback failed with Flags != 0\n");
    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    retb = p_BindIoCompletionCallback(NULL, iocp_callback, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded on a NULL file\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* vista */
       "Last error is %d\n", GetLastError());
}

static void CALLBACK timer_queue_cb1(PVOID p, BOOLEAN timedOut)
{
    int *pn = (int *) p;
    ok(timedOut, "Timer callbacks should always time out\n");
    ++*pn;
}

struct timer_queue_data1
{
    int num_calls;
    int max_calls;
    HANDLE q, t;
};

static void CALLBACK timer_queue_cb2(PVOID p, BOOLEAN timedOut)
{
    struct timer_queue_data1 *d = p;
    ok(timedOut, "Timer callbacks should always time out\n");
    if (d->t && ++d->num_calls == d->max_calls)
    {
        BOOL ret;
        SetLastError(0xdeadbeef);
        /* Note, XP SP2 does *not* do any deadlock checking, so passing
           INVALID_HANDLE_VALUE here will just hang.  */
        ret = pDeleteTimerQueueTimer(d->q, d->t, NULL);
        ok(!ret, "DeleteTimerQueueTimer\n");
        ok(GetLastError() == ERROR_IO_PENDING, "DeleteTimerQueueTimer\n");
    }
}

static void CALLBACK timer_queue_cb3(PVOID p, BOOLEAN timedOut)
{
    struct timer_queue_data1 *d = p;
    ok(timedOut, "Timer callbacks should always time out\n");
    if (d->t && ++d->num_calls == d->max_calls)
    {
        /* Basically kill the timer since it won't have time to run
           again.  */
        BOOL ret = pChangeTimerQueueTimer(d->q, d->t, 10000, 0);
        ok(ret, "ChangeTimerQueueTimer\n");
    }
}

static void CALLBACK timer_queue_cb4(PVOID p, BOOLEAN timedOut)
{
    struct timer_queue_data1 *d = p;
    ok(timedOut, "Timer callbacks should always time out\n");
    if (d->t)
    {
        /* This tests whether a timer gets flagged for deletion before
           or after the callback runs.  If we start this timer with a
           period of zero (run once), then ChangeTimerQueueTimer will
           fail if the timer is already flagged.  Hence we really run
           only once.  Otherwise we will run multiple times.  */
        BOOL ret = pChangeTimerQueueTimer(d->q, d->t, 50, 50);
        ok(ret, "ChangeTimerQueueTimer\n");
        ++d->num_calls;
    }
}

static void CALLBACK timer_queue_cb5(PVOID p, BOOLEAN timedOut)
{
    DWORD delay = (DWORD) p;
    ok(timedOut, "Timer callbacks should always time out\n");
    if (delay)
        Sleep(delay);
}

static void CALLBACK timer_queue_cb6(PVOID p, BOOLEAN timedOut)
{
    struct timer_queue_data1 *d = p;
    ok(timedOut, "Timer callbacks should always time out\n");
    /* This tests an original implementation bug where a deleted timer may get
       to run, but it is tricky to set up.  */
    if (d->q && d->num_calls++ == 0)
    {
        /* First run: delete ourselves, then insert and remove a timer
           that goes in front of us in the sorted timeout list.  Once
           removed, we will still timeout at the faster timer's due time,
           but this should be a no-op if we are bug-free.  There should
           not be a second run.  We can test the value of num_calls later.  */
        BOOL ret;
        HANDLE t;

        /* The delete will pend while we are in this callback.  */
        SetLastError(0xdeadbeef);
        ret = pDeleteTimerQueueTimer(d->q, d->t, NULL);
        ok(!ret, "DeleteTimerQueueTimer\n");
        ok(GetLastError() == ERROR_IO_PENDING, "DeleteTimerQueueTimer\n");

        ret = pCreateTimerQueueTimer(&t, d->q, timer_queue_cb1, NULL, 100, 0, 0);
        ok(ret, "CreateTimerQueueTimer\n");
        ok(t != NULL, "CreateTimerQueueTimer\n");

        ret = pDeleteTimerQueueTimer(d->q, t, INVALID_HANDLE_VALUE);
        ok(ret, "DeleteTimerQueueTimer\n");

        /* Now we stay alive by hanging around in the callback.  */
        Sleep(500);
    }
}

static void test_timer_queue(void)
{
    HANDLE q, t1, t2, t3, t4, t5;
    int n1, n2, n3, n4, n5;
    struct timer_queue_data1 d1, d2, d3, d4;
    HANDLE e, et1, et2;
    BOOL ret;

    if (!pChangeTimerQueueTimer || !pCreateTimerQueue || !pCreateTimerQueueTimer
        || !pDeleteTimerQueueEx || !pDeleteTimerQueueTimer)
    {
        skip("TimerQueue API not present\n");
        return;
    }

    /* Test asynchronous deletion of the queue. */
    q = pCreateTimerQueue();
    ok(q != NULL, "CreateTimerQueue\n");

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueEx(q, NULL);
    ok(ret /* vista */ || GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueEx, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());

    /* Test synchronous deletion of the queue and running timers. */
    q = pCreateTimerQueue();
    ok(q != NULL, "CreateTimerQueue\n");

    /* Called once.  */
    t1 = NULL;
    n1 = 0;
    ret = pCreateTimerQueueTimer(&t1, q, timer_queue_cb1, &n1, 0,
                                 0, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t1 != NULL, "CreateTimerQueueTimer\n");

    /* A slow one.  */
    t2 = NULL;
    n2 = 0;
    ret = pCreateTimerQueueTimer(&t2, q, timer_queue_cb1, &n2, 0,
                                 100, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t2 != NULL, "CreateTimerQueueTimer\n");

    /* A fast one.  */
    t3 = NULL;
    n3 = 0;
    ret = pCreateTimerQueueTimer(&t3, q, timer_queue_cb1, &n3, 0,
                                 10, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t3 != NULL, "CreateTimerQueueTimer\n");

    /* Start really late (it won't start).  */
    t4 = NULL;
    n4 = 0;
    ret = pCreateTimerQueueTimer(&t4, q, timer_queue_cb1, &n4, 10000,
                                 10, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t4 != NULL, "CreateTimerQueueTimer\n");

    /* Start soon, but delay so long it won't run again.  */
    t5 = NULL;
    n5 = 0;
    ret = pCreateTimerQueueTimer(&t5, q, timer_queue_cb1, &n5, 0,
                                 10000, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t5 != NULL, "CreateTimerQueueTimer\n");

    /* Give them a chance to do some work.  */
    Sleep(500);

    /* Test deleting a once-only timer.  */
    ret = pDeleteTimerQueueTimer(q, t1, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueTimer\n");

    /* A periodic timer.  */
    ret = pDeleteTimerQueueTimer(q, t2, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueTimer\n");

    ret = pDeleteTimerQueueEx(q, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueEx\n");
    ok(n1 == 1, "Timer callback 1\n");
    ok(n2 < n3, "Timer callback 2 should be much slower than 3\n");
    ok(n4 == 0, "Timer callback 4\n");
    ok(n5 == 1, "Timer callback 5\n");

    /* Test synchronous deletion of the timer/queue with event trigger. */
    e = CreateEvent(NULL, TRUE, FALSE, NULL);
    et1 = CreateEvent(NULL, TRUE, FALSE, NULL);
    et2 = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!e || !et1 || !et2)
    {
        skip("Failed to create timer queue descruction event\n");
        return;
    }

    q = pCreateTimerQueue();
    ok(q != NULL, "CreateTimerQueue\n");

    /* Run once and finish quickly (should be done when we delete it).  */
    t1 = NULL;
    ret = pCreateTimerQueueTimer(&t1, q, timer_queue_cb5, NULL, 0, 0, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t1 != NULL, "CreateTimerQueueTimer\n");

    /* Run once and finish slowly (shouldn't be done when we delete it).  */
    t2 = NULL;
    ret = pCreateTimerQueueTimer(&t2, q, timer_queue_cb5, (PVOID) 1000, 0,
                                 0, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t2 != NULL, "CreateTimerQueueTimer\n");

    /* Run once and finish quickly (should be done when we delete it).  */
    t3 = NULL;
    ret = pCreateTimerQueueTimer(&t3, q, timer_queue_cb5, NULL, 0, 0, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t3 != NULL, "CreateTimerQueueTimer\n");

    /* Run once and finish slowly (shouldn't be done when we delete it).  */
    t4 = NULL;
    ret = pCreateTimerQueueTimer(&t4, q, timer_queue_cb5, (PVOID) 1000, 0,
                                 0, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t4 != NULL, "CreateTimerQueueTimer\n");

    /* Give them a chance to start.  */
    Sleep(400);

    /* DeleteTimerQueueTimer always returns PENDING with a NULL event,
       even if the timer is finished.  */
    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueTimer(q, t1, NULL);
    ok(ret /* vista */ || GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueTimer, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueTimer(q, t2, NULL);
    ok(!ret, "DeleteTimerQueueTimer call was expected to fail\n");
    ok(GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueTimer, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueTimer(q, t3, et1);
    ok(ret, "DeleteTimerQueueTimer call was expected to fail\n");
    ok(GetLastError() == 0xdeadbeef,
       "DeleteTimerQueueTimer, GetLastError: expected 0xdeadbeef, got %d\n",
       GetLastError());
    ok(WaitForSingleObject(et1, 250) == WAIT_OBJECT_0,
       "Timer destruction event not triggered\n");

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueTimer(q, t4, et2);
    ok(!ret, "DeleteTimerQueueTimer call was expected to fail\n");
    ok(GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueTimer, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());
    ok(WaitForSingleObject(et2, 1000) == WAIT_OBJECT_0,
       "Timer destruction event not triggered\n");

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueEx(q, e);
    ok(ret /* vista */ || GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueEx, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());
    ok(WaitForSingleObject(e, 250) == WAIT_OBJECT_0,
       "Queue destruction event not triggered\n");
    CloseHandle(e);

    /* Test deleting/changing a timer in execution.  */
    q = pCreateTimerQueue();
    ok(q != NULL, "CreateTimerQueue\n");

    /* Test changing a once-only timer before it fires (this is allowed,
       whereas after it fires you cannot).  */
    n1 = 0;
    ret = pCreateTimerQueueTimer(&t1, q, timer_queue_cb1, &n1, 10000,
                                 0, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t1 != NULL, "CreateTimerQueueTimer\n");
    ret = pChangeTimerQueueTimer(q, t1, 0, 0);
    ok(ret, "ChangeTimerQueueTimer\n");

    d2.t = t2 = NULL;
    d2.num_calls = 0;
    d2.max_calls = 3;
    d2.q = q;
    ret = pCreateTimerQueueTimer(&t2, q, timer_queue_cb2, &d2, 10,
                                 10, 0);
    d2.t = t2;
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t2 != NULL, "CreateTimerQueueTimer\n");

    d3.t = t3 = NULL;
    d3.num_calls = 0;
    d3.max_calls = 4;
    d3.q = q;
    ret = pCreateTimerQueueTimer(&t3, q, timer_queue_cb3, &d3, 10,
                                 10, 0);
    d3.t = t3;
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t3 != NULL, "CreateTimerQueueTimer\n");

    d4.t = t4 = NULL;
    d4.num_calls = 0;
    d4.q = q;
    ret = pCreateTimerQueueTimer(&t4, q, timer_queue_cb4, &d4, 10,
                                 0, 0);
    d4.t = t4;
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t4 != NULL, "CreateTimerQueueTimer\n");

    Sleep(500);

    ret = pDeleteTimerQueueEx(q, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueEx\n");
    ok(n1 == 1, "ChangeTimerQueueTimer\n");
    ok(d2.num_calls == d2.max_calls, "DeleteTimerQueueTimer\n");
    ok(d3.num_calls == d3.max_calls, "ChangeTimerQueueTimer\n");
    ok(d4.num_calls == 1, "Timer flagged for deletion incorrectly\n");

    /* Test an obscure bug that was in the original implementation.  */
    q = pCreateTimerQueue();
    ok(q != NULL, "CreateTimerQueue\n");

    /* All the work is done in the callback.  */
    d1.t = t1 = NULL;
    d1.num_calls = 0;
    d1.q = q;
    ret = pCreateTimerQueueTimer(&t1, q, timer_queue_cb6, &d1, 100,
                                 100, WT_EXECUTELONGFUNCTION);
    d1.t = t1;
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t1 != NULL, "CreateTimerQueueTimer\n");

    Sleep(750);

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueEx(q, NULL);
    ok(ret /* vista */ || GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueEx, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());
    ok(d1.num_calls == 1, "DeleteTimerQueueTimer\n");

    /* Test functions on the default timer queue.  */
    t1 = NULL;
    n1 = 0;
    ret = pCreateTimerQueueTimer(&t1, NULL, timer_queue_cb1, &n1, 1000,
                                 1000, 0);
    ok(ret, "CreateTimerQueueTimer, default queue\n");
    ok(t1 != NULL, "CreateTimerQueueTimer, default queue\n");

    ret = pChangeTimerQueueTimer(NULL, t1, 2000, 2000);
    ok(ret, "ChangeTimerQueueTimer, default queue\n");

    ret = pDeleteTimerQueueTimer(NULL, t1, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueTimer, default queue\n");

    /* Try mixing default and non-default queues.  Apparently this works.  */
    q = pCreateTimerQueue();
    ok(q != NULL, "CreateTimerQueue\n");

    t1 = NULL;
    n1 = 0;
    ret = pCreateTimerQueueTimer(&t1, q, timer_queue_cb1, &n1, 1000,
                                 1000, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t1 != NULL, "CreateTimerQueueTimer\n");

    t2 = NULL;
    n2 = 0;
    ret = pCreateTimerQueueTimer(&t2, NULL, timer_queue_cb1, &n2, 1000,
                                 1000, 0);
    ok(ret, "CreateTimerQueueTimer\n");
    ok(t2 != NULL, "CreateTimerQueueTimer\n");

    ret = pChangeTimerQueueTimer(NULL, t1, 2000, 2000);
    ok(ret, "ChangeTimerQueueTimer\n");

    ret = pChangeTimerQueueTimer(q, t2, 2000, 2000);
    ok(ret, "ChangeTimerQueueTimer\n");

    ret = pDeleteTimerQueueTimer(NULL, t1, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueTimer\n");

    ret = pDeleteTimerQueueTimer(q, t2, INVALID_HANDLE_VALUE);
    ok(ret, "DeleteTimerQueueTimer\n");

    /* Try to delete the default queue?  In any case: not allowed.  */
    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueEx(NULL, NULL);
    ok(!ret, "DeleteTimerQueueEx call was expected to fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "DeleteTimerQueueEx, GetLastError: expected ERROR_INVALID_HANDLE, got %d\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pDeleteTimerQueueEx(q, NULL);
    ok(ret /* vista */ || GetLastError() == ERROR_IO_PENDING,
       "DeleteTimerQueueEx, GetLastError: expected ERROR_IO_PENDING, got %d\n",
       GetLastError());
}

START_TEST(sync)
{
    HMODULE hdll = GetModuleHandle("kernel32");
    pChangeTimerQueueTimer = (void*)GetProcAddress(hdll, "ChangeTimerQueueTimer");
    pCreateTimerQueue = (void*)GetProcAddress(hdll, "CreateTimerQueue");
    pCreateTimerQueueTimer = (void*)GetProcAddress(hdll, "CreateTimerQueueTimer");
    pCreateWaitableTimerA = (void*)GetProcAddress(hdll, "CreateWaitableTimerA");
    pDeleteTimerQueueEx = (void*)GetProcAddress(hdll, "DeleteTimerQueueEx");
    pDeleteTimerQueueTimer = (void*)GetProcAddress(hdll, "DeleteTimerQueueTimer");
    pOpenWaitableTimerA = (void*)GetProcAddress(hdll, "OpenWaitableTimerA");

    test_signalandwait();
    test_mutex();
    test_slist();
    test_event();
    test_semaphore();
    test_waitable_timer();
    test_iocp_callback();
    test_timer_queue();
}
