/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/pthread/kill.c
 * PURPOSE:     Send a signal to a thread
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <signal.h>
#include <errno.h>

int pthread_kill(pthread_t thread, int sig)
{
 return (ENOSYS);
}

/* EOF */

