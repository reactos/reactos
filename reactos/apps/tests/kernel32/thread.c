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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Define _WIN32_WINNT to get SetThreadIdealProcessor on Windows */
#define _WIN32_WINNT 0x0500

#include <stdarg.h>

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
  #ifdef __try
    #define __TRY __try
    #define __EXCEPT __except
    #define __ENDTRY
  #else
    #include "wine/exception.h"
  #endif
#endif

typedef BOOL (WINAPI *GetThreadPriorityBoost_t)(HANDLE,PBOOL);
static GetThreadPriorityBoost_t pGetThreadPriorityBoost=NULL;

typedef HANDLE (WINAPI *OpenThread_t)(DWORD,BOOL,DWORD);
static OpenThread_t pOpenThread=NULL;

typedef DWORD (WINAPI *SetThreadIdealProcessor_t)(HANDLE,DWORD);
static SetThreadIdealProcessor_t pSetThreadIdealProcessor=NULL;

typedef BOOL (WINAPI *SetThreadPriorityBoost_t)(HANDLE,BOOL);
static SetThreadPriorityBoost_t pSetThreadPriorityBoost=NULL;

/* Functions not tested yet:
  AttachThreadInput
  CreateRemoteThread
  SetThreadContext
  SwitchToThread

In addition there are no checks that the inheritance works properly in
CreateThread
*/

DWORD tlsIndex;

typedef struct {
  int threadnum;
  HANDLE *event;
  DWORD *threadmem;
} t1Struct;

/* Basic test that simulatneous threads can access shared memory,
   that the thread local storage routines work correctly, and that
   threads actually run concurrently
*/
VOID WINAPI threadFunc1(t1Struct *tstruct)
{
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
/* Check that noone cahnged our tls memory */
   ok((int)TlsGetValue(tlsIndex)-1==tstruct->threadnum,
      "TlsGetValue failed\n");
   ExitThread(NUM_THREADS+tstruct->threadnum);
}

VOID WINAPI threadFunc2()
{
   ExitThread(99);
}

VOID WINAPI threadFunc3()
{
   HANDLE thread;
   thread=GetCurrentThread();
   SuspendThread(thread);
   ExitThread(99);
}

VOID WINAPI threadFunc4(HANDLE event)
{
   if(event != NULL) {
     SetEvent(event);
   }
   Sleep(99000);
   ExitThread(0);
}

#if CHECK_STACK
VOID WINAPI threadFunc5(DWORD *exitCode)
{
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
   ExitThread(0);
}
#endif

/* Check basic funcationality of CreateThread and Tls* functions */
VOID test_CreateThread_basic()
{
   HANDLE thread[NUM_THREADS],event[NUM_THREADS];
   DWORD threadid[NUM_THREADS],curthreadId;
   DWORD threadmem[NUM_THREADS];
   DWORD exitCode;
   t1Struct tstruct[NUM_THREADS];
   int error;
   DWORD i,j;
/* Retrieve current Thread ID for later comparisons */
  curthreadId=GetCurrentThreadId();
/* Allocate some local storage */
  ok((tlsIndex=TlsAlloc())!=TLS_OUT_OF_INDEXES,"TlsAlloc failed\n");
/* Create events for thread synchronization */
  for(i=0;i<NUM_THREADS;i++) {
    threadmem[i]=0;
/* Note that it doesn't matter what type of event we chose here.  This
   test isn't trying to thoroughly test events
*/
    event[i]=CreateEventA(NULL,TRUE,FALSE,NULL);
    tstruct[i].threadnum=i;
    tstruct[i].threadmem=threadmem;
    tstruct[i].event=event;
  }

/* Test that passing arguments to threads works okay */
  for(i=0;i<NUM_THREADS;i++) {
    thread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)threadFunc1,
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
   and that each thread id was independant).  Note that we prove that the
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
}

/* Check that using the CREATE_SUSPENDED flag works */
VOID test_CreateThread_suspended()
{
  HANDLE thread;
  DWORD threadId;
  int error;

  thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)threadFunc2,NULL,
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
  ok(CloseHandle(thread)!=0,"CloseHandle failed\n");
}

/* Check that SuspendThread and ResumeThread work */
VOID test_SuspendThread()
{
  HANDLE thread,access_thread;
  DWORD threadId,exitCode;
  int i,error;

  thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)threadFunc3,NULL,
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
      ok(SuspendThread(access_thread)==-1,
         "SuspendThread did not obey access restrictions\n");
      ok(ResumeThread(access_thread)==-1,
         "ResumeThread did not obey access restrictions\n");
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
  ok(error==0xffffffff, "wrong return code: %d\n", error);
  ok(GetLastError()==ERROR_ACCESS_DENIED || GetLastError()==ERROR_NO_MORE_ITEMS, "unexpected error code: %ld\n", GetLastError());

  ok(CloseHandle(thread)!=0,"CloseHandle Failed\n");
}

/* Check that TerminateThread works properly
*/
VOID test_TerminateThread()
{
  HANDLE thread,access_thread,event;
  DWORD threadId,exitCode;
  int i,error;
  i=0; error=0;
  event=CreateEventA(NULL,TRUE,FALSE,NULL);
  thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)threadFunc4,
                        (LPVOID)event, 0,&threadId);
  ok(thread!=NULL,"Create Thread failed\n");
/* Terminate thread has a race condition in Wine.  If the thread is terminated
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
      ok(TerminateThread(access_thread,99)==0,
         "TerminateThread did not obey access restrictions\n");
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
VOID test_CreateThread_stack()
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
                           (LPTHREAD_START_ROUTINE)threadFunc5,&exitCode,
                           0,&threadId);
     ok(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,
        "TerminateThread didn't work\n");
     ok(exitCode==1,"CreateThread did not obey stack-size-limit\n");
     ok(CloseHandle(thread)!=0,"CloseHandle failed\n");
#endif
}

/* Check whether setting/retrieving thread priorities works */
VOID test_thread_priority()
{
   HANDLE curthread,access_thread;
   DWORD curthreadId,exitCode;
   int min_priority=-2,max_priority=2;
   BOOL disabled;
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
       ok(SetThreadPriority(access_thread,1)==0,
          "SetThreadPriority did not obey access restrictions\n");
       ok(GetThreadPriority(access_thread)==THREAD_PRIORITY_ERROR_RETURN,
          "GetThreadPriority did not obey access restrictions\n");
       if (pSetThreadPriorityBoost)
         ok(pSetThreadPriorityBoost(access_thread,1)==0,
            "SetThreadPriorityBoost did not obey access restrictions\n");
       if (pGetThreadPriorityBoost)
         ok(pGetThreadPriorityBoost(access_thread,&disabled)==0,
            "GetThreadPriorityBoost did not obey access restrictions\n");
       ok(GetExitCodeThread(access_thread,&exitCode)==0,
          "GetExitCodeThread did not obey access restrictions\n");
       ok(CloseHandle(access_thread),"Error Closing thread handle\n");
     }
#if USE_EXTENDED_PRIORITIES
     min_priority=-7; max_priority=6;
#endif
   }
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

/* Check thread priority boost */
   if (pGetThreadPriorityBoost && pSetThreadPriorityBoost) {
     BOOL rc;
     todo_wine {
         SetLastError(0);
         rc=pGetThreadPriorityBoost(curthread,&disabled);
         if (rc!=0 || GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED) {
             ok(rc!=0,"error=%ld\n",GetLastError());

             ok(pSetThreadPriorityBoost(curthread,1)!=0,
                "error=%ld\n",GetLastError());
             rc=pGetThreadPriorityBoost(curthread,&disabled);
             ok(rc!=0 && disabled==1,
                "rc=%d error=%ld disabled=%d\n",rc,GetLastError(),disabled);

             ok(pSetThreadPriorityBoost(curthread,0)!=0,
                "error=%ld\n",GetLastError());
             rc=pGetThreadPriorityBoost(curthread,&disabled);
             ok(rc!=0 && disabled==0,
                "rc=%d error=%ld disabled=%d\n",rc,GetLastError(),disabled);
         }
     }
   }
}

/* check the GetThreadTimes function */
VOID test_GetThreadTimes()
{
     HANDLE thread,access_thread=NULL;
     FILETIME creationTime,exitTime,kernelTime,userTime;
     DWORD threadId;
     int error;

     thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)threadFunc2,NULL,
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
     if(access_thread!=NULL) {
       error=GetThreadTimes(access_thread,&creationTime,&exitTime,
                            &kernelTime,&userTime);
       ok(error==0, "GetThreadTimes did not obey access restrictions\n");
       ok(CloseHandle(access_thread)!=0,"CloseHandle Failed\n");
     }
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
     }
}

/* Check the processor affinity functions */
/* NOTE: These functions should also be checked that they obey access control
*/
VOID test_thread_processor()
{
   HANDLE curthread,curproc;
   DWORD processMask,systemMask;
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
   ok(SetThreadAffinityMask(curthread,processMask)==1,
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

START_TEST(thread)
{
   HINSTANCE lib;
/* Neither Cygwin nor mingW export OpenThread, so do a dynamic check
   so that the compile passes
*/
   lib=LoadLibraryA("kernel32");
   ok(lib!=NULL,"Couldn't load kernel32.dll\n");
   pGetThreadPriorityBoost=(GetThreadPriorityBoost_t)GetProcAddress(lib,"GetThreadPriorityBoost");
   pOpenThread=(OpenThread_t)GetProcAddress(lib,"OpenThread");
   pSetThreadIdealProcessor=(SetThreadIdealProcessor_t)GetProcAddress(lib,"SetThreadIdealProcessor");
   pSetThreadPriorityBoost=(SetThreadPriorityBoost_t)GetProcAddress(lib,"SetThreadPriorityBoost");
   test_CreateThread_basic();
   test_CreateThread_suspended();
   test_SuspendThread();
   test_TerminateThread();
   test_CreateThread_stack();
   test_thread_priority();
   test_GetThreadTimes();
   test_thread_processor();
}
