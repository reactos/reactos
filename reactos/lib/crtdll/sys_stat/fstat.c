#include 	<sys/types.h>
#include	<sys/stat.h>
#include 	<fcntl.h>
#include	<string.h>
#include	<windows.h>
//#include	<libc/file.h>

void UnixTimeToFileTime( time_t unix_time, FILETIME *filetime, DWORD remainder );
time_t FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder );

int
fstat(int handle, struct stat *statbuf)
{
 
  BY_HANDLE_FILE_INFORMATION  FileInformation;

  if (!statbuf)
    {

      return -1;
    }

  if ( !GetFileInformationByHandle(_get_osfhandle(handle),&FileInformation) )
	return -1;
  statbuf->st_ctime = FileTimeToUnixTime( &FileInformation.ftCreationTime,NULL);
  statbuf->st_atime = FileTimeToUnixTime( &FileInformation.ftLastAccessTime,NULL);
  statbuf->st_mtime = FileTimeToUnixTime( &FileInformation.ftLastWriteTime,NULL);

  statbuf->st_dev = handle;
  statbuf->st_size = FileInformation.nFileSizeLow; 
  return 0;
}
