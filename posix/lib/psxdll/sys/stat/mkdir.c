/* $Id: mkdir.c,v 1.1 2002/05/17 02:10:41 hyperion Exp $
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

