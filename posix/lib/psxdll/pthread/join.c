/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/pthread/join.c
 * PURPOSE:     Wait for thread termination
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/ldr.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <psx/debug.h>
#include <psx/errno.h>

int pthread_join(pthread_t thread, void **value_ptr)
{
 HANDLE hThread;
 NTSTATUS nErrCode;
 OBJECT_ATTRIBUTES oaThreadAttrs;
 CLIENT_ID ciId;
 THREAD_BASIC_INFORMATION tbiThreadInfo;

 /* "[EDEADLK] A deadlock was detected or the value of thread specifies
    the calling thread" */
 if(thread == pthread_self())
  return (EDEADLK);

 /* initialize id */
 ciId.UniqueProcess = (HANDLE)-1;
 ciId.UniqueThread = (HANDLE)thread;

 /* initialize object attributes */
 oaThreadAttrs.Length = sizeof(OBJECT_ATTRIBUTES);
 oaThreadAttrs.RootDirectory = NULL;
 oaThreadAttrs.ObjectName = NULL;
 oaThreadAttrs.Attributes = 0;
 oaThreadAttrs.SecurityDescriptor = NULL;
 oaThreadAttrs.SecurityQualityOfService = NULL;

 /* open the thread */
 nErrCode = NtOpenThread
 (
  &hThread,
  SYNCHRONIZE | THREAD_QUERY_INFORMATION,
  &oaThreadAttrs,
  &ciId
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* wait for thread termination */
 nErrCode = NtWaitForSingleObject
 (
  hThread,
  FALSE,
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  NtClose(hThread);
  return (__status_to_errno(nErrCode));
 }

 /* get thread basic information (includes return code) */
 nErrCode = NtQueryInformationThread
 (
  hThread,
  ThreadBasicInformation,
  &tbiThreadInfo,
  sizeof(THREAD_BASIC_INFORMATION),
  NULL
 );

 NtClose(hThread);

 if(!value_ptr)
  return (EFAULT);

 *value_ptr = (void *)tbiThreadInfo.ExitStatus;

 return (0);

}

/* EOF */

