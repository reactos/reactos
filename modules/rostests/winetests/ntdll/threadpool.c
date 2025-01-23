/*
 * Unit test suite for thread pool functions
 *
 * Copyright 2015-2016 Sebastian Lackner
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

#include "ntdll_test.h"

static NTSTATUS (WINAPI *pTpAllocCleanupGroup)(TP_CLEANUP_GROUP **);
static NTSTATUS (WINAPI *pTpAllocIoCompletion)(TP_IO **,HANDLE,PTP_IO_CALLBACK,void *,TP_CALLBACK_ENVIRON *);
static NTSTATUS (WINAPI *pTpAllocPool)(TP_POOL **,PVOID);
static NTSTATUS (WINAPI *pTpAllocTimer)(TP_TIMER **,PTP_TIMER_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
static NTSTATUS (WINAPI *pTpAllocWait)(TP_WAIT **,PTP_WAIT_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
static NTSTATUS (WINAPI *pTpAllocWork)(TP_WORK **,PTP_WORK_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
static NTSTATUS (WINAPI *pTpCallbackMayRunLong)(TP_CALLBACK_INSTANCE *);
static VOID     (WINAPI *pTpCallbackReleaseSemaphoreOnCompletion)(TP_CALLBACK_INSTANCE *,HANDLE,DWORD);
static void     (WINAPI *pTpCancelAsyncIoOperation)(TP_IO *);
static VOID     (WINAPI *pTpDisassociateCallback)(TP_CALLBACK_INSTANCE *);
static BOOL     (WINAPI *pTpIsTimerSet)(TP_TIMER *);
static VOID     (WINAPI *pTpPostWork)(TP_WORK *);
static NTSTATUS (WINAPI *pTpQueryPoolStackInformation)(TP_POOL *,TP_POOL_STACK_INFORMATION *);
static VOID     (WINAPI *pTpReleaseCleanupGroup)(TP_CLEANUP_GROUP *);
static VOID     (WINAPI *pTpReleaseCleanupGroupMembers)(TP_CLEANUP_GROUP *,BOOL,PVOID);
static void     (WINAPI *pTpReleaseIoCompletion)(TP_IO *);
static VOID     (WINAPI *pTpReleasePool)(TP_POOL *);
static VOID     (WINAPI *pTpReleaseTimer)(TP_TIMER *);
static VOID     (WINAPI *pTpReleaseWait)(TP_WAIT *);
static VOID     (WINAPI *pTpReleaseWork)(TP_WORK *);
static VOID     (WINAPI *pTpSetPoolMaxThreads)(TP_POOL *,DWORD);
static NTSTATUS (WINAPI *pTpSetPoolStackInformation)(TP_POOL *,TP_POOL_STACK_INFORMATION *);
static VOID     (WINAPI *pTpSetTimer)(TP_TIMER *,LARGE_INTEGER *,LONG,LONG);
static VOID     (WINAPI *pTpSetWait)(TP_WAIT *,HANDLE,LARGE_INTEGER *);
static NTSTATUS (WINAPI *pTpSimpleTryPost)(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
static void     (WINAPI *pTpStartAsyncIoOperation)(TP_IO *);
static void     (WINAPI *pTpWaitForIoCompletion)(TP_IO *,BOOL);
static VOID     (WINAPI *pTpWaitForTimer)(TP_TIMER *,BOOL);
static VOID     (WINAPI *pTpWaitForWait)(TP_WAIT *,BOOL);
static VOID     (WINAPI *pTpWaitForWork)(TP_WORK *,BOOL);

static void (WINAPI *pCancelThreadpoolIo)(TP_IO *);
static void (WINAPI *pCloseThreadpoolIo)(TP_IO *);
static TP_IO *(WINAPI *pCreateThreadpoolIo)(HANDLE, PTP_WIN32_IO_CALLBACK, void *, TP_CALLBACK_ENVIRON *);
static void (WINAPI *pStartThreadpoolIo)(TP_IO *);
static void (WINAPI *pWaitForThreadpoolIoCallbacks)(TP_IO *, BOOL);

#define GET_PROC(func) \
    do \
    { \
        p ## func = (void *)GetProcAddress(module, #func); \
        if (!p ## func) trace("Failed to get address for %s\n", #func); \
    } \
    while (0)

static BOOL init_threadpool(void)
{
    HMODULE module = GetModuleHandleA("ntdll");
    GET_PROC(TpAllocCleanupGroup);
    GET_PROC(TpAllocIoCompletion);
    GET_PROC(TpAllocPool);
    GET_PROC(TpAllocTimer);
    GET_PROC(TpAllocWait);
    GET_PROC(TpAllocWork);
    GET_PROC(TpCallbackMayRunLong);
    GET_PROC(TpCallbackReleaseSemaphoreOnCompletion);
    GET_PROC(TpCancelAsyncIoOperation);
    GET_PROC(TpDisassociateCallback);
    GET_PROC(TpIsTimerSet);
    GET_PROC(TpPostWork);
    GET_PROC(TpQueryPoolStackInformation);
    GET_PROC(TpReleaseCleanupGroup);
    GET_PROC(TpReleaseCleanupGroupMembers);
    GET_PROC(TpReleaseIoCompletion);
    GET_PROC(TpReleasePool);
    GET_PROC(TpReleaseTimer);
    GET_PROC(TpReleaseWait);
    GET_PROC(TpReleaseWork);
    GET_PROC(TpSetPoolMaxThreads);
    GET_PROC(TpSetPoolStackInformation);
    GET_PROC(TpSetTimer);
    GET_PROC(TpSetWait);
    GET_PROC(TpSimpleTryPost);
    GET_PROC(TpStartAsyncIoOperation);
    GET_PROC(TpWaitForIoCompletion);
    GET_PROC(TpWaitForTimer);
    GET_PROC(TpWaitForWait);
    GET_PROC(TpWaitForWork);

    module = GetModuleHandleA("kernel32");
    GET_PROC(CancelThreadpoolIo);
    GET_PROC(CloseThreadpoolIo);
    GET_PROC(CreateThreadpoolIo);
    GET_PROC(StartThreadpoolIo);
    GET_PROC(WaitForThreadpoolIoCallbacks);

    if (!pTpAllocPool)
    {
        win_skip("Threadpool functions not supported, skipping tests\n");
        return FALSE;
    }

    return TRUE;
}

#undef NTDLL_GET_PROC


static DWORD CALLBACK rtl_work_cb(void *userdata)
{
    HANDLE semaphore = userdata;
    ReleaseSemaphore(semaphore, 1, NULL);
    return 0;
}

static void test_RtlQueueWorkItem(void)
{
    HANDLE semaphore;
    NTSTATUS status;
    DWORD result;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    status = RtlQueueWorkItem(rtl_work_cb, semaphore, WT_EXECUTEDEFAULT);
    ok(!status, "RtlQueueWorkItem failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    status = RtlQueueWorkItem(rtl_work_cb, semaphore, WT_EXECUTEINIOTHREAD);
    ok(!status, "RtlQueueWorkItem failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    status = RtlQueueWorkItem(rtl_work_cb, semaphore, WT_EXECUTEINPERSISTENTTHREAD);
    ok(!status, "RtlQueueWorkItem failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    status = RtlQueueWorkItem(rtl_work_cb, semaphore, WT_EXECUTELONGFUNCTION);
    ok(!status, "RtlQueueWorkItem failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    status = RtlQueueWorkItem(rtl_work_cb, semaphore, WT_TRANSFER_IMPERSONATION);
    ok(!status, "RtlQueueWorkItem failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    CloseHandle(semaphore);
}

struct rtl_wait_info
{
    HANDLE semaphore1;
    HANDLE semaphore2;
    DWORD wait_result;
    DWORD threadid;
    LONG userdata;
};

static void CALLBACK rtl_wait_cb(void *userdata, BOOLEAN timeout)
{
    struct rtl_wait_info *info = userdata;
    DWORD result;

    if (!timeout)
        InterlockedIncrement(&info->userdata);
    else
        InterlockedExchangeAdd(&info->userdata, 0x10000);
    info->threadid = GetCurrentThreadId();
    ReleaseSemaphore(info->semaphore1, 1, NULL);

    if (info->semaphore2)
    {
        result = WaitForSingleObject(info->semaphore2, 200);
        ok(result == info->wait_result, "expected %lu, got %lu\n", info->wait_result, result);
        ReleaseSemaphore(info->semaphore1, 1, NULL);
    }
}

static HANDLE rtl_wait_apc_semaphore;

static void CALLBACK rtl_wait_apc_cb(ULONG_PTR userdata)
{
    if (rtl_wait_apc_semaphore)
        ReleaseSemaphore(rtl_wait_apc_semaphore, 1, NULL);
}

static void test_RtlRegisterWait(void)
{
    HANDLE wait1, event, thread;
    struct rtl_wait_info info;
    HANDLE semaphores[2];
    NTSTATUS status;
    DWORD result, threadid;

    semaphores[0] = CreateSemaphoreW(NULL, 0, 2, NULL);
    ok(semaphores[0] != NULL, "failed to create semaphore\n");
    semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");
    info.semaphore1 = semaphores[0];
    info.semaphore2 = NULL;

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "failed to create event\n");

    /* basic test for RtlRegisterWait and RtlDeregisterWait */
    wait1 = NULL;
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ok(wait1 != NULL, "expected wait1 != NULL\n");
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);

    /* infinite timeout, signal the semaphore two times */
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 2, "expected info.userdata = 2, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* repeat test with WT_EXECUTEONLYONCE */
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* finite timeout, no event */
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 200, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* finite timeout, with event */
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 200, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* test RtlRegisterWait WT_EXECUTEINWAITTHREAD flag */
    info.userdata = 0;
    info.threadid = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 200, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ok(info.threadid && info.threadid != GetCurrentThreadId(), "unexpected wait thread id %lx\n", info.threadid);
    threadid = info.threadid;
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    info.userdata = 0;
    info.threadid = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 200, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ok(info.threadid == threadid, "unexpected different wait thread id %lx\n", info.threadid);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* test RtlRegisterWait WT_EXECUTEINWAITTHREAD flag with 0 timeout */
    info.userdata = 0;
    info.threadid = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 0, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    ok(info.threadid == threadid, "unexpected different wait thread id %lx\n", info.threadid);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* test RtlRegisterWait WT_EXECUTEINWAITTHREAD flag with already signaled event */
    info.userdata = 0;
    info.threadid = 0;
    ReleaseSemaphore(semaphores[1], 1, NULL);
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 200, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ok(info.threadid == threadid, "unexpected different wait thread id %lx\n", info.threadid);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* test for IO threads */
    info.userdata = 0;
    info.threadid = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEINIOTHREAD);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ok(info.threadid != 0, "expected info.threadid != 0, got %lu\n", info.threadid);
    thread = OpenThread(THREAD_SET_CONTEXT, FALSE, info.threadid);
    ok(thread != NULL, "OpenThread failed with %lu\n", GetLastError());
    rtl_wait_apc_semaphore = semaphores[0];
    result = QueueUserAPC(rtl_wait_apc_cb, thread, 0);
    ok(result != 0, "QueueUserAPC failed with %lu\n", GetLastError());
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    rtl_wait_apc_semaphore = 0;
    CloseHandle(thread);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 2, "expected info.userdata = 2, got %lu\n", info.userdata);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    info.userdata = 0;
    info.threadid = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    ok(info.threadid != 0, "expected info.threadid != 0, got %lu\n", info.threadid);
    thread = OpenThread(THREAD_SET_CONTEXT, FALSE, info.threadid);
    ok(thread != NULL, "OpenThread failed with %lu\n", GetLastError());
    rtl_wait_apc_semaphore = semaphores[0];
    result = QueueUserAPC(rtl_wait_apc_cb, thread, 0);
    ok(result != 0, "QueueUserAPC failed with %lu\n", GetLastError());
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_TIMEOUT || broken(result == WAIT_OBJECT_0) /* >= Win Vista */,
       "WaitForSingleObject returned %lu\n", result);
    rtl_wait_apc_semaphore = 0;
    CloseHandle(thread);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 2, "expected info.userdata = 2, got %lu\n", info.userdata);
    Sleep(50);
    status = RtlDeregisterWait(wait1);
    ok(!status, "RtlDeregisterWait failed with status %lx\n", status);

    /* test RtlDeregisterWaitEx before wait expired */
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    status = RtlDeregisterWaitEx(wait1, NULL);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    status = RtlDeregisterWaitEx(wait1, INVALID_HANDLE_VALUE);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEDEFAULT);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    status = RtlDeregisterWaitEx(wait1, event);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(event, 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* test RtlDeregisterWaitEx after wait expired */
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 0, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    Sleep(50);
    status = RtlDeregisterWaitEx(wait1, NULL);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 0, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    Sleep(50);
    status = RtlDeregisterWaitEx(wait1, INVALID_HANDLE_VALUE);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, 0, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    Sleep(50);
    status = RtlDeregisterWaitEx(wait1, event);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    result = WaitForSingleObject(event, 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* test RtlDeregisterWaitEx while callback is running */
    info.semaphore2 = semaphores[1];
    info.wait_result = WAIT_OBJECT_0;

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    status = RtlDeregisterWait(wait1);
    ok(status == STATUS_PENDING, "expected STATUS_PENDING, got %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    status = RtlDeregisterWait(wait1);
    ok(status == STATUS_PENDING, "expected STATUS_PENDING, got %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    status = RtlDeregisterWaitEx(wait1, NULL);
    ok(status == STATUS_PENDING, "expected STATUS_PENDING, got %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    info.wait_result = WAIT_TIMEOUT;
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    status = RtlDeregisterWaitEx(wait1, INVALID_HANDLE_VALUE);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 0);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    info.wait_result = WAIT_TIMEOUT;
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    status = RtlDeregisterWaitEx(wait1, INVALID_HANDLE_VALUE);
    ok(!status, "RtlDeregisterWaitEx failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 0);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    info.wait_result = WAIT_OBJECT_0;
    info.userdata = 0;
    status = RtlRegisterWait(&wait1, semaphores[1], rtl_wait_cb, &info, INFINITE, WT_EXECUTEONLYONCE);
    ok(!status, "RtlRegisterWait failed with status %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    status = RtlDeregisterWaitEx(wait1, event);
    ok(status == STATUS_PENDING, "expected STATUS_PENDING, got %lx\n", status);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(event, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphores[0], 0);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
    CloseHandle(event);
}

static void CALLBACK simple_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE semaphore = userdata;
    ReleaseSemaphore(semaphore, 1, NULL);
}

static void CALLBACK simple2_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    Sleep(50);
    InterlockedIncrement((LONG *)userdata);
}

static void test_tp_simple(void)
{
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
    TP_POOL_STACK_INFORMATION stack_info;
    TP_CALLBACK_ENVIRON environment;
    TP_CALLBACK_ENVIRON_V3 environment3;
    TP_CLEANUP_GROUP *group;
    HANDLE semaphore;
    NTSTATUS status;
    TP_POOL *pool;
    LONG userdata;
    DWORD result;
    int i;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* post the callback using the default threadpool */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = NULL;
    status = pTpSimpleTryPost(simple_cb, semaphore, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* post the callback using the new threadpool */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpSimpleTryPost(simple_cb, semaphore, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* test with environment version 3 */
    memset(&environment3, 0, sizeof(environment3));
    environment3.Version = 3;
    environment3.Pool = pool;
    environment3.Size = sizeof(environment3);

    for (i = 0; i < 3; ++i)
    {
        environment3.CallbackPriority = TP_CALLBACK_PRIORITY_HIGH + i;
        status = pTpSimpleTryPost(simple_cb, semaphore, (TP_CALLBACK_ENVIRON *)&environment3);
        ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
        result = WaitForSingleObject(semaphore, 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    }

    environment3.CallbackPriority = 10;
    status = pTpSimpleTryPost(simple_cb, semaphore, (TP_CALLBACK_ENVIRON *)&environment3);
    ok(status == STATUS_INVALID_PARAMETER || broken(!status) /* Vista does not support priorities */,
            "TpSimpleTryPost failed with status %lx\n", status);

    /* test with invalid version number */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 9999;
    environment.Pool = pool;
    status = pTpSimpleTryPost(simple_cb, semaphore, &environment);
    todo_wine
    ok(status == STATUS_INVALID_PARAMETER || broken(!status) /* Vista/2008 */,
       "TpSimpleTryPost unexpectedly returned status %lx\n", status);
    if (!status)
    {
        result = WaitForSingleObject(semaphore, 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    }

    /* allocate a cleanup group for synchronization */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %lx\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* use cleanup group to wait for a simple callback */
    userdata = 0;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpSimpleTryPost(simple2_cb, &userdata, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);
    ok(userdata == 1, "expected userdata = 1, got %lu\n", userdata);

    /* test cancellation of pending simple callbacks */
    userdata = 0;
    pTpSetPoolMaxThreads(pool, 10);
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    for (i = 0; i < 100; i++)
    {
        status = pTpSimpleTryPost(simple2_cb, &userdata, &environment);
        ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    }
    pTpReleaseCleanupGroupMembers(group, TRUE, NULL);
    ok(userdata < 100, "expected userdata < 100, got %lu\n", userdata);

    /* test querying and setting the stack size */
    status = pTpQueryPoolStackInformation(pool, &stack_info);
    ok(!status, "TpQueryPoolStackInformation failed: %lx\n", status);
    ok(stack_info.StackReserve == nt->OptionalHeader.SizeOfStackReserve, "expected default StackReserve, got %Ix\n", stack_info.StackReserve);
    ok(stack_info.StackCommit == nt->OptionalHeader.SizeOfStackCommit, "expected default StackCommit, got %Ix\n", stack_info.StackCommit);

    /* threadpool does not validate the stack size values */
    stack_info.StackReserve = stack_info.StackCommit = 1;
    status = pTpSetPoolStackInformation(pool, &stack_info);
    ok(!status, "TpSetPoolStackInformation failed: %lx\n", status);

    status = pTpQueryPoolStackInformation(pool, &stack_info);
    ok(!status, "TpQueryPoolStackInformation failed: %lx\n", status);
    ok(stack_info.StackReserve == 1, "expected 1 byte StackReserve, got %ld\n", (ULONG)stack_info.StackReserve);
    ok(stack_info.StackCommit == 1, "expected 1 byte StackCommit, got %ld\n", (ULONG)stack_info.StackCommit);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

static void CALLBACK work_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    Sleep(100);
    InterlockedIncrement((LONG *)userdata);
}

static void CALLBACK work2_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    Sleep(100);
    InterlockedExchangeAdd((LONG *)userdata, 0x10000);
}

static void test_tp_work(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_WORK *work;
    TP_POOL *pool;
    NTSTATUS status;
    LONG userdata;
    int i;

    /* allocate new threadpool with only one thread */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");
    pTpSetPoolMaxThreads(pool, 1);

    /* allocate new work item */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocWork(&work, work_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    /* post 5 identical work items at once */
    userdata = 0;
    for (i = 0; i < 5; i++)
        pTpPostWork(work);
    pTpWaitForWork(work, FALSE);
    ok(userdata == 5, "expected userdata = 5, got %lu\n", userdata);

    /* add more tasks and cancel them immediately */
    userdata = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);
    pTpWaitForWork(work, TRUE);
    ok(userdata < 10, "expected userdata < 10, got %lu\n", userdata);

    /* cleanup */
    pTpReleaseWork(work);
    pTpReleasePool(pool);
}

static void test_tp_work_scheduler(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    TP_WORK *work, *work2;
    TP_POOL *pool;
    NTSTATUS status;
    LONG userdata;
    int i;

    /* allocate new threadpool with only one thread */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");
    pTpSetPoolMaxThreads(pool, 1);

    /* create a cleanup group */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %lx\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* the first work item has no cleanup group associated */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocWork(&work, work_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    /* allocate a second work item with a cleanup group */
    work2 = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work2, work2_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work2 != NULL, "expected work2 != NULL\n");

    /* the 'work' callbacks are not blocking execution of 'work2' callbacks */
    userdata = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);
    for (i = 0; i < 10; i++)
        pTpPostWork(work2);
    Sleep(500);
    pTpWaitForWork(work, TRUE);
    pTpWaitForWork(work2, TRUE);
    ok(userdata & 0xffff, "expected userdata & 0xffff != 0, got %lu\n", userdata & 0xffff);
    ok(userdata >> 16, "expected userdata >> 16 != 0, got %lu\n", userdata >> 16);

    /* test TpReleaseCleanupGroupMembers on a work item */
    userdata = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);
    for (i = 0; i < 3; i++)
        pTpPostWork(work2);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);
    pTpWaitForWork(work, TRUE);
    ok((userdata & 0xffff) < 10, "expected userdata & 0xffff < 10, got %lu\n", userdata & 0xffff);
    ok((userdata >> 16) == 3, "expected userdata >> 16 == 3, got %lu\n", userdata >> 16);

    /* cleanup */
    pTpReleaseWork(work);
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
}

static void CALLBACK simple_release_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    ReleaseSemaphore(semaphores, 1, NULL);
    Sleep(200); /* wait until main thread is in TpReleaseCleanupGroupMembers */
}

static void CALLBACK work_release_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    HANDLE semaphore = userdata;
    ReleaseSemaphore(semaphore, 1, NULL);
    Sleep(200); /* wait until main thread is in TpReleaseCleanupGroupMembers */
    pTpReleaseWork(work);
}

static void CALLBACK timer_release_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_TIMER *timer)
{
    HANDLE semaphore = userdata;
    ReleaseSemaphore(semaphore, 1, NULL);
    Sleep(200); /* wait until main thread is in TpReleaseCleanupGroupMembers */
    pTpReleaseTimer(timer);
}

static void CALLBACK wait_release_cb(TP_CALLBACK_INSTANCE *instance, void *userdata,
                                     TP_WAIT *wait, TP_WAIT_RESULT result)
{
    HANDLE semaphore = userdata;
    ReleaseSemaphore(semaphore, 1, NULL);
    Sleep(200); /* wait until main thread is in TpReleaseCleanupGroupMembers */
    pTpReleaseWait(wait);
}

static void test_tp_group_wait(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    LARGE_INTEGER when;
    HANDLE semaphore;
    NTSTATUS status;
    TP_TIMER *timer;
    TP_WAIT *wait;
    TP_WORK *work;
    TP_POOL *pool;
    DWORD result;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* allocate a cleanup group */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %lx\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* release work object during TpReleaseCleanupGroupMembers */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work, work_release_cb, semaphore, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");
    pTpPostWork(work);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    /* release timer object during TpReleaseCleanupGroupMembers */
    timer = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocTimer(&timer, timer_release_cb, semaphore, &environment);
    ok(!status, "TpAllocTimer failed with status %lx\n", status);
    ok(timer != NULL, "expected timer != NULL\n");
    when.QuadPart = 0;
    pTpSetTimer(timer, &when, 0, 0);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    /* release wait object during TpReleaseCleanupGroupMembers */
    wait = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWait(&wait, wait_release_cb, semaphore, &environment);
    ok(!status, "TpAllocWait failed with status %lx\n", status);
    ok(wait != NULL, "expected wait != NULL\n");
    when.QuadPart = 0;
    pTpSetWait(wait, INVALID_HANDLE_VALUE, &when);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

static DWORD group_cancel_tid;

static void CALLBACK simple_group_cancel_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    NTSTATUS status;
    DWORD result;
    int i;

    status = pTpCallbackMayRunLong(instance);
    ok(status == STATUS_TOO_MANY_THREADS || broken(status == 1) /* Win Vista / 2008 */,
       "expected STATUS_TOO_MANY_THREADS, got %08lx\n", status);

    ReleaseSemaphore(semaphores[1], 1, NULL);
    for (i = 0; i < 4; i++)
    {
        result = WaitForSingleObject(semaphores[0], 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    }
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void CALLBACK work_group_cancel_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
}

static void CALLBACK group_cancel_cleanup_release_cb(void *object, void *userdata)
{
    HANDLE *semaphores = userdata;
    group_cancel_tid = GetCurrentThreadId();
    ok(object == (void *)0xdeadbeef, "expected 0xdeadbeef, got %p\n", object);
    ReleaseSemaphore(semaphores[0], 1, NULL);
}

static void CALLBACK group_cancel_cleanup_release2_cb(void *object, void *userdata)
{
    HANDLE *semaphores = userdata;
    group_cancel_tid = GetCurrentThreadId();
    ok(object == userdata, "expected %p, got %p\n", userdata, object);
    ReleaseSemaphore(semaphores[0], 1, NULL);
}

static void CALLBACK group_cancel_cleanup_increment_cb(void *object, void *userdata)
{
    group_cancel_tid = GetCurrentThreadId();
    InterlockedIncrement((LONG *)userdata);
}

static void CALLBACK unexpected_simple_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    ok(0, "Unexpected callback\n");
}

static void CALLBACK unexpected_work_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    ok(0, "Unexpected callback\n");
}

static void CALLBACK unexpected_timer_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_TIMER *timer)
{
    ok(0, "Unexpected callback\n");
}

static void CALLBACK unexpected_wait_cb(TP_CALLBACK_INSTANCE *instance, void *userdata,
                                        TP_WAIT *wait, TP_WAIT_RESULT result)
{
    ok(0, "Unexpected callback\n");
}

static void CALLBACK unexpected_group_cancel_cleanup_cb(void *object, void *userdata)
{
    ok(0, "Unexpected callback\n");
}

static void test_tp_group_cancel(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    LONG userdata, userdata2;
    HANDLE semaphores[2];
    NTSTATUS status;
    TP_TIMER *timer;
    TP_WAIT *wait;
    TP_WORK *work;
    TP_POOL *pool;
    DWORD result;
    int i;

    semaphores[0] = CreateSemaphoreA(NULL, 0, 4, NULL);
    ok(semaphores[0] != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());
    semaphores[1] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* allocate new threadpool with only one thread */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");
    pTpSetPoolMaxThreads(pool, 1);

    /* allocate a cleanup group */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %lx\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* test execution of cancellation callback */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpSimpleTryPost(simple_group_cancel_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = group_cancel_cleanup_release_cb;
    status = pTpSimpleTryPost(unexpected_simple_cb, (void *)0xdeadbeef, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);

    work = NULL;
    status = pTpAllocWork(&work, unexpected_work_cb, (void *)0xdeadbeef, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    timer = NULL;
    status = pTpAllocTimer(&timer, unexpected_timer_cb, (void *)0xdeadbeef, &environment);
    ok(!status, "TpAllocTimer failed with status %lx\n", status);
    ok(timer != NULL, "expected timer != NULL\n");

    wait = NULL;
    status = pTpAllocWait(&wait, unexpected_wait_cb, (void *)0xdeadbeef, &environment);
    ok(!status, "TpAllocWait failed with status %lx\n", status);
    ok(wait != NULL, "expected wait != NULL\n");

    group_cancel_tid = 0xdeadbeef;
    pTpReleaseCleanupGroupMembers(group, TRUE, semaphores);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(group_cancel_tid == GetCurrentThreadId(), "expected tid %lx, got %lx\n",
       GetCurrentThreadId(), group_cancel_tid);

    /* test if cancellation callbacks are executed before or after wait */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = group_cancel_cleanup_release2_cb;
    status = pTpAllocWork(&work, work_group_cancel_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");
    pTpPostWork(work);
    pTpPostWork(work);

    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    group_cancel_tid = 0xdeadbeef;
    pTpReleaseCleanupGroupMembers(group, TRUE, semaphores);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(group_cancel_tid == GetCurrentThreadId(), "expected tid %lx, got %lx\n",
       GetCurrentThreadId(), group_cancel_tid);

    /* group cancel callback is not executed if object is destroyed while waiting */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = unexpected_group_cancel_cleanup_cb;
    status = pTpAllocWork(&work, work_release_cb, semaphores[1], &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");
    pTpPostWork(work);

    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseCleanupGroupMembers(group, TRUE, NULL);

    /* terminated simple callbacks should not trigger the group cancel callback */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = unexpected_group_cancel_cleanup_cb;
    status = pTpSimpleTryPost(simple_release_cb, semaphores[1], &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseCleanupGroupMembers(group, TRUE, semaphores);

    /* test cancellation callback for objects with multiple instances */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = group_cancel_cleanup_increment_cb;
    status = pTpAllocWork(&work, work_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    /* post 10 identical work items at once */
    userdata = userdata2 = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);

    /* check if we get multiple cancellation callbacks */
    group_cancel_tid = 0xdeadbeef;
    pTpReleaseCleanupGroupMembers(group, TRUE, &userdata2);
    ok(userdata <= 5, "expected userdata <= 5, got %lu\n", userdata);
    ok(userdata2 == 1, "expected only one cancellation callback, got %lu\n", userdata2);
    ok(group_cancel_tid == GetCurrentThreadId(), "expected tid %lx, got %lx\n",
       GetCurrentThreadId(), group_cancel_tid);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static void CALLBACK instance_semaphore_completion_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    pTpCallbackReleaseSemaphoreOnCompletion(instance, semaphores[0], 1);
}

static void CALLBACK instance_finalization_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void test_tp_instance(void)
{
    TP_CALLBACK_ENVIRON environment;
    HANDLE semaphores[2];
    NTSTATUS status;
    TP_POOL *pool;
    DWORD result;

    semaphores[0] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[0] != NULL, "failed to create semaphore\n");
    semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* test for TpCallbackReleaseSemaphoreOnCompletion */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpSimpleTryPost(instance_semaphore_completion_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* test for finalization callback */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.FinalizationCallback = instance_finalization_cb;
    status = pTpSimpleTryPost(instance_semaphore_completion_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* cleanup */
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static void CALLBACK disassociate_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    pTpDisassociateCallback(instance);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void CALLBACK disassociate2_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    pTpDisassociateCallback(instance);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void CALLBACK disassociate3_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    pTpDisassociateCallback(instance);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void test_tp_disassociate(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    HANDLE semaphores[2];
    NTSTATUS status;
    TP_POOL *pool;
    TP_WORK *work;
    DWORD result;

    semaphores[0] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[0] != NULL, "failed to create semaphore\n");
    semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");

    /* allocate new threadpool and cleanup group */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %lx\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* test TpDisassociateCallback on work objects without group */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocWork(&work, disassociate_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    pTpPostWork(work);
    pTpWaitForWork(work, FALSE);

    result = WaitForSingleObject(semaphores[1], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseWork(work);

    /* test TpDisassociateCallback on work objects with group (1) */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work, disassociate_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    pTpPostWork(work);
    pTpWaitForWork(work, FALSE);

    result = WaitForSingleObject(semaphores[1], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    /* test TpDisassociateCallback on work objects with group (2) */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work, disassociate2_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %lx\n", status);
    ok(work != NULL, "expected work != NULL\n");

    pTpPostWork(work);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* test TpDisassociateCallback on simple callbacks */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpSimpleTryPost(disassociate3_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %lx\n", status);

    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static void CALLBACK timer_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_TIMER *timer)
{
    HANDLE semaphore = userdata;
    ReleaseSemaphore(semaphore, 1, NULL);
}

static void test_tp_timer(void)
{
    TP_CALLBACK_ENVIRON environment;
    DWORD result, ticks;
    LARGE_INTEGER when;
    HANDLE semaphore;
    NTSTATUS status;
    TP_TIMER *timer;
    TP_POOL *pool;
    BOOL success;
    int i;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* allocate new timer */
    timer = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocTimer(&timer, timer_cb, semaphore, &environment);
    ok(!status, "TpAllocTimer failed with status %lx\n", status);
    ok(timer != NULL, "expected timer != NULL\n");

    success = pTpIsTimerSet(timer);
    ok(!success, "TpIsTimerSet returned TRUE\n");

    /* test timer with a relative timeout */
    when.QuadPart = (ULONGLONG)200 * -10000;
    pTpSetTimer(timer, &when, 0, 0);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    pTpWaitForTimer(timer, FALSE);

    result = WaitForSingleObject(semaphore, 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphore, 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    /* test timer with an absolute timeout */
    NtQuerySystemTime( &when );
    when.QuadPart += (ULONGLONG)200 * 10000;
    pTpSetTimer(timer, &when, 0, 0);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    pTpWaitForTimer(timer, FALSE);

    result = WaitForSingleObject(semaphore, 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphore, 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    /* test timer with zero timeout */
    when.QuadPart = 0;
    pTpSetTimer(timer, &when, 0, 0);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    pTpWaitForTimer(timer, FALSE);

    result = WaitForSingleObject(semaphore, 50);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    /* unset the timer */
    pTpSetTimer(timer, NULL, 0, 0);
    success = pTpIsTimerSet(timer);
    ok(!success, "TpIsTimerSet returned TRUE\n");
    pTpWaitForTimer(timer, TRUE);

    pTpReleaseTimer(timer);
    CloseHandle(semaphore);

    semaphore = CreateSemaphoreA(NULL, 0, 3, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* allocate a new timer */
    timer = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocTimer(&timer, timer_cb, semaphore, &environment);
    ok(!status, "TpAllocTimer failed with status %lx\n", status);
    ok(timer != NULL, "expected timer != NULL\n");

    /* test a relative timeout repeated periodically */
    when.QuadPart = (ULONGLONG)200 * -10000;
    pTpSetTimer(timer, &when, 200, 0);
    success = pTpIsTimerSet(timer);
    ok(success, "TpIsTimerSet returned FALSE\n");

    /* wait until the timer was triggered three times */
    ticks = GetTickCount();
    for (i = 0; i < 3; i++)
    {
        result = WaitForSingleObject(semaphore, 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    }
    ticks = GetTickCount() - ticks;
    ok(ticks >= 500 && (ticks <= 700 || broken(ticks <= 750)) /* Win 7 */,
       "expected approximately 600 ticks, got %lu\n", ticks);

    /* unset the timer */
    pTpSetTimer(timer, NULL, 0, 0);
    success = pTpIsTimerSet(timer);
    ok(!success, "TpIsTimerSet returned TRUE\n");
    pTpWaitForTimer(timer, TRUE);

    /* cleanup */
    pTpReleaseTimer(timer);
    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

struct window_length_info
{
    HANDLE semaphore;
    DWORD ticks;
};

static void CALLBACK window_length_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_TIMER *timer)
{
    struct window_length_info *info = userdata;
    info->ticks = GetTickCount();
    ReleaseSemaphore(info->semaphore, 1, NULL);
}

static void test_tp_window_length(void)
{
    struct window_length_info info1, info2;
    TP_CALLBACK_ENVIRON environment;
    TP_TIMER *timer1, *timer2;
    LARGE_INTEGER when;
    HANDLE semaphore;
    NTSTATUS status;
    TP_POOL *pool;
    DWORD result;
    BOOL merged;

    semaphore = CreateSemaphoreA(NULL, 0, 2, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* allocate two identical timers */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;

    timer1 = NULL;
    info1.semaphore = semaphore;
    status = pTpAllocTimer(&timer1, window_length_cb, &info1, &environment);
    ok(!status, "TpAllocTimer failed with status %lx\n", status);
    ok(timer1 != NULL, "expected timer1 != NULL\n");

    timer2 = NULL;
    info2.semaphore = semaphore;
    status = pTpAllocTimer(&timer2, window_length_cb, &info2, &environment);
    ok(!status, "TpAllocTimer failed with status %lx\n", status);
    ok(timer2 != NULL, "expected timer2 != NULL\n");

    /* choose parameters so that timers are not merged */
    info1.ticks = 0;
    info2.ticks = 0;

    NtQuerySystemTime( &when );
    when.QuadPart += (ULONGLONG)250 * 10000;
    pTpSetTimer(timer2, &when, 0, 0);
    Sleep(50);
    when.QuadPart -= (ULONGLONG)150 * 10000;
    pTpSetTimer(timer1, &when, 0, 75);

    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info1.ticks != 0 && info2.ticks != 0, "expected that ticks are nonzero\n");
    ok(info2.ticks >= info1.ticks + 75 || broken(info2.ticks < info1.ticks + 75) /* Win 2008 */,
       "expected that timers are not merged\n");

    /* timers will be merged */
    info1.ticks = 0;
    info2.ticks = 0;

    NtQuerySystemTime( &when );
    when.QuadPart += (ULONGLONG)250 * 10000;
    pTpSetTimer(timer2, &when, 0, 0);
    Sleep(50);
    when.QuadPart -= (ULONGLONG)150 * 10000;
    pTpSetTimer(timer1, &when, 0, 200);

    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info1.ticks != 0 && info2.ticks != 0, "expected that ticks are nonzero\n");
    merged = info2.ticks >= info1.ticks - 50 && info2.ticks <= info1.ticks + 50;
    ok(merged || broken(!merged) /* Win 10 */, "expected that timers are merged\n");

    /* on Windows the timers also get merged in this case */
    info1.ticks = 0;
    info2.ticks = 0;

    NtQuerySystemTime( &when );
    when.QuadPart += (ULONGLONG)100 * 10000;
    pTpSetTimer(timer1, &when, 0, 200);
    Sleep(50);
    when.QuadPart += (ULONGLONG)150 * 10000;
    pTpSetTimer(timer2, &when, 0, 0);

    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info1.ticks != 0 && info2.ticks != 0, "expected that ticks are nonzero\n");
    merged = info2.ticks >= info1.ticks - 50 && info2.ticks <= info1.ticks + 50;
    todo_wine
    ok(merged || broken(!merged) /* Win 10 */, "expected that timers are merged\n");

    /* cleanup */
    pTpReleaseTimer(timer1);
    pTpReleaseTimer(timer2);
    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

struct wait_info
{
    HANDLE semaphore;
    LONG userdata;
};

static void CALLBACK wait_cb(TP_CALLBACK_INSTANCE *instance, void *userdata,
                             TP_WAIT *wait, TP_WAIT_RESULT result)
{
    struct wait_info *info = userdata;
    if (result == WAIT_OBJECT_0)
        InterlockedIncrement(&info->userdata);
    else if (result == WAIT_TIMEOUT)
        InterlockedExchangeAdd(&info->userdata, 0x10000);
    else
        ok(0, "unexpected result %lu\n", result);
    ReleaseSemaphore(info->semaphore, 1, NULL);
}

static void test_tp_wait(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_WAIT *wait1, *wait2;
    struct wait_info info;
    HANDLE semaphores[2];
    LARGE_INTEGER when;
    NTSTATUS status;
    TP_POOL *pool;
    DWORD result;

    semaphores[0] = CreateSemaphoreW(NULL, 0, 2, NULL);
    ok(semaphores[0] != NULL, "failed to create semaphore\n");
    semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");
    info.semaphore = semaphores[0];

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* allocate new wait items */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;

    wait1 = NULL;
    status = pTpAllocWait(&wait1, wait_cb, &info, &environment);
    ok(!status, "TpAllocWait failed with status %lx\n", status);
    ok(wait1 != NULL, "expected wait1 != NULL\n");

    wait2 = NULL;
    status = pTpAllocWait(&wait2, wait_cb, &info, &environment);
    ok(!status, "TpAllocWait failed with status %lx\n", status);
    ok(wait2 != NULL, "expected wait2 != NULL\n");

    /* infinite timeout, signal the semaphore immediately */
    info.userdata = 0;
    pTpSetWait(wait1, semaphores[1], NULL);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* relative timeout, no event */
    info.userdata = 0;
    when.QuadPart = (ULONGLONG)200 * -10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* repeat test with call to TpWaitForWait(..., TRUE) */
    info.userdata = 0;
    when.QuadPart = (ULONGLONG)200 * -10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    pTpWaitForWait(wait1, TRUE);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0 || broken(result == WAIT_TIMEOUT) /* Win 8 */,
       "WaitForSingleObject returned %lu\n", result);
    if (result == WAIT_OBJECT_0)
        ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    else
        ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* relative timeout, with event */
    info.userdata = 0;
    when.QuadPart = (ULONGLONG)200 * -10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* repeat test with call to TpWaitForWait(..., TRUE) */
    info.userdata = 0;
    when.QuadPart = (ULONGLONG)200 * -10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    pTpWaitForWait(wait1, TRUE);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0 || broken(result == WAIT_TIMEOUT) /* Win 8 */,
       "WaitForSingleObject returned %lu\n", result);
    if (result == WAIT_OBJECT_0)
    {
        ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
        result = WaitForSingleObject(semaphores[1], 0);
        ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    }
    else
    {
        ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
        result = WaitForSingleObject(semaphores[1], 0);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    }

    /* absolute timeout, no event */
    info.userdata = 0;
    NtQuerySystemTime( &when );
    when.QuadPart += (ULONGLONG)200 * 10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[0], 200);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* absolute timeout, with event */
    info.userdata = 0;
    NtQuerySystemTime( &when );
    when.QuadPart += (ULONGLONG)200 * 10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* test timeout of zero */
    info.userdata = 0;
    when.QuadPart = 0;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* cancel a pending wait */
    info.userdata = 0;
    when.QuadPart = (ULONGLONG)250 * -10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    pTpSetWait(wait1, NULL, (void *)0xdeadbeef);
    Sleep(50);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0, "expected info.userdata = 0, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    /* test with INVALID_HANDLE_VALUE */
    info.userdata = 0;
    when.QuadPart = 0;
    pTpSetWait(wait1, INVALID_HANDLE_VALUE, &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);

    /* cancel a pending wait with INVALID_HANDLE_VALUE */
    info.userdata = 0;
    when.QuadPart = (ULONGLONG)250 * -10000;
    pTpSetWait(wait1, semaphores[1], &when);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    when.QuadPart = 0;
    pTpSetWait(wait1, INVALID_HANDLE_VALUE, &when);
    Sleep(50);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 0x10000, "expected info.userdata = 0x10000, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);

    CloseHandle(semaphores[1]);
    semaphores[1] = CreateSemaphoreW(NULL, 0, 2, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");

    /* add two wait objects with the same semaphore */
    info.userdata = 0;
    pTpSetWait(wait1, semaphores[1], NULL);
    pTpSetWait(wait2, semaphores[1], NULL);
    Sleep(50);
    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 1, "expected info.userdata = 1, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* repeat test above with release count 2 */
    info.userdata = 0;
    pTpSetWait(wait1, semaphores[1], NULL);
    pTpSetWait(wait2, semaphores[1], NULL);
    Sleep(50);
    result = ReleaseSemaphore(semaphores[1], 2, NULL);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    ok(info.userdata == 2, "expected info.userdata = 2, got %lu\n", info.userdata);
    result = WaitForSingleObject(semaphores[1], 0);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", result);

    /* cleanup */
    pTpReleaseWait(wait1);
    pTpReleaseWait(wait2);
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static struct
{
    HANDLE semaphore;
    DWORD result;
} multi_wait_info;

static void CALLBACK multi_wait_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WAIT *wait, TP_WAIT_RESULT result)
{
    DWORD index = (DWORD)(DWORD_PTR)userdata;

    if (result == WAIT_OBJECT_0)
        multi_wait_info.result = index;
    else if (result == WAIT_TIMEOUT)
        multi_wait_info.result = 0x10000 | index;
    else
        ok(0, "unexpected result %lu\n", result);
    ReleaseSemaphore(multi_wait_info.semaphore, 1, NULL);
}

static void test_tp_multi_wait(void)
{
    TP_POOL_STACK_INFORMATION stack_info;
    TP_CALLBACK_ENVIRON environment;
    HANDLE semaphores[512];
    TP_WAIT *waits[512];
    LARGE_INTEGER when;
    HANDLE semaphore;
    NTSTATUS status;
    TP_POOL *pool;
    DWORD result;
    int i;

    semaphore = CreateSemaphoreW(NULL, 0, 512, NULL);
    ok(semaphore != NULL, "failed to create semaphore\n");
    multi_wait_info.semaphore = semaphore;

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %lx\n", status);
    ok(pool != NULL, "expected pool != NULL\n");
    /* many threads -> use the smallest stack possible */
    stack_info.StackReserve = 256 * 1024;
    stack_info.StackCommit = 4 * 1024;
    status = pTpSetPoolStackInformation(pool, &stack_info);
    ok(!status, "TpQueryPoolStackInformation failed: %lx\n", status);

    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;

    /* create semaphores and corresponding wait objects */
    for (i = 0; i < ARRAY_SIZE(semaphores); i++)
    {
        semaphores[i] = CreateSemaphoreW(NULL, 0, 1, NULL);
        ok(semaphores[i] != NULL, "failed to create semaphore %i\n", i);

        waits[i] = NULL;
        status = pTpAllocWait(&waits[i], multi_wait_cb, (void *)(DWORD_PTR)i, &environment);
        ok(!status, "TpAllocWait failed with status %lx\n", status);
        ok(waits[i] != NULL, "expected waits[%d] != NULL\n", i);

        pTpSetWait(waits[i], semaphores[i], NULL);
    }

    /* release all semaphores and wait for callback */
    for (i = 0; i < ARRAY_SIZE(semaphores); i++)
    {
        multi_wait_info.result = 0;
        ReleaseSemaphore(semaphores[i], 1, NULL);

        result = WaitForSingleObject(semaphore, 2000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
        ok(multi_wait_info.result == i, "expected result %d, got %lu\n", i, multi_wait_info.result);

        pTpSetWait(waits[i], semaphores[i], NULL);
    }

    /* repeat the same test in reverse order */
    for (i = ARRAY_SIZE(semaphores) - 1; i >= 0; i--)
    {
        multi_wait_info.result = 0;
        ReleaseSemaphore(semaphores[i], 1, NULL);

        result = WaitForSingleObject(semaphore, 2000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
        ok(multi_wait_info.result == i, "expected result %d, got %lu\n", i, multi_wait_info.result);

        pTpSetWait(waits[i], semaphores[i], NULL);
    }

    /* test timeout of wait objects */
    multi_wait_info.result = 0;
    for (i = 0; i < ARRAY_SIZE(semaphores); i++)
    {
        when.QuadPart = (ULONGLONG)50 * -10000;
        pTpSetWait(waits[i], semaphores[i], &when);
    }

    for (i = 0; i < ARRAY_SIZE(semaphores); i++)
    {
        result = WaitForSingleObject(semaphore, 2000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
    }

    ok(multi_wait_info.result >> 16, "expected multi_wait_info.result >> 16 != 0\n");

    /* destroy the wait objects and semaphores while waiting */
    for (i = 0; i < ARRAY_SIZE(semaphores); i++)
    {
        pTpSetWait(waits[i], semaphores[i], NULL);
    }

    Sleep(50);

    for (i = 0; i < ARRAY_SIZE(semaphores); i++)
    {
        pTpReleaseWait(waits[i]);
        NtClose(semaphores[i]);
    }

    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

struct io_cb_ctx
{
    unsigned int count;
    void *ovl;
    NTSTATUS ret;
    ULONG_PTR length;
    TP_IO *io;
};

static void CALLBACK io_cb(TP_CALLBACK_INSTANCE *instance, void *userdata,
        void *cvalue, IO_STATUS_BLOCK *iosb, TP_IO *io)
{
    struct io_cb_ctx *ctx = userdata;
    ++ctx->count;
    ctx->ovl = cvalue;
    ctx->ret = iosb->Status;
    ctx->length = iosb->Information;
    ctx->io = io;
}

static DWORD WINAPI io_wait_thread(void *arg)
{
    TP_IO *io = arg;
    pTpWaitForIoCompletion(io, FALSE);
    return 0;
}

static void test_tp_io(void)
{
    TP_CALLBACK_ENVIRON environment = {.Version = 1};
    OVERLAPPED ovl = {}, ovl2 = {};
    HANDLE client, server, thread;
    struct io_cb_ctx userdata;
    char in[1], in2[1];
    const char out[1];
    NTSTATUS status;
    DWORD ret_size;
    TP_POOL *pool;
    TP_IO *io;
    BOOL ret;

    ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    status = pTpAllocPool(&pool, NULL);
    ok(!status, "failed to allocate pool, status %#lx\n", status);

    server = CreateNamedPipeA("\\\\.\\pipe\\wine_tp_test",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1, 1024, 1024, 0, NULL);
    ok(server != INVALID_HANDLE_VALUE, "Failed to create server pipe, error %lu.\n", GetLastError());
    client = CreateFileA("\\\\.\\pipe\\wine_tp_test", GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, 0);
    ok(client != INVALID_HANDLE_VALUE, "Failed to create client pipe, error %lu.\n", GetLastError());

    environment.Pool = pool;
    io = NULL;
    status = pTpAllocIoCompletion(&io, server, io_cb, &userdata, &environment);
    ok(!status, "got %#lx\n", status);
    ok(!!io, "expected non-NULL TP_IO\n");

    pTpWaitForIoCompletion(io, FALSE);

    userdata.count = 0;
    pTpStartAsyncIoOperation(io);

    thread = CreateThread(NULL, 0, io_wait_thread, io, 0, NULL);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "TpWaitForIoCompletion() should not return\n");

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());

    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());

    pTpWaitForIoCompletion(io, FALSE);
    ok(userdata.count == 1, "callback ran %u times\n", userdata.count);
    ok(userdata.ovl == &ovl, "expected %p, got %p\n", &ovl, userdata.ovl);
    ok(userdata.ret == STATUS_SUCCESS, "got status %#lx\n", userdata.ret);
    ok(userdata.length == 1, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    ok(!WaitForSingleObject(thread, 1000), "wait timed out\n");
    CloseHandle(thread);

    userdata.count = 0;
    pTpStartAsyncIoOperation(io);
    pTpStartAsyncIoOperation(io);

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());
    ret = ReadFile(server, in2, sizeof(in2), NULL, &ovl2);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());

    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());
    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());

    pTpWaitForIoCompletion(io, FALSE);
    ok(userdata.count == 2, "callback ran %u times\n", userdata.count);
    ok(userdata.ret == STATUS_SUCCESS, "got status %#lx\n", userdata.ret);
    ok(userdata.length == 1, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    /* The documentation is a bit unclear about passing TRUE to
     * WaitForThreadpoolIoCallbacks()—"pending I/O requests are not canceled"
     * [as with CancelIoEx()], but pending threadpool callbacks are, even those
     * which have not yet reached the completion port [as with
     * TpCancelAsyncIoOperation()]. */
    userdata.count = 0;
    pTpStartAsyncIoOperation(io);

    pTpWaitForIoCompletion(io, TRUE);
    ok(!userdata.count, "callback ran %u times\n", userdata.count);

    pTpStartAsyncIoOperation(io);

    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(ret, "wrong ret %d\n", ret);

    pTpWaitForIoCompletion(io, FALSE);
    ok(userdata.count == 1, "callback ran %u times\n", userdata.count);
    ok(userdata.ovl == &ovl, "expected %p, got %p\n", &ovl, userdata.ovl);
    ok(userdata.ret == STATUS_SUCCESS, "got status %#lx\n", userdata.ret);
    ok(userdata.length == 1, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    userdata.count = 0;
    pTpStartAsyncIoOperation(io);

    ret = ReadFile(server, NULL, 1, NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError());

    pTpCancelAsyncIoOperation(io);
    pTpWaitForIoCompletion(io, FALSE);
    ok(!userdata.count, "callback ran %u times\n", userdata.count);

    userdata.count = 0;
    pTpStartAsyncIoOperation(io);

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());
    ret = CancelIo(server);
    ok(ret, "CancelIo() failed, error %lu\n", GetLastError());

    pTpWaitForIoCompletion(io, FALSE);
    ok(userdata.count == 1, "callback ran %u times\n", userdata.count);
    ok(userdata.ovl == &ovl, "expected %p, got %p\n", &ovl, userdata.ovl);
    ok(userdata.ret == STATUS_CANCELLED, "got status %#lx\n", userdata.ret);
    ok(!userdata.length, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    userdata.count = 0;
    pTpStartAsyncIoOperation(io);
    pTpCancelAsyncIoOperation(io);
    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());

    pTpWaitForIoCompletion(io, FALSE);
    if (0)
    {
        /* Add a sleep to check that callback is not called later. Commented out to
         * save the test time. */
        Sleep(200);
    }
    ok(userdata.count == 0, "callback ran %u times\n", userdata.count);

    pTpReleaseIoCompletion(io);
    CloseHandle(server);

    /* Test TPIO object destruction. */
    server = CreateNamedPipeA("\\\\.\\pipe\\wine_tp_test",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1, 1024, 1024, 0, NULL);
    ok(server != INVALID_HANDLE_VALUE, "Failed to create server pipe, error %lu.\n", GetLastError());
    io = NULL;
    status = pTpAllocIoCompletion(&io, server, io_cb, &userdata, &environment);
    ok(!status, "got %#lx\n", status);

    ret = HeapValidate(GetProcessHeap(), 0, io);
    ok(ret, "Got unexpected ret %#x.\n", ret);
    pTpReleaseIoCompletion(io);
    ret = HeapValidate(GetProcessHeap(), 0, io);
    ok(!ret, "Got unexpected ret %#x.\n", ret);
    CloseHandle(server);
    CloseHandle(client);

    server = CreateNamedPipeA("\\\\.\\pipe\\wine_tp_test",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1, 1024, 1024, 0, NULL);
    ok(server != INVALID_HANDLE_VALUE, "Failed to create server pipe, error %lu.\n", GetLastError());
    client = CreateFileA("\\\\.\\pipe\\wine_tp_test", GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, 0);
    ok(client != INVALID_HANDLE_VALUE, "Failed to create client pipe, error %lu.\n", GetLastError());

    io = NULL;
    status = pTpAllocIoCompletion(&io, server, io_cb, &userdata, &environment);
    ok(!status, "got %#lx\n", status);
    pTpStartAsyncIoOperation(io);
    pTpWaitForIoCompletion(io, TRUE);
    ret = HeapValidate(GetProcessHeap(), 0, io);
    ok(ret, "Got unexpected ret %#x.\n", ret);
    pTpReleaseIoCompletion(io);
    ret = HeapValidate(GetProcessHeap(), 0, io);
    ok(ret, "Got unexpected ret %#x.\n", ret);

    if (0)
    {
        /* Object destruction will wait until one completion arrives (which was started but not cancelled).
         * Commented out to save test time. */
        Sleep(1000);
        ret = HeapValidate(GetProcessHeap(), 0, io);
        ok(ret, "Got unexpected ret %#x.\n", ret);
        ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
        ok(!ret, "wrong ret %d\n", ret);
        ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
        ok(ret, "WriteFile() failed, error %lu\n", GetLastError());
        Sleep(2000);
        ret = HeapValidate(GetProcessHeap(), 0, io);
        ok(!ret, "Got unexpected ret %#x.\n", ret);
    }

    CloseHandle(server);
    CloseHandle(ovl.hEvent);
    CloseHandle(client);
    pTpReleasePool(pool);
}

static void CALLBACK kernel32_io_cb(TP_CALLBACK_INSTANCE *instance, void *userdata,
        void *ovl, ULONG ret, ULONG_PTR length, TP_IO *io)
{
    struct io_cb_ctx *ctx = userdata;
    ++ctx->count;
    ctx->ovl = ovl;
    ctx->ret = ret;
    ctx->length = length;
    ctx->io = io;
}

static void test_kernel32_tp_io(void)
{
    TP_CALLBACK_ENVIRON environment = {.Version = 1};
    OVERLAPPED ovl = {}, ovl2 = {};
    HANDLE client, server, thread;
    struct io_cb_ctx userdata;
    char in[1], in2[1];
    const char out[1];
    NTSTATUS status;
    DWORD ret_size;
    TP_POOL *pool;
    TP_IO *io;
    BOOL ret;

    ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    status = pTpAllocPool(&pool, NULL);
    ok(!status, "failed to allocate pool, status %#lx\n", status);

    server = CreateNamedPipeA("\\\\.\\pipe\\wine_tp_test",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1, 1024, 1024, 0, NULL);
    ok(server != INVALID_HANDLE_VALUE, "Failed to create server pipe, error %lu.\n", GetLastError());
    client = CreateFileA("\\\\.\\pipe\\wine_tp_test", GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, 0);
    ok(client != INVALID_HANDLE_VALUE, "Failed to create client pipe, error %lu.\n", GetLastError());

    environment.Pool = pool;
    io = NULL;
    io = pCreateThreadpoolIo(server, kernel32_io_cb, &userdata, &environment);
    ok(!!io, "expected non-NULL TP_IO\n");

    pWaitForThreadpoolIoCallbacks(io, FALSE);

    userdata.count = 0;
    pStartThreadpoolIo(io);

    thread = CreateThread(NULL, 0, io_wait_thread, io, 0, NULL);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "TpWaitForIoCompletion() should not return\n");

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());

    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());

    pWaitForThreadpoolIoCallbacks(io, FALSE);
    ok(userdata.count == 1, "callback ran %u times\n", userdata.count);
    ok(userdata.ovl == &ovl, "expected %p, got %p\n", &ovl, userdata.ovl);
    ok(userdata.ret == ERROR_SUCCESS, "got status %#lx\n", userdata.ret);
    ok(userdata.length == 1, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    ok(!WaitForSingleObject(thread, 1000), "wait timed out\n");
    CloseHandle(thread);

    userdata.count = 0;
    pStartThreadpoolIo(io);
    pStartThreadpoolIo(io);

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());
    ret = ReadFile(server, in2, sizeof(in2), NULL, &ovl2);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());

    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());
    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());

    pWaitForThreadpoolIoCallbacks(io, FALSE);
    ok(userdata.count == 2, "callback ran %u times\n", userdata.count);
    ok(userdata.ret == STATUS_SUCCESS, "got status %#lx\n", userdata.ret);
    ok(userdata.length == 1, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    userdata.count = 0;
    pStartThreadpoolIo(io);
    pWaitForThreadpoolIoCallbacks(io, TRUE);
    ok(!userdata.count, "callback ran %u times\n", userdata.count);

    pStartThreadpoolIo(io);

    ret = WriteFile(client, out, sizeof(out), &ret_size, NULL);
    ok(ret, "WriteFile() failed, error %lu\n", GetLastError());

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(ret, "wrong ret %d\n", ret);

    pWaitForThreadpoolIoCallbacks(io, FALSE);
    ok(userdata.count == 1, "callback ran %u times\n", userdata.count);
    ok(userdata.ovl == &ovl, "expected %p, got %p\n", &ovl, userdata.ovl);
    ok(userdata.ret == ERROR_SUCCESS, "got status %#lx\n", userdata.ret);
    ok(userdata.length == 1, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    userdata.count = 0;
    pStartThreadpoolIo(io);

    ret = ReadFile(server, NULL, 1, NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError());

    pCancelThreadpoolIo(io);
    pWaitForThreadpoolIoCallbacks(io, FALSE);
    ok(!userdata.count, "callback ran %u times\n", userdata.count);

    userdata.count = 0;
    pStartThreadpoolIo(io);

    ret = ReadFile(server, in, sizeof(in), NULL, &ovl);
    ok(!ret, "wrong ret %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "wrong error %lu\n", GetLastError());
    ret = CancelIo(server);
    ok(ret, "CancelIo() failed, error %lu\n", GetLastError());

    pWaitForThreadpoolIoCallbacks(io, FALSE);
    ok(userdata.count == 1, "callback ran %u times\n", userdata.count);
    ok(userdata.ovl == &ovl, "expected %p, got %p\n", &ovl, userdata.ovl);
    ok(userdata.ret == ERROR_OPERATION_ABORTED, "got status %#lx\n", userdata.ret);
    ok(!userdata.length, "got length %Iu\n", userdata.length);
    ok(userdata.io == io, "expected %p, got %p\n", io, userdata.io);

    CloseHandle(ovl.hEvent);
    CloseHandle(client);
    CloseHandle(server);
    pCloseThreadpoolIo(io);
    pTpReleasePool(pool);
}

START_TEST(threadpool)
{
    test_RtlQueueWorkItem();
    test_RtlRegisterWait();

    if (!init_threadpool())
        return;

    test_tp_simple();
    test_tp_work();
    test_tp_work_scheduler();
    test_tp_group_wait();
    test_tp_group_cancel();
    test_tp_instance();
    test_tp_disassociate();
    test_tp_timer();
    test_tp_window_length();
    test_tp_wait();
    test_tp_multi_wait();
    test_tp_io();
    test_kernel32_tp_io();
}
