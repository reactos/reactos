/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/sys/fstat.c
 * PURPOSE:     Gather file information
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include	<windows.h>
#include 	<crtdll/sys/types.h>
#include	<crtdll/sys/stat.h>
#include 	<crtdll/fcntl.h>
#include	<crtdll/string.h>
#include	<crtdll/errno.h>
#include	<crtdll/internal/file.h>


int
_fstat(int fd, struct stat *statbuf)
{
 
  BY_HANDLE_FILE_INFORMATION  FileInformation;

  if (!statbuf)
    {
      __set_errno(EINVAL);	
      return -1;
    }

  if ( !GetFileInformationByHandle(_get_osfhandle(fd),&FileInformation) ) {
	__set_errno (EBADF);
	return -1;
  }
  statbuf->st_ctime = FileTimeToUnixTime( &FileInformation.ftCreationTime,NULL);
  statbuf->st_atime = FileTimeToUnixTime( &FileInformation.ftLastAccessTime,NULL);
  statbuf->st_mtime = FileTimeToUnixTime( &FileInformation.ftLastWriteTime,NULL);
  if (statbuf->st_atime ==0)
    statbuf->st_atime = statbuf->st_mtime;
  if (statbuf->st_ctime ==0)
    statbuf->st_ctime = statbuf->st_mtime;

  statbuf->st_dev = FileInformation.dwVolumeSerialNumber; 
  statbuf->st_size = FileInformation.nFileSizeLow; 
  statbuf->st_nlink = FileInformation.nNumberOfLinks;
  statbuf->st_mode = S_IREAD;
  if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    statbuf->st_mode |= S_IFDIR | S_IEXEC;
  else
    statbuf->st_mode |= S_IFREG;
  if ( !(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    statbuf->st_mode |= S_IWRITE;
  return 0;
}
