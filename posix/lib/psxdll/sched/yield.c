/* $Id:
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/sched/yield.c
 * PURPOSE:     Yield processor
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <sched.h>
#include <psx/errno.h>

int sched_yield(void)
{
 NTSTATUS nErrCode;

 nErrCode = NtYieldExecution();

 if(!NT_SUCCESS(nErrCode))
 {
  errno = __status_to_errno(nErrCode);
  return (-1);
 }

 return (0);
}

/* EOF */

