/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/unistd/dup.c
 * PURPOSE:     Duplicate an open file descriptor
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              13/02/2002: Created
 */

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>

int dup(int fildes)
{
 return (fcntl(fildes, F_DUPFD, 0));
}

int dup2(int fildes, int fildes2)
{
 if(fildes < 0 || fildes >= OPEN_MAX)
 {
  errno = EBADF;
  return (-1);
 }

 /* TODO: check if fildes is valid */

 if(fildes == fildes2)
  return fildes2;

 close(fildes2);
 return (fcntl(fildes, F_DUPFD, fildes2));
}

/* EOF */

