/* $Id: getppid.c,v 1.2 2002/02/20 09:17:58 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/getppid.c
 * PURPOSE:     Get the parent process ID
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <sys/types.h>
#include <unistd.h>
#include <psx/errno.h>

pid_t getppid(void)
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

 return (pbiInfo.InheritedFromUniqueProcessId);
}

/* EOF */

