#include <windows.h>
#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/string.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
int _wstat (const wchar_t *path, struct stat *buffer)
{
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
  wchar_t *ext;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(wcschr(path, L'*') || wcschr(path, L'?'))
  {
    __set_errno(ENOENT);
    return -1;
  }

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &fileAttributeData))
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
    ext = wcsrchr(path, L'.');
    if (ext && (!_wcsicmp(ext, L".exe") || 
	        !_wcsicmp(ext, L".com") || 
		!_wcsicmp(ext, L".bat") || 
		!_wcsicmp(ext, L".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}

/*
 * @implemented
 */
__int64 _wstati64 (const wchar_t *path, struct _stati64 *buffer)
{
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
  wchar_t *ext;

  if (!buffer)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if(wcschr(path, L'*') || wcschr(path, L'?'))
  {
    __set_errno(ENOENT);
    return -1;
  }

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &fileAttributeData))
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
    ext = wcsrchr(path, L'.');
    if (ext && (!_wcsicmp(ext, L".exe") || 
	        !_wcsicmp(ext, L".com") || 
		!_wcsicmp(ext, L".bat") || 
		!_wcsicmp(ext, L".cmd")))
      buffer->st_mode |= S_IEXEC;
  }
  if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) 
    buffer->st_mode |= S_IWRITE;

  return 0;
}
