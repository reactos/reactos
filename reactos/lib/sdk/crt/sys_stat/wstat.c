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
#include <sys/stat.h>
#include <wchar.h>
#include <direct.h>

/* for stat mode, permissions apply to all,owner and group */
#define ALL_S_IREAD  (S_IREAD  | (S_IREAD  >> 3) | (S_IREAD  >> 6))
#define ALL_S_IWRITE (S_IWRITE | (S_IWRITE >> 3) | (S_IWRITE >> 6))
#define ALL_S_IEXEC  (S_IEXEC  | (S_IEXEC  >> 3) | (S_IEXEC  >> 6))

/*
 * @implemented
 */
int _wstat (const wchar_t *path, struct _stat *buffer)
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
int _wstati64 (const wchar_t *path, struct _stati64 *buffer)
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

/*********************************************************************
*              _wstat64 (MSVCRT.@)
*/
int CDECL _wstat64(const wchar_t* path, struct __stat64 * buf)
{
    DWORD dw;
    WIN32_FILE_ATTRIBUTE_DATA hfi;
    unsigned short mode = ALL_S_IREAD;
    int plen;

    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &hfi))
    {
       __set_errno(ERROR_FILE_NOT_FOUND);
       return -1;
    }

    memset(buf,0,sizeof(struct __stat64));

    /* FIXME: rdev isn't drive num, despite what the docs says-what is it? */
    if (iswalpha(*path))
        buf->st_dev = buf->st_rdev = towupper(*path - 'A'); /* drive num */
    else
        buf->st_dev = buf->st_rdev = _getdrive() - 1;

    plen = wcslen(path);

    /* Dir, or regular file? */
    if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (path[plen-1] == '\\'))
        mode |= (S_IFDIR | ALL_S_IEXEC);
    else
    {
        mode |= S_IFREG;
        /* executable? */
        if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
            
        {
            wchar_t* ext = wcsrchr(path, L'.');
            if (ext && (!_wcsicmp(ext, L".exe") || !_wcsicmp(ext, L".com") ||
                !_wcsicmp(ext, L".bat") || !_wcsicmp(ext, L".cmd")))
                mode |= S_IEXEC;
        }
    }

    if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
        mode |= ALL_S_IWRITE;

    buf->st_mode  = mode;
    buf->st_nlink = 1;
    buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
    buf->st_atime = dw;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
    buf->st_mtime = buf->st_ctime = dw;

    return 0;
}
