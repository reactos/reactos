/* $Id: fstat.c,v 1.13 2002/11/24 18:42:25 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/sys/fstat.c
 * PURPOSE:     Gather file information
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/string.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


int _fstat(int fd, struct stat* statbuf)
{
  BY_HANDLE_FILE_INFORMATION  FileInformation;
  DWORD dwFileType;
  void* handle;

  if (!statbuf)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if ((void*)-1 == (handle = _get_osfhandle(fd)))
  {
    __set_errno(EBADF);
    return -1;
  }

  fflush(NULL);

  memset (statbuf, 0, sizeof(struct stat));

  dwFileType = GetFileType(handle);

  if (dwFileType == FILE_TYPE_DISK)
  {
    if (!GetFileInformationByHandle(handle,&FileInformation))
    {
      __set_errno(EBADF);
      return -1;
    }
    statbuf->st_ctime = FileTimeToUnixTime(&FileInformation.ftCreationTime,NULL);
    statbuf->st_atime = FileTimeToUnixTime(&FileInformation.ftLastAccessTime,NULL);
    statbuf->st_mtime = FileTimeToUnixTime(&FileInformation.ftLastWriteTime,NULL);

    statbuf->st_dev = fd;
    statbuf->st_size = FileInformation.nFileSizeLow;
    statbuf->st_mode = S_IREAD;
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
      statbuf->st_mode |= S_IFDIR;
    else
      statbuf->st_mode |= S_IFREG;
    if (!(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      statbuf->st_mode |= S_IWRITE;
  }
  else if (dwFileType == FILE_TYPE_CHAR)
  {
    statbuf->st_dev = fd;
    statbuf->st_mode = S_IFCHR;
  }
  else if (dwFileType == FILE_TYPE_PIPE)
  {
    statbuf->st_dev = fd;
    statbuf->st_mode = S_IFIFO;
  }
  else
  {
    // dwFileType is FILE_TYPE_UNKNOWN or has a bad value
    __set_errno(EBADF);
    return -1;
  }
  return 0;
}
