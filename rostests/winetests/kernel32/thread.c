/*
 * Unit test suite for directory functions.
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

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>

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

typedef BOOL (WINAPI *GetThreadPriorityBoost_t)(HANDLE,PBOOL);
static GetThreadPriorityBoost_t pGetThreadPriorityBoost=NULL;

typedef HANDLE (WINAPI *OpenThread_t)(DWORD,BOOL,DWORD);
static OpenThread_t pOpenThread=NULL;

typedef BOOL (WINAPI *QueueUserWorkItem_t)(LPTHREAD_START_ROUTINE,PVOID,ULONG);
static QueueUserWorkItem_t pQueueUserWorkItem=NULL;

typedef DWORD (WINAPI *SetThreadIdealProcessor_t)(HANDLE,DWORD);
static SetThreadIdealProcessor_t pSetThreadIdealProcessor=NULL;

typedef BOOL (WINAPI *SetThreadPriorityBoost_t)(HANDLE,BOOL);
static SetThreadPriorityBoost_t pSetThreadPriorityBoost=NULL;

typedef BOOL (WINAPI *RegisterWaitForSingleObject_t)(PHANDLE,HANDLE,WAITORTIMERCALLBACK,PVOID,ULONG,ULONG);
static RegisterWaitForSingleObject_t pRegisterWaitForSingleObject=NULL;

typedef BOOL (WINAPI *UnregisterWait_t)(HANDLE);
static UnregisterWait_t pUnregisterWait=NULL;

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs( &argv );
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ok(CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL,
                     &si, &pi) != 0, "error: %u\n", GetLastError());
    ok(CloseHandle(pi.hThread) != 0, "error %u\n", GetLastError());
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
  start_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  ok(start_event != NULL, "CreateEvent failed\n");
  stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
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

DWORD tlsIndex;

typedef struct {
  int threadnum;
  HANDLE *event;
  DWORD *threadmem;
} t1Struct;

/* WinME supports OpenThread but doesn't know about access restrictions so
   we require them to be either completely ignored or always obeyed.
*/
INT obeying_ars = 0; /* -1 == no, 0 == dunno yet, 1 == yes */
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
    t1Struct *tstruct = (t1Struct *)p;
   int i;
/* write our thread # into shared memory */
   tstruct->threadmem[tstruct->threadnum]=GetCurrentThreadId();
   ok(TlsSetValue(tlsIndex,(LPVOID)(tstruct->threadnum+1))!=0,
      "TlsSetValue failed\n");
/* The threads synchronize before terminating.  This is done by
   Signaling an event, and waiting for all events to occur
*/
   SetEvent(tstruct->event[tstruct->threadnum]);
   WaitForMultipleObjects(NUM_THREADS,tstruct->event,TRUE,INFINITE);
/* Double check that all threads really did run by validating that
   they have all written to the shared memory. There should be no race
   here, since all threads were synchronized after the write.*/
   for(i=0;i<NUM_THREADS;i++) {
     while(tstruct->threadmem[i]==0) ;
   }

   /* lstrlenA contains an exception handler so this makes sure exceptions work in threads */
   ok( lstrlenA( (char *)0xdeadbeef ) == 0, "lstrlenA: unexpected success\n" );

/* Check that no one changed our tls memory */
   ok((int)TlsGetValue(tlsIndex)-1==tstruct->threadnum,
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
    HANDLE event = (HANDLE)p;
   if(event != NULL) {
     SetEvent(event);
   }
   Sleep(99000);
   return 0;
}

#if CHECK_STACK
static DWORD WINAPI threadFunc5(LPVOID p)
{
  DWORD *exitCode = (DWORD *)p;
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
    SetEvent((HANDLE) p);
    return 0;
}

static DWORD WINAPI threadFunc_CloseHandle(LPVOID p)
{
    CloseHandle((HANDLE) p);
    return 0;
}

static void create_function_addr_events(HANDLE events[2])
{
    char buffer[256];

    sprintf(buffer, "threadFunc_SetEvent %p", threadFunc_SetEvent);
    events[0] = CreateEvent(NULL, FALSE, FALSE, buffer);

    sprintf(buffer, "threadFunc_CloseHandle %p", threadFunc_CloseHandle);
    events[1] = CreateEvent(NULL, FALSE, FALSE, buffer);
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

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
        skip("CreateRemoteThread is not implemented\n");
        goto cleanup;
    }
    ok(hThread != NULL, "CreateRemoteThread failed, err=%u\n", GetLastError());
    ok(tid != 0, "null tid\n");
    ret = SuspendThread(hThread);
    ok(ret == 1, "ret=%u, err=%u\n", ret, GetLastError());
    ret = ResumeThread(hThread);
    ok(ret == 2, "ret=%u, err=%u\n", ret, GetLastError());

    /* thread still suspended, so wait times out */
    ret = WaitForSingleObject(hEvent, 100);
    ok(ret == WAIT_TIMEOUT, "wait did not time out, ret=%u\n", ret);

    ret = ResumeThread(hThread);
    ok(ret == 1, "ret=%u, err=%u\n", ret, GetLastError());

    /* wait that doesn't time out */
    ret = WaitForSingleObject(hEvent, 100);
    ok(ret == WAIT_OBJECT_0, "object not signaled, ret=%u\n", ret);

    /* wait for thread end */
    ret = WaitForSingleObject(hThread, 100);
    ok(ret == WAIT_OBJECT_0, "waiting for thread failed, ret=%u\n", ret);
    CloseHandle(hThread);

    /* create and wait for remote thread with entry point CloseHandle() */
    hThread = CreateRemoteThread(hProcess, NULL, 0,
                                 threadFunc_CloseHandle,
                                 hRemoteEvent, 0, &tid);
    ok(hThread != NULL, "CreateRemoteThread failed, err=%u\n", GetLastError());
    ret = WaitForSingleObject(hThread, 100);
    ok(ret == WAIT_OBJECT_0, "waiting for thread failed, ret=%u\n", ret);
    CloseHandle(hThread);

    /* create remote thread with entry point SetEvent() */
    hThread = CreateRemoteThread(hProcess, NULL, 0,
                                 threadFunc_SetEvent,
                                 hRemoteEvent, 0, &tid);
    ok(hThread != NULL, "CreateRemoteThread failed, err=%u\n", GetLastError());

    /* closed handle, so wait times out */
    ret = WaitForSingleObject(hEvent, 100);
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
  ok(TlsFree(tlsIndex)!=0,"TlsFree failed\n");

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
    access_thread=pOpenThread(THREAD_ALL_ACCESS & (~THREAD_SUSPEND_RESUME),
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
  thread = CreateThread(NULL,0,threadFunc4,
                        (LPVOID)event, 0,&threadId);
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
    access_thread=pOpenThread(THREAD_ALL_ACCESS & (~THREAD_TERMINATE),
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
     access_thread=pOpenThread(THREAD_ALL_ACCESS &
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
     return; /* WinME */

   todo_wine
     ok(rc!=0,"error=%d\n",GetLastError());

   if (pOpenThread) {
/* check that access control is obeyed */
     access_thread=pOpenThread(THREAD_ALL_ACCESS &
                       (~THREAD_QUERY_INFORMATION) & (~THREAD_SET_INFORMATION),
                       0,curthreadId);
     ok(access_thread!=NULL,"OpenThread returned an invalid handle\n");
     if (access_thread!=NULL) {
       obey_ar(pSetThreadPriorityBoost(access_thread,1)==0);
       obey_ar(pGetThreadPriorityBoost(access_thread,&disabled)==0);
       ok(CloseHandle(access_thread),"Error Closing thread handle\n");
     }
   }

   todo_wine {
     rc = pSetThreadPriorityBoost(curthread,1);
     ok( rc != 0, "error=%d\n",GetLastError());
     rc=pGetThreadPriorityBoost(curthread,&disabled);
     ok(rc!=0 && disabled==1,
        "rc=%d error=%d disabled=%d\n",rc,GetLastError(),disabled);

     rc = pSetThreadPriorityBoost(curthread,0);
     ok( rc != 0, "error=%d\n",GetLastError());
     rc=pGetThreadPriorityBoost(curthread,&disabled);
     ok(rc!=0 && disabled==0,
        "rc=%d error=%d disabled=%d\n",rc,GetLastError(),disabled);
   }
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
       access_thread=pOpenThread(THREAD_ALL_ACCESS &
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
     if (error!=0 || GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED) {
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
   DWORD_PTR processMask,systemMask;
   SYSTEM_INFO sysInfo;
   int error=0;

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
/* NOTE: This only works on WinNT/2000/XP) */
   if (pSetThreadIdealProcessor) {
     todo_wine {
       SetLastError(0);
       error=pSetThreadIdealProcessor(curthread,0);
       if (GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED) {
         ok(error!=-1, "SetThreadIdealProcessor failed\n");
       }
     }
     if (GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED) {
       error=pSetThreadIdealProcessor(curthread,MAXIMUM_PROCESSORS+1);
       ok(error==-1,
          "SetThreadIdealProcessor succeeded with an illegal processor #\n");
       todo_wine {
         error=pSetThreadIdealProcessor(curthread,MAXIMUM_PROCESSORS);
         ok(error==0, "SetThreadIdealProcessor returned an incorrect value\n");
       }
     }
   }
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
    event = CreateEvent( NULL, TRUE, FALSE, NULL );
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
        ok( SetThreadContext( thread, &ctx ), "SetThreadContext failed : (%d)\n", GetLastError() );
    }

    SetLastError(0xdeadbeef);
    prevcount = ResumeThread( thread );
    ok ( prevcount == 1, "Previous suspend count (%d) instead of 1, last error : (%d)\n",
                         prevcount, GetLastError() );

    WaitForSingleObject( thread, INFINITE );
    ok( test_value == 20, "test_value %d instead of 20\n", test_value );
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
    int i;
    DWORD wait_result;
    DWORD before, after;

    /* QueueUserWorkItem not present on win9x */
    if (!pQueueUserWorkItem) return;

    finish_event = CreateEvent(NULL, TRUE, FALSE, NULL);

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
        skip("RegisterWaitForSingleObject or UnregisterWait not implemented\n");
        return;
    }

    skip("ROS-HACK: Skipping RegisterWaitForSingleObject tests\n");
    return;

    /* test signaled case */

    handle = CreateEvent(NULL, TRUE, TRUE, NULL);
    complete_event = CreateEvent(NULL, FALSE, FALSE, NULL);

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
  LONG id = (LONG) p;
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
  LONG i;
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
  ok(ret == WAIT_OBJECT_0 || ret == WAIT_OBJECT_0+1 /* nt4 */, "WaitForMultipleObjects failed %u\n",ret);

  for (i = 0; i < 2; ++i)
    CloseHandle(threads[i]);

  suc = TlsFree(TLS_main);
  ok(suc, "TlsFree failed\n");
  cleanup_thread_sync_helpers();
}

START_TEST(thread)
{
   HINSTANCE lib;
   int argc;
   char **argv;
   argc = winetest_get_mainargs( &argv );
/* Neither Cygwin nor mingW export OpenThread, so do a dynamic check
   so that the compile passes
*/
   lib=GetModuleHandleA("kernel32.dll");
   ok(lib!=NULL,"Couldn't get a handle for kernel32.dll\n");
   pGetThreadPriorityBoost=(GetThreadPriorityBoost_t)GetProcAddress(lib,"GetThreadPriorityBoost");
   pOpenThread=(OpenThread_t)GetProcAddress(lib,"OpenThread");
   pQueueUserWorkItem=(QueueUserWorkItem_t)GetProcAddress(lib,"QueueUserWorkItem");
   pSetThreadIdealProcessor=(SetThreadIdealProcessor_t)GetProcAddress(lib,"SetThreadIdealProcessor");
   pSetThreadPriorityBoost=(SetThreadPriorityBoost_t)GetProcAddress(lib,"SetThreadPriorityBoost");
   pRegisterWaitForSingleObject=(RegisterWaitForSingleObject_t)GetProcAddress(lib,"RegisterWaitForSingleObject");
   pUnregisterWait=(UnregisterWait_t)GetProcAddress(lib,"UnregisterWait");

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
}
