/* $Id: getpid.c,v 1.2 2002/02/20 09:17:58 hyperion Exp $
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

pid_t getpid(void)
{
 return ((pid_t)NtCurrentTeb()->Cid.UniqueThread);
}

/* EOF */

