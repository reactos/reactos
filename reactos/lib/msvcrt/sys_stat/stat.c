#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>

#include <windows.h>


int _stat(const char *path, struct stat *buffer)
{
  HANDLE findHandle;
  WIN32_FIND_DATAA findData;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(strchr(path, '*') || strchr(path, '?'))
  {
    __set_errno(EINVAL);
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
    buffer->st_mode |= S_IFREG;
  if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}

__int64 _stati64 (const char *path, struct _stati64 *buffer)
{
  HANDLE findHandle;
  WIN32_FIND_DATAA findData;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(strchr(path, '*') || strchr(path, '?'))
  {
    __set_errno(EINVAL);
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
  buffer->st_size = (((__int64)findData.nFileSizeHigh) << 32) +
		     findData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
    buffer->st_mode |= S_IFDIR;
  else
    buffer->st_mode |= S_IFREG;
  if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}

int _wstat (const wchar_t *path, struct stat *buffer)
{
  HANDLE findHandle;
  WIN32_FIND_DATAW findData;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(wcschr(path, L'*') || wcschr(path, L'?'))
  {
    __set_errno(EINVAL);
    return -1;
  }

  findHandle = FindFirstFileW(path, &findData);
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
    buffer->st_mode |= S_IFREG;
  if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}

__int64 _wstati64 (const wchar_t *path, struct _stati64 *buffer)
{
  HANDLE findHandle;
  WIN32_FIND_DATAW findData;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(wcschr(path, L'*') || wcschr(path, L'?'))
  {
    __set_errno(EINVAL);
    return -1;
  }

  findHandle = FindFirstFileW(path, &findData);
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
  buffer->st_size = (((__int64)findData.nFileSizeHigh) << 32) +
		     findData.nFileSizeLow;
  buffer->st_mode = S_IREAD;
  if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
    buffer->st_mode |= S_IFDIR;
  else
    buffer->st_mode |= S_IFREG;
  if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}
