/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
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

