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

/* Define _WIN32_WINNT to get SetThreadIdealProcessor on Windows */
#define _WIN32_WINNT 0x0600

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

/* the tests intentionally pass invalid pointers and need an exception handler */
#define WINE_NO_INLINE_STRING

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>
#include <winnls.h>
#include <wine/winternl.h>
#include <wine/test.h>

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

static BOOL (WINAPI *pGetThreadPriorityBoost)(HANDLE,PBOOL);
static HANDLE (WINAPI *pOpenThread)(DWORD,BOOL,DWORD);
static BOOL (WINAPI *pQueueUserWorkItem)(LPTHREAD_START_ROUTINE,PVOID,ULONG);
static DWORD (WINAPI *pSetThreadIdealProcessor)(HANDLE,DWORD);
static BOOL (WINAPI *pSetThreadPriorityBoost)(HANDLE,BOOL);
static BOOL (WINAPI *pRegisterWaitForSingleObject)(PHANDLE,HANDLE,WAITORTIMERCALLBACK,PVOID,ULONG,ULONG);
static BOOL (WINAPI *pUnregisterWait)(HANDLE);
static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);
static BOOL (WINAPI *pSetThreadErrorMode)(DWORD,PDWORD);
static DWORD (WINAPI *pGetThreadErrorMode)(void);
static DWORD (WINAPI *pRtlGetThreadErrorMode)(void);
static BOOL   (WINAPI *pActivateActCtx)(HANDLE,ULONG_PTR*);
static HANDLE (WINAPI *pCreateActCtxW)(PCACTCTXW);
static BOOL   (WINAPI *pDeactivateActCtx)(DWORD,ULONG_PTR);
static BOOL   (WINAPI *pGetCurrentActCtx)(HANDLE *);
static void   (WINAPI *pReleaseActCtx)(HANDLE);
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
    ok(ret, "error: %u\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
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
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed %x\n",ret);
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
    ret = pGetCurrentActCtx(&cur);
    ok(ret, "thread GetCurrentActCtx failed, %u\n", GetLastError());
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
    ok(ret == WAIT_OBJECT_0 || broken(ret == WAIT_OBJECT_0+1 /* nt4,w2k */), "WaitForAllObjects 2 events %d\n", ret);

    hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(hEvent != NULL, "Can't create event, err=%u\n", GetLastError());
    ret = DuplicateHandle(GetCurrentProcess(), hEvent, hProcess, &hRemoteEvent,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret != 0, "DuplicateHandle failed, err=%u\n", GetLastError());

    /* create suspended remote thread with entry point SetEvent() */
    SetLastError(0xdeadbeef);
    hThread = CreateRemoteThread(hProcess, NULL, 0, threadFunc_SetEvent,
                                 hRemoteEvent, CREATE_SUSPENDED, &tid);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("CreateRemoteThread is not implemented\n");
        goto cleanup;
    }
    ok(hThread != NULL, "CreateRemoteThread failed, err=%u\n", GetLastError());
    ok(tid != 0, "null tid\n");
    ret = SuspendThread(hThread);
    ok(ret == 1, "ret=%u, err=%u\n", ret, GetLastError());
    ret = ResumeThread(hThread);
    ok(ret == 2, "ret=%u, err=%u\n", ret, GetLastError());

    /* thread still suspended, so wait times out */
    ret = WaitForSingleObject(hEvent, 1000);
    ok(ret == WAIT_TIMEOUT, "wait did not time out, ret=%u\n", ret);

    ret = ResumeThread(hThread);
    ok(ret == 1, "ret=%u, err=%u\n", ret, GetLastError());

    /* wait that doesn't time out */
    ret = WaitForSingleObject(hEvent, 1000);
    ok(ret == WAIT_OBJECT_0, "object not signaled, ret=%u\n", ret);

    /* wait for thread end */
    ret = WaitForSingleObject(hThread, 1000);
    ok(ret == WAIT_OBJECT_0, "waiting for thread failed, ret=%u\n", ret);
    CloseHandle(hThread);

    /* create and wait for remote thread with entry point CloseHandle() */
    hThread = CreateRemoteThread(hProcess, NULL, 0,
                                 threadFunc_CloseHandle,
                                 hRemoteEvent, 0, &tid);
    ok(hThread != NULL, "CreateRemoteThread failed, err=%u\n", GetLastError());
    ret = WaitForSingleObject(hThread, 1000);
    ok(ret == WAIT_OBJECT_0, "waiting for thread failed, ret=%u\n", ret);
    CloseHandle(hThread);

    /* create remote thread with entry point SetEvent() */
    hThread = CreateRemoteThread(hProcess, NULL, 0,
                                 threadFunc_SetEvent,
                                 hRemoteEvent, 0, &tid);
    ok(hThread != NULL, "CreateRemoteThread failed, err=%u\n", GetLastError());

    /* closed handle, so wait times out */
    ret = WaitForSingleObject(hEvent, 1000);
    ok(ret == WAIT_TIMEOUT, "wait did not time out, ret=%u\n", ret);

    /* check that remote SetEvent() failed */
    ret = GetExitCodeThread(hThread, &exitcode);
    ok(ret != 0, "GetExitCodeThread failed, err=%u\n", GetLastError());
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
  ok(bRet, "TlsFree failed: %08x\n", GetLastError());
  ok(GetLastError()==0xCAFEF00D,
     "GetLastError: expected 0xCAFEF00D, got %08x\n", GetLastError());

  /* Test freeing an already freed TLS index */
  SetLastError(0xCAFEF00D);
  ok(TlsFree(tlsIndex)==0,"TlsFree succeeded\n");
  ok(GetLastError()==ERROR_INVALID_PARAMETER,
     "GetLastError: expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());

  /* Test how passing NULL as a pointer to threadid works */
  SetLastError(0xFACEaBAD);
  thread[0] = CreateThread(NULL,0,threadFunc2,NULL,0,&tid);
  GLE = GetLastError();
  if (thread[0]) { /* NT */
    ok(GLE==0xFACEaBAD, "CreateThread set last error to %d, expected 4207848365\n", GLE);
    ret = WaitForSingleObject(thread[0],100);
    ok(ret==WAIT_OBJECT_0, "threadFunc2 did not exit during 100 ms\n");
    ret = GetExitCodeThread(thread[0],&exitCode);
    ok(ret!=0, "GetExitCodeThread returned %d (expected nonzero)\n", ret);
    ok(exitCode==99, "threadFunc2 exited with code: %d (expected 99)\n", exitCode);
    ok(CloseHandle(thread[0])!=0,"Error closing thread handle\n");
  }
  else { /* 9x */
    ok(GLE==ERROR_INVALID_PARAMETER, "CreateThread set last error to %d, expected 87\n", GLE);
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
  ok(suspend_count == -1, "SuspendThread returned %d, expected -1\n", suspend_count);

  suspend_count = ResumeThread(thread);
  ok(suspend_count == 0 ||
     broken(suspend_count == -1), /* win9x */
     "ResumeThread returned %d, expected 0\n", suspend_count);

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
  ok(error==~0U, "wrong return code: %d\n", error);
  ok(GetLastError()==ERROR_ACCESS_DENIED || GetLastError()==ERROR_NO_MORE_ITEMS, "unexpected error code: %d\n", GetLastError());

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
      "SetThreadPriority error %d, expected ERROR_INVALID_PARAMETER or ERROR_INVALID_PRIORITY\n",
      GetLastError());
   ok(GetThreadPriority(curthread)==min_priority,
      "GetThreadPriority didn't return min_priority\n");

   SetThreadPriority(curthread,max_priority);
   SetLastError(0xdeadbeef);
   rc = SetThreadPriority(curthread,max_priority+1);

   ok(rc == FALSE, "SetThreadPriority passed with a bad argument\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER ||
      GetLastError() == ERROR_INVALID_PRIORITY /* Win9x */,
      "SetThreadPriority error %d, expected ERROR_INVALID_PARAMETER or ERROR_INVALID_PRIORITY\n",
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

   ok(rc!=0,"error=%d\n",GetLastError());

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
   ok( rc != 0, "error=%d\n",GetLastError());
   todo_wine {
     rc=pGetThreadPriorityBoost(curthread,&disabled);
     ok(rc!=0 && disabled==1,
        "rc=%d error=%d disabled=%d\n",rc,GetLastError(),disabled);
   }

   rc = pSetThreadPriorityBoost(curthread,0);
   ok( rc != 0, "error=%d\n",GetLastError());
   rc=pGetThreadPriorityBoost(curthread,&disabled);
   ok(rc!=0 && disabled==0,
      "rc=%d error=%d disabled=%d\n",rc,GetLastError(),disabled);
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
   int error=0;
   BOOL is_wow64;

   if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

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
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS in NtSetInformationThread, got %x\n", status);
    }

   if (retMask == processMask && sizeof(ULONG_PTR) > sizeof(ULONG))
   {
       /* only the low 32-bits matter */
       retMask = SetThreadAffinityMask(curthread,~(ULONG_PTR)0);
       ok(retMask == processMask, "SetThreadAffinityMask failed\n");
       retMask = SetThreadAffinityMask(curthread,~(ULONG_PTR)0 >> 3);
       ok(retMask == processMask, "SetThreadAffinityMask failed\n");
   }
/* NOTE: This only works on WinNT/2000/XP) */
    if (pSetThreadIdealProcessor)
    {
        SetLastError(0xdeadbeef);
        error=pSetThreadIdealProcessor(curthread,0);
        if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        {
            ok(error!=-1, "SetThreadIdealProcessor failed\n");

            if (is_wow64)
            {
                SetLastError(0xdeadbeef);
                error=pSetThreadIdealProcessor(curthread,MAXIMUM_PROCESSORS+1);
                todo_wine
                ok(error!=-1, "SetThreadIdealProcessor failed for %u on Wow64\n", MAXIMUM_PROCESSORS+1);

                SetLastError(0xdeadbeef);
                error=pSetThreadIdealProcessor(curthread,65);
                ok(error==-1, "SetThreadIdealProcessor succeeded with an illegal processor #\n");
                ok(GetLastError()==ERROR_INVALID_PARAMETER,
                   "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
            }
            else
            {
                SetLastError(0xdeadbeef);
                error=pSetThreadIdealProcessor(curthread,MAXIMUM_PROCESSORS+1);
                ok(error==-1, "SetThreadIdealProcessor succeeded with an illegal processor #\n");
                ok(GetLastError()==ERROR_INVALID_PARAMETER,
                   "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
            }

            error=pSetThreadIdealProcessor(curthread,MAXIMUM_PROCESSORS);
            ok(error!=-1, "SetThreadIdealProcessor failed\n");
        }
        else
            win_skip("SetThreadIdealProcessor is not implemented\n");
    }

    if (pGetThreadGroupAffinity && pSetThreadGroupAffinity)
    {
        GROUP_AFFINITY affinity, affinity_new;
        NTSTATUS status;

        memset(&affinity, 0, sizeof(affinity));
        ok(pGetThreadGroupAffinity(curthread, &affinity), "GetThreadGroupAffinity failed\n");

        SetLastError(0xdeadbeef);
        ok(!pGetThreadGroupAffinity(curthread, NULL), "GetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_NOACCESS), /* Win 7 and 8 */
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        ok(affinity.Group == 0, "Expected group 0 got %u\n", affinity.Group);

        memset(&affinity_new, 0, sizeof(affinity_new));
        affinity_new.Group = 0;
        affinity_new.Mask  = affinity.Mask;
        ok(pSetThreadGroupAffinity(curthread, &affinity_new, &affinity), "SetThreadGroupAffinity failed\n");
        ok(affinity_new.Mask == affinity.Mask, "Expected old affinity mask %lx, got %lx\n",
           affinity_new.Mask, affinity.Mask);

        /* show that the "all processors" flag is not supported for SetThreadGroupAffinity */
        affinity_new.Group = 0;
        affinity_new.Mask  = ~0u;
        SetLastError(0xdeadbeef);
        ok(!pSetThreadGroupAffinity(curthread, &affinity_new, NULL), "SetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

        affinity_new.Group = 1; /* assumes that you have less than 64 logical processors */
        affinity_new.Mask  = 0x1;
        SetLastError(0xdeadbeef);
        ok(!pSetThreadGroupAffinity(curthread, &affinity_new, NULL), "SetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        ok(!pSetThreadGroupAffinity(curthread, NULL, NULL), "SetThreadGroupAffinity succeeded\n");
        ok(GetLastError() == ERROR_NOACCESS,
           "Expected ERROR_NOACCESS, got %d\n", GetLastError());

        /* show that the access violation was detected in ntdll */
        status = pNtSetInformationThread(curthread, ThreadGroupInformation, NULL, sizeof(affinity_new));
        ok(status == STATUS_ACCESS_VIOLATION,
           "Expected STATUS_ACCESS_VIOLATION, got %08x\n", status);

        /* restore original mask */
        affinity_new.Group = 0;
        affinity_new.Mask  = affinity.Mask;
        SetLastError(0xdeadbeef);
        ok(pSetThreadGroupAffinity(curthread, &affinity_new, &affinity), "SetThreadGroupAffinity failed\n");
        ok(affinity_new.Mask == affinity.Mask, "Expected old affinity mask %lx, got %lx\n",
           affinity_new.Mask, affinity.Mask);
    }
    else
        win_skip("Get/SetThreadGroupAffinity not available\n");
}

static VOID test_GetThreadExitCode(void)
{
    DWORD exitCode, threadid;
    DWORD GLE, ret;
    HANDLE thread;

    ret = GetExitCodeThread((HANDLE)0x2bad2bad,&exitCode);
    ok(ret==0, "GetExitCodeThread returned non zero value: %d\n", ret);
    GLE = GetLastError();
    ok(GLE==ERROR_INVALID_HANDLE, "GetLastError returned %d (expected 6)\n", GLE);

    thread = CreateThread(NULL,0,threadFunc2,NULL,0,&threadid);
    ret = WaitForSingleObject(thread,100);
    ok(ret==WAIT_OBJECT_0, "threadFunc2 did not exit during 100 ms\n");
    ret = GetExitCodeThread(thread,&exitCode);
    ok(ret==exitCode || ret==1, 
       "GetExitCodeThread returned %d (expected 1 or %d)\n", ret, exitCode);
    ok(exitCode==99, "threadFunc2 exited with code %d (expected 99)\n", exitCode);
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
    ok( thread != NULL, "CreateThread failed : (%d)\n", GetLastError() );
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
    ok( ret, "GetThreadContext failed : (%u)\n", GetLastError() );

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
        ok( ret, "SetThreadContext failed : (%d)\n", GetLastError() );
    }

    SetLastError(0xdeadbeef);
    prevcount = ResumeThread( thread );
    ok ( prevcount == 1, "Previous suspend count (%d) instead of 1, last error : (%d)\n",
                         prevcount, GetLastError() );

    WaitForSingleObject( thread, INFINITE );
    ok( test_value == 10, "test_value %d\n", test_value );

    ctx.ContextFlags = CONTEXT_FULL;
    SetLastError(0xdeadbeef);
    ret = GetThreadContext( thread, &ctx );
    ok( (!ret && (GetLastError() == ERROR_GEN_FAILURE)) ||
        (!ret && broken(GetLastError() == ERROR_INVALID_HANDLE)) || /* win2k */
        broken(ret),   /* 32bit application on NT 5.x 64bit */
        "got %d with %u (expected FALSE with ERROR_GEN_FAILURE)\n",
        ret, GetLastError() );

    SetLastError(0xdeadbeef);
    ret = SetThreadContext( thread, &ctx );
    ok( (!ret && ((GetLastError() == ERROR_GEN_FAILURE) || (GetLastError() == ERROR_ACCESS_DENIED))) ||
        (!ret && broken(GetLastError() == ERROR_INVALID_HANDLE)) || /* win2k */
        broken(ret),   /* 32bit application on NT 5.x 64bit */
        "got %d with %u (expected FALSE with ERROR_GEN_FAILURE or ERROR_ACCESS_DENIED)\n",
        ret, GetLastError() );

    CloseHandle( thread );
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
        ok(ret, "QueueUserWorkItem failed with error %d\n", GetLastError());
    }

    wait_result = WaitForSingleObject(finish_event, 10000);

    after = GetTickCount();
    trace("100 QueueUserWorkItem calls took %dms\n", after - before);
    ok(wait_result == WAIT_OBJECT_0, "wait failed with error 0x%x\n", wait_result);

    ok(times_executed == 100, "didn't execute all of the work items\n");
}

static void CALLBACK signaled_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    HANDLE event = p;
    SetEvent(event);
    ok(!TimerOrWaitFired, "wait shouldn't have timed out\n");
}

static void CALLBACK timeout_function(PVOID p, BOOLEAN TimerOrWaitFired)
{
    HANDLE event = p;
    SetEvent(event);
    ok(TimerOrWaitFired, "wait should have timed out\n");
}

static void test_RegisterWaitForSingleObject(void)
{
    BOOL ret;
    HANDLE wait_handle;
    HANDLE handle;
    HANDLE complete_event;

    if (!pRegisterWaitForSingleObject || !pUnregisterWait)
    {
        win_skip("RegisterWaitForSingleObject or UnregisterWait not implemented\n");
        return;
    }

    /* test signaled case */

    handle = CreateEventW(NULL, TRUE, TRUE, NULL);
    complete_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %d\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    /* give worker thread chance to complete */
    Sleep(100);

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %d\n", GetLastError());

    /* test cancel case */

    ResetEvent(handle);

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %d\n", GetLastError());

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %d\n", GetLastError());

    /* test timeout case */

    ret = pRegisterWaitForSingleObject(&wait_handle, handle, timeout_function, complete_event, 0, WT_EXECUTEONLYONCE);
    ok(ret, "RegisterWaitForSingleObject failed with error %d\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    /* give worker thread chance to complete */
    Sleep(100);

    ret = pUnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %d\n", GetLastError());
}

static DWORD TLS_main;
static DWORD TLS_index0, TLS_index1;

static DWORD WINAPI TLS_InheritanceProc(LPVOID p)
{
  /* We should NOT inherit the TLS values from our parent or from the
     main thread.  */
  LPVOID val;

  val = TlsGetValue(TLS_main);
  ok(val == NULL, "TLS inheritance failed\n");

  val = TlsGetValue(TLS_index0);
  ok(val == NULL, "TLS inheritance failed\n");

  val = TlsGetValue(TLS_index1);
  ok(val == NULL, "TLS inheritance failed\n");

  return 0;
}

/* Basic TLS usage test.  Make sure we can create slots and the values we
   store in them are separate among threads.  Also test TLS value
   inheritance with TLS_InheritanceProc.  */
static DWORD WINAPI TLS_ThreadProc(LPVOID p)
{
  LONG_PTR id = (LONG_PTR) p;
  LPVOID val;
  BOOL ret;

  if (sync_threads_and_run_one(0, id))
  {
    TLS_index0 = TlsAlloc();
    ok(TLS_index0 != TLS_OUT_OF_INDEXES, "TlsAlloc failed\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(1, id))
  {
    TLS_index1 = TlsAlloc();
    ok(TLS_index1 != TLS_OUT_OF_INDEXES, "TlsAlloc failed\n");

    /* Slot indices should be different even if created in different
       threads.  */
    ok(TLS_index0 != TLS_index1, "TlsAlloc failed\n");

    /* Both slots should be initialized to NULL */
    val = TlsGetValue(TLS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == NULL, "TLS slot not initialized correctly\n");

    val = TlsGetValue(TLS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == NULL, "TLS slot not initialized correctly\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(0, id))
  {
    val = TlsGetValue(TLS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == NULL, "TLS slot not initialized correctly\n");

    val = TlsGetValue(TLS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == NULL, "TLS slot not initialized correctly\n");

    ret = TlsSetValue(TLS_index0, (LPVOID) 1);
    ok(ret, "TlsSetValue failed\n");

    ret = TlsSetValue(TLS_index1, (LPVOID) 2);
    ok(ret, "TlsSetValue failed\n");

    val = TlsGetValue(TLS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == (LPVOID) 1, "TLS slot not initialized correctly\n");

    val = TlsGetValue(TLS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == (LPVOID) 2, "TLS slot not initialized correctly\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(1, id))
  {
    val = TlsGetValue(TLS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == NULL, "TLS slot not initialized correctly\n");

    val = TlsGetValue(TLS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == NULL, "TLS slot not initialized correctly\n");

    ret = TlsSetValue(TLS_index0, (LPVOID) 3);
    ok(ret, "TlsSetValue failed\n");

    ret = TlsSetValue(TLS_index1, (LPVOID) 4);
    ok(ret, "TlsSetValue failed\n");

    val = TlsGetValue(TLS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == (LPVOID) 3, "TLS slot not initialized correctly\n");

    val = TlsGetValue(TLS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == (LPVOID) 4, "TLS slot not initialized correctly\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(0, id))
  {
    HANDLE thread;
    DWORD waitret, tid;

    val = TlsGetValue(TLS_index0);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == (LPVOID) 1, "TLS slot not initialized correctly\n");

    val = TlsGetValue(TLS_index1);
    ok(GetLastError() == ERROR_SUCCESS, "TlsGetValue failed\n");
    ok(val == (LPVOID) 2, "TLS slot not initialized correctly\n");

    thread = CreateThread(NULL, 0, TLS_InheritanceProc, 0, 0, &tid);
    ok(thread != NULL, "CreateThread failed\n");
    waitret = WaitForSingleObject(thread, 60000);
    ok(waitret == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    ret = TlsFree(TLS_index0);
    ok(ret, "TlsFree failed\n");
  }
  resync_after_run();

  if (sync_threads_and_run_one(1, id))
  {
    ret = TlsFree(TLS_index1);
    ok(ret, "TlsFree failed\n");
  }
  resync_after_run();

  return 0;
}

static void test_TLS(void)
{
  HANDLE threads[2];
  LONG_PTR i;
  DWORD ret;
  BOOL suc;

  init_thread_sync_helpers();

  /* Allocate a TLS slot in the main thread to test for inheritance.  */
  TLS_main = TlsAlloc();
  ok(TLS_main != TLS_OUT_OF_INDEXES, "TlsAlloc failed\n");
  suc = TlsSetValue(TLS_main, (LPVOID) 4114);
  ok(suc, "TlsSetValue failed\n");

  for (i = 0; i < 2; ++i)
  {
    DWORD tid;

    threads[i] = CreateThread(NULL, 0, TLS_ThreadProc, (LPVOID) i, 0, &tid);
    ok(threads[i] != NULL, "CreateThread failed\n");
  }

  ret = WaitForMultipleObjects(2, threads, TRUE, 60000);
  ok(ret == WAIT_OBJECT_0 || broken(ret == WAIT_OBJECT_0+1 /* nt4,w2k */), "WaitForAllObjects 2 threads %d\n",ret);

  for (i = 0; i < 2; ++i)
    CloseHandle(threads[i]);

  suc = TlsFree(TLS_main);
  ok(suc, "TlsFree failed\n");
  cleanup_thread_sync_helpers();
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
       "SetThreadErrorMode returned old mode 0x%x, expected 0x%x\n",
       mode, oldmode);
    mode = pGetThreadErrorMode();
    ok(mode == 0, "GetThreadErrorMode returned mode 0x%x, expected 0\n", mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0,
       "RtlGetThreadErrorMode returned mode 0x%x, expected 0\n", mode);

    ret = pSetThreadErrorMode(SEM_FAILCRITICALERRORS, &mode);
    ok(ret, "SetThreadErrorMode failed\n");
    ok(mode == 0,
       "SetThreadErrorMode returned old mode 0x%x, expected 0\n", mode);
    mode = pGetThreadErrorMode();
    ok(mode == SEM_FAILCRITICALERRORS,
       "GetThreadErrorMode returned mode 0x%x, expected SEM_FAILCRITICALERRORS\n",
       mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0x10,
       "RtlGetThreadErrorMode returned mode 0x%x, expected 0x10\n", mode);

    ret = pSetThreadErrorMode(SEM_NOGPFAULTERRORBOX, &mode);
    ok(ret, "SetThreadErrorMode failed\n");
    ok(mode == SEM_FAILCRITICALERRORS,
       "SetThreadErrorMode returned old mode 0x%x, expected SEM_FAILCRITICALERRORS\n",
       mode);
    mode = pGetThreadErrorMode();
    ok(mode == SEM_NOGPFAULTERRORBOX,
       "GetThreadErrorMode returned mode 0x%x, expected SEM_NOGPFAULTERRORBOX\n",
       mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0x20,
       "RtlGetThreadErrorMode returned mode 0x%x, expected 0x20\n", mode);

    ret = pSetThreadErrorMode(SEM_NOOPENFILEERRORBOX, NULL);
    ok(ret, "SetThreadErrorMode failed\n");
    mode = pGetThreadErrorMode();
    ok(mode == SEM_NOOPENFILEERRORBOX,
       "GetThreadErrorMode returned mode 0x%x, expected SEM_NOOPENFILEERRORBOX\n",
       mode);
    rtlmode = pRtlGetThreadErrorMode();
    ok(rtlmode == 0x40,
       "RtlGetThreadErrorMode returned mode 0x%x, expected 0x40\n", rtlmode);

    for (mode = 1; mode; mode <<= 1)
    {
        ret = pSetThreadErrorMode(mode, NULL);
        if (mode & (SEM_FAILCRITICALERRORS |
                    SEM_NOGPFAULTERRORBOX |
                    SEM_NOOPENFILEERRORBOX))
        {
            ok(ret,
               "SetThreadErrorMode(0x%x,NULL) failed with error %d\n",
               mode, GetLastError());
        }
        else
        {
            DWORD GLE = GetLastError();
            ok(!ret,
               "SetThreadErrorMode(0x%x,NULL) succeeded, expected failure\n",
               mode);
            ok(GLE == ERROR_INVALID_PARAMETER,
               "SetThreadErrorMode(0x%x,NULL) failed with %d, "
               "expected ERROR_INVALID_PARAMETER\n",
               mode, GLE);
        }
    }

    pSetThreadErrorMode(oldmode, NULL);
}

#if (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))) || (defined(_MSC_VER) && defined(__i386__))
static inline void set_fpu_cw(WORD cw)
{
#ifdef _MSC_VER
    __asm { fnclex }
    __asm { fldcw [cw] }
#else
    __asm__ volatile ("fnclex; fldcw %0" : : "m" (cw));
#endif
}

static inline WORD get_fpu_cw(void)
{
    WORD cw = 0;
#ifdef _MSC_VER
    __asm { fnstcw [cw] }
#else
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
#endif
    return cw;
}

struct fpu_thread_ctx
{
    WORD cw;
    HANDLE finished;
};

static DWORD WINAPI fpu_thread(void *param)
{
    struct fpu_thread_ctx *ctx = param;
    BOOL ret;

    ctx->cw = get_fpu_cw();

    ret = SetEvent(ctx->finished);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    return 0;
}

static WORD get_thread_fpu_cw(void)
{
    struct fpu_thread_ctx ctx;
    DWORD tid, res;
    HANDLE thread;

    ctx.finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!ctx.finished, "Failed to create event, last error %#x.\n", GetLastError());

    thread = CreateThread(NULL, 0, fpu_thread, &ctx, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

    res = WaitForSingleObject(ctx.finished, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

    res = CloseHandle(ctx.finished);
    ok(!!res, "Failed to close event handle, last error %#x.\n", GetLastError());

    return ctx.cw;
}

static void test_thread_fpu_cw(void)
{
    WORD initial_cw, cw;

    initial_cw = get_fpu_cw();
    ok(initial_cw == 0x27f, "Expected FPU control word 0x27f, got %#x.\n", initial_cw);

    cw = get_thread_fpu_cw();
    ok(cw == 0x27f, "Expected FPU control word 0x27f, got %#x.\n", cw);

    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "Expected FPU control word 0xf60, got %#x.\n", cw);

    cw = get_thread_fpu_cw();
    ok(cw == 0x27f, "Expected FPU control word 0x27f, got %#x.\n", cw);

    cw = get_fpu_cw();
    ok(cw == 0xf60, "Expected FPU control word 0xf60, got %#x.\n", cw);

    set_fpu_cw(initial_cw);
    cw = get_fpu_cw();
    ok(cw == initial_cw, "Expected FPU control word %#x, got %#x.\n", initial_cw, cw);
}
#endif

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
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
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

    handle = pCreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create context, error %u\n", GetLastError());

    ok(actctx.cbSize == sizeof(actctx), "cbSize=%d\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "dwFlags=%d\n", actctx.dwFlags);
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

    if (!pActivateActCtx)
    {
        win_skip("skipping activation context tests\n");
        return;
    }

    create_manifest_file("testdep1.manifest", manifest_dep);
    create_manifest_file("main.manifest", manifest_main);

    context = test_create("main.manifest");
    DeleteFileA("testdep1.manifest");
    DeleteFileA("main.manifest");

    handle = (void*)0xdeadbeef;
    b = pGetCurrentActCtx(&handle);
    ok(b, "GetCurentActCtx failed: %u\n", GetLastError());
    ok(handle == 0, "active context %p\n", handle);

    /* without active context */
    param.thread_context = (void*)0xdeadbeef;
    param.handle = NULL;
    thread = CreateThread(NULL, 0, thread_actctx_func, &param, 0, &tid);
    ok(thread != NULL, "failed, got %u\n", GetLastError());

    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "wait timeout\n");
    ok(param.thread_context == NULL, "got wrong thread context %p\n", param.thread_context);
    CloseHandle(thread);

    b = pActivateActCtx(context, &cookie);
    ok(b, "activation failed: %u\n", GetLastError());

    handle = 0;
    b = pGetCurrentActCtx(&handle);
    ok(b, "GetCurentActCtx failed: %u\n", GetLastError());
    ok(handle != 0, "no active context\n");
    pReleaseActCtx(handle);

    param.handle = NULL;
    b = pGetCurrentActCtx(&param.handle);
    ok(b && param.handle != NULL, "failed to get context, %u\n", GetLastError());

    param.thread_context = (void*)0xdeadbeef;
    thread = CreateThread(NULL, 0, thread_actctx_func, &param, 0, &tid);
    ok(thread != NULL, "failed, got %u\n", GetLastError());

    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "wait timeout\n");
    ok(param.thread_context == context, "got wrong thread context %p, %p\n", param.thread_context, context);
    pReleaseActCtx(param.thread_context);
    CloseHandle(thread);

    /* similar test for CreateRemoteThread() */
    param.thread_context = (void*)0xdeadbeef;
    thread = CreateRemoteThread(GetCurrentProcess(), NULL, 0, thread_actctx_func, &param, 0, &tid);
    ok(thread != NULL, "failed, got %u\n", GetLastError());

    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "wait timeout\n");
    ok(param.thread_context == context, "got wrong thread context %p, %p\n", param.thread_context, context);
    pReleaseActCtx(param.thread_context);
    CloseHandle(thread);

    pReleaseActCtx(param.handle);

    b = pDeactivateActCtx(0, cookie);
    ok(b, "DeactivateActCtx failed: %u\n", GetLastError());
    pReleaseActCtx(context);
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
    ok (work != NULL, "Error %d in CreateThreadpoolWork\n", GetLastError());
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
    ok(tls && tls != TLS_OUT_OF_INDEXES, "tls = %x\n", tls);
    TlsSetValue(tls, (void*)1);

    val = TlsGetValue(0);
    ok(!val, "TlsGetValue(0) = %p\n", val);

    TlsFree(tls);

    /* The following is too ugly to be run by default */
    if(0) {
        /* Set TLS index 0 value and see that this works and doesn't cause problems
         * for remaining tests. */
        ret = TlsSetValue(0, (void*)1);
        ok(ret, "TlsSetValue(0, 1) failed: %u\n", GetLastError());

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

    for (i = 0; i < sizeof(info_size)/sizeof(info_size[0]); i++)
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
            ok(status == STATUS_SUCCESS, "for info %u expected STATUS_SUCCESS, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

#ifdef __i386__
        case ThreadDescriptorTableEntry:
            ok(status == STATUS_SUCCESS || broken(status == STATUS_ACCESS_DENIED) /* testbot VM is broken */,
               "for info %u expected STATUS_SUCCESS, got %08x (ret_len %u)\n", i, status, ret_len);
            break;
#endif

        case ThreadTimes:
todo_wine
            ok(status == STATUS_SUCCESS, "for info %u expected STATUS_SUCCESS, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

        case ThreadAffinityMask:
        case ThreadQuerySetWin32StartAddress:
todo_wine
            ok(status == STATUS_ACCESS_DENIED, "for info %u expected STATUS_ACCESS_DENIED, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

        default:
            ok(status == STATUS_ACCESS_DENIED, "for info %u expected STATUS_ACCESS_DENIED, got %08x (ret_len %u)\n", i, status, ret_len);
            break;
        }
    }

    CloseHandle(thread);
}

static void init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

/* Neither Cygwin nor mingW export OpenThread, so do a dynamic check
   so that the compile passes */

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f)
    X(GetThreadPriorityBoost);
    X(OpenThread);
    X(QueueUserWorkItem);
    X(SetThreadIdealProcessor);
    X(SetThreadPriorityBoost);
    X(RegisterWaitForSingleObject);
    X(UnregisterWait);
    X(IsWow64Process);
    X(SetThreadErrorMode);
    X(GetThreadErrorMode);
    X(ActivateActCtx);
    X(CreateActCtxW);
    X(DeactivateActCtx);
    X(GetCurrentActCtx);
    X(ReleaseActCtx);

    X(CreateThreadpool);
    X(CloseThreadpool);
    X(CreateThreadpoolWork);
    X(SubmitThreadpoolWork);
    X(WaitForThreadpoolWorkCallbacks);
    X(CloseThreadpoolWork);

    X(GetThreadGroupAffinity);
    X(SetThreadGroupAffinity);
#undef X

#define X(f) p##f = (void*)GetProcAddress(ntdll, #f)
   if (ntdll)
   {
       X(NtQueryInformationThread);
       X(RtlGetThreadErrorMode);
       X(NtSetInformationThread);
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
           ok(hThread != NULL, "CreateThread failed, error %u\n",
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
   test_GetThreadTimes();
   test_thread_processor();
   test_GetThreadExitCode();
#ifdef __i386__
   test_SetThreadContext();
#endif
   test_QueueUserWorkItem();
   test_RegisterWaitForSingleObject();
   test_TLS();
   test_ThreadErrorMode();
#if (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))) || (defined(_MSC_VER) && defined(__i386__))
   test_thread_fpu_cw();
#endif
   test_thread_actctx();

   test_threadpool();
}
