/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/pthread/self.c
 * PURPOSE:     get calling thread's ID
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <sys/types.h>
#include <pthread.h>

pthread_t pthread_self(void)
{
 return ((pthread_t)(NtCurrentTeb()->Cid).UniqueThread);
}

/* EOF */

