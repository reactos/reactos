/* $Id: raise.c,v 1.2 2002/02/20 09:17:57 hyperion Exp $
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

