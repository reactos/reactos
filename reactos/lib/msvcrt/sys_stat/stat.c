#include <windows.h>
#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/string.h>
#include <msvcrt/internal/file.h>


int _stat(const char* path, struct stat* buffer)
{
  HANDLE findHandle;
  WIN32_FIND_DATAA findData;
  char* ext;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if (strchr(path, '*') || strchr(path, '?'))
  {
    __set_errno(ENOENT);
    return -1;
  }

  findHandle = FindFirstFileA(path, &findData);
  if (findHandle == INVALID_HANDLE_VALUE)
  {
    __set_errno(ENOENT);
    return -1;
  }

  FindClose(findHandle);

  memset (buffer, 0, sizeof(struct stat));

  buffer->st_ctime = FileTimeToUnixTime(&findData.ftCreationTime,NULL);
  buffer->st_atime = FileTimeToUnixTime(&findData.ftLastAccessTime,NULL);
  buffer->st_mtime = FileTimeToUnixTime(&findData.ftLastWriteTime,NULL);

//  statbuf->st_dev = fd;
  buffer->st_size = findData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
    buffer->st_mode |= S_IFDIR;
  else
  {
    buffer->st_mode |= S_IFREG;
    ext = strrchr(path, '.');
    if (ext && (!stricmp(ext, ".exe") || 
	        !stricmp(ext, ".com") || 
		!stricmp(ext, ".bat") || 
		!stricmp(ext, ".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}
