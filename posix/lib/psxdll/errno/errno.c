/* $Id: errno.c,v 1.2 2002/02/20 09:17:56 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/errno/errno.c
 * PURPOSE:     Internal errno implementation
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <psx/debug.h>
#include <psx/errno.h>

static int __errno_storage = 0;

int * __PdxGetThreadErrNum(void)
{
 FIXME("errno currently not thread-safe");
 return (&__errno_storage);
}

/* EOF */

