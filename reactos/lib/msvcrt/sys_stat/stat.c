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
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
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

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fileAttributeData))
  {
    __set_errno(ENOENT);
    return -1;
  }

  memset (buffer, 0, sizeof(struct stat));

  buffer->st_ctime = FileTimeToUnixTime(&fileAttributeData.ftCreationTime,NULL);
  buffer->st_atime = FileTimeToUnixTime(&fileAttributeData.ftLastAccessTime,NULL);
  buffer->st_mtime = FileTimeToUnixTime(&fileAttributeData.ftLastWriteTime,NULL);

//  statbuf->st_dev = fd;
  buffer->st_size = fileAttributeData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
    buffer->st_mode |= S_IFDIR;
  else
  {
    buffer->st_mode |= S_IFREG;
    ext = strrchr(path, '.');
    if (ext && (!_stricmp(ext, ".exe") || 
	        !_stricmp(ext, ".com") || 
		!_stricmp(ext, ".bat") || 
		!_stricmp(ext, ".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}

__int64 _stati64 (const char *path, struct _stati64 *buffer)
{
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
  char* ext;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(strchr(path, '*') || strchr(path, '?'))
  {
    __set_errno(ENOENT);
    return -1;
  }

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fileAttributeData))
  {
    __set_errno(ENOENT);
    return -1;
  }

  memset (buffer, 0, sizeof(struct _stati64));

  buffer->st_ctime = FileTimeToUnixTime(&fileAttributeData.ftCreationTime,NULL);
  buffer->st_atime = FileTimeToUnixTime(&fileAttributeData.ftLastAccessTime,NULL);
  buffer->st_mtime = FileTimeToUnixTime(&fileAttributeData.ftLastWriteTime,NULL);

//  statbuf->st_dev = fd;
  buffer->st_size = ((((__int64)fileAttributeData.nFileSizeHigh) << 16) << 16) +
             fileAttributeData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
    buffer->st_mode |= S_IFDIR;
  else
  {
    buffer->st_mode |= S_IFREG;
    ext = strrchr(path, '.');
    if (ext && (!_stricmp(ext, ".exe") || 
	        !_stricmp(ext, ".com") || 
		!_stricmp(ext, ".bat") || 
		!_stricmp(ext, ".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}

