#include 	<crtdll/sys/types.h>
#include	<crtdll/sys/stat.h>
#include 	<crtdll/fcntl.h>
#include	<crtdll/string.h>
#include	<windows.h>
#include	<crtdll/internal/file.h>


int
_fstat(int fd, struct stat *statbuf)
{
 
  BY_HANDLE_FILE_INFORMATION  FileInformation;

  if (!statbuf)
    {

      return -1;
    }

  if ( !GetFileInformationByHandle(_get_osfhandle(fd),&FileInformation) )
	return -1;
//  statbuf->st_ctime = FileTimeToUnixTime( &FileInformation.ftCreationTime,NULL);
//  statbuf->st_atime = FileTimeToUnixTime( &FileInformation.ftLastAccessTime,NULL);
//  statbuf->st_mtime = FileTimeToUnixTime( &FileInformation.ftLastWriteTime,NULL);

  statbuf->st_dev = fd;
  statbuf->st_size = FileInformation.nFileSizeLow; 
  return 0;
}
