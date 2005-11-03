/* $Id: exit.c,v 1.4 2002/10/29 04:45:38 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/pthread/exit.c
 * PURPOSE:     Thread termination
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/ldr.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <psx/debug.h>

void pthread_exit(void *value_ptr)
{
 NTSTATUS nErrCode;
 BOOLEAN fLastThread;

 /* terminate process if this is the last thread of the current process */
 nErrCode = NtQueryInformationThread
 (
  NtCurrentThread(),
  ThreadAmILastThread,
  &fLastThread,
  sizeof(BOOLEAN),
  NULL
 );

 if(NT_SUCCESS(nErrCode))
 {
  if(fLastThread)
  {
   INFO("this thread is the last in the current process - about to call exit(0)");
   exit(0);
  }
 }
 else
 {
  WARN
  (
   "NtQueryInformationThread(ThreadAmILastThread) failed with status %#x. \
Can't determine if the current thread is the last in the process. The process \
could hang",
   nErrCode
  );

 }

 TODO("Notify psxss of thread termination");

 LdrShutdownThread(); /* detach DLLs */

 /* kill this thread */

 WARNIF(
  sizeof(ULONG) < sizeof(typeof(value_ptr)),
  "\
the value returned from the current thread will be truncated (pointers shorter \
than long integers on this architecture?) - expect trouble"
 );

 INFO("bye bye. Current thread about to die");

 NtTerminateThread(NtCurrentThread(), (ULONG)value_ptr);

 /* "The pthread_exit() function cannot return to its caller." */
 NtDelayExecution(FALSE, NULL);

}

/* EOF */

