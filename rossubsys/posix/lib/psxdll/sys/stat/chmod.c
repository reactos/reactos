/* $Id: chmod.c,v 1.3 2002/10/29 04:45:44 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/sys/stat/chmod.c
 * PURPOSE:     Change mode of a file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/05/2002: Created
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int chmod(const char *path, mode_t mode)
{
 errno = ENOSYS;
 return (-1);
}

int fchmod(int fildes, mode_t mode)
{
 errno = ENOSYS;
 return (-1);
}

/* EOF */

