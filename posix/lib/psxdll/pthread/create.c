/* $Id: create.c,v 1.3 2002/10/18 21:56:39 ea Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/pthread/create.c
 * PURPOSE:     Thread creation
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <psx/debug.h>
#include <psx/errno.h>
#include <napi/i386/segment.h>

/* thread creation code adapted from kernel32's CreateRemoteThread() function */

static void __threadentry (void *(*start_routine)(void*), void *arg)
{
 INFO("hello world! thread successfully created");

 TODO("initialize thread data");
 TODO("notify DLLs");
 TODO("notify psxss");

 INFO("about to call start routine at %#x with argument %#x", start_routine, arg);

 pthread_exit(start_routine(arg));
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void*), void *arg)
{
 HANDLE hThread;
 OBJECT_ATTRIBUTES oaThreadAttrs;
 CLIENT_ID ciId;
 CONTEXT cxThreadContext;
 INITIAL_TEB itInitialTeb;
 BOOLEAN fSuspended;
 ULONG nOldPageProtection;
 NTSTATUS nErrCode;

 /* initialize generic object attributes */
 oaThreadAttrs.Length = sizeof(OBJECT_ATTRIBUTES);
 oaThreadAttrs.RootDirectory = NULL;
 oaThreadAttrs.ObjectName = NULL;
 oaThreadAttrs.Attributes = 0;
 oaThreadAttrs.SecurityDescriptor = NULL;
 oaThreadAttrs.SecurityQualityOfService = NULL;

 /* initialize thread attributes */
 fSuspended = FALSE; /* FIXME? really needed? can we hardcode this to FALSE? */

 /* stack attributes */
 FIXME("stack size defaulted to 0x100000 - thread attributes ignored");

 /* stack reserve size */
 itInitialTeb.StackReserve = 0x100000;

 /* stack commit size */
 itInitialTeb.StackCommit = itInitialTeb.StackReserve - PAGE_SIZE;

 /* guard page */
 itInitialTeb.StackCommit += PAGE_SIZE;

 /* reserve stack */
 itInitialTeb.StackAllocate = NULL;

 nErrCode = NtAllocateVirtualMemory
 (
  NtCurrentProcess(),
  &itInitialTeb.StackAllocate,
  0,
  &itInitialTeb.StackReserve,
  MEM_RESERVE,
  PAGE_READWRITE
 );

 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode)); /* FIXME? TODO? pthread specific error codes? */
 }

 itInitialTeb.StackBase = (PVOID)((ULONG)itInitialTeb.StackAllocate + itInitialTeb.StackReserve);
 itInitialTeb.StackLimit = (PVOID)((ULONG)itInitialTeb.StackBase - itInitialTeb.StackCommit);

 /* commit stack */
 nErrCode = NtAllocateVirtualMemory
 (
  NtCurrentProcess(),
  &itInitialTeb.StackLimit,
  0,
  &itInitialTeb.StackCommit,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 if(!NT_SUCCESS(nErrCode))
 {
  NtFreeVirtualMemory
  (
   NtCurrentProcess(),
   itInitialTeb.StackAllocate,
   &itInitialTeb.StackReserve,
   MEM_RELEASE
  );

  return (__status_to_errno(nErrCode));
 }

 /* protect guard page */
 nErrCode = NtProtectVirtualMemory
 (
  NtCurrentProcess(),
  itInitialTeb.StackLimit,
  PAGE_SIZE,
  PAGE_GUARD | PAGE_READWRITE,
  &nOldPageProtection
 );

 if(!NT_SUCCESS(nErrCode))
 {
  NtFreeVirtualMemory
  (
   NtCurrentProcess(),
   itInitialTeb.StackAllocate,
   &itInitialTeb.StackReserve,
   MEM_RELEASE
  );

  return (__status_to_errno(nErrCode));
 }

 /* initialize thread registers */

//#ifdef __i386__
 memset(&cxThreadContext, 0, sizeof(CONTEXT));
 cxThreadContext.Eip = (LONG)__threadentry;
 cxThreadContext.SegGs = USER_DS;
 cxThreadContext.SegFs = TEB_SELECTOR;
 cxThreadContext.SegEs = USER_DS;
 cxThreadContext.SegDs = USER_DS;
 cxThreadContext.SegCs = USER_CS;
 cxThreadContext.SegSs = USER_DS;
 cxThreadContext.Esp = (ULONG)itInitialTeb.StackBase - 12;
 cxThreadContext.EFlags = (1<<1) + (1<<9);

 /* initialize call stack */
 *((PULONG)((ULONG)itInitialTeb.StackBase - 4)) = (ULONG)arg; /* thread argument */
 *((PULONG)((ULONG)itInitialTeb.StackBase - 8)) = (ULONG)start_routine; /* thread start routine */
 *((PULONG)((ULONG)itInitialTeb.StackBase - 12)) = 0xDEADBEEF; /* "shouldn't see me" */
//#else
//#error Unsupported architecture
//#endif

 INFO("about to create new thread - start routine at %#x, argument %#x", start_routine, arg);

 /* create thread */
 nErrCode = NtCreateThread
 (
  &hThread,
  THREAD_ALL_ACCESS,
  &oaThreadAttrs,
  NtCurrentProcess(),
  &ciId,
  &cxThreadContext,
  &itInitialTeb,
  fSuspended
 );

 if(!NT_SUCCESS(nErrCode))
 {
  NtFreeVirtualMemory
  (
   NtCurrentProcess(),
   itInitialTeb.StackAllocate,
   &itInitialTeb.StackReserve,
   MEM_RELEASE
  );

  return (__status_to_errno(nErrCode));
 }

 /* FIXME? should we return the thread handle or the thread id? */
 if(thread != 0)
  *thread = (pthread_t)&ciId.UniqueThread; /* for the moment, we return the id */

 return (0);

}

/* EOF */

