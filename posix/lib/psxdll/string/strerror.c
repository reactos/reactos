/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/string/strerror.c
 * PURPOSE:     Get error message string
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              20/01/2002: Created
 */

#include <errno.h>
#include <string.h>
#include <psx/debug.h>

static char *__errstr = "<strerror() unsupported>";
char *strerror(int errnum)
{
 INFO("errnum %#x", errnum);
 TODO("getting error string not currently implemented");
 errno = EINVAL;
 return (__errstr);
}

/* EOF */

