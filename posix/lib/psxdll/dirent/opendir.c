/* $Id: opendir.c,v 1.1 2002/02/20 07:06:50 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/dirent/opendir.c
 * PURPOSE:     Open a directory
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/01/2002: Created
 *              13/02/2002: KJK::Hyperion: modified to use file descriptors
 */

#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <wchar.h>
#include <errno.h>
#include <psx/debug.h>
#include <psx/stdlib.h>
#include <psx/dirent.h>
#include <psx/safeobj.h>

DIR *opendir(const char *dirname)
{
 ANSI_STRING    strDirName;
 UNICODE_STRING wstrDirName;
 DIR           *pdData;

 RtlInitAnsiString(&strDirName, (PCSZ)dirname);
 RtlAnsiStringToUnicodeString(&wstrDirName, &strDirName, TRUE);

 pdData = (DIR *)_Wopendir(wstrDirName.Buffer);

 RtlFreeUnicodeString(&wstrDirName);

 return (pdData);

}

DIR *_Wopendir(const wchar_t *dirname)
{
 struct __internal_DIR *pidData;
 int                    nFileNo;

 /* allocate internal object */
 pidData = __malloc(sizeof(*pidData));

 /* allocation failed */
 if(pidData == 0)
 {
  errno = ENOMEM;
  return (0);
 }

 /* open the directory */
 nFileNo = _Wopen(dirname, O_RDONLY | _O_DIRFILE);

 /* failure */
 if(nFileNo < 0)
 {
  __free(pidData);
  return (0);
 }

 /* directory file descriptors must be closed on exec() */
 if(fcntl(nFileNo, F_SETFD, FD_CLOEXEC) == -1)
  WARN
  (
   "couldn't set FD_CLOEXEC flag on file number %u, errno %u",
   nFileNo,
   errno
  );

 /* associate the internal data to the file descriptor */
 if(fcntl(nFileNo, F_SETXP, pidData) == -1)
  WARN
  (
   "couldn't associate the object at 0x%X to the file number %u, errno %u",
   pidData,
   nFileNo,
   errno
  );

 if(fcntl(nFileNo, F_SETXS, sizeof(*pidData)) == -1)
  WARN
  (
   "couldn't set the extra data size of the file number %u, errno %u",
   nFileNo,
   errno
  );

 pidData->signature = __IDIR_MAGIC;

 /* success */
 return ((DIR *)pidData);
}

/* EOF */

