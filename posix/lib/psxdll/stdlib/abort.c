/* $Id: abort.c,v 1.2 2002/02/20 09:17:58 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/stdlib/abort.c
 * PURPOSE:     Generate an abnormal process abort
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <stdlib.h>
#include <signal.h>

void abort(void)
{
 raise(SIGABRT);
}

/* EOF */

