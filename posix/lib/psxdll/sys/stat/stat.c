/* $Id: stat.c,v 1.1 2002/05/17 02:10:41 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/sys/stat/stat.c
 * PURPOSE:     Get file status
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/05/2002: Created
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int fstat(int fildes, struct stat *buf)
{
 errno = ENOSYS;
 return (-1);
}

int lstat(const char *path, struct stat *buf)
{
 errno = ENOSYS;
 return (-1);
}

int stat(const char *path, struct stat *buf)
{
 errno = ENOSYS;
 return (-1);
}

/* EOF */

