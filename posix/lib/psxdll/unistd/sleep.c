/* $Id: sleep.c,v 1.3 2002/10/29 04:45:48 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/sleep.c
 * PURPOSE:     Suspend execution for an interval of time
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              14/05/2002: Created
 */

#include <ddk/ntddk.h>
#include <unistd.h>

unsigned int sleep(unsigned int seconds)
{
 LARGE_INTEGER lnDelay = RtlEnlargedIntegerMultiply(seconds, 10000000);

 if(!NT_SUCCESS(NtDelayExecution(FALSE, &lnDelay)))
  return seconds;

 return 0;
}

/* EOF */

