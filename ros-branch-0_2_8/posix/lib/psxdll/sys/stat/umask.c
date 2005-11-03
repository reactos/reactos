/* $Id: umask.c,v 1.3 2002/10/29 04:45:46 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/sys/stat/umask.c
 * PURPOSE:     
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/05/2002: Created
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

mode_t umask(mode_t cmask)
{
 errno = ENOSYS;
 return (-1);
}

/* EOF */

