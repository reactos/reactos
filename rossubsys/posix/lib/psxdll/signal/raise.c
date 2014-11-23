/* $Id: raise.c,v 1.4 2002/10/29 04:45:41 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/signal/raise.c
 * PURPOSE:     Send a signal to the executing process
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <signal.h>
#include <pthread.h>
#include <errno.h>

int raise(int sig)
{
 /* returns zero if pthread_kill() returned zero, non-zero otherwise */
 /* pthread_kill() returns the error number and doesn't set errno */
 return (((errno = pthread_kill(pthread_self(), sig))) == 0 ? (0) : (1));
}

/* EOF */

