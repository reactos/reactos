/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <sys/utime.h>

/*
 * @implemented
 */
int _futime (int nHandle, struct _utimbuf *pTimes)
{
  FILETIME  LastAccessTime;
  FILETIME  LastWriteTime;

  // check for stdin / stdout  handles ??
  if (nHandle == -1) {
      __set_errno(EBADF);
      return -1;
  }

  if (pTimes == NULL) {
      pTimes = _alloca(sizeof(struct _utimbuf));
      time(&pTimes->actime);
      time(&pTimes->modtime);
  }

  if (pTimes->actime < pTimes->modtime) {
      __set_errno(EINVAL);
      return -1;
  }

  UnixTimeToFileTime(pTimes->actime,&LastAccessTime,0);
  UnixTimeToFileTime(pTimes->modtime,&LastWriteTime,0);
  if (!SetFileTime((HANDLE)_get_osfhandle(nHandle),NULL, &LastAccessTime, &LastWriteTime)) {
      __set_errno(EBADF);
      return -1;
  }

  return 0;
}
