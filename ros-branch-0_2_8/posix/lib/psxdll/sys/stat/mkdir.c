/* $Id: mkdir.c,v 1.3 2002/10/29 04:45:44 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/sys/stat/mkdir.c
 * PURPOSE:     Make a directory
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/05/2002: Created
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int mkdir(const char *path, mode_t mode)
{
 int nFileNo;
 
 switch((nFileNo = open(path, O_CREAT | O_EXCL | _O_DIRFILE, mode)))
 {
  case -1:
   return (-1);
  
  default:
   close(nFileNo);
   return (0);
 }
}

/* EOF */

