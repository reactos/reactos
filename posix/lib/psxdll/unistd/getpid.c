/* $Id: getpid.c,v 1.3 2002/05/17 01:55:34 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/getpid.c
 * PURPOSE:     Get the process ID
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <sys/types.h>
#include <unistd.h>
#include <psx/errno.h>

pid_t getpid(void)
{
 PROCESS_BASIC_INFORMATION pbiInfo;
 NTSTATUS                  nErrCode;

 nErrCode = NtQueryInformationProcess
 (
  NtCurrentProcess(),
  ProcessBasicInformation,
  &pbiInfo,
  sizeof(pbiInfo),
  NULL
 );

 if(!NT_SUCCESS(nErrCode))
 {
  errno = __status_to_errno(nErrCode);
  return (0);
 }

 return (pbiInfo.UniqueProcessId);
#if 0
 return ((pid_t)NtCurrentTeb()->Cid.UniqueProcess);
#endif
}

/* EOF */

