/* $Id: mknod.c,v 1.1 2002/05/17 02:10:41 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/sys/stat/mknod.c
 * PURPOSE:     Make a directory, a special or regular file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/05/2002: Created
 */

#include <sys/stat.h>
#include <errno.h>

int mknod(const char *path, mode_t mode, dev_t dev)
{
 errno = ENOSYS;
 return (-1);
}

/* EOF */

