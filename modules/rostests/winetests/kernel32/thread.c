/*
 * Unit test suite for thread functions.
 *
 * Copyright 2002 Geoffrey Hausheer
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

/* the tests intentionally pass invalid pointers and need an exception handler */
#define WINE_NO_INLINE_STRING

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>
#include <winnls.h>
#include <winternl.h>
#include "wine/test.h"

/* THREAD_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define THREAD_ALL_ACCESS_NT4 (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff)

/* Specify the number of simultaneous threads to test */
#define NUM_THREADS 4
/* Specify whether to test the extended priorities for Win2k/XP */
#define USE_EXTENDED_PRIORITIES 0
/* Specify whether to test the stack allocation in CreateThread */
#define CHECK_STACK 0

/* Set CHECK_STACK to 1 if you want to try to test the stack-limit from
   CreateThread.  So far I have been unable to make this work, and
   I am in doubt as to how portable it is.  Also, according to MSDN,
   you shouldn't mix C-run-time-libraries (i.e. alloca) with CreateThread.
   Anyhow, the check is currently commented out
*/
#if CHECK_STACK
# ifdef __try
#  define __TRY __try
#  define __EXCEPT __except
#  define __ENDTRY
# else
#  include "wine/exception.h"
# endif
#endif

#ifdef __i386__
#define ARCH "x86"
#elif defined __x86_64__
#define ARCH "amd64"
#elif defined __arm__
#define ARCH "arm"
#elif defined __aarch64__
#define ARCH "arm64"
#else
#define ARCH "none"
#endif

#ifdef __REACTOS__
#define EXCEPTION_WINE_NAME_THREAD 0x406D1388
#endif

static void (WINAPI *pGetCurrentThreadStackLimits)(PULONG_PTR,PULONG_PTR);
static BOOL (WINAPI *pGetThreadPriorityBoost)(HANDLE,PBOOL);
static HANDLE (WINAPI *pOpenThread)(DWORD,BOOL,DWORD);
static BOOL (WINAPI *pQueueUserWorkItem)(LPTHREAD_START_ROUTINE,PVOID,ULONG);
static BOOL (WINAPI *pSetThreadPriorityBoost)(HANDLE,BOOL);
static BOOL (WINAPI *pSetThreadStackGuarantee)(ULONG*);
static BOOL (WINAPI *pRegisterWaitForSingleObject)(PHANDLE,HANDLE,WAITORTIMERCALLBACK,PVOID,ULONG,ULONG);
static BOOL (WINAPI *pUnregisterWait)(HANDLE);
static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);
static BOOL (WINAPI *pSetThreadErrorMode)(DWORD,PDWORD);
static DWORD (WINAPI *pGetThreadErrorMode)(void);
static DWORD (WINAPI *pRtlGetThreadErrorMode)(void);
static PTP_POOL (WINAPI *pCreateThreadpool)(PVOID);
static void (WINAPI *pCloseThreadpool)(PTP_POOL);
static PTP_WORK (WINAPI *pCreateThreadpoolWork)(PTP_WORK_CALLBACK,PVOID,PTP_CALLBACK_ENVIRON);
static void (WINAPI *pSubmitThreadpoolWork)(PTP_WORK);
static void (WINAPI *pWaitForThreadpoolWorkCallbacks)(PTP_WORK,BOOL);
static void (WINAPI *pCloseThreadpoolWork)(PTP_WORK);
static NTSTATUS (WINAPI *pNtQueryInformationThread)(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);
static BOOL (WINAPI *pGetThreadGroupAffinity)(HANDLE,GROUP_AFFINITY*);
static BOOL (WINAPI *pSetThreadGroupAffinity)(HANDLE,const GROUP_AFFINITY*,GROUP_AFFINITY*);
static NTSTATUS (WINAPI *pNtSetInformationThread)(HANDLE,THREADINFOCLASS,LPCVOID,ULONG);
static HRESULT (WINAPI *pSetThreadDescription)(HANDLE,const WCHAR *);
static HRESULT (WINAPI *pGetThreadDescription)(HANDLE,WCHAR **);
static PVOID (WINAPI *pRtlAddVectoredExceptionHandler)(ULONG,PVECTORED_EXCEPTION_HANDLER);
static ULONG (WINAPI *pRtlRemoveVectoredExceptionHandler)(PVOID);

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    BOOL ret;
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs( &argv );
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "error: %lu\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    return pi.hProcess;
}

/* Functions not tested yet:
  AttachThreadInput
  SetThreadContext
  SwitchToThread

In addition there are no checks that the inheritance works properly in
CreateThread
*/

/* Functions to ensure that from a group of threads, only one executes
   certain chunks of code at a time, and we know which one is executing
   it.  It basically makes multithreaded execution linear, which defeats
   the purpose of multiple threads, but makes testing easy.  */
static HANDLE start_event, stop_event;
static LONG num_synced;

static void init_thread_sync_helpers(void)
{
  start_event = CreateEventW(NULL, TRUE, FALSE, NULL);
  ok(start_event != NULL, "CreateEvent failed\n");
  stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
  ok(stop_event != NULL, "CreateEvent failed\n");
  num_synced = -1;
}

static BOOL sync_threads_and_run_one(DWORD sync_id, DWORD my_id)
{
  LONG num = InterlockedIncrement(&num_synced);
  assert(-1 <= num && num <= 1);
  if (num == 1)
  {
      ResetEvent( stop_event );
      SetEvent( start_event );
  }
  else
  {
    DWORD ret = WaitForSingleObject(start_event, 10000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed %lx\n",ret);
  }
  return sync_id == my_id;
}

static void resync_after_run(void)
{
  LONG num = InterlockedDecrement(&num_synced);
  assert(-1 <= num && num <= 1);
  if (num == -1)
  {
      ResetEvent( start_event );
      SetEvent( stop_event );
  }
  else
  {
    DWORD ret = WaitForSingleObject(stop_event, 10000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
  }
}

static void cleanup_thread_sync_helpers(void)
{
  CloseHandle(start_event);
  CloseHandle(stop_event);
}

static DWORD tlsIndex;

typedef struct {
  int threadnum;
  HANDLE *event;
  DWORD *threadmem;
} t1Struct;

/* WinME supports OpenThread but doesn't know about access restrictions so
   we require them to be either completely ignored or always obeyed.
*/
static INT obeying_ars = 0; /* -1 == no, 0 == dunno yet, 1 == yes */
#define obey_ar(x) \
  (obeying_ars == 0 \
    ? ((x) \
      ? (obeying_ars = +1) \
      : ((obeying_ars = -1), \
         trace("not restricted, assuming consistent behaviour\n"))) \
    : (obeying_ars < 0) \
      ? ok(!(x), "access restrictions obeyed\n") \
      : ok( (x), "access restrictions not obeyed\n"))

/* Basic test that simultaneous threads can access shared memory,
   that the thread local storage routines work correctly, and that
   threads actually run concurrently
*/
static DWORD WINAPI threadFunc1(LPVOID p)
{
   t1Struct *tstruct = p;
   int i;
/* write our thread # into shared memory */
   tstruct->threadmem[tstruct->threadnum]=GetCurrentThreadId();
   ok(TlsSetValue(tlsIndex,(LPVOID)(INT_PTR)(tstruct->threadnum+1))!=0,
      "TlsSetValue failed\n");
/* The threads synchronize before terminating.  This is done by
   Signaling an event, and waiting for all events to occur
*/
   SetEvent(tstruct->event[tstruct->threadnum]);
   WaitForMultipleObjects(NUM_THREADS,tstruct->event,TRUE,INFINITE);
/* Double check that all threads really did run by validating that
   they have all written to the shared memory. There should be no race
   here, since all threads were synchronized after the write.*/
   for (i = 0; i < NUM_THREADS; i++)
      ok(tstruct->threadmem[i] != 0, "expected threadmem[%d] != 0\n", i);

   /* lstrlenA contains an exception handler so this makes sure exceptions work in threads */
   ok( lstrlenA( (char *)0xdeadbeef ) == 0, "lstrlenA: unexpected success\n" );

/* Check that no one changed our tls memory */
   ok((INT_PTR)TlsGetValue(tlsIndex)-1==tstruct->threadnum,
      "TlsGetValue failed\n");
   return NUM_THREADS+tstruct->threadnum;
}

static DWORD WINAPI threadFunc2(LPVOID p)
{
   return 99;
}

static DWORD WINAPI threadFunc3(LPVOID p)
{
   HANDLE thread;
   thread=GetCurrentThread();
   SuspendThread(thread);
   return 99;
}

static DWORD WINAPI threadFunc4(LPVOID p)
{
   HANDLE event = p;
   if(event != NULL) {
     SetEvent(event);
   }
   Sleep(99000);
   return 0;
}

#if CHECK_STACK
static DWORD WINAPI threadFunc5(LPVOID p)
{
  DWORD *exitCode = p;
  SYSTEM_INFO sysInfo;
  sysInfo.dwPageSize=0;
  GetSystemInfo(&sysInfo);
  *exitCode=0;
   __TRY
   {
     alloca(2*sysInfo.dwPageSize);
   }
    __EXCEPT(1) {
     *exitCode=1;
   }
   __ENDTRY
   return 0;
}
#endif

static DWORD WINAPI threadFunc_SetEvent(LPVOID p)
{
    SetEvent(p);
    return 0;
}

static DWORD WINAPI threadFunc_CloseHandle(LPVOID p)
{
    CloseHandle(p);
    return 0;
}

struct thread_actctx_param
{
    HANDLE thread_context;
    HANDLE handle;
};

static DWORD WINAPI thread_actctx_func(void *p)
{
    struct thread_actctx_param *param = (struct thread_actctx_param*)p;
    HANDLE cur;
    BOOL ret;

    cur = (void*)0xdeadbeef;
    ret = GetCurrentActCtx(&cur);
    ok(ret, "thread GetCurrentActCtx failed, %lu\n", GetLastError());
    ok(cur == param->handle, "got %p, expected %p\n", cur, param->handle);
    param->thread_context = cur;

    return 0;
}

static void create_function_addr_events(HANDLE events[2])
{
    char buffer[256];

    sprintf(buffer, "threadFunc_SetEvent %p", threadFunc_SetEvent);
    events[0] = CreateEventA(NULL, FALSE, FALSE, buffer);

    sprintf(buffer, "threadFunc_CloseHandle %p", threadFunc_CloseHandle);
    events[1] = CreateEventA(NULL, FALSE, FALSE, buffer);
}

/* check CreateRemoteThread */
static VOID test_CreateRemoteThread(void)
{
    HANDLE hProcess, hThread, hEvent, hRemoteEvent;
    DWORD tid, ret, exitcode;
    HANDLE hAddrEvents[2];

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    /* ensure threadFunc_SetEvent & threadFunc_CloseHandle are the same
     * address as in the child process */
    create_function_addr_events(hAddrEvents);
    ret = WaitForMultipleObjects(2, hAddrEvents, TRUE, 5000);
    if (ret == WAIT_TIMEOUT)
    {
        skip("child process wasn't mapped at same address, so can't do CreateRemoteThread tests.\n");
        return;
    }
    ok(ret == WAIT_OBJECT_0 || broken(ret == WAIT_OBJECT_0+1 /* nt4,w2k */), "WaitForAllObjects 2 events %ld\n", ret);

    hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(hEvent != NULL, "Can't create event, err=%lu\n", GetLastError());
    ret = DuplicateHandle(GetCurrentProcess(), hEvent, hProcess, &hRemoteEvent,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret != 0, "DuplicateHandle failed, err=%lu\n", GetLastError());

    /* create suspended remote thread with entry point SetEvent() */
    SetLastError(0xdeadbeef);
    hThread = CreateRemoteThread(hProcess, NULL, 0, threadFunc_SetEvent,
                                 hRemoteEvent, CREATE_SUSPENDED, &tid);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("CreateRemoteThread is not implemented\n");
        goto cleanup;
    }
    ok(hThread != NULL, "CreateRemoteThread failed, err=%lu\n", GetLastError());
    ok(tid != 0, "null tid\n");
    ret = SuspendThread(hThread);
    ok(ret == 1, "ret=%lu, err=%lu\n", ret, GetLastError());
    ret = ResumeThread(hThread);
    ok(ret == 2, "ret=%lu, err=%lu\n", ret, GetLastError());

    /* thread still suspended, so wait times out */
    ret = WaitForSingleObject(hEvent, 1000);
    ok(ret == WAIT_TIMEOUT, "wait did not time out, ret=%lu\n", ret);

    ret = ResumeThread(hThread);
    ok(ret == 1, "ret=%lu, err=%lu\n", ret, GetLastError());

    /* wait that doesn't time out */
    ret = WaitForSingleObject(hEvent, 1000);
    ok(ret == WAIT_OBJECT_0, "object not signaled, ret=%lu\n", ret);

    /* wait for thread end */
    ret = WaitForSingleObject(hThread, 1000);
    ok(ret == WAIT_OBJECT_0, "waiting for thread failed, ret=%lu\n", ret);
    CloseHandle(hThread);

    /* create and wait for remote thread with entry point CloseHandle() */
    hThread = CreateRemoteThread(hProcess, NULL, 0,
                                 threadFunc_CloseHandle,
                                 hRemoteEvent, 0, &tid);
    ok(hThread != NULL, "CreateRemoteThread failed, err=%lu\n", GetLastError());
    ret = WaitForSingleObject(hThread, 1000);
    ok(ret == WAIT_OBJECT_0, "waiting for thread failed, ret=%lu\n", ret);
    CloseHandle(hThread);

    /* create remote thread with entry point SetEvent() */
    hThread = CreateRemoteThread(hProcess, NULL, 0,
                                 threadFunc_SetEvent,
                                 hRemoteEvent, 0, &tid);
    ok(hThread != NULL, "CreateRemoteThread failed, err=%lu\n", GetLastError());

    /* closed handle, so wait times out */
    ret = WaitForSingleObject(hEvent, 1000);
    ok(ret == WAIT_TIMEOUT, "wait did not time out, ret=%lu\n", ret);

    /* check that remote SetEvent() failed */
    ret = GetExitCodeThread(hThread, &exitcode);
    ok(ret != 0, "GetExitCodeThread failed, err=%lu\n", GetLastError());
    if (ret) ok(exitcode == 0, "SetEvent succeeded, expected to fail\n");
    CloseHandle(hThread);

cleanup:
    TerminateProcess(hProcess, 0);
    CloseHandle(hEvent);
    CloseHandle(hProcess);
}

/* Check basic functionality of CreateThread and Tls* functions */
static VOID test_CreateThread_basic(void)
{
   HANDLE thread[NUM_THREADS],event[NUM_THREADS];
   DWORD threadid[NUM_THREADS],curthreadId;
   DWORD threadmem[NUM_THREADS];
   DWORD exitCode;
   t1Struct tstruct[NUM_THREADS];
   int error;
   DWORD i,j;
   DWORD GLE, ret;
   DWORD tid;
   BOOL bRet;

   /* lstrlenA contains an exception handler so this makes sure exceptions work in the main thread */
   ok( lstrlenA( (char *)0xdeadbeef ) == 0, "lstrlenA: unexpected success\n" );

/* Retrieve current Thread ID for later comparisons */
  curthreadId=GetCurrentThreadId();
/* Allocate some local storage */
  ok((tlsIndex=TlsAlloc())!=TLS_OUT_OF_INDEXES,"TlsAlloc failed\n");
/* Create events for thread synchronization */
  for(i=0;i<NUM_THREADS;i++) {
    threadmem[i]=0;
/* Note that it doesn't matter what type of event we choose here.  This
   test isn't trying to thoroughly test events
*/
    event[i]=CreateEventA(NULL,TRUE,FALSE,NULL);
    tstruct[i].threadnum=i;
    tstruct[i].threadmem=threadmem;
    tstruct[i].event=event;
  }

/* Test that passing arguments to threads works okay */
  for(i=0;i<NUM_THREADS;i++) {
    thread[i] = CreateThread(NULL,0,threadFunc1,
                             &tstruct[i],0,&threadid[i]);
    ok(thread[i]!=NULL,"Create Thread failed\n");
  }
/* Test that the threads actually complete */
  for(i=0;i<NUM_THREADS;i++) {
    error=WaitForSingleObject(thread[i],5000);
    ok(error==WAIT_OBJECT_0, "Thread did not complete within timelimit\n");
    if(error!=WAIT_OBJECT_0) {
      TerminateThread(thread[i],i+NUM_THREADS);
    }
    ok(GetExitCodeThread(thread[i],&exitCode),"Could not retrieve ext code\n");
    ok(exitCode==i+NUM_THREADS,"Thread returned an incorrect exit code\n");
  }
/* Test that each thread executed in its parent's address space
   (it was able to change threadmem and pass that change back to its parent)
   and that each thread id was independent).  Note that we prove that the
   threads actually execute concurrently by having them block on each other
   in threadFunc1
*/
  for(i=0;i<NUM_THREADS;i++) {
    error=0;
    for(j=i+1;j<NUM_THREADS;j++) {
      if (threadmem[i]==threadmem[j]) {
        error=1;
      }
    }
    ok(!error && threadmem[i]==threadid[i] && threadmem[i]!=curthreadId,
         "Thread did not execute successfully\n");
    ok(CloseHandle(thread[i])!=0,"CloseHandle failed\n");
  }

  SetLastError(0xCAFEF00D);
  bRet = TlsFree(tlsIndex);
  ok(bRet, "TlsFree failed: %08lx\n", GetLastError());
  ok(GetLastError()==0xCAFEF00D,
     "GetLastError: expected 0xCAFEF00D, got %08lx\n", GetLastError());

  /* Test freeing an already freed TLS index */
  SetLastError(0xCAFEF00D);
  ok(TlsFree(tlsIndex)==0,"TlsFree succeeded\n");
  ok(GetLastError()==ERROR_INVALID_PARAMETER,
     "GetLastError: expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());

  /* Test how passing NULL as a pointer to threadid works */
  SetLastError(0xFACEaBAD);
  thread[0] = CreateThread(NULL,0,threadFunc2,NULL,0,&tid);
  GLE = GetLastError();
  if (thread[0]) { /* NT */
    ok(GLE==0xFACEaBAD, "CreateThread set last error to %ld, expected 4207848365\n", GLE);
    ret = WaitForSingleObject(thread[0],100);
    ok(ret==WAIT_OBJECT_0, "threadFunc2 did not exit during 100 ms\n");
    ret = GetExitCodeThread(thread[0],&exitCode);
    ok(ret!=0, "GetExitCodeThread returned %ld (expected nonzero)\n", ret);
    ok(exitCode==99, "threadFunc2 exited with code: %ld (expected 99)\n", exitCode);
    ok(CloseHandle(thread[0])!=0,"Error closing thread handle\n");
  }
  else { /* 9x */
    ok(GLE==ERROR_INVALID_PARAMETER, "CreateThread set last error to %ld, expected 87\n", GLE);
  }
}

/* Check that using the CREATE_SUSPENDED flag works */
static VOID test_CreateThread_suspended(void)
{
  HANDLE thread;
  DWORD threadId;
  DWORD suspend_count;
  int error;

  thread = CreateThread(NULL,0,threadFunc2,NULL,
                        CREATE_SUSPENDED,&threadId);
  ok(thread!=NULL,"Create Thread failed\n");
/* Check that the thread is suspended */
  ok(SuspendThread(thread)==1,"Thread did not start suspended\n");
  ok(ResumeThread(thread)==2,"Resume thread returned an invalid value\n");
/* Check that resume thread didn't actually start the thread.  I can't think
   of a better way of checking this than just waiting.  I am not sure if this
   will work on slow computers.
*/
  ok(WaitForSingleObject(thread,1000)==WAIT_TIMEOUT,
     "ResumeThread should not have actually started the thread\n");
/* Now actually resume the thread and make sure that it actually completes*/
  ok(ResumeThread(thread)==1,"Resume thread returned an invalid value\n");
  ok((error=WaitForSingleObject(thread,1000))==WAIT_OBJECT_0,
     "Thread did not resume\n");
  if(error!=WAIT_OBJECT_0) {
    TerminateThread(thread,1);
  }

  suspend_count = SuspendThread(thread);
  ok(suspend_count == -1, "SuspendThread returned %ld, expected -1\n", suspend_count);

  suspend_count = ResumeThread(thread);
  ok(suspend_count == 0 ||
     broken(suspend_count == -1), /* win9x */
     "ResumeThread returned %ld, expected 0\n", suspend_count);

  ok(CloseHandle(thread)!=0,"CloseHandle failed\n");
}

/* Check that SuspendThread and ResumeThread work */
static VOID test_SuspendThread(void)
{
  HANDLE thread,access_thread;
  DWORD threadId,exitCode,error;
  int i;

  thread = CreateThread(NULL,0,threadFunc3,NULL,
                        0,&threadId);
  ok(thread!=NULL,"Create Thread failed\n");
/* Check that the thread is suspended */
/* Note that this is a polling method, and there is a race between
   SuspendThread being called (in the child, and the loop below timing out,
   so the test could fail on a heavily loaded or slow computer.
*/
  error=0;
  for(i=0;error==0 && i<100;i++) {
    error=SuspendThread(thread);
    ResumeThread(thread);
    if(error==0) {
      Sleep(50);
      i++;
    }
  }
  ok(error==1,"SuspendThread did not work\n");
/* check that access restrictions are obeyed */
  if (pOpenThread) {
    access_thread=pOpenThread(THREAD_ALL_ACCESS_NT4 & (~THREAD_SUSPEND_RESUME),
                           0,threadId);
    ok(access_thread!=NULL,"OpenThread returned an invalid handle\n");
    if (access_thread!=NULL) {
      obey_ar(SuspendThread(access_thread)==~0U);
      obey_ar(ResumeThread(access_thread)==~0U);
      ok(CloseHandle(access_thread)!=0,"CloseHandle Failed\n");
    }
  }
/* Double check that the thread really is suspended */
  ok((error=GetExitCodeThread(thread,&exitCode))!=0 && exitCode==STILL_ACTIVE,
     "Thread did not really suspend\n");
/* Resume the thread, and make sure it actually completes */
  ok(ResumeThread(thread)==1,"Resume thread returned an invalid value\n");
  ok((error=WaitForSingleObject(thread,1000))==WAIT_OBJECT_0,
     "Thread did not resume\n");
  if(error!=WAIT_OBJECT_0) {
    TerminateThread(thread,1);
  }
  /* Trying to suspend a terminated thread should fail */
  error=SuspendThread(thread);
  ok(error==~0U, "wrong return code: %ld\n", error);
  ok(GetLastError()==ERROR_ACCESS_DENIED || GetLastError()==ERROR_NO_MORE_ITEMS, "unexpected error code: %ld\n", GetLastError());

  ok(CloseHandle(thread)!=0,"CloseHandle Failed\n");
}

/* Check that TerminateThread works properly
*/
static VOID test_TerminateThread(void)
{
  HANDLE thread,access_thread,event;
  DWORD threadId,exitCode;
  event=CreateEventA(NULL,TRUE,FALSE,NULL);
  thread = CreateThread(NULL,0,threadFunc4,event,0,&threadId);
  ok(thread!=NULL,"Create Thread failed\n");
/* TerminateThread has a race condition in Wine.  If the thread is terminated
   before it starts, it leaves a process behind.  Therefore, we wait for the
   thread to signal that it has started.  There is no easy way to force the
   race to occur, so we don't try to find it.
*/
  ok(WaitForSingleObject(event,5000)==WAIT_OBJECT_0,
     "TerminateThread didn't work\n");
/* check that access restrictions are obeyed */
  if (pOpenThread) {
    access_thread=pOpenThread(THREAD_ALL_ACCESS_NT4 & (~THREAD_TERMINATE),
                             0,threadId);
    ok(access_thread!=NULL,"OpenThread returned an invalid handle\n");
    if (access_thread!=NULL) {
      obey_ar(TerminateThread(access_thread,99)==0);
      ok(CloseHandle(access_thread)!=0,"CloseHandle Failed\n");
    }
  }
/* terminate a job and make sure it terminates */
  ok(TerminateThread(thread,99)!=0,"TerminateThread failed\n");
  ok(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,
     "TerminateThread didn't work\n");
  ok(GetExitCodeThread(thread,&exitCode)!=STILL_ACTIVE,
     "TerminateThread should not leave the thread 'STILL_ACTIVE'\n");
  ok(exitCode==99, "TerminateThread returned invalid exit code\n");
  ok(CloseHandle(thread)!=0,"Error Closing thread handle\n");
}

/* Check if CreateThread obeys the specified stack size.  This code does
   not work properly, and is currently disabled
*/
static VOID test_CreateThread_stack(void)
{
#if CHECK_STACK
/* The only way I know of to test the stack size is to use alloca
   and __try/__except.  However, this is probably not portable,
   and I couldn't get it to work under Wine anyhow.  However, here
   is the code which should allow for testing that CreateThread
   respects the stack-size limit
*/
     HANDLE thread;
     DWORD threadId,exitCode;

     SYSTEM_INFO sysInfo;
     sysInfo.dwPageSize=0;
     GetSystemInfo(&sysInfo);
     ok(sysInfo.dwPageSize>0,"GetSystemInfo should return a valid page size\n");
     thread = CreateThread(NULL,sysInfo.dwPageSize,
                           threadFunc5,&exitCode,
                           0,&threadId);
     ok(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,
        "TerminateThread didn't work\n");
     ok(exitCode==1,"CreateThread did not obey stack-size-limit\n");
     ok(CloseHandle(thread)!=0,"CloseHandle failed\n");
#endif
}

/* Check whether setting/retrieving thread priorities works */
static VOID test_thread_priority(void)
{
   HANDLE curthread,access_thread;
   DWORD curthreadId,exitCode;
   int min_priority=-2,max_priority=2;
   BOOL disabled,rc;
   int i;

   curthread=GetCurrentThread();
   curthreadId=GetCurrentThreadId();
/* Check thread priority */
/* NOTE: on Win2k/XP priority can be from -7 to 6.  All other platforms it
         is -2 to 2.  However, even on a real Win2k system, using thread
         priorities beyond the -2 to 2 range does not work.  If you want to try
         anyway, enable USE_EXTENDED_PRIORITIES
*/
   ok(GetThreadPriority(curthread)==THREAD_PRIORITY_NORMAL,
      "GetThreadPriority Failed\n");

   if (pOpenThread) {
/* check that access control is obeyed */
     access_thread=pOpenThread(THREAD_ALL_ACCESS_NT4 &
                       (~THREAD_QUERY_INFORMATION) & (~THREAD_SET_INFORMATION),
                       0,curthreadId);
     ok(access_thread!=NULL,"OpenThread returned an invalid handle\n");
     if (access_thread!=NULL) {
       obey_ar(SetThreadPriority(access_thread,1)==0);
       obey_ar(GetThreadPriority(access_thread)==THREAD_PRIORITY_ERROR_RETURN);
       obey_ar(GetExitCodeThread(access_thread,&exitCode)==0);
       ok(CloseHandle(access_thread),"Error Closing thread handle\n");
     }
   }
#if USE_EXTENDED_PRIORITIES
   min_priority=-7; max_priority=6;
#endif
   for(i=min_priority;i<=max_priority;i++) {
     ok(SetThreadPriority(curthread,i)!=0,
        "SetThreadPriority Failed for priority: %d\n",i);
     ok(GetThreadPriority(curthread)==i,
        "GetThreadPriority Failed for priority: %d\n",i);
   }
   ok(SetThreadPriority(curthread,THREAD_PRIORITY_TIME_CRITICAL)!=0,
      "SetThreadPriority Failed\n");
   ok(GetThreadPriority(curthread)==THREAD_PRIORITY_TIME_CRITICAL,
      "GetThreadPriority Failed\n");
   ok(SetThreadPriority(curthread,THREAD_PRIORITY_IDLE)!=0,
       "SetThreadPriority Failed\n");
   ok(GetThreadPriority(curthread)==THREAD_PRIORITY_IDLE,
       "GetThreadPriority Failed\n");
   ok(SetThreadPriority(curthread,0)!=0,"SetThreadPriority Failed\n");

/* Check that the thread priority is not changed if SetThreadPriority
   is called with a value outside of the max/min range */
   SetThreadPriority(curthread,min_priority);
   SetLastError(0xdeadbeef);
   rc = SetThreadPriority(curthread,min_priority-1);

   ok(rc == FALSE, "SetThreadPriority passed with a bad argument\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER ||
      GetLastError() == ERROR_INVALID_PRIORITY /* Win9x */,
      "SetThreadPriority error %ld, expected ERROR_INVALID_PARAMETER or ERROR_INVALID_PRIORITY\n",
      GetLastError());
   ok(GetThreadPriority(curthread)==min_priority,
      "GetThreadPriority didn't return min_priority\n");

   SetThreadPriority(curthread,max_priority);
   SetLastError(0xdeadbeef);
   rc = SetThreadPriority(curthread,max_priority+1);

   ok(rc == FALSE, "SetThreadPriority passed with a bad argument\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER ||
      GetLastError() == ERROR_INVALID_PRIORITY /* Win9x */,
      "SetThreadPriority error %ld, expected ERROR_INVALID_PARAMETER or ERROR_INVALID_PRIORITY\n",
      GetLastError());
   ok(GetThreadPriority(curthread)==max_priority,
      "GetThreadPriority didn't return max_priority\n");

/* Check thread priority boost */
   if (!pGetThreadPriorityBoost || !pSetThreadPriorityBoost) 
     return; /* Win9x */

   SetLastError(0xdeadbeef);
   rc=pGetThreadPriorityBoost(curthread,&disabled);
   if (rc==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
   {
      win_skip("GetThreadPriorityBoost is not implemented on WinME\n");
      return;
   }

   ok(rc!=0,"error=%ld\n",GetLastError());

   if (pOpenThread) {
/* check that access control is obeyed */
     access_thread=pOpenThread(THREAD_ALL_ACCESS_NT4 &
                       (~THREAD_QUERY_INFORMATION) & (~THREAD_SET_INFORMATION),
                       0,curthreadId);
     ok(access_thread!=NULL,"OpenThread returned an invalid handle\n");
     if (access_thread!=NULL) {
       todo_wine obey_ar(pSetThreadPriorityBoost(access_thread,1)==0);
       todo_wine obey_ar(pGetThreadPriorityBoost(access_thread,&disabled)==0);
       ok(CloseHandle(access_thread),"Error Closing thread handle\n");
     }
   }

   rc = pSetThreadPriorityBoost(curthread,1);
   ok( rc != 0, "error=%ld\n",GetLastError());
   todo_wine {
     rc=pGetThreadPriorityBoost(curthread,&disabled);
     ok(rc!=0 && disabled==1,
        "rc=%d error=%ld disabled=%d\n",rc,GetLastError(),disabled);
   }

   rc = pSetThreadPriorityBoost(curthread,0);
   ok( rc != 0, "error=%ld\n",GetLastError());
   rc=pGetThreadPriorityBoost(curthread,&disabled);
   ok(rc!=0 && disabled==0,
      "rc=%d error=%ld disabled=%d\n",rc,GetLastError(),disabled);
}

/* check the GetThreadTimes function */
static VOID test_GetThreadTimes(void)
{
     HANDLE thread,access_thread=NULL;
     FILETIME creationTime,exitTime,kernelTime,userTime;
     DWORD threadId;
     int error;

     thread = CreateThread(NULL,0,threadFunc2,NULL,
                           CREATE_SUSPENDED,&threadId);

     ok(thread!=NULL,"Create Thread failed\n");
/* check that access control is obeyed */
     if (pOpenThread) {
       access_thread=pOpenThread(THREAD_ALL_ACCESS_NT4 &
                                   (~THREAD_QUERY_INFORMATION), 0,threadId);
       ok(access_thread!=NULL,
          "OpenThread returned an invalid handle\n");
     }
     ok(ResumeThread(thread)==1,"Resume thread returned an invalid value\n");
     ok(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,
        "ResumeThread didn't work\n");
     creationTime.dwLowDateTime=99; creationTime.dwHighDateTime=99;
     exitTime.dwLowDateTime=99;     exitTime.dwHighDateTime=99;
     kernelTime.dwLowDateTime=99;   kernelTime.dwHighDateTime=99;
     userTime.dwLowDateTime=99;     userTime.dwHighDateTime=99;
/* GetThreadTimes should set all of the parameters passed to it */
     error=GetThreadTimes(thread,&creationTime,&exitTime,
                          &kernelTime,&userTime);

     if (error == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
       win_skip("GetThreadTimes is not implemented\n");
     else {
       ok(error!=0,"GetThreadTimes failed\n");
       ok(creationTime.dwLowDateTime!=99 || creationTime.dwHighDateTime!=99,
          "creationTime was invalid\n");
       ok(exitTime.dwLowDateTime!=99 || exitTime.dwHighDateTime!=99,
          "exitTime was invalid\n");
       ok(kernelTime.dwLowDateTime!=99 || kernelTime.dwHighDateTime!=99,
          "kernelTimewas invalid\n");
       ok(userTime.dwLowDateTime!=99 || userTime.dwHighDateTime!=99,
          "userTime was invalid\n");
       ok(CloseHandle(thread)!=0,"CloseHandle failed\n");
       if(access_thread!=NULL)
       {
         error=GetThreadTimes(access_thread,&creationTime,&exitTime,
                              &kernelTime,&userTime);
         obey_ar(error==0);
       }
     }
     if(access_thread!=NULL) {
       ok(CloseHandle(access_thread)!=0,"CloseHandle Failed\n");
     }
}

/* Check the processor affinity functions */
/* NOTE: These functions should also be checked that they obey access control
*/
static VOID test_thread_processor(void)
{
   HANDLE curthread,curproc;
   DWORD_PTR processMask,systemMask,retMask;
   SYSTEM_INFO sysInfo;
   BOOL is_wow64, old_wow64 = FALSE;
   DWORD ret;

   if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    if (is_wow64)
    {
        TEB64 *teb64 = ULongToPtr(NtCurrentTeb()->GdiBatchCount);
        if (teb64)
        {
            PEB64 *peb64 = ULongToPtr(teb64->Peb);
            old_wow64 = !peb64->LdrData;
        }
    }

   sysInfo.dwNumberOfProcessors=0;
   GetSystemInfo(&sysInfo);
   ok(sysInfo.dwNumberOfProcessors>0,
      "GetSystemInfo failed to return a valid # of processors\n");
/* Use the current Thread/process for all tests */
   curthread=GetCurrentThread();
   ok(curthread!=NULL,"GetCurrentThread failed\n");
   curproc=GetCurrentProcess();
   ok(curproc!=NULL,"GetCurrentProcess failed\n");
/* Check the Affinity Mask functions */
   ok(GetProcessAffinityMask(curproc,&processMask,&systemMask)!=0,
      "GetProcessAffinityMask failed\n");
   ok(SetThreadAffinityMask(curthread,processMask)==processMask,
      "SetThreadAffinityMask failed\n");
   ok(SetThreadAffinityMask(curthread,processMask+1)==0,
      "SetThreadAffinityMask passed for an illegal processor\n");
/* NOTE: Pre-Vista does not recognize the "all processors" flag (all bits set) */
   retMask = SetThreadAffinityMask(curthread,~0);
   ok(broken(retMask==0) || retMask==processMask,
      "SetThreadAffinityMask(thread,-1) failed to request all processors.\n");

    if (retMask == processMask)
    {
        /* Show that the "all processors" flag is handled in ntdll */
        DWORD_PTR mask = ~0u;
        NTSTATUS status = pNtSetInformationThread(curthread, ThreadAffinityMask, &mask, sizeof(mask));
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS in NtSetInformationThread, got %lx\n", status);
    }

   if (retMask == processMask && sizeof(ULONG_PTR) > sizeof(ULONG))
   {
       /* only the low 32-bits matter */
       retMask = SetThreadAffinityMask(curthread,~(ULONG_PTR)0);
       ok(retMask == processMask, "SetThreadAffinityMask failed\n");
       retMask = SetThreadAffinityMask(curthread,~(ULONG_PTR)0 >> 3);
       ok(retMask == processMask, "SetThreadAffinityMask failed\n");
   }

    SetLastError(0xdeadbeef);
    ret = SetThreadIdealProcessor(GetCurrentThread(), 0);
    ok(ret != ~0u, "Unexpected return value %lu.\n", ret);

    if (is_wow64)
    {
        SetLastError(0xdeadbeef);
        ret = SetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS + 1);
        todo_wine_if(old_wow64)
        ok(ret != ~0u, "Unexpected return value %lu.\n", ret);

        SetLastError(0xdeadbeef);
        ret = SetThreadIdealProcessor(GetCurrentThread(), 65);
        ok(ret == ~0u, "Unexpected return value %lu.\n", ret);
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected error %ld.\n", GetLastError());
    }
    else
    {
        SetLastError(0xdeadbeef);
        ret = SetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS+1);
        ok(ret == ~0u, "Unexpected return value %lu.\n", ret);
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected error %ld.\n", GetLastError());
    }

    ret = SetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS);
    ok(ret != ~0u, "Unexpected return value %lu.\n", ret);

    if (pGetThreadGroupAffinity && pSetThreadGroupAffinity)
    {
        GROUP_AFFINITY affinity, affinity_new;
        NTSTATUS status;

        memset(&affinity, 0, sizeof(affinity));
        ok(pGetThreadGroupAffinity(curthread, &affinity), "GetThreadGroupAffinity failed\n");

        SetLastError(0xdeadbeef);
        ok(!pGetThreadGroupAffinity(curthread, NULL), "GetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_NOACCESS), /* Win 7 and 8 */
           "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
        ok(affinity.Group == 0, "Expected group 0 got %u\n", affinity.Group);

        memset(&affinity_new, 0, sizeof(affinity_new));
        affinity_new.Group = 0;
        affinity_new.Mask  = affinity.Mask;
        ok(pSetThreadGroupAffinity(curthread, &affinity_new, &affinity), "SetThreadGroupAffinity failed\n");
        ok(affinity_new.Mask == affinity.Mask, "Expected old affinity mask %Ix, got %Ix\n",
           affinity_new.Mask, affinity.Mask);

        /* show that the "all processors" flag is not supported for SetThreadGroupAffinity */
        if (sysInfo.dwNumberOfProcessors < 8 * sizeof(DWORD_PTR))
        {
            affinity_new.Group = 0;
            affinity_new.Mask  = ~(DWORD_PTR)0;
            SetLastError(0xdeadbeef);
            ok(!pSetThreadGroupAffinity(curthread, &affinity_new, NULL), "SetThreadGroupAffinity succeeded\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
        }

        affinity_new.Group = 1; /* assumes that you have less than 64 logical processors */
        affinity_new.Mask  = 0x1;
        SetLastError(0xdeadbeef);
        ok(!pSetThreadGroupAffinity(curthread, &affinity_new, NULL), "SetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

        SetLastError(0xdeadbeef);
        ok(!pSetThreadGroupAffinity(curthread, NULL, NULL), "SetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_NOACCESS,
           "Expected ERROR_NOACCESS, got %ld\n", GetLastError());

        /* show that the access violation was detected in ntdll */
        status = pNtSetInformationThread(curthread, ThreadGroupInformation, NULL, sizeof(affinity_new));
        ok(status == STATUS_ACCESS_VIOLATION,
           "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

        /* restore original mask */
        affinity_new.Group = 0;
        affinity_new.Mask  = affinity.Mask;
        SetLastError(0xdeadbeef);
        ok(pSetThreadGroupAffinity(curthread, &affinity_new, &affinity), "SetThreadGroupAffinity failed\n");
        ok(affinity_new.Mask == affinity.Mask, "Expected old affinity mask %Ix, got %Ix\n",
           affinity_new.Mask, affinity.Mask);
    }
    else
        win_skip("Get/SetThreadGroupAffinity not available\n");
}

static VOID test_GetCurrentThreadStackLimits(void)
{
    ULONG_PTR low = 0, high = 0;

    if (!pGetCurrentThreadStackLimits)
    {
        win_skip("GetCurrentThreadStackLimits not available.\n");
        return;
    }

    if (0)
    {
        /* crashes on native */
        pGetCurrentThreadStackLimits(NULL, NULL);
        pGetCurrentThreadStackLimits(NULL, &high);
        pGetCurrentThreadStackLimits(&low, NULL);
    }

    pGetCurrentThreadStackLimits(&low, &high);
    ok(low == (ULONG_PTR)NtCurrentTeb()->DeallocationStack, "expected %p, got %Ix\n", NtCurrentTeb()->DeallocationStack, low);
    ok(high == (ULONG_PTR)NtCurrentTeb()->Tib.StackBase, "expected %p, got %Ix\n", NtCurrentTeb()->Tib.StackBase, high);
}

static void test_SetThreadStackGuarantee(void)
{
    ULONG size;
    BOOL ret;

    if (!pSetThreadStackGuarantee)
    {
        win_skip("SetThreadStackGuarantee not available.\n");
        return;
    }
    size = 0;
    ret = pSetThreadStackGuarantee( &size );
    ok( ret, "failed err %lu\n", GetLastError() );
    ok( size == 0, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 0, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
    size = 0xdeadbef;
    ret = pSetThreadStackGuarantee( &size );
    ok( !ret, "succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_ADDRESS,
        "wrong error %lu\n", GetLastError());
    ok( size == 0, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 0, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
    size = 200;
    ret = pSetThreadStackGuarantee( &size );
    ok( ret, "failed err %lu\n", GetLastError() );
    ok( size == 0, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 4096 * sizeof(void *) / 4, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
    size = 5000;
    ret = pSetThreadStackGuarantee( &size );
    ok( ret, "failed err %lu\n", GetLastError() );
    ok( size == 4096 * sizeof(void *) / 4, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 8192, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
    size = 2000;
    ret = pSetThreadStackGuarantee( &size );
    ok( ret, "failed err %lu\n", GetLastError() );
    ok( size == 8192, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 8192, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
    size = 10000;
    ret = pSetThreadStackGuarantee( &size );
    ok( ret, "failed err %lu\n", GetLastError() );
    ok( size == 8192, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 12288, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
    ret = pSetThreadStackGuarantee( &size );
    ok( ret, "failed err %lu\n", GetLastError() );
    ok( size == 12288, "wrong size %lu\n", size );
    ok( NtCurrentTeb()->GuaranteedStackBytes == 12288, "wrong teb %lu\n",
        NtCurrentTeb()->GuaranteedStackBytes );
}

static VOID test_GetThreadExitCode(void)
{
    DWORD exitCode, threadid;
    DWORD GLE, ret;
    HANDLE thread;

    ret = GetExitCodeThread((HANDLE)0x2bad2bad,&exitCode);
    ok(ret==0, "GetExitCodeThread returned non zero value: %ld\n", ret);
    GLE = GetLastError();
    ok(GLE==ERROR_INVALID_HANDLE, "GetLastError returned %ld (expected 6)\n", GLE);

    thread = CreateThread(NULL,0,threadFunc2,NULL,0,&threadid);
    ret = WaitForSingleObject(thread,100);
    ok(ret==WAIT_OBJECT_0, "threadFunc2 did not exit during 100 ms\n");
    ret = GetExitCodeThread(thread,&exitCode);
    ok(ret==exitCode || ret==1, 
       "GetExitCodeThread returned %ld (expected 1 or %ld)\n", ret, exitCode);
    ok(exitCode==99, "threadFunc2 exited with code %ld (expected 99)\n", exitCode);
    ok(CloseHandle(thread)!=0,"Error closing thread handle\n");
}

#ifdef __i386__

static int test_value = 0;
static HANDLE event;

static void WINAPI set_test_val( int val )
{
    test_value += val;
    ExitThread(0);
}

static DWORD WINAPI threadFunc6(LPVOID p)
{
    SetEvent( event );
    Sleep( 1000 );
    test_value *= (int)p;
    return 0;
}

static void test_SetThreadContext(void)
{
    CONTEXT ctx;
    int *stack;
    HANDLE thread;
    DWORD threadid;
    DWORD prevcount;
    BOOL ret;

    SetLastError(0xdeadbeef);
    event = CreateEventW( NULL, TRUE, FALSE, NULL );
    thread = CreateThread( NULL, 0, threadFunc6, (void *)2, 0, &threadid );
    ok( thread != NULL, "CreateThread failed : (%ld)\n", GetLastError() );
    if (!thread)
    {
        trace("Thread creation failed, skipping rest of test\n");
        return;
    }
    WaitForSingleObject( event, INFINITE );
    SuspendThread( thread );
    CloseHandle( event );

    ctx.ContextFlags = CONTEXT_FULL;
    SetLastError(0xdeadbeef);
    ret = GetThreadContext( thread, &ctx );
    ok( ret, "GetThreadContext failed : (%lu)\n", GetLastError() );

    if (ret)
    {
        /* simulate a call to set_test_val(10) */
        stack = (int *)ctx.Esp;
        stack[-1] = 10;
        stack[-2] = ctx.Eip;
        ctx.Esp -= 2 * sizeof(int *);
        ctx.Eip = (DWORD)set_test_val;
        SetLastError(0xdeadbeef);
        ret = SetThreadContext( thread, &ctx );
        ok( ret, "SetThreadContext failed : (%ld)\n", GetLastError() );
    }

    SetLastError(0xdeadbeef);
    prevcount = ResumeThread( thread );
    ok ( prevcount == 1, "Previous suspend count (%ld) instead of 1, last error : (%ld)\n",
                         prevcount, GetLastError() );

    WaitForSingleObject( thread, INFINITE );
    ok( test_value == 10, "test_value %d\n", test_value );

    ctx.ContextFlags = CONTEXT_FULL;
    SetLastError(0xdeadbeef);
    ret = GetThreadContext( thread, &ctx );
    ok( (!ret && ((GetLastError() == ERROR_GEN_FAILURE) || (GetLastError() == ERROR_ACCESS_DENIED))) ||
        (!ret && broken(GetLastError() == ERROR_INVALID_HANDLE)) || /* win2k */
        broken(ret),   /* 32bit application on NT 5.x 64bit */
        "got %d with %lu (expected FALSE with ERROR_GEN_FAILURE or ERROR_ACCESS_DENIED)\n",
        ret, GetLastError() );

    SetLastError(0xdeadbeef);
    ret = SetThreadContext( thread, &ctx );
    ok( (!ret && ((GetLastError() == ERROR_GEN_FAILURE) || (GetLastError() == ERROR_ACCESS_DENIED))) ||
        (!ret && broken(GetLastError() == ERROR_INVALID_HANDLE)) || /* win2k */
        broken(ret),   /* 32bit application on NT 5.x 64bit */
        "got %d with %lu (expected FALSE with ERROR_GEN_FAILURE or ERROR_ACCESS_DENIED)\n",
        ret, GetLastError() );

    CloseHandle( thread );
}

static DWORD WINAPI test_stack( void *arg )
{
    DWORD *stack = (DWORD *)(((DWORD)&arg & ~0xfff) + 0x1000);

    ok( stack == NtCurrentTeb()->Tib.StackBase, "wrong stack %p/%p\n",
        stack, NtCurrentTeb()->Tib.StackBase );
    ok( !stack[-1], "wrong data %p = %08lx\n", stack - 1, stack[-1] );
    return 0;
}

static void test_GetThreadContext(void)
{
    CONTEXT ctx;
    BOOL ret;
    HANDLE thread;

    memset(&ctx, 0xcc, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ret = GetThreadContext(GetCurrentThread(), &ctx);
    ok(ret, "GetThreadContext failed: %lu\n", GetLastError());
    ok(ctx.ContextFlags == CONTEXT_DEBUG_REGISTERS, "ContextFlags = %lx\n", ctx.ContextFlags);
    ok(!ctx.Dr0, "Dr0 = %lx\n", ctx.Dr0);
    ok(!ctx.Dr1, "Dr0 = %lx\n", ctx.Dr0);

    thread = CreateThread( NULL, 0, test_stack, (void *)0x1234, 0, NULL );
    WaitForSingleObject( thread, 1000 );
    CloseHandle( thread );
}

static void test_GetThreadSelectorEntry(void)
{
    LDT_ENTRY entry;
    CONTEXT ctx;
    DWORD limit;
    void *base;
    BOOL ret;

    memset(&ctx, 0x11, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_SEGMENTS | CONTEXT_CONTROL;
    ret = GetThreadContext(GetCurrentThread(), &ctx);
    ok(ret, "GetThreadContext error %lu\n", GetLastError());
    ok(!HIWORD(ctx.SegCs), "expected HIWORD(SegCs) == 0, got %lu\n", ctx.SegCs);
    ok(!HIWORD(ctx.SegDs), "expected HIWORD(SegDs) == 0, got %lu\n", ctx.SegDs);
    ok(!HIWORD(ctx.SegFs), "expected HIWORD(SegFs) == 0, got %lu\n", ctx.SegFs);

    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegCs, &entry);
    ok(ret, "GetThreadSelectorEntry(SegCs) error %lu\n", GetLastError());
    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegDs, &entry);
    ok(ret, "GetThreadSelectorEntry(SegDs) error %lu\n", GetLastError());
    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegDs & ~3, &entry);
    ok(ret, "GetThreadSelectorEntry(SegDs) error %lu\n", GetLastError());
    ret = GetThreadSelectorEntry(GetCurrentThread(), 0, &entry);
    ok(ret, "GetThreadSelectorEntry(SegDs) error %lu\n", GetLastError());
    ret = GetThreadSelectorEntry(GetCurrentThread(), 3, &entry);
    ok(ret, "GetThreadSelectorEntry(SegDs) error %lu\n", GetLastError());
    SetLastError( 0xdeadbeef );
    ret = GetThreadSelectorEntry(GetCurrentThread(), 0xdeadbeef, &entry);
    ok(!ret, "GetThreadSelectorEntry(invalid) succeeded\n");
    ok( GetLastError() == ERROR_GEN_FAILURE
        || GetLastError() == ERROR_INVALID_THREAD_ID /* 32-bit */, "wrong error %lu\n", GetLastError() );
    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegDs + 0x100, &entry);
    ok(!ret, "GetThreadSelectorEntry(invalid) succeeded\n");
    ok( GetLastError() == ERROR_GEN_FAILURE
        || GetLastError() == ERROR_NOACCESS /* 32-bit */, "wrong error %lu\n", GetLastError() );

    memset(&entry, 0x11, sizeof(entry));
    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegFs, &entry);
    ok(ret, "GetThreadSelectorEntry(SegFs) error %lu\n", GetLastError());
    entry.HighWord.Bits.Type &= ~1; /* ignore accessed bit */

    base  = (void *)((entry.HighWord.Bits.BaseHi << 24) | (entry.HighWord.Bits.BaseMid << 16) | entry.BaseLow);
    limit = (entry.HighWord.Bits.LimitHi << 16) | entry.LimitLow;

    ok(base == NtCurrentTeb(),                "expected %p, got %p\n", NtCurrentTeb(), base);
    ok(limit == 0x0fff || limit == 0x4000,    "expected 0x0fff or 0x4000, got %#lx\n", limit);
    ok(entry.HighWord.Bits.Type == 0x12,      "expected 0x12, got %#x\n", entry.HighWord.Bits.Type);
    ok(entry.HighWord.Bits.Dpl == 3,          "expected 3, got %u\n", entry.HighWord.Bits.Dpl);
    ok(entry.HighWord.Bits.Pres == 1,         "expected 1, got %u\n", entry.HighWord.Bits.Pres);
    ok(entry.HighWord.Bits.Sys == 0,          "expected 0, got %u\n", entry.HighWord.Bits.Sys);
    ok(entry.HighWord.Bits.Reserved_0 == 0,   "expected 0, got %u\n", entry.HighWord.Bits.Reserved_0);
    ok(entry.HighWord.Bits.Default_Big == 1,  "expected 1, got %u\n", entry.HighWord.Bits.Default_Big);
    ok(entry.HighWord.Bits.Granularity == 0,  "expected 0, got %u\n", entry.HighWord.Bits.Granularity);

    memset(&entry, 0x11, sizeof(entry));
    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegCs, &entry);
    ok(ret, "GetThreadSelectorEntry(SegDs) error %lu\n", GetLastError());
    entry.HighWord.Bits.Type &= ~1; /* ignore accessed bit */
    base  = (void *)((entry.HighWord.Bits.BaseHi << 24) | (entry.HighWord.Bits.BaseMid << 16) | entry.BaseLow);
    limit = (entry.HighWord.Bits.LimitHi << 16) | entry.LimitLow;

    ok(base == 0, "got base %p\n", base);
    ok(limit == ~0u >> 12, "got limit %#lx\n", limit);
    ok(entry.HighWord.Bits.Type == 0x1a,      "expected 0x12, got %#x\n", entry.HighWord.Bits.Type);
    ok(entry.HighWord.Bits.Dpl == 3,          "expected 3, got %u\n", entry.HighWord.Bits.Dpl);
    ok(entry.HighWord.Bits.Pres == 1,         "expected 1, got %u\n", entry.HighWord.Bits.Pres);
    ok(entry.HighWord.Bits.Sys == 0,          "expected 0, got %u\n", entry.HighWord.Bits.Sys);
    ok(entry.HighWord.Bits.Reserved_0 == 0,   "expected 0, got %u\n", entry.HighWord.Bits.Reserved_0);
    ok(entry.HighWord.Bits.Default_Big == 1,  "expected 1, got %u\n", entry.HighWord.Bits.Default_Big);
    ok(entry.HighWord.Bits.Granularity == 1,  "expected 1, got %u\n", entry.HighWord.Bits.Granularity);

    memset(&entry, 0x11, sizeof(entry));
    ret = GetThreadSelectorEntry(GetCurrentThread(), ctx.SegDs, &entry);
    ok(ret, "GetThreadSelectorEntry(SegDs) error %lu\n", GetLastError());
    entry.HighWord.Bits.Type &= ~1; /* ignore accessed bit */
    base  = (void *)((entry.HighWord.Bits.BaseHi << 24) | (entry.HighWord.Bits.BaseMid << 16) | entry.BaseLow);
    limit = (entry.HighWord.Bits.LimitHi << 16) | entry.LimitLow;

    ok(base == 0, "got base %p\n", base);
    ok(limit == ~0u >> 12, "got limit %#lx\n", limit);
    ok(entry.HighWord.Bits.Type == 0x12,      "expected 0x12, got %#x\n", entry.HighWord.Bits.Type);
    ok(entry.HighWord.Bits.Dpl == 3,          "expected 3, got %u\n", entry.HighWord.Bits.Dpl);
    ok(entry.HighWord.Bits.Pres == 1,         "expected 1, got %u\n", entry.HighWord.Bits.Pres);
    ok(entry.HighWord.Bits.Sys == 0,          "expected 0, got %u\n", entry.HighWord.Bits.Sys);
    ok(entry.HighWord.Bits.Reserved_0 == 0,   "expected 0, got %u\n", entry.HighWord.Bits.Reserved_0);
    ok(entry.HighWord.Bits.Default_Big == 1,  "expected 1, got %u\n", entry.HighWord.Bits.Default_Big);
    ok(entry.HighWord.Bits.Granularity == 1,  "expected 1, got %u\n", entry.HighWord.Bits.Granularity);
}

#endif  /* __i386__ */

static HANDLE finish_event;
static LONG times_executed;

static DWORD CALLBACK work_function(void *p)
{
    LONG executed = InterlockedIncrement(&times_executed);

    if (executed == 100)
        SetEvent(finish_event);
    return 0;
}

static void test_QueueUserWorkItem(void)
{
    INT_PTR i;
    DWORD wait_result;
    DWORD before, after;

    /* QueueUserWorkItem not present on win9x */
    if (!pQueueUserWorkItem) return;

    finish_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    before = GetTickCount();

    for (i = 0; i < 100; i++)
    {
        BOOL ret = pQueueUserWorkItem(work_function, (void *)i, WT_EXECUTEDEFAULT);
        ok(ret, "QueueUserWorkItem failed with error %ld\n", GetLastError());
    }

    wait_result = WaitForSingleObject(finish_event, 10000);

    after = GetTickCount();
    trace("100 QueueUserWorkItem calls took %ldms\n", after - before);
    ok(wait_result == WAIT_OBJECT_0, "wait failed with error 0x%lx\n", wait_result);

    ok(times_executed == 100, "didn't execute all of the work items\n");
}

static void CALLBACK signaled_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    HANDLE event = p;
    SetEvent(event);
    ok(!TimerOrWaitFired, "wait shouldn't have timed out\n");
}

static void CALLBACK wait_complete_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    HANDLE event = p;
    DWORD res;
    ok(!TimerOrWaitFired, "wait shouldn't have timed out\n");
    res = WaitForSingleObject(event, INFINITE);
    ok(res == WAIT_OBJECT_0, "WaitForSingleObject returned %lx\n", res);
}

static void CALLBACK timeout_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    HANDLE event = p;
    SetEvent(event);
    ok(TimerOrWaitFired, "wait should have timed out\n");
}

struct waitthread_test_param
{
    HANDLE trigger_event;
    HANDLE wait_event;
    HANDLE complete_event;
};

static void CALLBACK waitthread_test_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    struct waitthread_test_param *param = p;
    DWORD ret;

    SetEvent(param->trigger_event);
    ret = WaitForSingleObject(param->wait_event, 100);
    ok(ret == WAIT_TIMEOUT, "wait should have timed out\n");
    SetEvent(param->complete_event);
}

struct unregister_params
{
    HANDLE wait_handle;
    HANDLE complete_event;
};

static void CALLBACK unregister_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    struct unregister_params *param = p;
    HANDLE wait_handle = param->wait_handle;
    BOOL ret;
    ok(wait_handle != INVALID_HANDLE_VALUE, "invalid wait handle\n");
    ret = pUnregisterWait(param->wait_handle);
    todo_wine ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());
    SetEvent(param->complete_event);
}

static void test_RegisterWaitForSingleObject(void)
{
    BOOL ret;
    HANDLE wait_handle, wait_handle2;
    HANDLE handle;
    HANDLE complete_event;
    HANDLE waitthread_trigger_event, waitthread_wait_event;
    struct waitthread_test_param param;
    struct unregister_params unregister_param;
    DWORD i;

    if (!pRegisterWaitForSingleObject || !pUnregisterWait)
    {
        win_skip("RegisterWaitForSingleObject or UnregisterWait not implemented\n");
        return;
    }

    /* test signaled case */

    handle = CreateEventW(NULL, TRUE, TRUE, NULL);
    complete_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    /* give worker thread chance to complete */
    Sleep(100);

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    /* test cancel case */

    ResetEvent(handle);

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    /* test unregister while running */

    SetEvent(handle);
    ret = pRegisterWaitForSingleObject(&wait_handle, handle, wait_complete_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    /* give worker thread chance to start */
    Sleep(50);
    ret = pUnregisterWait(wait_handle);
    ok(!ret, "UnregisterWait succeeded\n");
    ok(GetLastError() == ERROR_IO_PENDING, "UnregisterWait failed with error %ld\n", GetLastError());

    /* give worker thread chance to complete */
    SetEvent(complete_event);
    Sleep(50);

    /* test timeout case */

    ResetEvent(handle);

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, timeout_function, complete_event, 0, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    /* give worker thread chance to complete */
    Sleep(100);

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pUnregisterWait(NULL);
    ok(!ret, "Expected UnregisterWait to fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    /* test WT_EXECUTEINWAITTHREAD */

    SetEvent(handle);
    ret = pRegisterWaitForSingleObject(&wait_handle, handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    /* give worker thread chance to complete */
    Sleep(100);

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    /* the callback execution should be sequentially consistent with the wait handle return,
       even if the event is already set */

    for (i = 0; i < 100; ++i)
    {
        SetEvent(handle);
        unregister_param.complete_event = complete_event;
        unregister_param.wait_handle = INVALID_HANDLE_VALUE;

        ret = pRegisterWaitForSingleObject(&unregister_param.wait_handle, handle, unregister_function, &unregister_param, INFINITE, WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD);
        ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

        WaitForSingleObject(complete_event, INFINITE);
    }

    /* test multiple waits with WT_EXECUTEINWAITTHREAD.
     * Windows puts multiple waits on the same wait thread, and using WT_EXECUTEINWAITTHREAD causes the callbacks to run serially.
     */

    SetEvent(handle);
    waitthread_trigger_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    waitthread_wait_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    param.trigger_event = waitthread_trigger_event;
    param.wait_event = waitthread_wait_event;
    param.complete_event = complete_event;

    ret = pRegisterWaitForSingleObject(&wait_handle2, waitthread_trigger_event, signaled_function, waitthread_wait_event,
                                       INFINITE, WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, waitthread_test_function, &param, INFINITE, WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD);
    ok(ret, "RegisterWaitForSingleObject failed with error %ld\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    /* give worker thread chance to complete */
    Sleep(100);

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    ret = pUnregisterWait(wait_handle2);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    CloseHandle(waitthread_wait_event);
    CloseHandle(waitthread_trigger_event);
    CloseHandle(complete_event);
    CloseHandle(handle);
}

static DWORD LS_main;
static DWORD LS_index0, LS_index1;
static DWORD LS_OutOfIndexesValue;

/* Function pointers to the FLS/TLS functions to test in LS_ThreadProc() */
static DWORD (WINAPI *LS_AllocFunc)(void);
static PVOID (WINAPI *LS_GetValueFunc)(DWORD);
static BOOL (WINAPI *LS_SetValueFunc)(DWORD, PVOID);
static BOOL (WINAPI *LS_FreeFunc)(DWORD);

/* Names of the functions tested in LS_ThreadProc(), for error messages */
static const char* LS_AllocFuncName = "";
static const char* LS_GetValueFuncName = "";
static const char* LS_SetValueFuncName = "";
static const char* LS_FreeFuncName = "";

/* FLS entry points, dynamically loaded in platforms that support them */
static DWORD (WINAPI *pFlsAlloc)(PFLS_CALLBACK_FUNCTION);
static BOOL (WINAPI *pFlsFree)(DWORD);
static PVOID (WINAPI *pFlsGetValue)(DWORD);
static BOOL (WINAPI *pFlsSetValue)(DWORD,PVOID);

/* A thunk function to make FlsAlloc compatible with the signature of TlsAlloc */
static DWORD WINAPI FLS_AllocFuncThunk(void)
{
  return pFlsAlloc(NULL);
}

static DWORD WINAPI LS_InheritanceProc(LPVOID p)
{
  /* We should NOT inherit the FLS/TLS values from our parent or from the
     main thread.  */
  LPVOID val;

  val = LS_GetValueFunc(LS_main);
  ok(val == NULL, "%s inheritance failed\n", LS_GetValueFuncName);

  val = LS_GetValueFunc(LS_index0);
  ok(val == NULL, "%s inheritance failed\n", LS_GetValueFuncName);

  val = LS_GetValueFunc(LS_index1);
  ok(val == NULL, "%s inheritance failed\n", LS_GetValueFuncName);

  return 0;
}

/* Basic FLS/TLS usage test.  Make sure we can create slots and the values we
   store in them are separate among threads.  Also test FLS/TLS value
   inheritance with LS_InheritanceProc.  */
static DWORD WINAPI LS_ThreadProc(LPVOID p)
{
  LONG_PTR id = (LONG_PTR) p;
  LPVOID val;
  BOOL ret;

  if (sync_threads_and_run_one(0, id))
  {
    LS_index0 = LS_AllocFunc();
    ok(LS_index0 != LS_OutOfIndexesValue, "%s failed\n", LS_AllocFuncName);
  }
  resync_after_run();

  if (sync_threads_and_run_one(1, id))
  {
    LS_index1 = LS_AllocFunc();
    ok(LS_index1 != LS_OutOfIndexesValue, "%s failed\n", LS_AllocFuncName);

    /* Slot indices should be different even if created in different
       threads.  */
    ok(LS_index0 != LS_index1, "%s failed\n", LS_AllocFuncName);

    /* Both slots should be initialized to NULL */
    SetLastError(0xdeadbeef);
    val = LS_GetValueFunc(LS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == NULL, "Slot not initialized correctly\n");

    SetLastError(0xdeadbeef);
    val = LS_GetValueFunc(LS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == NULL, "Slot not initialized correctly\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(0, id))
  {
    SetLastError(0xdeadbeef);
    val = LS_GetValueFunc(LS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == NULL, "Slot not initialized correctly\n");

    SetLastError(0xdeadbeef);
    val = LS_GetValueFunc(LS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == NULL, "Slot not initialized correctly\n");

    ret = LS_SetValueFunc(LS_index0, (LPVOID) 1);
    ok(ret, "%s failed\n", LS_SetValueFuncName);

    ret = LS_SetValueFunc(LS_index1, (LPVOID) 2);
    ok(ret, "%s failed\n", LS_SetValueFuncName);

    SetLastError(0xdeadbeef);
    val = LS_GetValueFunc(LS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == (LPVOID) 1, "Slot not initialized correctly\n");

    SetLastError(0xdeadbeef);
    val = LS_GetValueFunc(LS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == (LPVOID) 2, "Slot not initialized correctly\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(1, id))
  {
    val = LS_GetValueFunc(LS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == NULL, "Slot not initialized correctly\n");

    val = LS_GetValueFunc(LS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == NULL, "Slot not initialized correctly\n");

    ret = LS_SetValueFunc(LS_index0, (LPVOID) 3);
    ok(ret, "%s failed\n", LS_SetValueFuncName);

    ret = LS_SetValueFunc(LS_index1, (LPVOID) 4);
    ok(ret, "%s failed\n", LS_SetValueFuncName);

    val = LS_GetValueFunc(LS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == (LPVOID) 3, "Slot not initialized correctly\n");

    val = LS_GetValueFunc(LS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == (LPVOID) 4, "Slot not initialized correctly\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(0, id))
  {
    HANDLE thread;
    DWORD waitret, tid;

    val = LS_GetValueFunc(LS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == (LPVOID) 1, "Slot not initialized correctly\n");

    val = LS_GetValueFunc(LS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "%s failed\n", LS_GetValueFuncName);
    ok(val == (LPVOID) 2, "Slot not initialized correctly\n");

    thread = CreateThread(NULL, 0, LS_InheritanceProc, 0, 0, &tid);
    ok(thread != NULL, "CreateThread failed\n");
    waitret = WaitForSingleObject(thread, 60000);
    ok(waitret == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    ret = LS_FreeFunc(LS_index0);
    ok(ret, "%s failed\n", LS_FreeFuncName);
  }
  resync_after_run();

  if (sync_threads_and_run_one(1, id))
  {
    ret = LS_FreeFunc(LS_index1);
    ok(ret, "%s failed\n", LS_FreeFuncName);
  }
  resync_after_run();

  return 0;
}

static void run_LS_tests(void)
{
  HANDLE threads[2];
  LONG_PTR i;
  DWORD ret;
  BOOL suc;

  init_thread_sync_helpers();

  /* Allocate a slot in the main thread to test for inheritance.  */
  LS_main = LS_AllocFunc();
  ok(LS_main != LS_OutOfIndexesValue, "%s failed\n", LS_AllocFuncName);
  suc = LS_SetValueFunc(LS_main, (LPVOID) 4114);
  ok(suc, "%s failed\n", LS_SetValueFuncName);

  for (i = 0; i < 2; ++i)
  {
    DWORD tid;

    threads[i] = CreateThread(NULL, 0, LS_ThreadProc, (LPVOID) i, 0, &tid);
    ok(threads[i] != NULL, "CreateThread failed\n");
  }

  ret = WaitForMultipleObjects(2, threads, TRUE, 60000);
  ok(ret == WAIT_OBJECT_0 || broken(ret == WAIT_OBJECT_0+1 /* nt4,w2k */), "WaitForAllObjects 2 threads %ld\n",ret);

  for (i = 0; i < 2; ++i)
    CloseHandle(threads[i]);

  suc = LS_FreeFunc(LS_main);
  ok(suc, "%s failed\n", LS_FreeFuncName);
  cleanup_thread_sync_helpers();
}

static void test_TLS(void)
{
  LS_OutOfIndexesValue = TLS_OUT_OF_INDEXES;

  LS_AllocFunc = &TlsAlloc;
  LS_GetValueFunc = &TlsGetValue;
  LS_SetValueFunc = &TlsSetValue;
  LS_FreeFunc = &TlsFree;

  LS_AllocFuncName = "TlsAlloc";
  LS_GetValueFuncName = "TlsGetValue";
  LS_SetValueFuncName = "TlsSetValue";
  LS_FreeFuncName = "TlsFree";

  run_LS_tests();
}

static void test_FLS(void)
{
  if (!pFlsAlloc || !pFlsFree || !pFlsGetValue || !pFlsSetValue)
  {
     win_skip("Fiber Local Storage not supported\n");
     return;
  }

  LS_OutOfIndexesValue = FLS_OUT_OF_INDEXES;

  LS_AllocFunc = &FLS_AllocFuncThunk;
  LS_GetValueFunc = pFlsGetValue;
  LS_SetValueFunc = pFlsSetValue;
  LS_FreeFunc = pFlsFree;

  LS_AllocFuncName = "FlsAlloc";
  LS_GetValueFuncName = "FlsGetValue";
  LS_SetValueFuncName = "FlsSetValue";
  LS_FreeFuncName = "FlsFree";

  run_LS_tests();
}

static void test_ThreadErrorMode(void)
{
    DWORD oldmode;
    DWORD mode;
    DWORD rtlmode;
    BOOL ret;

    if (!pSetThreadErrorMode || !pGetThreadErrorMode)
    {
        win_skip("SetThreadErrorMode and/or GetThreadErrorMode unavailable (added in Windows 7)\n");
        return;
    }

    if (!pRtlGetThreadErrorMode) {
        win_skip("RtlGetThreadErrorMode not available\n");
        return;
    }

    oldmode = pGetThreadErrorMode();

    ret = pSetThreadErrorMode(0, &mode);
    ok(ret, "SetThreadErrorMode failed\n");
    ok(mode == oldmode,
       "SetThreadErrorMode returned old mode 0x%lx, expected 0x%lx\n",
       mode, oldmode);
    mode = pGetThreadErrorMode();
    ok(mode == 0, "GetThreadErrorMode returned mode 0x%lx, expected 0\n", mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0,
       "RtlGetThreadErrorMode returned mode 0x%lx, expected 0\n", mode);

    ret = pSetThreadErrorMode(SEM_FAILCRITICALERRORS, &mode);
    ok(ret, "SetThreadErrorMode failed\n");
    ok(mode == 0,
       "SetThreadErrorMode returned old mode 0x%lx, expected 0\n", mode);
    mode = pGetThreadErrorMode();
    ok(mode == SEM_FAILCRITICALERRORS,
       "GetThreadErrorMode returned mode 0x%lx, expected SEM_FAILCRITICALERRORS\n",
       mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0x10,
       "RtlGetThreadErrorMode returned mode 0x%lx, expected 0x10\n", mode);

    ret = pSetThreadErrorMode(SEM_NOGPFAULTERRORBOX, &mode);
    ok(ret, "SetThreadErrorMode failed\n");
    ok(mode == SEM_FAILCRITICALERRORS,
       "SetThreadErrorMode returned old mode 0x%lx, expected SEM_FAILCRITICALERRORS\n",
       mode);
    mode = pGetThreadErrorMode();
    ok(mode == SEM_NOGPFAULTERRORBOX,
       "GetThreadErrorMode returned mode 0x%lx, expected SEM_NOGPFAULTERRORBOX\n",
       mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0x20,
       "RtlGetThreadErrorMode returned mode 0x%lx, expected 0x20\n", mode);

    ret = pSetThreadErrorMode(SEM_NOOPENFILEERRORBOX, NULL);
    ok(ret, "SetThreadErrorMode failed\n");
    mode = pGetThreadErrorMode();
    ok(mode == SEM_NOOPENFILEERRORBOX,
       "GetThreadErrorMode returned mode 0x%lx, expected SEM_NOOPENFILEERRORBOX\n",
       mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0x40,
       "RtlGetThreadErrorMode returned mode 0x%lx, expected 0x40\n", rtlmode);

    for (mode = 1; mode; mode <<= 1)
    {
        ret = pSetThreadErrorMode(mode, NULL);
        if (mode & (SEM_FAILCRITICALERRORS |
                    SEM_NOGPFAULTERRORBOX |
                    SEM_NOOPENFILEERRORBOX))
        {
            ok(ret,
               "SetThreadErrorMode(0x%lx,NULL) failed with error %ld\n",
               mode, GetLastError());
        }
        else
        {
            DWORD GLE = GetLastError();
            ok(!ret,
               "SetThreadErrorMode(0x%lx,NULL) succeeded, expected failure\n",
               mode);
            ok(GLE == ERROR_INVALID_PARAMETER,
               "SetThreadErrorMode(0x%lx,NULL) failed with %ld, "
               "expected ERROR_INVALID_PARAMETER\n",
               mode, GLE);
        }
    }

    pSetThreadErrorMode(oldmode, NULL);
}

struct fpu_thread_ctx
{
    unsigned int cw;
    unsigned long fpu_cw;
    HANDLE finished;
};

static inline unsigned long get_fpu_cw(void)
{
#ifdef __arm64ec__
    extern NTSTATUS (*__os_arm64x_get_x64_information)(ULONG,void*,void*);
    unsigned int cw, sse;
    __os_arm64x_get_x64_information( 0, &sse, NULL );
    __os_arm64x_get_x64_information( 2, &cw, NULL );
    return MAKELONG( cw, sse );
#elif defined(__i386__) || defined(__x86_64__)
    WORD cw = 0;
    unsigned int sse = 0;
#ifdef _MSC_VER
#if defined(__REACTOS__) && defined (__x86_64__)
    return 0;
#else
    __asm { fnstcw [cw] }
    __asm { stmxcsr [sse] }
#endif
#else
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
    __asm__ volatile ("stmxcsr %0" : "=m" (sse));
#endif
    return MAKELONG( cw, sse );
#elif defined(__aarch64__)
    ULONG_PTR cw;
    __asm__ __volatile__( "mrs %0, fpcr" : "=r" (cw) );
    return cw;
#else
    return 0;
#endif
}

static inline void fpu_invalid_operation(void)
{
    double d;

#if defined(__i386__)
    unsigned int sse;
#ifdef _MSC_VER
    __asm { stmxcsr [sse] }
    sse |= 1; /* invalid operation flag */
    __asm { ldmxcsr [sse] }
#else
    __asm__ volatile ("stmxcsr %0" : "=m" (sse));
    sse |= 1;
    __asm__ volatile ("ldmxcsr %0" : : "m" (sse));
#endif
#endif

    d = acos(2.0);
    ok(_isnan(d), "d = %lf\n", d);
    ok(_statusfp() & _SW_INVALID, "_statusfp() = %x\n", _statusfp());
}

static DWORD WINAPI fpu_thread(void *param)
{
    struct fpu_thread_ctx *ctx = param;
    BOOL ret;

    ctx->cw = _control87( 0, 0 );
    ctx->fpu_cw = get_fpu_cw();

    ret = SetEvent(ctx->finished);
    ok(ret, "SetEvent failed, last error %#lx.\n", GetLastError());

    return 0;
}

static unsigned int get_thread_fpu_cw( unsigned long *fpu_cw )
{
    struct fpu_thread_ctx ctx;
    DWORD tid, res;
    HANDLE thread;

    ctx.finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!ctx.finished, "Failed to create event, last error %#lx.\n", GetLastError());

    thread = CreateThread(NULL, 0, fpu_thread, &ctx, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#lx.\n", GetLastError());

    res = WaitForSingleObject(ctx.finished, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#lx), last error %#lx.\n", res, GetLastError());

    res = CloseHandle(ctx.finished);
    ok(!!res, "Failed to close event handle, last error %#lx.\n", GetLastError());

    CloseHandle(thread);
    *fpu_cw = ctx.fpu_cw;
    return ctx.cw;
}

static void test_thread_fpu_cw(void)
{
    static const struct {
        unsigned int cw; unsigned long fpu_cw; unsigned long fpu_cw_broken;
    } expected_cw[8] =
    {
#ifdef __i386__
        { _MCW_EM | _PC_53, MAKELONG( 0x27f, 0x1f80 ) },
        { _MCW_EM | _PC_53, MAKELONG( 0x27f, 0x1f80 ) },
        { _EM_INEXACT | _RC_CHOP | _PC_24, MAKELONG( 0xc60, 0x7000 ), MAKELONG( 0xc60, 0x1f80 ) },
        { _MCW_EM | _PC_53, MAKELONG( 0x27f, 0x1f80 ) },
        { _EM_INEXACT | _RC_CHOP | _PC_24, MAKELONG( 0xc60, 0x7000 ), MAKELONG( 0xc60, 0x1f80 ) },
        { _MCW_EM | _PC_53, MAKELONG( 0x27f, 0x1f80 ) },
        { _MCW_EM | _PC_53, MAKELONG( 0x27f, 0x1f81 ) },
        { _MCW_EM | _PC_53, MAKELONG( 0x27f, 0x1f81 ) }
#elif defined(__x86_64__)
        { _MCW_EM | _PC_64, MAKELONG( 0x27f, 0x1f80 ) },
        { _MCW_EM | _PC_64, MAKELONG( 0x27f, 0x1f80 ) },
        { _EM_INEXACT | _RC_CHOP | _PC_64, MAKELONG( 0x27f, 0x7000 ) },
        { _MCW_EM | _PC_64, MAKELONG( 0x27f, 0x1f80 ) },
        { _EM_INEXACT | _RC_CHOP | _PC_64, MAKELONG( 0x27f, 0x7000 ) },
        { _MCW_EM | _PC_64, MAKELONG( 0x27f, 0x1f80 ) },
        { _MCW_EM | _PC_64, MAKELONG( 0x27f, 0x1f81 ) },
        { _MCW_EM | _PC_64, MAKELONG( 0x27f, 0x1f81 ) }
#elif defined(__aarch64__)
        { _MCW_EM | _PC_64, 0 },
        { _MCW_EM | _PC_64, 0 },
        { _EM_INEXACT | _RC_CHOP | _PC_64, 0xc08f00 },
        { _MCW_EM | _PC_64, 0 },
        { _EM_INEXACT | _RC_CHOP | _PC_64, 0xc08f00 },
        { _MCW_EM | _PC_64, 0 },
        { _MCW_EM | _PC_64, 0 },
        { _MCW_EM | _PC_64, 0 }
#else
        { 0xdeadbeef, 0xdeadbeef }
#endif
    };
    unsigned int initial_cw, cw;
    unsigned long fpu_cw;

    fpu_cw = get_fpu_cw();
    initial_cw = _control87( 0, 0 );
    ok(initial_cw == expected_cw[0].cw, "expected %#x got %#x\n", expected_cw[0].cw, initial_cw);
    ok(fpu_cw == expected_cw[0].fpu_cw, "expected %#lx got %#lx\n", expected_cw[0].fpu_cw, fpu_cw);

    cw = get_thread_fpu_cw( &fpu_cw );
    ok(cw == expected_cw[1].cw, "expected %#x got %#x\n", expected_cw[1].cw, cw);
    ok(fpu_cw == expected_cw[1].fpu_cw, "expected %#lx got %#lx\n", expected_cw[1].fpu_cw, fpu_cw);

    _control87( _EM_INEXACT | _RC_CHOP | _PC_24, _MCW_EM | _MCW_RC | _MCW_PC );
    cw = _control87( 0, 0 );
    fpu_cw = get_fpu_cw();
    ok(cw == expected_cw[2].cw, "expected %#x got %#x\n", expected_cw[2].cw, cw);
    ok(fpu_cw == expected_cw[2].fpu_cw ||
            broken(expected_cw[2].fpu_cw_broken && fpu_cw == expected_cw[2].fpu_cw_broken),
        "expected %#lx got %#lx\n", expected_cw[2].fpu_cw, fpu_cw);

    cw = get_thread_fpu_cw( &fpu_cw );
    ok(cw == expected_cw[3].cw, "expected %#x got %#x\n", expected_cw[3].cw, cw);
    ok(fpu_cw == expected_cw[3].fpu_cw, "expected %#lx got %#lx\n", expected_cw[3].fpu_cw, fpu_cw);

    cw = _control87( 0, 0 );
    fpu_cw = get_fpu_cw();
    ok(cw == expected_cw[4].cw, "expected %#x got %#x\n", expected_cw[4].cw, cw);
    ok(fpu_cw == expected_cw[4].fpu_cw ||
            broken(expected_cw[4].fpu_cw_broken && fpu_cw == expected_cw[4].fpu_cw_broken),
        "expected %#lx got %#lx\n", expected_cw[4].fpu_cw, fpu_cw);

    _control87( initial_cw, _MCW_EM | _MCW_RC | _MCW_PC );
    cw = _control87( 0, 0 );
    fpu_cw = get_fpu_cw();
    ok(cw == expected_cw[5].cw, "expected %#x got %#x\n", expected_cw[5].cw, cw);
    ok(fpu_cw == expected_cw[5].fpu_cw, "expected %#lx got %#lx\n", expected_cw[5].fpu_cw, fpu_cw);

    fpu_invalid_operation();
    cw = _control87( 0, 0 );
    fpu_cw = get_fpu_cw();
    ok(cw == expected_cw[6].cw, "expected %#x got %#x\n", expected_cw[6].cw, cw);
    ok(fpu_cw == expected_cw[6].fpu_cw, "expected %#lx got %#lx\n", expected_cw[6].fpu_cw, fpu_cw);

    cw = _control87( initial_cw, _MCW_EM | _MCW_RC | _MCW_PC );
    fpu_cw = get_fpu_cw();
    ok(cw == expected_cw[7].cw, "expected %#x got %#x\n", expected_cw[6].cw, cw);
    ok(fpu_cw == expected_cw[7].fpu_cw, "expected %#lx got %#lx\n", expected_cw[6].fpu_cw, fpu_cw);
    _clearfp();
}

static const char manifest_dep[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"testdep1\" type=\"win32\" processorArchitecture=\"" ARCH "\"/>"
"    <file name=\"testdep.dll\" />"
"</assembly>";

static const char manifest_main[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\" />"
"<dependency>"
" <dependentAssembly>"
"  <assemblyIdentity type=\"win32\" name=\"testdep1\" version=\"1.2.3.4\" processorArchitecture=\"" ARCH "\" />"
" </dependentAssembly>"
"</dependency>"
"</assembly>";

static void create_manifest_file(const char *filename, const char *manifest)
{
    WCHAR path[MAX_PATH];
    HANDLE file;
    DWORD size;

    MultiByteToWideChar( CP_ACP, 0, filename, -1, path, MAX_PATH );
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    WriteFile(file, manifest, strlen(manifest), &size, NULL);
    CloseHandle(file);
}

static HANDLE test_create(const char *file)
{
    WCHAR path[MAX_PATH];
    ACTCTXW actctx;
    HANDLE handle;

    MultiByteToWideChar(CP_ACP, 0, file, -1, path, MAX_PATH);
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = CreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create context, error %lu\n", GetLastError());

    ok(actctx.cbSize == sizeof(actctx), "cbSize=%ld\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "dwFlags=%ld\n", actctx.dwFlags);
    ok(actctx.lpSource == path, "lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0, "wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL, "lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "lpApplicationName=%p\n", actctx.lpApplicationName);
    ok(actctx.hModule == NULL, "hModule=%p\n", actctx.hModule);

    return handle;
}

static void test_thread_actctx(void)
{
    struct thread_actctx_param param;
    HANDLE thread, handle, context;
    ULONG_PTR cookie;
    DWORD tid, ret;
    BOOL b;

    create_manifest_file("testdep1.manifest", manifest_dep);
    create_manifest_file("main.manifest", manifest_main);

    context = test_create("main.manifest");
    DeleteFileA("testdep1.manifest");
    DeleteFileA("main.manifest");

    handle = (void*)0xdeadbeef;
    b = GetCurrentActCtx(&handle);
    ok(b, "GetCurrentActCtx failed: %lu\n", GetLastError());
    ok(handle == 0, "active context %p\n", handle);

    /* without active context */
    param.thread_context = (void*)0xdeadbeef;
    param.handle = NULL;
    thread = CreateThread(NULL, 0, thread_actctx_func, &param, 0, &tid);
    ok(thread != NULL, "failed, got %lu\n", GetLastError());

    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "wait timeout\n");
    ok(param.thread_context == NULL, "got wrong thread context %p\n", param.thread_context);
    CloseHandle(thread);

    b = ActivateActCtx(context, &cookie);
    ok(b, "activation failed: %lu\n", GetLastError());

    handle = 0;
    b = GetCurrentActCtx(&handle);
    ok(b, "GetCurrentActCtx failed: %lu\n", GetLastError());
    ok(handle != 0, "no active context\n");
    ReleaseActCtx(handle);

    param.handle = NULL;
    b = GetCurrentActCtx(&param.handle);
    ok(b && param.handle != NULL, "failed to get context, %lu\n", GetLastError());

    param.thread_context = (void*)0xdeadbeef;
    thread = CreateThread(NULL, 0, thread_actctx_func, &param, 0, &tid);
    ok(thread != NULL, "failed, got %lu\n", GetLastError());

    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "wait timeout\n");
    ok(param.thread_context == context, "got wrong thread context %p, %p\n", param.thread_context, context);
    ReleaseActCtx(param.thread_context);
    CloseHandle(thread);

    /* similar test for CreateRemoteThread() */
    param.thread_context = (void*)0xdeadbeef;
    thread = CreateRemoteThread(GetCurrentProcess(), NULL, 0, thread_actctx_func, &param, 0, &tid);
    ok(thread != NULL, "failed, got %lu\n", GetLastError());

    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "wait timeout\n");
    ok(param.thread_context == context, "got wrong thread context %p, %p\n", param.thread_context, context);
    ReleaseActCtx(param.thread_context);
    CloseHandle(thread);

    ReleaseActCtx(param.handle);

    b = DeactivateActCtx(0, cookie);
    ok(b, "DeactivateActCtx failed: %lu\n", GetLastError());
    ReleaseActCtx(context);
}

static void WINAPI threadpool_workcallback(PTP_CALLBACK_INSTANCE instance, void *context, PTP_WORK work) {
    int *foo = (int*)context;

    (*foo)++;
}


static void test_threadpool(void)
{
    PTP_POOL pool;
    PTP_WORK work;
    int workcalled = 0;

    if (!pCreateThreadpool) {
        win_skip("thread pool apis not supported.\n");
        return;
    }

    work = pCreateThreadpoolWork(threadpool_workcallback, &workcalled, NULL);
    ok (work != NULL, "Error %ld in CreateThreadpoolWork\n", GetLastError());
    pSubmitThreadpoolWork(work);
    pWaitForThreadpoolWorkCallbacks(work, FALSE);
    pCloseThreadpoolWork(work);

    ok (workcalled == 1, "expected work to be called once, got %d\n", workcalled);

    pool = pCreateThreadpool(NULL);
    ok (pool != NULL, "CreateThreadpool failed\n");
    pCloseThreadpool(pool);
}

static void test_reserved_tls(void)
{
    void *val;
    DWORD tls;
    BOOL ret;

    /* This seems to be a WinXP SP2+ feature. */
    if(!pIsWow64Process) {
        win_skip("Skipping reserved TLS slot on too old Windows.\n");
        return;
    }

    val = TlsGetValue(0);
    ok(!val, "TlsGetValue(0) = %p\n", val);

    /* Also make sure that there is a TLS allocated. */
    tls = TlsAlloc();
    ok(tls && tls != TLS_OUT_OF_INDEXES, "tls = %lx\n", tls);
    TlsSetValue(tls, (void*)1);

    val = TlsGetValue(0);
    ok(!val, "TlsGetValue(0) = %p\n", val);

    TlsFree(tls);

    /* The following is too ugly to be run by default */
    if(0) {
        /* Set TLS index 0 value and see that this works and doesn't cause problems
         * for remaining tests. */
        ret = TlsSetValue(0, (void*)1);
        ok(ret, "TlsSetValue(0, 1) failed: %lu\n", GetLastError());

        val = TlsGetValue(0);
        ok(val == (void*)1, "TlsGetValue(0) = %p\n", val);
    }
}

static void test_thread_info(void)
{
    char buf[4096];
    static const ULONG info_size[] =
    {
        sizeof(THREAD_BASIC_INFORMATION), /* ThreadBasicInformation */
        sizeof(KERNEL_USER_TIMES), /* ThreadTimes */
        sizeof(ULONG), /* ThreadPriority */
        sizeof(ULONG), /* ThreadBasePriority */
        sizeof(ULONG_PTR), /* ThreadAffinityMask */
        sizeof(HANDLE), /* ThreadImpersonationToken */
        sizeof(THREAD_DESCRIPTOR_INFORMATION), /* ThreadDescriptorTableEntry */
        sizeof(BOOLEAN), /* ThreadEnableAlignmentFaultFixup */
        0, /* ThreadEventPair_Reusable */
        sizeof(ULONG_PTR), /* ThreadQuerySetWin32StartAddress */
        sizeof(ULONG), /* ThreadZeroTlsCell */
        sizeof(LARGE_INTEGER), /* ThreadPerformanceCount */
        sizeof(ULONG), /* ThreadAmILastThread */
        sizeof(ULONG), /* ThreadIdealProcessor */
        sizeof(ULONG), /* ThreadPriorityBoost */
        sizeof(ULONG_PTR), /* ThreadSetTlsArrayAddress */
        sizeof(ULONG), /* ThreadIsIoPending */
        sizeof(BOOLEAN), /* ThreadHideFromDebugger */
        /* FIXME: Add remaining classes */
    };
    HANDLE thread;
    ULONG i, status, ret_len;

    if (!pOpenThread)
    {
        win_skip("OpenThread is not available on this platform\n");
        return;
    }

    if (!pNtQueryInformationThread)
    {
        win_skip("NtQueryInformationThread is not available on this platform\n");
        return;
    }

    thread = pOpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentThreadId());
    if (!thread)
    {
        win_skip("THREAD_QUERY_LIMITED_INFORMATION is not supported on this platform\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(info_size); i++)
    {
        memset(buf, 0, sizeof(buf));

#ifdef __i386__
        if (i == ThreadDescriptorTableEntry)
        {
            CONTEXT ctx;
            THREAD_DESCRIPTOR_INFORMATION *tdi = (void *)buf;

            ctx.ContextFlags = CONTEXT_SEGMENTS;
            GetThreadContext(GetCurrentThread(), &ctx);
            tdi->Selector = ctx.SegDs;
        }
#endif
        ret_len = 0;
        status = pNtQueryInformationThread(thread, i, buf, info_size[i], &ret_len);
        if (status == STATUS_NOT_IMPLEMENTED) continue;
        if (status == STATUS_INVALID_INFO_CLASS) continue;
        if (status == STATUS_UNSUCCESSFUL) continue;

        switch (i)
        {
        case ThreadBasicInformation:
        case ThreadAmILastThread:
        case ThreadPriorityBoost:
            ok(status == STATUS_SUCCESS, "for info %lu expected STATUS_SUCCESS, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;

#ifdef __i386__
        case ThreadDescriptorTableEntry:
            ok(status == STATUS_SUCCESS || broken(status == STATUS_ACCESS_DENIED) /* testbot VM is broken */,
               "for info %lu expected STATUS_SUCCESS, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;
#endif

        case ThreadTimes:
            ok(status == STATUS_SUCCESS, "for info %lu expected STATUS_SUCCESS, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;

        case ThreadIsIoPending:
            todo_wine
            ok(status == STATUS_ACCESS_DENIED, "for info %lu expected STATUS_ACCESS_DENIED, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;

        default:
            ok(status == STATUS_ACCESS_DENIED, "for info %lu expected STATUS_ACCESS_DENIED, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;
        }
    }

    CloseHandle(thread);
}

typedef struct tagTHREADNAME_INFO
{
    DWORD   dwType;     /* Must be 0x1000. */
    LPCSTR  szName;     /* Pointer to name (in user addr space). */
    DWORD   dwThreadID; /* Thread ID (-1 = caller thread). */
    DWORD   dwFlags;    /* Reserved for future use, must be zero. */
} THREADNAME_INFO;

static LONG CALLBACK msvc_threadname_vec_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    if (ExceptionInfo->ExceptionRecord != NULL &&
        ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_WINE_NAME_THREAD)
        return EXCEPTION_CONTINUE_EXECUTION;

    return EXCEPTION_CONTINUE_SEARCH;
}

static void test_thread_description(void)
{
    THREAD_NAME_INFORMATION *thread_desc;
    static const WCHAR *desc = L"thread_desc";
    ULONG len, len2, desc_len;
    NTSTATUS status;
    char buff[128];
    WCHAR *ptr;
    HRESULT hr;
    HANDLE thread;
    PVOID vectored_handler;
    THREADNAME_INFO info;

    if (!pGetThreadDescription)
    {
        win_skip("Thread description API is not supported.\n");
        return;
    }

    desc_len = lstrlenW(desc) * sizeof(*desc);
    thread_desc = (THREAD_NAME_INFORMATION *)buff;

    /* Initial description. */
    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, L""), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    len = 0;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadNameInformation, NULL, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Unexpected status %#lx.\n", status);
    ok(len == sizeof(*thread_desc), "Unexpected structure length %lu.\n", len);

    len2 = 0;
    thread_desc->ThreadName.Length = 1;
    thread_desc->ThreadName.MaximumLength = 0;
    thread_desc->ThreadName.Buffer = (WCHAR *)thread_desc;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadNameInformation, thread_desc, len, &len2);
    ok(!status, "Failed to get thread info, status %#lx.\n", status);
    ok(len2 == sizeof(*thread_desc), "Unexpected structure length %lu.\n", len);
    ok(!thread_desc->ThreadName.Length, "Unexpected description length %#x.\n", thread_desc->ThreadName.Length);
    ok(thread_desc->ThreadName.Buffer == (WCHAR *)(thread_desc + 1),
            "Unexpected description string pointer %p, %p.\n", thread_desc->ThreadName.Buffer, thread_desc);

    hr = pSetThreadDescription(GetCurrentThread(), NULL);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    hr = pSetThreadDescription(GetCurrentThread(), desc);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, desc), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    len = 0;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadNameInformation, NULL, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Failed to get thread info, status %#lx.\n", status);
    ok(len == sizeof(*thread_desc) + desc_len, "Unexpected structure length %lu.\n", len);

    len = 0;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadNameInformation, buff, sizeof(buff), &len);
    ok(!status, "Failed to get thread info.\n");
    ok(len == sizeof(*thread_desc) + desc_len, "Unexpected structure length %lu.\n", len);

    ok(thread_desc->ThreadName.Length == desc_len && thread_desc->ThreadName.MaximumLength == desc_len,
            "Unexpected description length %u.\n", thread_desc->ThreadName.Length);
    ok(thread_desc->ThreadName.Buffer == (WCHAR *)(thread_desc + 1),
            "Unexpected description string pointer %p, %p.\n", thread_desc->ThreadName.Buffer, thread_desc);
    ok(!memcmp(thread_desc->ThreadName.Buffer, desc, desc_len), "Unexpected description string.\n");

    /* Partial results. */
    len = 0;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadNameInformation, NULL, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Unexpected status %#lx.\n", status);
    ok(len == sizeof(*thread_desc) + desc_len, "Unexpected structure length %lu.\n", len);

    status = pNtQueryInformationThread(GetCurrentThread(), ThreadNameInformation, buff, len - sizeof(WCHAR), &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Unexpected status %#lx.\n", status);
    ok(len == sizeof(*thread_desc) + desc_len, "Unexpected structure length %lu.\n", len);

    /* Change description. */
    thread_desc->ThreadName.Length = thread_desc->ThreadName.MaximumLength = 8;
    lstrcpyW((WCHAR *)(thread_desc + 1), L"desc");

    status = pNtSetInformationThread(GetCurrentThread(), ThreadNameInformation, thread_desc, sizeof(*thread_desc));
    ok(status == STATUS_SUCCESS, "Failed to set thread description, status %#lx.\n", status);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, L"desc"), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    status = pNtSetInformationThread(GetCurrentThread(), ThreadNameInformation, thread_desc, sizeof(*thread_desc) - 1);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#lx.\n", status);

    status = NtSetInformationThread(GetCurrentThread(), ThreadNameInformation, NULL, sizeof(*thread_desc));
    ok(status == STATUS_ACCESS_VIOLATION, "Unexpected status %#lx.\n", status);

    thread_desc->ThreadName.Buffer = NULL;
    status = pNtSetInformationThread(GetCurrentThread(), ThreadNameInformation, thread_desc, sizeof(*thread_desc));
    ok(status == STATUS_ACCESS_VIOLATION, "Unexpected status %#lx.\n", status);

    hr = pSetThreadDescription(GetCurrentThread(), NULL);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, L""), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    /* Set with a string from RtlInitUnicodeString. */
    hr = pSetThreadDescription(GetCurrentThread(), L"123");
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    lstrcpyW((WCHAR *)(thread_desc + 1), L"desc");
    RtlInitUnicodeString(&thread_desc->ThreadName, (WCHAR *)(thread_desc + 1));

    status = pNtSetInformationThread(GetCurrentThread(), ThreadNameInformation, thread_desc, sizeof(*thread_desc));
    ok(status == STATUS_SUCCESS, "Failed to set thread description, status %#lx.\n", status);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, L"desc"), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    /* Set with 0 length/NULL pointer. */
    hr = pSetThreadDescription(GetCurrentThread(), L"123");
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    memset(thread_desc, 0, sizeof(*thread_desc));
    status = pNtSetInformationThread(GetCurrentThread(), ThreadNameInformation, thread_desc, sizeof(*thread_desc));
    ok(!status, "Failed to set thread description, status %#lx.\n", status);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, L""), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    /* Get with only THREAD_QUERY_LIMITED_INFORMATION access. */
    thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentThreadId());

    ptr = NULL;
    hr = pGetThreadDescription(thread, &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, L""), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    len = 0;
    status = pNtQueryInformationThread(thread, ThreadNameInformation, NULL, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Unexpected status %#lx.\n", status);
    ok(len == sizeof(*thread_desc), "Unexpected structure length %lu.\n", len);

    CloseHandle(thread);

    /* Set with only THREAD_SET_LIMITED_INFORMATION access. */
    thread = OpenThread(THREAD_SET_LIMITED_INFORMATION, FALSE, GetCurrentThreadId());

    hr = pSetThreadDescription(thread, desc);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, desc), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);

    CloseHandle(thread);

    /* The old exception-based thread name method should not affect GetThreadDescription. */
    hr = pSetThreadDescription(GetCurrentThread(), desc);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to set thread description, hr %#lx.\n", hr);

    vectored_handler = pRtlAddVectoredExceptionHandler(FALSE, &msvc_threadname_vec_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    info.dwType = 0x1000;
    info.szName = "123";
    info.dwThreadID = -1;
    info.dwFlags = 0;
    RaiseException(EXCEPTION_WINE_NAME_THREAD, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);

    ptr = NULL;
    hr = pGetThreadDescription(GetCurrentThread(), &ptr);
    ok(hr == HRESULT_FROM_NT(STATUS_SUCCESS), "Failed to get thread description, hr %#lx.\n", hr);
    ok(!lstrcmpW(ptr, desc), "Unexpected description %s.\n", wine_dbgstr_w(ptr));
    LocalFree(ptr);
}

static void init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

/* Neither Cygwin nor mingW export OpenThread, so do a dynamic check
   so that the compile passes */

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f)
    X(GetCurrentThreadStackLimits);
    X(GetThreadPriorityBoost);
    X(OpenThread);
    X(QueueUserWorkItem);
    X(SetThreadPriorityBoost);
    X(SetThreadStackGuarantee);
    X(RegisterWaitForSingleObject);
    X(UnregisterWait);
    X(IsWow64Process);
    X(SetThreadErrorMode);
    X(GetThreadErrorMode);

    X(CreateThreadpool);
    X(CloseThreadpool);
    X(CreateThreadpoolWork);
    X(SubmitThreadpoolWork);
    X(WaitForThreadpoolWorkCallbacks);
    X(CloseThreadpoolWork);

    X(GetThreadGroupAffinity);
    X(SetThreadGroupAffinity);
    X(SetThreadDescription);
    X(GetThreadDescription);

    X(FlsAlloc);
    X(FlsFree);
    X(FlsSetValue);
    X(FlsGetValue);
#undef X

#define X(f) p##f = (void*)GetProcAddress(ntdll, #f)
   if (ntdll)
   {
       X(NtQueryInformationThread);
       X(RtlGetThreadErrorMode);
       X(NtSetInformationThread);
       X(RtlAddVectoredExceptionHandler);
       X(RtlRemoveVectoredExceptionHandler);
   }
#undef X
}

START_TEST(thread)
{
   int argc;
   char **argv;
   argc = winetest_get_mainargs( &argv );

   init_funcs();

   if (argc >= 3)
   {
       if (!strcmp(argv[2], "sleep"))
       {
           HANDLE hAddrEvents[2];
           create_function_addr_events(hAddrEvents);
           SetEvent(hAddrEvents[0]);
           SetEvent(hAddrEvents[1]);
           Sleep(5000); /* spawned process runs for at most 5 seconds */
           return;
       }
       while (1)
       {
           HANDLE hThread;
           DWORD tid;
           hThread = CreateThread(NULL, 0, threadFunc2, NULL, 0, &tid);
           ok(hThread != NULL, "CreateThread failed, error %lu\n",
              GetLastError());
           ok(WaitForSingleObject(hThread, 200) == WAIT_OBJECT_0,
              "Thread did not exit in time\n");
           if (hThread == NULL) break;
           CloseHandle(hThread);
       }
       return;
   }

   test_thread_info();
   test_reserved_tls();
   test_CreateRemoteThread();
   test_CreateThread_basic();
   test_CreateThread_suspended();
   test_SuspendThread();
   test_TerminateThread();
   test_CreateThread_stack();
   test_thread_priority();
   test_GetCurrentThreadStackLimits();
   test_SetThreadStackGuarantee();
   test_GetThreadTimes();
   test_thread_processor();
   test_GetThreadExitCode();
#ifdef __i386__
   test_SetThreadContext();
   test_GetThreadSelectorEntry();
   test_GetThreadContext();
#endif
   test_QueueUserWorkItem();
   test_RegisterWaitForSingleObject();
   test_TLS();
   test_FLS();
   test_ThreadErrorMode();
   test_thread_fpu_cw();
   test_thread_actctx();
   test_thread_description();

   test_threadpool();
}
